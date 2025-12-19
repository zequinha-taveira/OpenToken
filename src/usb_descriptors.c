#include "opentoken.h"
#include "tusb.h"
#include "tusb_config.h"
#include "ccid_device.h"


//--------------------------------------------------------------------+
// USB DEVICE DESCRIPTOR
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200, // USB 2.0

    // Composite Device Configuration
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = OPENTOKEN_VID,
    .idProduct = OPENTOKEN_PID,
    .bcdDevice = 0x0100, // Version 1.00

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};

//--------------------------------------------------------------------+
// USB CONFIGURATION DESCRIPTOR
//--------------------------------------------------------------------+
enum { 
    ITF_NUM_HID,    // Interface 0: HID (FIDO2/CTAP2)
    ITF_NUM_CCID,   // Interface 1: CCID (OATH/OpenPGP)
    ITF_NUM_TOTAL 
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CCID_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Configuration Descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 500),

    // Interface 0: HID (FIDO2/CTAP2) - with interface string descriptor
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_NONE, 
                       CFG_TUD_HID_REPORT_DESC_LEN, EPNUM_HID_IN, 
                       CFG_TUD_HID_EP_BUFSIZE, 1),

    // Interface 1: CCID (OATH/OpenPGP) - with interface string descriptor
    TUD_CCID_DESCRIPTOR(ITF_NUM_CCID, 4, EPNUM_CCID_OUT, EPNUM_CCID_IN, 
                        CFG_TUD_CCID_EP_BUFSIZE)
};

//--------------------------------------------------------------------+
// HID REPORT DESCRIPTOR (FIDO2/CTAP2)
//--------------------------------------------------------------------+
uint8_t const desc_hid_report[] = {
    // FIDO Alliance HID Report Descriptor for CTAP2
    0x06, 0xd0, 0xf1, // Usage Page (FIDO Alliance)
    0x09, 0x01,       // Usage (CTAP HID)
    0xa1, 0x01,       // Collection (Application)
    0x09, 0x20,       //   Usage (Input Report Data)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x40,       //   Report Count (64)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    0x09, 0x21,       //   Usage (Output Report Data)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x40,       //   Report Count (64)
    0x91, 0x02,       //   Output (Data, Variable, Absolute)
    0xc0              // End Collection
};

//--------------------------------------------------------------------+
// USB STRING DESCRIPTORS
//--------------------------------------------------------------------+
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: Language (English US)
    "OpenToken Project",        // 1: Manufacturer (brand-neutral)
    "FIDO2 Security Key",       // 2: Product (FIDO2 compatible description)
    "000000000001",             // 3: Serial Number
    "CCID Interface",           // 4: CCID Interface Description
    "FIDO2 Interface"           // 5: HID Interface Description
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t chr_count;

  if (index == 0) {
    return (uint16_t *)string_desc_arr[0];
  }

  if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
    return NULL;

  const char *str = string_desc_arr[index];

  // Converte string C para string UTF-16LE
  static uint16_t utf16_buf[32];
  chr_count = strlen(str);
  if (chr_count > 31)
    chr_count = 31;

  for (uint8_t i = 0; i < chr_count; i++) {
    utf16_buf[i + 1] = str[i];
  }

  // Primeiro byte é o tamanho total (em bytes), segundo é o tipo
  // (TUSB_DESC_STRING)
  utf16_buf[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

  return utf16_buf;
}

//--------------------------------------------------------------------+
// HID CALLBACKS
//--------------------------------------------------------------------+
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, 
                               hid_report_type_t report_type, uint8_t* buffer, 
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;

  // Process CTAP2/FIDO2 command received via USB HID
  opentoken_process_ctap2_command((uint8_t *)buffer, bufsize);
}

//--------------------------------------------------------------------+
// CCID CALLBACKS
//--------------------------------------------------------------------+
void tud_ccid_set_param_cb(uint8_t param_num, uint8_t const *buffer,
                           uint16_t bufsize) {
  // CCID parameter setting - not used for basic smartcard operation
  (void)param_num;
  (void)buffer;
  (void)bufsize;
}

void tud_ccid_icc_power_on_cb(uint8_t slot, uint32_t voltage) {
  // ICC (smartcard) power on - simulate successful power on
  (void)slot;
  (void)voltage;
  
  // Return ATR (Answer To Reset) - basic ISO7816 compliant ATR
  uint8_t atr[] = {0x3B, 0x00}; // Minimal ATR: Direct convention, no historical bytes
  tud_ccid_icc_power_on_response(slot, CCID_STATUS_SUCCESS, 0, atr, sizeof(atr));
}

void tud_ccid_icc_power_off_cb(uint8_t slot) {
  // ICC power off
  (void)slot;
  tud_ccid_icc_power_off_response(slot, CCID_STATUS_SUCCESS, 0);
}

void tud_ccid_xfr_block_cb(uint8_t slot, uint8_t const *buffer,
                           uint16_t bufsize) {
  // APDU processing buffer (max APDU size + response codes)
  uint8_t response[280];
  uint16_t response_len = 0;

  // Process APDU (OATH/OpenPGP) received via USB CCID
  opentoken_process_ccid_apdu(buffer, bufsize, response, &response_len);

  // Send APDU response back to host
  tud_ccid_xfr_block_response(slot, CCID_STATUS_SUCCESS, 0, response, response_len);
}
