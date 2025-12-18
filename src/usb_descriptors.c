#include "tusb.h"
#include "tusb_config.h"
#include "opentoken.h"

//--------------------------------------------------------------------+
// DESCRITORES DE DISPOSITIVO
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0
    
    // Dispositivo Composto (Interface Association Descriptor - IAD)
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT_MAX,

    .idVendor           = OPENTOKEN_VID,
    .idProduct          = OPENTOKEN_PID,
    .bcdDevice          = 0x0100, // Versão 1.00

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

//--------------------------------------------------------------------+
// DESCRITORES DE CONFIGURAÇÃO
//--------------------------------------------------------------------+
enum
{
  ITF_NUM_HID,
  ITF_NUM_CCID,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CCID_DESC_LEN)

uint8_t const desc_configuration[] =
{
  // Configuração Principal
  TUD_CONFIG_DESCRIPTOR(1, CONFIG_TOTAL_LEN, ITF_NUM_TOTAL, 0, 500, 0x00),

  // 1. Interface HID (FIDO2/CTAP2)
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, CFG_TUD_HID_REPORT_DESC_LEN, 0x81, 0x01, CFG_TUD_HID_EP_BUFSIZE, 1),

  // 2. Interface CCID (OATH/OpenPGP)
  TUD_CCID_DESCRIPTOR(ITF_NUM_CCID, 0, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF),
};

//--------------------------------------------------------------------+
// DESCRITOR DE RELATÓRIO HID (CTAP2/FIDO2)
//--------------------------------------------------------------------+
uint8_t const desc_hid_report[] =
{
  // Descritor de Relatório FIDO/CTAP2 (64 bytes IN/OUT)
  0x06, 0xd0, 0xf1, // Usage Page (FIDO Alliance)
  0x09, 0x01,       // Usage (CTAP)
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
// DESCRITORES DE STRING
//--------------------------------------------------------------------+
char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: Suporte a Inglês (US)
  "OpenToken",                    // 1: Fabricante
  "OpenToken Security Key",       // 2: Produto
  "1234567890",                   // 3: Número de Série
  "OpenToken CCID Interface"      // 4: Interface CCID
};

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;
  size_t chr_count;

  if ( index == 0)
  {
    return (uint16_t*) string_desc_arr[0];
  }

  if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) return NULL;

  const char* str = string_desc_arr[index];

  // Converte string C para string UTF-16LE
  static uint16_t utf16_buf[32];
  chr_count = strlen(str);
  if ( chr_count > 31 ) chr_count = 31;

  for(uint8_t i=0; i<chr_count; i++)
  {
    utf16_buf[i+1] = str[i];
  }

  // Primeiro byte é o tamanho total (em bytes), segundo é o tipo (TUSB_DESC_STRING)
  utf16_buf[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return utf16_buf;
}

//--------------------------------------------------------------------+
// CALLBACKS DE RELATÓRIO HID
//--------------------------------------------------------------------+
uint8_t const * tud_hid_get_report_descriptor_cb(uint8_t instance)
{
  (void) instance;
  return desc_hid_report;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;
  (void) report_id;
  (void) report_type;

  // Aqui o comando CTAP2/FIDO2 é recebido via USB HID
  opentoken_process_ctap2_command((uint8_t*)buffer, bufsize);

  // Se for um comando que requer resposta imediata, envie o relatório IN
  // tud_hid_report(0x81, buffer, bufsize);
}

//--------------------------------------------------------------------+
// CALLBACKS CCID
//--------------------------------------------------------------------+
void tud_ccid_set_param_cb(uint8_t param_num, uint8_t const* buffer, uint16_t bufsize)
{
  // Placeholder para processamento de parâmetros CCID
  (void) param_num;
  (void) buffer;
  (void) bufsize;
}

void tud_ccid_icc_power_on_cb(uint8_t slot, uint32_t voltage)
{
  // Placeholder para ligar o ICC (Smartcard)
  (void) slot;
  (void) voltage;
  tud_ccid_icc_power_on_response(slot, CCID_STATUS_SUCCESS, 0, NULL, 0);
}

void tud_ccid_xfr_block_cb(uint8_t slot, uint8_t const* buffer, uint16_t bufsize)
{
  // Aqui o APDU (OATH/OpenPGP) é recebido via USB CCID
  opentoken_process_ccid_apdu((uint8_t*)buffer, bufsize);

  // Resposta APDU (placeholder)
  uint8_t response[] = {0x90, 0x00};
  tud_ccid_xfr_block_response(slot, CCID_STATUS_SUCCESS, 0, response, sizeof(response));
}
