#!/usr/bin/env python3
"""
OpenToken Factory Signer
------------------------
Offline tool to sign firmware images.
Simulates the "Factory Backend" authority.

Usage:
    python factory_signer.py --generate-keys
    python factory_signer.py --sign <firmware.bin> --key <private_key.pem>

Dependencies:
    pip install cryptography
"""

import argparse
import sys
import os
import struct
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization import load_pem_private_key

def generate_keys():
    """Generates a P-256 key pair for the Factory Root of Trust."""
    print("Generating Factory Root Key (P-256)...")
    private_key = ec.generate_private_key(ec.SECP256R1())
    
    # Save Private Key (SECURE STORAGE ONLY)
    with open("factory_private.pem", "wb") as f:
        f.write(private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        ))
    print("Saved: factory_private.pem (KEEP OFFLINE!)")

    # Save Public Key
    public_key = private_key.public_key()
    with open("factory_public.pem", "wb") as f:
        f.write(public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))
    print("Saved: factory_public.pem (Embed hash of this in OTP)")

def sign_firmware(firmware_path, key_path):
    """Signs the firmware binary."""
    print(f"Signing {firmware_path} with {key_path}...")
    
    # 1. Load Key
    with open(key_path, "rb") as f:
        private_key = load_pem_private_key(f.read(), password=None)

    # 2. Read Firmware
    with open(firmware_path, "rb") as f:
        firmware_data = f.read()

    # 3. Hash Firmware
    digest = hashes.Hash(hashes.SHA256())
    digest.update(firmware_data)
    firmware_hash = digest.finalize()
    
    print(f"SHA-256: {firmware_hash.hex()}")

    # 4. Sign Hash (ECDSA P-256)
    signature = private_key.sign(
        firmware_data,
        ec.ECDSA(hashes.SHA256())
    )
    
    print(f"Signature ({len(signature)} bytes): {signature.hex()}")

    # 5. Create Signed Bundle (TLV or simple suffix)
    # For MVP: Firmware + Signature (Simple concatenation for now, proper header later)
    # Format: [Firmware Data] [Magic Marker] [Signature Length (4B)] [Signature]
    magic = b"OTSIG"
    sig_len = len(signature)
    
    output_path = firmware_path + ".signed"
    with open(output_path, "wb") as f:
        f.write(firmware_data)
        f.write(magic)
        f.write(struct.pack("<I", sig_len))
        f.write(signature)
        
    print(f"Signed firmware saved to: {output_path}")

def main():
    parser = argparse.ArgumentParser(description="OpenToken Factory Signer")
    parser.add_argument("--generate-keys", action="store_true", help="Generate new Factory Root Keys")
    parser.add_argument("--sign", help="Path to firmware binary to sign")
    parser.add_argument("--key", default="factory_private.pem", help="Path to private key")

    args = parser.parse_args()

    if args.generate_keys:
        generate_keys()
    elif args.sign:
        if not os.path.exists(args.key):
            print(f"Error: Key file '{args.key}' not found. Generate it first?")
            sys.exit(1)
        sign_firmware(args.sign, args.key)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
