import sys
import usb.core
import usb.util
import base64
import time
import struct
try:
    import cbor2
except ImportError:
    # Fallback or alert user
    cbor2 = None
from smartcard.System import readers
from smartcard.util import toBytes

# --- Configuration ---
OPENTOKEN_VID = 0x1209
OPENTOKEN_PID = 0x0001
OATH_AID = [0xA0, 0x00, 0x00, 0x05, 0x27, 0x21, 0x01, 0x01]

# CTAP2 Constants
CTAPHID_CMD_MSG = 0x03
CTAPHID_CMD_CBOR = 0x10
CTAPHID_CMD_INIT = 0x06
CTAPHID_INIT_FLAG = 0x80

CTAP2_CMD_GET_INFO = 0x04
CTAP2_CMD_CRED_MGMT = 0x41

class OpenTokenDevice:
    """Represents a connected OpenToken device."""
    def __init__(self, usb_dev=None, serial=None):
        self.serial = serial
        self.usb_dev = usb_dev
        self.sc_reader = None

    def __repr__(self):
        return f"<OpenToken Device (Serial: {self.serial})>"

class OpenTokenSDK:
    @staticmethod
    def list_devices():
        """Finds all connected OpenToken devices by VID."""
        devices = []
        # Find via USB (for FIDO2/Management)
        usb_devices = usb.core.find(find_all=True, idVendor=OPENTOKEN_VID)
        for dev in usb_devices:
            serial = dev.serial_number if dev.serial_number else "Unknown"
            ot_dev = OpenTokenDevice(usb_dev=dev, serial=serial)
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

class CTAP2Client:
    """Handles CTAPHID communication with the CTAP2 engine."""
    def __init__(self, device):
        self.device = device
        self.cid = 0xFFFFFFFF
        self.interface = 0
        self.endpoint_out = None
        self.endpoint_in = None

    def connect(self):
        """Prepares USB HID endpoints and performs CTAPHID_INIT."""
        if not self.device.usb_dev:
            raise Exception("Device does not have a USB attachment")
        
        dev = self.device.usb_dev
        # Detach kernel driver if needed
        try:
            if dev.is_kernel_driver_active(self.interface):
                dev.detach_kernel_driver(self.interface)
        except:
            pass

        # Use the first configuration
        dev.set_configuration()
        cfg = dev.get_active_configuration()
        intf = cfg[(self.interface, 0)]

        self.endpoint_out = usb.util.find_descriptor(intf, custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)
        self.endpoint_in = usb.util.find_descriptor(intf, custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)

        if not self.endpoint_out or not self.endpoint_in:
            raise Exception("HID endpoints not found")

        # Initialize Channel ID
        nonce = list(range(8))
        init_pkt = struct.pack(">I B H", 0xFFFFFFFF, CTAPHID_CMD_INIT | CTAPHID_INIT_FLAG, 8) + bytes(nonce)
        init_pkt = init_pkt.ljust(64, b'\x00')
        
        self.endpoint_out.write(init_pkt)
        resp = self.endpoint_in.read(64)
        
        # Parse INIT response: Nonce(8), CID(4), ProtocolVersion(1), Versions(3), Capabilities(1)
        r_nonce = resp[7:15]
        if r_nonce != bytes(nonce):
            raise Exception("CTAPHID_INIT nonce mismatch")
        
        self.cid = struct.unpack(">I", resp[15:19])[0]
        return True

    def send_cbor(self, cmd, data_dict=None):
        """Sends a CTAP2 command with CBOR payload and returns the response."""
        if cbor2 is None:
            raise Exception("cbor2 library is required for CTAP2 operations")

        payload = bytes([cmd])
        if data_dict is not None:
            payload += cbor2.dumps(data_dict)

        # Message fragmentation (simplified for small packets)
        msg_len = len(payload)
        pkt = struct.pack(">I B H", self.cid, CTAPHID_CMD_CBOR | CTAPHID_INIT_FLAG, msg_len) + payload
        pkt = pkt.ljust(64, b'\x00')
        
        self.endpoint_out.write(pkt)
        
        # Read response (simplified - assuming single packet for now)
        resp = self.endpoint_in.read(64)
        # cid(4), cmd(1), len(2), status(1), cbor(...)
        r_len = (resp[5] << 8) | resp[6]
        r_status = resp[7]
        
        if r_status != 0x00:
            return {"status": r_status}

        if r_len > 1:
            return {"status": 0x00, "data": cbor2.loads(resp[8:8+r_len-1])}
        return {"status": 0x00}

    def get_info(self):
        return self.send_cbor(CTAP2_CMD_GET_INFO)

    def list_fido2_credentials(self, pin=None):
        """Lists Resident Keys using Credential Management (requires PIN if set)."""
        # 1. Enumerate RPs
        # Command 0x41, Subcommand 0x01 (enumerateRPsBegin)
        resp = self.send_cbor(CTAP2_CMD_CRED_MGMT, {0x01: 0x01})
        if resp["status"] != 0:
            return []
        
        rps = []
        # In a full implementation, we would loop with enumerateRPsNext (0x02)
        # For OpenToken demo, assuming small number of RPs
        if "data" in resp and 0x03 in resp["data"]:
            rps.append(resp["data"][0x03]) # RP
            
        return rps
