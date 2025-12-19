#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// USB COMPOSITE DEVICE CONFIGURATION
//--------------------------------------------------------------------

// USB Device Configuration
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_POWER_MA 500
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)

// USB Class Configuration - Composite Device with HID + CCID
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 1 // FIDO2/CTAP2 Interface
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 1 // WebUSB Management Interface
#define CFG_TUD_CCID 1   // OATH/OpenPGP Interface

//--------------------------------------------------------------------
// HID INTERFACE CONFIGURATION (FIDO2/CTAP2)
//--------------------------------------------------------------------
#define CFG_TUD_HID_EP_BUFSIZE 64
#define CFG_TUD_HID_REPORT_DESC_LEN 34

//--------------------------------------------------------------------
// CCID INTERFACE CONFIGURATION (OATH/OpenPGP)
//--------------------------------------------------------------------
#define CFG_TUD_CCID_MAX_SLOTS 1
#define CFG_TUD_CCID_EP_BUFSIZE 64

//--------------------------------------------------------------------
// ENDPOINT CONFIGURATION
//--------------------------------------------------------------------
// Control endpoint
#define CFG_TUD_ENDPOINT0_SIZE 64

// Endpoint assignments for composite device:
// EP0: Control (bidirectional)
// EP1: HID IN (FIDO2 responses)
// EP2: HID OUT (FIDO2 commands)
// EP3: CCID IN (APDU responses)
// EP4: CCID OUT (APDU commands)
// EP5: Vendor IN (WebUSB responses)
// EP6: Vendor OUT (WebUSB commands)
#define CFG_TUD_ENDPOINT_MAX 7
#define CFG_TUD_TASK_QUEUE_LEN 16

// HID Endpoint Numbers
#define EPNUM_HID_IN 0x81
#define EPNUM_HID_OUT 0x01

// CCID Endpoint Numbers
#define EPNUM_CCID_IN 0x82
#define EPNUM_CCID_OUT 0x02

// Vendor (WebUSB) Endpoint Numbers
#define EPNUM_VENDOR_IN 0x83
#define EPNUM_VENDOR_OUT 0x03

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
