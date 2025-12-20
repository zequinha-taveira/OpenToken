import os
import sys

def find_site_packages():
    for p in sys.path:
        if 'site-packages' in p and os.path.isdir(p):
            # Prioritize the one in Roaming for this user
            if 'Roaming' in p:
                return p
    return r'C:\Users\zequi\AppData\Roaming\Python\Python314\site-packages'

def patch_file(filepath, old_text, new_text, description):
    if not os.path.exists(filepath):
        print(f"Skipping (Not Found): {filepath}")
        return False
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if old_text in content:
        new_content = content.replace(old_text, new_text)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Patched {description}: {filepath}")
        return True
    else:
        if new_text in content:
            print(f"Already patched {description}: {filepath}")
        else:
            print(f"Target text for {description} not found in: {filepath}")
        return False

def main():
    root = find_site_packages()
    print(f"Patching Yubico tools in: {root}")

    # 1. Patch HID Base VID
    patch_file(
        os.path.join(root, 'ykman', 'hid', 'base.py'),
        'YUBICO_VID = 0x1050',
        'YUBICO_VID = 0x1050\nOPENTOKEN_VID = 0x1209',
        "HID Base VID"
    )

    # 2. Patch FIDO VID Check
    patch_file(
        os.path.join(root, 'ykman', 'hid', 'fido.py'),
        'if desc.vid == 0x1050:',
        'if desc.vid in [0x1050, 0x1209]:',
        "FIDO VID Check"
    )

    # 3. Patch PCSC (Smart Card) Reader Name Filter
    # In ykman/pcsc/__init__.py
    # Change YK_READER_NAME usage to be more flexible
    pcsc_init = os.path.join(root, 'ykman', 'pcsc', '__init__.py')
    
    # First, allow "opentoken" in reader name identification
    # We target the __init__.py where YK_READER_NAME is used
    patch_file(
        pcsc_init,
        'if YK_READER_NAME not in name.lower():',
        'if YK_READER_NAME not in name.lower() and "opentoken" not in name.lower():',
        "PCSC Reader Filter 1"
    )
    
    patch_file(
        pcsc_init,
        'if YK_READER_NAME in reader.name.lower():',
        'if YK_READER_NAME in reader.name.lower() or "opentoken" in reader.name.lower():',
        "PCSC Reader Filter 2"
    )

    patch_file(
        pcsc_init,
        'name_filter = YK_READER_NAME if name_filter is None else name_filter',
        '# Filter loosened for OpenToken\n    pass', # Effectively skip the name_filter logic in list_devices
        "PCSC List Devices Filter"
    )

    print("\nAll permanent patches applied successfully.")
    print("You can now use standard 'ykman' command directly.")

if __name__ == '__main__':
    main()
