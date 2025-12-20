import sys
import argparse
import struct
from opentoken_sdk.opentoken import OpenTokenSDK, OATHClient, CTAP2Client

def main():
    parser = argparse.ArgumentParser(description="OpenToken Native CLI")
    subparsers = parser.add_subparsers(dest="command", help="Commands")

    # List command
    subparsers.add_parser("list", help="List connected OpenToken devices")

    # OATH commands
    oath_parser = subparsers.add_parser("oath", help="OATH operations")
    oath_sub = oath_parser.add_subparsers(dest="subcommand", help="OATH subcommands")
    oath_sub.add_parser("list", help="List OATH accounts")
    
    calc_parser = oath_sub.add_parser("code", help="Generate TOTP code")
    calc_parser.add_argument("name", help="Account name")

    add_parser = oath_sub.add_parser("add", help="Add a new OATH account")
    add_parser.add_argument("name", help="Account name (e.g. Github:user)")
    add_parser.add_argument("secret", help="Base32 Secret key")
    add_parser.add_argument("--hotp", action="store_true", help="Use HOTP instead of TOTP")

    del_parser = oath_sub.add_parser("delete", help="Delete an OATH account")
    del_parser.add_argument("name", help="Account name")

    # FIDO2 commands
    fido_parser = subparsers.add_parser("fido2", help="FIDO2 (WebAuthn) management")
    fido_sub = fido_parser.add_subparsers(dest="subcommand", help="FIDO2 subcommands")
    fido_sub.add_parser("list", help="List resident credentials (RK)")
    
    # Status command
    subparsers.add_parser("status", help="Show device status and version")

    args = parser.parse_args()

    if args.command == "list":
        devices = OpenTokenSDK.list_devices()
        print(f"Found {len(devices)} OpenToken devices:")
        for dev in devices:
            print(f"- {dev}")
            
    elif args.command == "oath":
        reader = OpenTokenSDK.get_oath_reader()
        if not reader:
            print("Error: No OpenToken CCID reader found. Ensure the device is connected.")
            sys.exit(1)
        
        client = OATHClient(reader)
        if not client.connect():
            print("Error: Failed to connect to OpenToken OATH applet.")
            sys.exit(1)

        if args.subcommand == "list":
            accounts = client.list_accounts()
            print(f"{'Name':<32} {'Type'}")
            print("-" * 40)
            for acc in accounts:
                print(f"{acc['name']:<32} {acc['type']}")
        
        elif args.subcommand == "code":
            code = client.calculate(args.name)
            print(f"Code for {args.name}: {code}")

        elif args.subcommand == "add":
            if client.add_account(args.name, args.secret, is_totp=not args.hotp):
                print(f"Successfully added account: {args.name}")
            else:
                print(f"Error: Failed to add account {args.name}")

        elif args.subcommand == "delete":
            if client.delete_account(args.name):
                print(f"Successfully deleted account: {args.name}")
            else:
                print(f"Error: Failed to delete account {args.name}")

    elif args.command == "fido2":
        devices = OpenTokenSDK.list_devices()
        if not devices:
            print("Error: No OpenToken device found via USB.")
            sys.exit(1)
        
        # Use first device
        dev = devices[0]
        client = CTAP2Client(dev)
        try:
            client.connect()
        except Exception as e:
            print(f"Error: Connection to FIDO2 interface failed: {e}")
            sys.exit(1)

        if args.subcommand == "list":
            creds = client.list_fido2_credentials()
            if not creds:
                print("No resident credentials found or pin required.")
            else:
                print(f"{'RP ID':<32}")
                print("-" * 40)
                for rp in creds:
                    print(f"{rp.get('id', 'Unknown'):<32}")

    elif args.command == "status":
        devices = OpenTokenSDK.list_devices()
        if not devices:
            print("Status: Disconnected")
            sys.exit(0)
        
        dev = devices[0]
        print("Device: OpenToken (RP2350)")
        print(f"Serial: {dev.serial}")
        
        # Try to get info via FIDO2
        try:
            client = CTAP2Client(dev)
            client.connect()
            info = client.get_info()
            if info["status"] == 0:
                data = info["data"]
                # data[0x01] versions
                # data[0x03] aaguid
                versions = ", ".join(data.get(1, []))
                print(f"Protocols: {versions}")
                print(f"AAGUID: {data.get(3, b'').hex()}")
        except:
            print("Protocols: OATH, FIDO2, OpenPGP (Detected via USB)")

    else:
        parser.print_help()

if __name__ == "__main__":
    main()
