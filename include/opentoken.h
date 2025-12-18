#ifndef OPENTOKEN_H
#define OPENTOKEN_H

#include <stdint.h>

// Definições de constantes e estruturas de dados para o OpenToken
// Exemplo: VID/PID (Vendor ID / Product ID)
#define OPENTOKEN_VID 0x1209 // Exemplo: USB Implementers Forum, Inc.
#define OPENTOKEN_PID 0x0001 // Exemplo: OpenToken Security Key

// Funções de processamento de comandos (declaradas no main.c ou em outros arquivos)
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len);
void opentoken_process_ccid_apdu(uint8_t *buffer, uint16_t len);

#endif // OPENTOKEN_H
