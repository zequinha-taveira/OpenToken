#ifndef CCID_ENGINE_H
#define CCID_ENGINE_H

#include <stdint.h>
#include "opentoken.h" // Para a declaração da função de processamento

// Funções de processamento de comandos CCID APDU
// Implementa a função declarada em opentoken.h
void opentoken_process_ccid_apdu(uint8_t const* buffer, uint16_t len, uint8_t *out_buffer, uint16_t *out_len);

#endif // CCID_ENGINE_H
