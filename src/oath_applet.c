#include "oath_applet.h"
#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// AID OATH (Exemplo: A0 00 00 05 27 21 01 01 - Yubico OATH)
const uint8_t OATH_AID[OATH_AID_LEN] = {0xA0, 0x00, 0x00, 0x05, 0x27, 0x21, 0x01, 0x01};

static bool is_selected = false;

bool oath_applet_select(const uint8_t *aid, uint8_t len) {
    if (len == OATH_AID_LEN && memcmp(aid, OATH_AID, OATH_AID_LEN) == 0) {
        is_selected = true;
        printf("OATH Applet: Selecionado.\n");
        return true;
    }
    is_selected = false;
    return false;
}

void oath_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len) {
    if (!is_selected) {
        // Retornar erro: Applet não selecionado
        response[0] = 0x6A; response[1] = 0x82; // File Not Found
        *response_len = 2;
        return;
    }

    uint8_t ins = apdu[1];

    if (ins == 0xB0) { // Exemplo: CALCULATE APDU (simplificado)
        printf("OATH Applet: Recebido comando CALCULATE.\n");
        hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_CALCULATE_OATH, NULL, 0);
        
        // Simulação de resposta APDU com dados
        response[0] = 0x00; // Tag
        response[1] = hsm_resp.len; // Length
        memcpy(response + 2, hsm_resp.data, hsm_resp.len);
        response[2 + hsm_resp.len] = 0x90; // SW1
        response[3 + hsm_resp.len] = 0x00; // SW2
        *response_len = hsm_resp.len + 4;
    } else {
        // Comando não suportado
        response[0] = 0x6D; response[1] = 0x00;
        *response_len = 2;
    }
}
