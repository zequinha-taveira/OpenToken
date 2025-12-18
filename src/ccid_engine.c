#include "ccid_engine.h"
#include "hsm_layer.h"
#include <stdio.h>

// Implementação da função de processamento de comandos CCID APDU
void opentoken_process_ccid_apdu(uint8_t *buffer, uint16_t len) {
    printf("CCID Engine: Recebido APDU com %d bytes.\n", len);

    // APDU Structure: CLA INS P1 P2 Lc Data Le
    uint8_t cla = buffer[0];
    uint8_t ins = buffer[1];

    // Simulação de roteamento de comando (OATH vs OpenPGP)
    if (cla == 0x00 && ins == 0xA4) { // SELECT command
        printf("CCID Engine: Comando SELECT recebido.\n");
        // Lógica para determinar se é OATH ou OpenPGP pelo AID
    } else if (cla == 0x00 && ins == 0x20) { // VERIFY command (PIN)
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_VERIFY_PIN, buffer + 5, len - 5);
        if (hsm_resp.success) {
            printf("CCID Engine: PIN verificado com sucesso.\n");
            // TODO: Enviar resposta APDU 90 00
        }
    } else if (cla == 0x00 && ins == 0xB0) { // OATH CALCULATE (simplificado)
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_CALCULATE_OATH, NULL, 0);
        if (hsm_resp.success) {
            printf("CCID Engine: Código OATH calculado.\n");
            // TODO: Enviar resposta APDU com o código
        }
    } else {
        printf("CCID Engine: APDU desconhecido (CLA: 0x%02X, INS: 0x%02X).\n", cla, ins);
    }
}
