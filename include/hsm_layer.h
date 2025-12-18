#ifndef HSM_LAYER_H
#define HSM_LAYER_H

#include <stdint.h>
#include <stdbool.h>

// Definições de comandos HSM (simplificadas)
typedef enum {
    HSM_CMD_GENERATE_KEY,
    HSM_CMD_SIGN,
    HSM_CMD_VERIFY_PIN,
    HSM_CMD_CALCULATE_OATH
} hsm_command_t;

// Estrutura de resposta HSM (simplificada)
typedef struct {
    bool success;
    uint8_t data[64];
    uint16_t len;
} hsm_response_t;

// Funções de interface do HSM
hsm_response_t hsm_execute_command(hsm_command_t cmd, const uint8_t *input_data, uint16_t input_len);

#endif // HSM_LAYER_H
