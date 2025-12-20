# Python OpenToken SDK

The official library for interacting with **OpenToken** security hardware (RP2350).

## Installation
```bash
pip install ./libraries/python-opentoken
```

## Quick Start
```python
from opentoken import list_devices, OATHClient

# 1. Discovery
devices = list_devices()
print(f"Found: {devices}")

# 2. OATH (TOTP)
if devices and devices[0].pcsc_reader:
    client = OATHClient(devices[0].pcsc_reader)
    if client.connect():
        accounts = client.list_accounts()
        for acc in accounts:
            code = client.calculate(acc['name'])
            print(f"{acc['name']}: {code}")
```

## Features
- Native Discovery (VID 0x1209)
- Full OATH Management (Add/Delete/List/Calc)
- Zero Yubico Dependencies
