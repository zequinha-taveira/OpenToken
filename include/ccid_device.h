#ifndef _CCID_DEVICE_H_
#define _CCID_DEVICE_H_

#include "common/tusb_common.h"

#ifdef __cplusplus
 extern "C" {
#endif

// CCID Class Driver Constants
#define TUSB_CLASS_CCID              TUSB_CLASS_SMART_CARD

// CCID Descriptor Length
#define TUD_CCID_DESC_LEN            54

// CCID Status Codes
#define CCID_STATUS_SUCCESS          0x00

// CCID Descriptor Template
#define TUD_CCID_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _bufsize) \
  /* Interface Descriptor */ \
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, TUSB_CLASS_CCID, 0, 0, _stridx, \
  /* CCID Functional Descriptor */ \
  54, 0x21, /* bDescriptorType = Functional */ \
  U16_TO_U8S_LE(0x0110), /* bcdCCID = 1.10 */ \
  0, /* bMaxSlotIndex */ \
  0x07, /* bVoltageSupport = 5V, 3V, 1.8V */ \
  U32_TO_U8S_LE(0x00000002), /* dwProtocols = T=1 */ \
  U32_TO_U8S_LE(4000), /* dwDefaultClock = 4MHz */ \
  U32_TO_U8S_LE(4000), /* dwMaximumClock = 4MHz */ \
  0, /* bNumClockSupported */ \
  U32_TO_U8S_LE(9600), /* dwDataRate = 9600 bps */ \
  U32_TO_U8S_LE(9600), /* dwMaxDataRate = 9600 bps */ \
  0, /* bNumDataRatesSupported */ \
  U32_TO_U8S_LE(0x000000FE), /* dwMaxIFSD = 254 */ \
  U32_TO_U8S_LE(0x00000000), /* dwSynchProtocols */ \
  U32_TO_U8S_LE(0x00000000), /* dwMechanical */ \
  U32_TO_U8S_LE(0x00020440), /* dwFeatures: Short APDU, Automatic BAUD, Automatic clock */ \
  U32_TO_U8S_LE(0x00000100), /* dwMaxCCIDMessageLength = 256 */ \
  0x00, /* bClassGetResponse */ \
  0x00, /* bClassEnvelope */ \
  U16_TO_U8S_LE(0x0000), /* wLcdLayout */ \
  0x00, /* bPINSupport */ \
  0x01, /* bMaxCCIDBusySlots */ \
  /* Bulk OUT Endpoint */ \
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_BULK, U16_TO_U8S_LE(_bufsize), 0, \
  /* Bulk IN Endpoint */ \
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_BULK, U16_TO_U8S_LE(_bufsize), 0

// Prototype for user callbacks (implemented in usb_descriptors.c usually)
void tud_ccid_icc_power_on_response(uint8_t slot, uint8_t status, uint8_t error, uint8_t const* atr, uint16_t atr_len);
void tud_ccid_icc_power_off_response(uint8_t slot, uint8_t status, uint8_t error);
void tud_ccid_xfr_block_response(uint8_t slot, uint8_t status, uint8_t error, uint8_t const* response, uint16_t response_len);

#ifdef __cplusplus
 }
#endif

#endif
