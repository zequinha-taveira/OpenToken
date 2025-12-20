import os
import time
import sys
from opentoken_sdk.opentoken import OpenTokenSDK, OATHClient

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def main():
    print("OpenToken Native Authenticator - Searching for device...")
    
    reader = OpenTokenSDK.get_oath_reader()
    if not reader:
        print("Error: No OpenToken CCID reader found.")
        return

    client = OATHClient(reader)
    if not client.connect():
        print("Error: Failed to connect to OATH applet.")
        return

    try:
        while True:
            accounts = client.list_accounts()
            clear_screen()
            print("=== OpenToken Native Authenticator ===")
            print(f"{'Account Name':<32} {'Code'}")
            print("-" * 45)
            
            for acc in accounts:
                if acc['type'] == 'TOTP':
                    code = client.calculate(acc['name'])
                    print(f"{acc['name']:<32} {code}")
                else:
                    print(f"{acc['name']:<32} [HOTP - Use CLI]")
            
            print("\nUpdating every 10 seconds. Press Ctrl+C to exit.")
            time.sleep(10)
    except KeyboardInterrupt:
        print("\nExiting...")

if __name__ == "__main__":
    main()
