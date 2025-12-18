#ifndef OPENTOKEN_H
#define OPENTOKEN_H

#include "tusb_config.h" // Inclui as definições de configuração do TinyUSB
#include <stdint.h>


// Definições de constantes e estruturas de dados para o OpenToken
// Usando as definições de tusb_config.h para VID/PID
#define OPENTOKEN_VID CFG_TUD_VENDOR_AND_PRODUCT_ID
#define OPENTOKEN_PID CFG_TUD_VENDOR_AND_PRODUCT_ID

// Funções de processamento de comandos (declaradas no main.c ou em outros
// arquivos)
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len);
void opentoken_process_ccid_apdu(uint8_t const *buffer, uint16_t len,
                                 uint8_t *out_buffer, uint16_t *out_len);

#endif // OPENTOKEN_H
