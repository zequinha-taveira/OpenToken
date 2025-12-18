#ifndef CTAP2_ENGINE_H
#define CTAP2_ENGINE_H

#include <stdint.h>
#include "opentoken.h" // Para a declaração da função de processamento

// Funções de processamento de comandos CTAP2
// Implementa a função declarada em opentoken.h
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len);

#endif // CTAP2_ENGINE_H
