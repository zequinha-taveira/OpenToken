#ifndef OATH_APPLET_H
#define OATH_APPLET_H

#include <stdbool.h>
#include <stdint.h>


// Application Identifier (AID) para OATH (Yubico)
#define OATH_AID_LEN 8
extern const uint8_t OATH_AID[OATH_AID_LEN];

// OATH APDU Commands (INS - Instruction Byte)
#define OATH_INS_PUT 0x01
#define OATH_INS_DELETE 0x02
#define OATH_INS_SET_CODE 0x03
#define OATH_INS_RESET 0x04
#define OATH_INS_LIST 0xA1
#define OATH_INS_CALCULATE 0xA2
#define OATH_INS_VALIDATE 0xA3
#define OATH_INS_CALCULATE_ALL 0xA4

// OATH TLV Tags
#define OATH_TAG_NAME 0x71
#define OATH_TAG_NAME_LIST 0x72
#define OATH_TAG_KEY 0x73
#define OATH_TAG_CHALLENGE 0x74
#define OATH_TAG_PROPERTY 0x75
#define OATH_TAG_RESPONSE_VAL 0x76
#define OATH_TAG_NO_RESP 0x77
#define OATH_TAG_EXTENDED_KEY 0x78

// OATH Algorithm Types (for property byte)
#define OATH_TYPE_HOTP 0x10
#define OATH_TYPE_TOTP 0x20

// OATH Hash Algorithms (for property byte)
#define OATH_HASH_SHA1 0x01
#define OATH_HASH_SHA256 0x02

// Status Words (SW1 SW2)
#define OATH_SW_OK 0x9000
#define OATH_SW_FILE_NOT_FOUND 0x6A82
#define OATH_SW_WRONG_P1P2 0x6A86
#define OATH_SW_COMMAND_NOT_ALLOWED 0x6986
#define OATH_SW_INCORRECT_P1P2 0x6A86
#define OATH_SW_WRONG_LENGTH 0x6700
#define OATH_SW_SECURITY_STATUS_NOT_SATISFIED 0x6982

// Funções de processamento de comandos OATH
bool oath_applet_select(const uint8_t *aid, uint8_t len);
void oath_applet_process_apdu(const uint8_t *apdu, uint16_t len,
                              uint8_t *response, uint16_t *response_len);

#endif // OATH_APPLET_H
