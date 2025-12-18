#include "openpgp_applet.h"
#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// AID OpenPGP (Exemplo: D2 76 00 01 24 01 01)
const uint8_t OPENPGP_AID[OPENPGP_AID_LEN] = {0xD2, 0x76, 0x00, 0x01, 0x24, 0x01, 0x01};

static bool is_selected = false;

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
    if (!is_selected) {
        // Retornar erro: Applet não selecionado
        response[0] = 0x6A; response[1] = 0x82; // File Not Found
        *response_len = 2;
        return;
    }

    uint8_t ins = apdu[1];

    if (ins == 0x20) { // VERIFY (PIN)
        printf("OpenPGP Applet: Recebido comando VERIFY (PIN).\n");
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_VERIFY_PIN, NULL, 0);
        
        if (hsm_resp.success) {
            response[0] = 0x90; response[1] = 0x00; // Success
        } else {
            response[0] = 0x63; response[1] = 0xC0; // Verification failed
        }
        *response_len = 2;
    } else if (ins == 0x2A) { // PSO: Sign (simplificado)
        printf("OpenPGP Applet: Recebido comando PSO (Sign).\n");
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_SIGN, NULL, 0);
        
        // Simulação de resposta APDU com dados
        memcpy(response, hsm_resp.data, hsm_resp.len);
        response[hsm_resp.len] = 0x90; // SW1
        response[hsm_resp.len + 1] = 0x00; // SW2
        *response_len = hsm_resp.len + 2;
    } else {
        // Comando não suportado
        response[0] = 0x6D; response[1] = 0x00;
        *response_len = 2;
    }
}
