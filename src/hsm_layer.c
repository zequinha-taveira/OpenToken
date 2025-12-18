#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// Implementação simplificada do HSM Layer (apenas para roteamento)
hsm_response_t hsm_execute_command(hsm_command_t cmd, const uint8_t *input_data, uint16_t input_len) {
    hsm_response_t resp = {0};
    resp.success = true;
    resp.len = 0;

    switch (cmd) {
        case HSM_CMD_GENERATE_KEY:
            printf("HSM: Gerando chave...\n");
            // Simulação de chave pública
            resp.len = snprintf((char*)resp.data, 64, "PUBKEY_HSM_SIMULATED");
            break;
        case HSM_CMD_SIGN:
            printf("HSM: Assinando dados...\n");
            // Simulação de assinatura
            resp.len = snprintf((char*)resp.data, 64, "SIGNATURE_HSM_SIMULATED");
            break;
        case HSM_CMD_VERIFY_PIN:
            printf("HSM: Verificando PIN...\n");
            // Simulação de sucesso
            resp.len = 1;
            resp.data[0] = 0x01;
            break;
        case HSM_CMD_CALCULATE_OATH:
            printf("HSM: Calculando OATH...\n");
            // Simulação de código OATH (6 dígitos)
            resp.len = 6;
            resp.data[0] = 0x12; resp.data[1] = 0x34; resp.data[2] = 0x56;
            resp.data[3] = 0x78; resp.data[4] = 0x90; resp.data[5] = 0xab;
            break;
        default:
            resp.success = false;
            break;
    }

    return resp;
}
