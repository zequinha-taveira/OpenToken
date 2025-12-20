import sys
import usb.core
import usb.util
import base64
from smartcard.System import readers
from smartcard.util import toBytes

# --- Configuration ---
OPENTOKEN_VID = 0x1209
OPENTOKEN_PID = 0x0001
OATH_AID = [0xA0, 0x00, 0x00, 0x05, 0x27, 0x21, 0x01, 0x01]

class OpenTokenDevice:
    """Represents a connected OpenToken device."""
    def __init__(self, serial=None):
        self.serial = serial
        self.usb_dev = None
        self.sc_reader = None

    def __repr__(self):
        return f"<OpenToken Device (Serial: {self.serial})>"

class OpenTokenSDK:
    @staticmethod
    def list_devices():
        """Finds all connected OpenToken devices by VID."""
        devices = []
        # Find via USB (for WebUSB/Management)
        usb_devices = usb.core.find(find_all=True, idVendor=OPENTOKEN_VID)
        for dev in usb_devices:
            # We don't strictly check PID here to allow future variations
            serial = dev.serial_number if dev.serial_number else "Unknown"
            ot_dev = OpenTokenDevice(serial=serial)
            ot_dev.usb_dev = dev
            devices.append(ot_dev)
        return devices

    @staticmethod
    def get_oath_reader():
        """Returns the first PCSC reader named 'OpenToken'."""
        for reader in readers():
            if "opentoken" in reader.name.lower():
                return reader
        return None

class OATHClient:
    """Encapsulates CCID communication with the OATH applet."""
    def __init__(self, reader):
        self.reader = reader
        self.connection = None

    def connect(self):
        self.connection = self.reader.createConnection()
        self.connection.connect()
        # Select OATH applet
        SELECT_APDU = [0x00, 0xA4, 0x04, 0x00, len(OATH_AID)] + OATH_AID
        data, sw1, sw2 = self.connection.transmit(SELECT_APDU)
        return sw1 == 0x90

    def list_accounts(self):
        """Sends INS_LIST (0xA1) and parses accounts."""
        if not self.connection:
            raise Exception("Not connected to PCSC reader")
        
        LIST_APDU = [0x00, 0xA1, 0x00, 0x00, 0x00]
        data, sw1, sw2 = self.connection.transmit(LIST_APDU)
        
        accounts = []
        if sw1 == 0x90:
            # Simple TLV parser for 0x72 [ 0x71 name 0x75 prop ]
            i = 0
            while i < len(data):
                if data[i] == 0x72: # Name List Tag
                    list_len = data[i+1]
                    inner_data = data[i+2:i+2+list_len]
                    
                    # Parse inner
                    name = ""
                    prop = 0
                    j = 0
                    while j < len(inner_data):
                        tag = inner_data[j]
                        length = inner_data[j+1]
                        val = inner_data[j+2:j+2+length]
                        if tag == 0x71: # Name
                            name = bytes(val).decode('utf-8', errors='ignore')
                        if tag == 0x75: # Property
                            prop = val[0]
                        j += 2 + length
                    
                    accounts.append({"name": name, "type": "TOTP" if (prop & 0x20) else "HOTP"})
                    i += 2 + list_len
                else:
                    i += 1
        return accounts

    def calculate(self, name):
        """Sends INS_CALCULATE (0xA2) with name TLV."""
        # Wrap name in tag 0x71
        name_bytes = name.encode('utf-8')
        DATA = [0x71, len(name_bytes)] + list(name_bytes)
        CALC_APDU = [0x00, 0xA2, 0x00, 0x01, len(DATA)] + DATA + [0x00]
        
        data, sw1, sw2 = self.connection.transmit(CALC_APDU)
        if sw1 == 0x90 and len(data) >= 5:
            # 0x76 0x05 0x06 [4 bytes code]
            code_bytes = data[3:7]
            code = (code_bytes[0] << 24) | (code_bytes[1] << 16) | (code_bytes[2] << 8) | code_bytes[3]
            return f"{code % 1000000:06d}"
        return "Error"

    def add_account(self, name, secret, is_totp=True):
        """Sends INS_PUT (0x01) to save a NEW account."""
        if not self.connection:
            raise Exception("Not connected to PCSC reader")
        
        # 1. Decode Base32 secret
        try:
            # Fix padding if needed
            secret = secret.replace(" ", "").upper()
            padding = len(secret) % 8
            if padding:
                secret += "=" * (8 - padding)
            key_bytes = base64.b32decode(secret)
        except Exception as e:
            raise Exception(f"Invalid Base32 secret: {e}")

        # 2. Build TLV data
        name_bytes = name.encode('utf-8')
        DATA = []
        # Tag 0x71: Name
        DATA += [0x71, len(name_bytes)] + list(name_bytes)
        # Tag 0x73: Key
        DATA += [0x73, len(key_bytes)] + list(key_bytes)
        # Tag 0x75: Property (0x21 = TOTP+SHA1, 0x11 = HOTP+SHA1)
        prop = 0x21 if is_totp else 0x11
        DATA += [0x75, 0x01, prop]

        # 3. Send APDU (INS_PUT = 0x01)
        PUT_APDU = [0x00, 0x01, 0x00, 0x00, len(DATA)] + DATA
        data, sw1, sw2 = self.connection.transmit(PUT_APDU)
        return sw1 == 0x90

    def delete_account(self, name):
        """Sends INS_DELETE (0x02) to remove an account."""
        if not self.connection:
            raise Exception("Not connected to PCSC reader")
            
        # Wrap name in tag 0x71
        name_bytes = name.encode('utf-8')
        DATA = [0x71, len(name_bytes)] + list(name_bytes)
        
        # INS_DELETE = 0x02
        DEL_APDU = [0x00, 0x02, 0x00, 0x00, len(DATA)] + DATA
        data, sw1, sw2 = self.connection.transmit(DEL_APDU)
        return sw1 == 0x90
