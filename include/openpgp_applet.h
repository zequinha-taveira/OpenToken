#ifndef OPENPGP_APPLET_H
#define OPENPGP_APPLET_H

#include <stdint.h>
#include <stdbool.h>

// Application Identifier (AID) para OpenPGP Card
#define OPENPGP_AID_LEN 7
extern const uint8_t OPENPGP_AID[OPENPGP_AID_LEN];

// Comandos APDU OpenPGP (INS - Instruction Byte)
#define OPENPGP_INS_SELECT      0xA4
#define OPENPGP_INS_VERIFY      0x20
#define OPENPGP_INS_GET_DATA    0xCA
#define OPENPGP_INS_PUT_DATA    0xDA
#define OPENPGP_INS_PSO         0x2A // Perform Security Operation (e.g., Sign)

// Status Words (SW1 SW2)
#define OPENPGP_SW_OK                   0x9000
#define OPENPGP_SW_FILE_NOT_FOUND       0x6A82
#define OPENPGP_SW_WRONG_P1P2           0x6A86
#define OPENPGP_SW_SECURITY_STATUS_NOT_SATISFIED 0x6982
#define OPENPGP_SW_VERIFICATION_FAILED  0x63C0 // 0x63Cx onde x é o número de tentativas restantes

// Funções de processamento de comandos OpenPGP
bool openpgp_applet_select(const uint8_t *aid, uint8_t len);
void openpgp_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len);

#endif // OPENPGP_APPLET_H
