#ifndef OATH_APPLET_H
#define OATH_APPLET_H

#include <stdint.h>
#include <stdbool.h>

// Application Identifier (AID) para OATH (Yubico)
#define OATH_AID_LEN 8
extern const uint8_t OATH_AID[OATH_AID_LEN];

// Comandos APDU OATH (INS - Instruction Byte)
#define OATH_INS_CALCULATE      0xA2
#define OATH_INS_PUT            0x01
#define OATH_INS_DELETE         0x02
#define OATH_INS_LIST           0x03
#define OATH_INS_SET_CODE       0x04

// Status Words (SW1 SW2)
#define OATH_SW_OK              0x9000
#define OATH_SW_FILE_NOT_FOUND  0x6A82
#define OATH_SW_WRONG_P1P2      0x6A86
#define OATH_SW_COMMAND_NOT_ALLOWED 0x6986

// Funções de processamento de comandos OATH
bool oath_applet_select(const uint8_t *aid, uint8_t len);
void oath_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len);

#endif // OATH_APPLET_H
