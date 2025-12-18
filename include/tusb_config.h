#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// CONFIGURAÇÃO DO DISPOSITIVO USB
//--------------------------------------------------------------------

#define CFG_TUD_VENDOR_AND_PRODUCT_ID 0x1209, 0x0001 // Exemplo: OpenToken VID/PID
#define CFG_TUD_MAX_POWER_MA          500

// Configuração de classes USB
#define CFG_TUD_CDC                 0
#define CFG_TUD_MSC                 0
#define CFG_TUD_HID                 1 // FIDO2/CTAP2 (Interface 0)
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0
#define CFG_TUD_CCID                1 // OATH/OpenPGP (Interface 1)

//--------------------------------------------------------------------
// CONFIGURAÇÃO HID (FIDO2/CTAP2)
//--------------------------------------------------------------------
#define CFG_TUD_HID_EP_BUFSIZE      64
#define CFG_TUD_HID_REPORT_DESC_LEN 34 // Tamanho do descritor de relatório CTAP2/FIDO2

//--------------------------------------------------------------------
// CONFIGURAÇÃO CCID (OATH/OpenPGP)
//--------------------------------------------------------------------
#define CFG_TUD_CCID_MAX_SLOTS      1
#define CFG_TUD_CCID_EP_BUFSIZE     64

//--------------------------------------------------------------------
// CONFIGURAÇÃO GERAL
//--------------------------------------------------------------------
#define CFG_TUD_ENDPOINT_MAX          4 // 2 para HID (IN/OUT) + 2 para CCID (IN/OUT)
#define CFG_TUD_TASK_QUEUE_LEN        16

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */
