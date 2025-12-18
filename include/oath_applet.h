#ifndef OATH_APPLET_H
#define OATH_APPLET_H

#include <stdint.h>
#include <stdbool.h>

// Application Identifier (AID) para OATH (Yubico)
#define OATH_AID_LEN 8
extern const uint8_t OATH_AID[OATH_AID_LEN];

// Funções de processamento de comandos OATH
bool oath_applet_select(const uint8_t *aid, uint8_t len);
void oath_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len);

#endif // OATH_APPLET_H
