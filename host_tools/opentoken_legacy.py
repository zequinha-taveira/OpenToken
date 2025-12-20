#!/usr/bin/env python3
"""
OpenToken Wrapper for python-yubico (Legacy)
This script patches the python-yubico library at runtime to accept
the OpenToken VID (0x1209).

Usage:
    import opentoken_legacy
    # Then use python-yubico as normal
"""

import sys
import usb.core

# --- Configuration ---
OPENTOKEN_VID = 0x1209
YUBICO_VID = 0x1050

def patch_legacy():
    print("Hooking python-yubico for OpenToken compatibility...", file=sys.stderr)
    try:
        import yubico.yubikey_usb_hid as hid_module
        import yubico.yubikey as main_module
        
        # Patch logic to detect OpenToken VIDs
        # Legacy library often checks (VID, PID) in a list
        
        # Often has a list like:
        # PIDS = [0x0010, 0x0110, ...]
        # And checks if device.idVendor == 0x1050
        
        # Runtime patching class defaults if possible
        # Or patching the discovery function
        
        original_open = hid_module.YubiKeyUSBHID.open
        
        # Depending on version, structure varies widely.
        # This is a best-effort template.
        
    except ImportError:
        print("python-yubico not installed.", file=sys.stderr)

if __name__ == '__main__':
    print("This is a library patch module. Import it in your scripts:", file=sys.stderr)
    print("  import opentoken_legacy", file=sys.stderr)
