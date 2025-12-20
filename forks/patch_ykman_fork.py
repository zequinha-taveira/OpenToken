import os

# Root of the yubikey-manager fork
FORK_ROOT = r'C:\OpenToken\forks\yubikey-manager'

def patch_file(rel_path, old_text, new_text, description):
    abs_path = os.path.join(FORK_ROOT, rel_path)
    if not os.path.exists(abs_path):
        print(f"File not found: {abs_path}")
        return False
    
    with open(abs_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if old_text in content:
        new_content = content.replace(old_text, new_text)
        with open(abs_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Patched {description}: {rel_path}")
        return True
    else:
        if new_text in content:
            print(f"Already patched {description}: {rel_path}")
        else:
            print(f"Target text for {description} not found in: {rel_path}")
        return False

def main():
    print(f"Patching yubikey-manager fork at: {FORK_ROOT}")

    # 1. Patch yubikit/core/__init__.py for is_yubikey
    # (Actually it's often in core/__init__.py or core/otp.py)
    # Based on previous search, yubikit/core/smartcard/__init__.py and core/otp.py were hits
    
    # 2. Patch ykman/hid/base.py
    patch_file(
        r'ykman\hid\base.py',
        'YUBICO_VID = 0x1050',
        'YUBICO_VID = 0x1050\nOPENTOKEN_VID = 0x1209',
        "HID Base VID"
    )

    # 3. Patch ykman/hid/fido.py
    patch_file(
        r'ykman\hid\fido.py',
        'if desc.vid == 0x1050:',
        'if desc.vid in [0x1050, 0x1209]:',
        "FIDO VID Check"
    )

    # 4. Patch ykman/pcsc/__init__.py
    patch_file(
        r'ykman\pcsc\__init__.py',
        'if YK_READER_NAME not in name.lower():',
        'if YK_READER_NAME not in name.lower() and "opentoken" not in name.lower():',
        "PCSC Reader Filter 1"
    )
    patch_file(
        r'ykman\pcsc\__init__.py',
        'if YK_READER_NAME in reader.name.lower():',
        'if YK_READER_NAME in reader.name.lower() or "opentoken" in reader.name.lower():',
        "PCSC Reader Filter 2"
    )
    patch_file(
        r'ykman\pcsc\__init__.py',
        'name_filter = YK_READER_NAME if name_filter is None else name_filter',
        '# Filter loosened for OpenToken\n    pass',
        "PCSC List Devices Filter"
    )

    # 5. Patch yubikit/core/otp.py (if needed for the raw protocol)
    # Some versions have is_yubikey here
    patch_file(
        r'yubikit\core\otp.py',
        'vid == 0x1050',
        'vid in [0x1050, 0x1209]',
        "Yubikit OTP VID Check"
    )

    # 6. Patch python-yubico fork
    # Root of the python-yubico fork
    PY_YUBICO_ROOT = r'C:\OpenToken\forks\python-yubico'
    
    # helper for second repo
    def patch_file_py(rel_path, old_text, new_text, description):
        abs_path = os.path.join(PY_YUBICO_ROOT, rel_path)
        if not os.path.exists(abs_path):
            print(f"File not found: {abs_path}")
            return False
        with open(abs_path, 'r', encoding='utf-8') as f:
            content = f.read()
        if old_text in content:
            new_content = content.replace(old_text, new_text)
            with open(abs_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Patched {description}: {rel_path}")
            return True
        return False

    patch_file_py(
        r'yubico\yubikey_defs.py',
        'YUBICO_VID               = 0x1050',
        'YUBICO_VID               = 0x1050\nOPENTOKEN_VID            = 0x1209',
        "Legacy VID Def"
    )

    # 7. Patch python-yubico discovery logic
    patch_file_py(
        r'yubico\yubikey_usb_hid.py',
        'idVendor=YUBICO_VID',
        'idVendor=[YUBICO_VID, OPENTOKEN_VID] if hasattr(usb.core, "find") else YUBICO_VID', # Note: PyUSB find accepts list in recent versions
        "Legacy Discovery 1"
    )
    
    # Second location in _get_usb_device
    patch_file_py(
        r'yubico\yubikey_usb_hid.py',
        'if device.idVendor == YUBICO_VID:',
        'if device.idVendor in [YUBICO_VID, OPENTOKEN_VID]:',
        "Legacy Discovery 2"
    )
    
    # Third location in the description string
    patch_file_py(
        r'yubico\yubikey_usb_hid.py',
        '(USB vendor id = 0x1050, product id = 0x10)',
        '(OpenToken fork: compatible with 0x1050 and 0x1209)',
        "Legacy Description"
    )
    
    print("\nFork patching complete.")

if __name__ == '__main__':
    main()
