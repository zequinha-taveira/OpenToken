#ifndef OPENPGP_APPLET_H
#define OPENPGP_APPLET_H

#include <stdint.h>
#include <stdbool.h>

// Application Identifier (AID) para OpenPGP Card
#define OPENPGP_AID_LEN 7
extern const uint8_t OPENPGP_AID[OPENPGP_AID_LEN];

// Funções de processamento de comandos OpenPGP
bool openpgp_applet_select(const uint8_t *aid, uint8_t len);
void openpgp_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len);

#endif // OPENPGP_APPLET_H
