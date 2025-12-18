#ifndef OPENTOKEN_H
#define OPENTOKEN_H

#include <stdint.h>
#include "tusb_config.h" // Inclui as definições de configuração do TinyUSB

// Definições de constantes e estruturas de dados para o OpenToken
// Usando as definições de tusb_config.h para VID/PID
#define OPENTOKEN_VID CFG_TUD_VENDOR_AND_PRODUCT_ID
#define OPENTOKEN_PID CFG_TUD_VENDOR_AND_PRODUCT_ID

// Funções de processamento de comandos (declaradas no main.c ou em outros arquivos)
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len);
void opentoken_process_ccid_apdu(uint8_t *buffer, uint16_t len);

#endif // OPENTOKEN_H
