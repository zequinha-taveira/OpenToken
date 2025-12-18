#include "openpgp_applet.h"
#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// AID OpenPGP (Exemplo: D2 76 00 01 24 01 01)
const uint8_t OPENPGP_AID[OPENPGP_AID_LEN] = {0xD2, 0x76, 0x00, 0x01, 0x24, 0x01, 0x01};

static bool is_selected = false;

// Função auxiliar para definir o Status Word (SW)
#define SET_SW(sw) do { \
    response[0] = (uint8_t)((sw) >> 8); \
    response[1] = (uint8_t)((sw) & 0xFF); \
    *response_len = 2; \
} while(0)

bool openpgp_applet_select(const uint8_t *aid, uint8_t len) {
    if (len == OPENPGP_AID_LEN && memcmp(aid, OPENPGP_AID, OPENPGP_AID_LEN) == 0) {
        is_selected = true;
        printf("OpenPGP Applet: Selecionado.\n");
        return true;
    }
    is_selected = false;
    return false;
}

void openpgp_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len) {
    // APDU Structure: CLA INS P1 P2 Lc Data Le
    uint8_t ins = apdu[1];
    uint8_t p1 = apdu[2];
    uint8_t p2 = apdu[3];
    uint8_t lc = apdu[4];
    const uint8_t *data = apdu + 5;

    if (!is_selected) {
        SET_SW(OPENPGP_SW_FILE_NOT_FOUND);
        return;
    }

    switch (ins) {
        case OPENPGP_INS_VERIFY:
            // P2 indica o PIN a ser verificado (0x81: PIN 1, 0x82: PIN Admin)
            printf("OpenPGP Applet: Recebido comando VERIFY (PIN 0x%02X).\n", p2);
            
            // Simulação de chamada ao HSM para verificar o PIN
            // Na implementação real, o PIN seria extraído do campo Data
            hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_VERIFY_PIN, data, lc);
            
            if (hsm_resp.success) {
                SET_SW(OPENPGP_SW_OK);
            } else {
                // Simulação de tentativas restantes (ex: 3 tentativas)
                SET_SW(OPENPGP_SW_VERIFICATION_FAILED | 0x03);
            }
            break;

        case OPENPGP_INS_PSO:
            // P1 P2 indicam a operação (e.g., 0x9E 0x9A para Assinatura)
            printf("OpenPGP Applet: Recebido comando PSO (Perform Security Operation).\n");
            
            // Simulação de chamada ao HSM para assinatura
            hsm_response_t sign_resp = hsm_execute_command(HSM_CMD_SIGN, data, lc);
            
            if (sign_resp.success) {
                // Retorna a assinatura (simulada) e o Status Word de sucesso
                memcpy(response, sign_resp.data, sign_resp.len);
                *response_len = sign_resp.len;
                SET_SW(OPENPGP_SW_OK);
                *response_len += 2;
            } else {
                SET_SW(OPENPGP_SW_SECURITY_STATUS_NOT_SATISFIED);
            }
            break;

        case OPENPGP_INS_GET_DATA:
            // Usado para obter metadados do cartão (e.g., AID, chaves públicas)
            printf("OpenPGP Applet: Recebido comando GET DATA (Ignorado na simulação).\n");
            // Simulação de resposta vazia com sucesso
            SET_SW(OPENPGP_SW_OK);
            break;

        default:
            printf("OpenPGP Applet: Comando INS 0x%02X não suportado.\n", ins);
            SET_SW(OPENPGP_SW_WRONG_P1P2);
            break;
    }
}
