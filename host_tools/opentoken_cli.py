import sys
import argparse
from opentoken_sdk.opentoken import OpenTokenSDK, OATHClient

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

    else:
        parser.print_help()

if __name__ == "__main__":
    main()
