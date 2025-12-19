#include "ccid_device.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct {
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;

  /* State for current transfer */
  uint8_t epout_buf[CFG_TUD_CCID_EP_BUFSIZE];
  uint8_t epin_buf[CFG_TUD_CCID_EP_BUFSIZE];
} ccid_interface_t;

static ccid_interface_t _ccid_itf;

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// CALLBACKS IMPLEMENTED IN USB_DESCRIPTORS.C
//--------------------------------------------------------------------+
extern void tud_ccid_icc_power_on_cb(uint8_t slot, uint32_t voltage);
extern void tud_ccid_icc_power_off_cb(uint8_t slot);
extern void tud_ccid_xfr_block_cb(uint8_t slot, uint8_t const *buffer,
                                  uint16_t bufsize);

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
void tud_ccid_icc_power_on_response(uint8_t slot, uint8_t status, uint8_t error,
                                    uint8_t const *atr, uint16_t atr_len) {
  (void)slot;
  uint8_t res[10 + atr_len];
  res[0] = 0x80; // RDR_to_PC_DataBlock
  res[1] = atr_len & 0xFF;
  res[2] = (atr_len >> 8) & 0xFF;
  res[3] = (atr_len >> 16) & 0xFF;
  res[4] = (atr_len >> 24) & 0xFF;
  res[5] = 0; // Slot
  res[6] = 0; // Seq
  res[7] = status;
  res[8] = error;
  res[9] = 0; // Chain
  if (atr_len > 0)
    memcpy(res + 10, atr, atr_len);

  usbd_edpt_xfer(0, _ccid_itf.ep_in, res, sizeof(res));
}

void tud_ccid_icc_power_off_response(uint8_t slot, uint8_t status,
                                     uint8_t error) {
  (void)slot;
  uint8_t res[10];
  res[0] = 0x81; // RDR_to_PC_SlotStatus
  res[1] = 0;
  res[2] = 0;
  res[3] = 0;
  res[4] = 0;
  res[5] = 0;
  res[6] = 0;
  res[7] = status;
  res[8] = error;
  res[9] = 0;

  usbd_edpt_xfer(0, _ccid_itf.ep_in, res, sizeof(res));
}

void tud_ccid_xfr_block_response(uint8_t slot, uint8_t status, uint8_t error,
                                 uint8_t const *response,
                                 uint16_t response_len) {
  (void)slot;
  uint16_t total_len = 10 + response_len;
  uint8_t res[total_len];
  res[0] = 0x80; // RDR_to_PC_DataBlock
  res[1] = response_len & 0xFF;
  res[2] = (response_len >> 8) & 0xFF;
  res[3] = (response_len >> 16) & 0xFF;
  res[4] = (response_len >> 24) & 0xFF;
  res[5] = 0;
  res[6] = 0;
  res[7] = status;
  res[8] = error;
  res[9] = 0;
  if (response_len > 0)
    memcpy(res + 10, response, response_len);

  usbd_edpt_xfer(0, _ccid_itf.ep_in, res, total_len);
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void ccid_init(void) { memset(&_ccid_itf, 0, sizeof(ccid_interface_t)); }

void ccid_reset(uint8_t rhport) {
  (void)rhport;
  memset(&_ccid_itf, 0, sizeof(ccid_interface_t));
}

uint16_t ccid_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                   uint16_t max_len) {
  TU_VERIFY(TUSB_CLASS_CCID == itf_desc->bInterfaceClass, 0);

  uint16_t const drv_len = sizeof(tusb_desc_interface_t) + TUD_CCID_DESC_LEN +
                           2 * sizeof(tusb_desc_endpoint_t);
  TU_VERIFY(max_len >= drv_len, 0);

  _ccid_itf.itf_num = itf_desc->bInterfaceNumber;

  uint8_t const *p_desc = tu_desc_next(itf_desc);

  // Skip CCID Functional Descriptor
  p_desc = tu_desc_next(p_desc);

  // Open Endpoints
  TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, 2, TUSB_XFER_BULK,
                                &_ccid_itf.ep_out, &_ccid_itf.ep_in),
            0);

  // Prepare for first OUT packet
  usbd_edpt_xfer(rhport, _ccid_itf.ep_out, _ccid_itf.epout_buf,
                 sizeof(_ccid_itf.epout_buf));

  return drv_len;
}

bool ccid_control_xfer_cb(uint8_t rhport, uint8_t stage,
                          tusb_control_request_t const *request) {
  (void)rhport;
  (void)stage;
  (void)request;
  return false; // No control requests supported yet
}

bool ccid_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                  uint32_t xferred_bytes) {
  (void)result;

  if (ep_addr == _ccid_itf.ep_out) {
    if (xferred_bytes >= 10) {
      uint8_t msg_type = _ccid_itf.epout_buf[0];
      uint8_t slot = _ccid_itf.epout_buf[5];

      if (msg_type == 0x62) // PC_to_RDR_IccPowerOn
      {
        tud_ccid_icc_power_on_cb(slot, 0);
      } else if (msg_type == 0x63) // PC_to_RDR_IccPowerOff
      {
        tud_ccid_icc_power_off_cb(slot);
      } else if (msg_type == 0x6f) // PC_to_RDR_XfrBlock
      {
        uint32_t data_len =
            tu_le32toh(*((uint32_t const *)(_ccid_itf.epout_buf + 1)));
        tud_ccid_xfr_block_cb(slot, _ccid_itf.epout_buf + 10,
                              (uint16_t)data_len);
      }
    }

    // Prepare for next OUT packet
    return usbd_edpt_xfer(rhport, _ccid_itf.ep_out, _ccid_itf.epout_buf,
                          sizeof(_ccid_itf.epout_buf));
  }

  return true;
}

static usbd_class_driver_t const _ccid_driver = {
#if CFG_TUSB_DEBUG >= 2
    .name = "CCID",
#endif
    .init = ccid_init,
    .reset = ccid_reset,
    .open = ccid_open,
    .control_xfer_cb = ccid_control_xfer_cb,
    .xfer_cb = ccid_xfer_cb,
    .sof = NULL};

// This function is required by TinyUSB to get the application-level drivers
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;
  return &_ccid_driver;
}
