#ifndef YUBIKEY_MGMT_H
#define YUBIKEY_MGMT_H

#include <stdint.h>
#include <stdbool.h>

// YubiKey Manager Application Identifier (AID)
#define YUBIKEY_MGMT_AID_LEN 8
extern const uint8_t YUBIKEY_MGMT_AID[YUBIKEY_MGMT_AID_LEN];

// YubiKey OTP AID (for explicit rejection)
#define YUBIKEY_OTP_AID_LEN 8
extern const uint8_t YUBIKEY_OTP_AID[YUBIKEY_OTP_AID_LEN];

// YubiKey Manager INS codes (proprietary commands)
#define YUBIKEY_INS_API_REQUEST      0x01
#define YUBIKEY_INS_OTP_NDEF         0x02
#define YUBIKEY_INS_SET_MODE         0x16
#define YUBIKEY_INS_GET_SERIAL       0x10
#define YUBIKEY_INS_GET_VERSION      0x1D
#define YUBIKEY_INS_RESET            0x1F
#define YUBIKEY_INS_SET_DEVICE_INFO  0x15

// YubiKey Manager capability flags
#define YUBIKEY_CAP_OTP              0x01
#define YUBIKEY_CAP_CCID             0x02
#define YUBIKEY_CAP_FIDO2            0x04
#define YUBIKEY_CAP_OATH             0x08
#define YUBIKEY_CAP_PIV              0x10
#define YUBIKEY_CAP_OPENPGP          0x20

// Supported capabilities (only standard protocols)
#define OPENTOKEN_SUPPORTED_CAPS     (YUBIKEY_CAP_CCID | YUBIKEY_CAP_FIDO2 | YUBIKEY_CAP_OATH | YUBIKEY_CAP_OPENPGP)

// YubiKey Manager compatibility functions
bool yubikey_mgmt_select(const uint8_t *aid, uint8_t aid_len);
void yubikey_mgmt_process_apdu(const uint8_t *apdu, uint16_t len,
                               uint8_t *response, uint16_t *response_len);
void yubikey_mgmt_init_compatibility_layer(void);
void yubikey_mgmt_handle_reconnection(void);

#endif // YUBIKEY_MGMT_H