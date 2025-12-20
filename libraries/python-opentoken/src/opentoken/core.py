import base64
import usb.core
import usb.util
import time
from typing import List, Dict, Optional, Any
from smartcard.System import readers
from smartcard.util import toBytes

# OpenToken Identity Constants
OPENTOKEN_VID = 0x1209
OPENTOKEN_PID = 0x0001

# OATH Application Identifier (Standard OATH AID)
OATH_AID = [0xA0, 0x00, 0x00, 0x05, 0x27, 0x21, 0x01, 0x01]

class OpenTokenDevice:
    """Represents a connected OpenToken hardware device."""
    def __init__(self, usb_dev: Optional[usb.core.Device] = None, pcsc_reader: Any = None):
        self.usb_dev = usb_dev
        self.pcsc_reader = pcsc_reader
        self.serial = usb_dev.serial_number if usb_dev else "Unknown"

    def __repr__(self) -> str:
        return f"<OpenToken Device (Serial: {self.serial})>"

class ManagementClient:
    """Handles vendor-specific management commands via USB (WebUSB interface)."""
    CMD_GET_VERSION = 0x01
    CMD_GET_STATUS = 0x04
    CMD_RESET_DEVICE = 0x05

    def __init__(self, usb_dev: usb.core.Device):
        self.dev = usb_dev

    def get_version(self) -> str:
        """Retrieves and formats the firmware version."""
        try:
            # Send command to Vendor Out EP (Interface 3 usually)
            self.dev.write(0x04, [self.CMD_GET_VERSION])
            res = self.dev.read(0x84, 64)
            if res[0] == 0x00: # STATUS_OK
                return f"{res[1]}.{res[2]}.{res[3]}"
        except Exception as e:
            return f"Error: {e}"
        return "Unknown"

    def factory_reset(self) -> bool:
        """Triggers a full secure wipe of the device."""
        try:
            self.dev.write(0x04, [self.CMD_RESET_DEVICE])
            res = self.dev.read(0x84, 64)
            return res[0] == 0x00
        except:
            return False

class OATHClient:
    """Encapsulates CCID communication with the OATH applet."""
    def __init__(self, pcsc_reader: Any):
        self.reader = pcsc_reader
        self.conn: Any = None

    def connect(self) -> bool:
        """Establishes a CCID connection and selects the OATH applet."""
        self.conn = self.reader.createConnection()
        self.conn.connect()
        SELECT_APDU = [0x00, 0xA4, 0x04, 0x00, len(OATH_AID)] + OATH_AID
        data, sw1, sw2 = self.conn.transmit(SELECT_APDU)
        return sw1 == 0x90

    def list_accounts(self) -> List[Dict[str, str]]:
        """Enumerates all OATH accounts stored on the device."""
        LIST_APDU = [0x00, 0xA1, 0x00, 0x00, 0x00]
        data, sw1, sw2 = self.conn.transmit(LIST_APDU)
        accounts = []
        if sw1 == 0x90:
            i = 0
            while i < len(data):
                if data[i] == 0x72: # Name List Tag
                    l = data[i+1]
                    inner = data[i+2:i+2+l]
                    name, prop = "", 0
                    j = 0
                    while j < len(inner):
                        t, ln = inner[j], inner[j+1]
                        v = inner[j+2:j+2+ln]
                        if t == 0x71: name = bytes(v).decode('utf-8', errors='ignore')
                        if t == 0x75: prop = v[0]
                        j += 2 + ln
                    accounts.append({"name": name, "type": "TOTP" if (prop & 0x20) else "HOTP"})
                    i += 2 + l
                else: i += 1
        return accounts

    def calculate(self, name: str) -> str:
        """Requests an OATH code for a specific account."""
        nb = name.encode('utf-8')
        DATA = [0x71, len(nb)] + list(nb)
        CALC_APDU = [0x00, 0xA2, 0x00, 0x01, len(DATA)] + DATA + [0x00]
        d, sw1, sw2 = self.conn.transmit(CALC_APDU)
        if sw1 == 0x90 and len(d) >= 5:
            cb = d[3:7]
            code = (cb[0] << 24) | (cb[1] << 16) | (cb[2] << 8) | cb[3]
            return f"{code % 1000000:06d}"
        return "Error"

    def add_account(self, name: str, secret: str, is_totp: bool = True) -> bool:
        """Adds a new OATH credential to the hardware."""
        try:
            secret = secret.replace(" ", "").upper()
            padding = len(secret) % 8
            if padding: secret += "=" * (8 - padding)
            kb = base64.b32decode(secret)
        except: raise ValueError("Invalid Base32 secret")

        nb = name.encode('utf-8')
        DATA = [0x71, len(nb)] + list(nb) + [0x73, len(kb)] + list(kb)
        prop = 0x21 if is_totp else 0x11
        DATA += [0x75, 0x01, prop]

        PUT_APDU = [0x00, 0x01, 0x00, 0x00, len(DATA)] + DATA
        d, sw1, sw2 = self.conn.transmit(PUT_APDU)
        return sw1 == 0x90

    def delete_account(self, name: str) -> bool:
        """Removes a specific OATH credential from the hardware."""
        nb = name.encode('utf-8')
        DATA = [0x71, len(nb)] + list(nb)
        DEL_APDU = [0x00, 0x02, 0x00, 0x00, len(DATA)] + DATA
        d, sw1, sw2 = self.conn.transmit(DEL_APDU)
        return sw1 == 0x90

def list_devices() -> List[OpenTokenDevice]:
    """Scans the system for OpenToken hardware via USB and CCID."""
    devices = []
    usb_devs = usb.core.find(find_all=True, idVendor=OPENTOKEN_VID)
    all_readers = readers()
    for dev in usb_devs:
        # Match with PCSC readers by name mapping
        reader = next((r for r in all_readers if "opentoken" in r.name.lower()), None)
        devices.append(OpenTokenDevice(usb_dev=dev, pcsc_reader=reader))
    return devices
