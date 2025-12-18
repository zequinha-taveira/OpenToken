#include "ctap2_engine.h"
#include "hsm_layer.h"
#include <stdio.h>

// Implementação da função de processamento de comandos CTAP2
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len) {
    printf("CTAP2 Engine: Recebido comando com %d bytes.\n", len);

    // O primeiro byte do CTAP2 é o comando (simplificado)
    uint8_t command = buffer[0];

    // Simulação de roteamento de comando
    if (command == 0x01) { // Exemplo: makeCredential
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_GENERATE_KEY, NULL, 0);
        if (hsm_resp.success) {
            printf("CTAP2 Engine: makeCredential processado. Chave pública gerada: %s\n", hsm_resp.data);
            // TODO: Enviar resposta CTAP2 via tud_hid_report
        }
    } else if (command == 0x02) { // Exemplo: getAssertion
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_SIGN, buffer + 1, len - 1);
        if (hsm_resp.success) {
            printf("CTAP2 Engine: getAssertion processado. Assinatura gerada: %s\n", hsm_resp.data);
            // TODO: Enviar resposta CTAP2 via tud_hid_report
        }
    } else {
        printf("CTAP2 Engine: Comando desconhecido (0x%02X).\n", command);
    }
}
