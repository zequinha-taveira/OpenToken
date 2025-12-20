#!/usr/bin/env python3
"""
OpenToken Wrapper for YubiKey Manager (ykman)
This script patches the yubikey-manager library at runtime to accept
the OpenToken VID (0x1209) as a valid YubiKey.

Usage:
    python opentoken_manager.py [ykman arguments]
    
Example:
    python opentoken_manager.py list
    python opentoken_manager.py info
"""

import sys
import unittest.mock as mock
from ykman._cli.__main__ import main as ykman_main

# --- Configuration ---
OPENTOKEN_VID = 0x1209
YUBICO_VID = 0x1050

# --- Patching Logic ---
def apply_patches():
    print("Hooking yubikey-manager for OpenToken compatibility...", file=sys.stderr)
    
    try:
        # Patch 1: yubikit.core.smartcard
        # We need to ensure the library attempts to connect to our VID
        import yubikit.core.smartcard as sc_module
        
        # We wrap standard detection to include our VID
        original_is_yubikey = getattr(sc_module, 'is_yubikey', lambda vid: vid == YUBICO_VID)
        
        def patched_is_yubikey(vid):
            return original_is_yubikey(vid) or vid == OPENTOKEN_VID
            
        sc_module.is_yubikey = patched_is_yubikey
        
        # Patch 2: yubikit.core.otp
        try:
            import yubikit.core.otp as otp_module
            otp_module.is_yubikey = patched_is_yubikey
        except ImportError:
            pass # OTP module might not be loaded/available

        # Patch 3: ykman.device.list_all_devices
        # This is where high-level scanning happens
        try:
            import ykman.device as device_module
            # Scan logic usually checks specific PIDs.
            # Newer ykman versions rely on yubikit.core, so earlier checks might suffice.
            # But let's verify if there are other hardcoded checks.
            pass 
        except ImportError:
            pass

    except ImportError as e:
        print(f"Error: Could not find yubikey-manager libraries: {e}", file=sys.stderr)
        print("Please install ykman: pip install yubikey-manager", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    apply_patches()
    sys.exit(ykman_main())
