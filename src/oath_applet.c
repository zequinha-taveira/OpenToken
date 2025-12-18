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
    // APDU Structure: CLA INS P1 P2 Lc Data Le
    uint8_t ins = apdu[1];
    uint8_t p1 = apdu[2];
    uint8_t p2 = apdu[3];
    uint8_t lc = apdu[4];
    const uint8_t *data = apdu + 5;

    // Função auxiliar para definir o Status Word (SW)
    #define SET_SW(sw) do { \
        response[0] = (uint8_t)((sw) >> 8); \
        response[1] = (uint8_t)((sw) & 0xFF); \
        *response_len = 2; \
    } while(0)

    if (!is_selected) {
        SET_SW(OATH_SW_FILE_NOT_FOUND);
        return;
    }

    switch (ins) {
        case OATH_INS_CALCULATE:
            // O comando CALCULATE é complexo e requer parsing do TLV no campo Data.
            // Aqui, simulamos apenas a chamada ao HSM.
            printf("OATH Applet: Recebido comando CALCULATE.\n");
            
            // Simulação de chamada ao HSM para calcular o código
            hsm_response_t hsm_resp = hsm_execute_command(HSM_CMD_CALCULATE_OATH, data, lc);
            
            if (hsm_resp.success) {
                // Resposta OATH é um TLV (Tag, Length, Value)
                // Tag 0x70: Response Data
                // Tag 0x80: OATH Code
                
                // Simulação de resposta TLV (0x70 [Length] 0x80 [Length] [Code])
                uint8_t code_tag = 0x80;
                uint8_t code_len = hsm_resp.len;
                
                response[0] = 0x70; // Tag 70
                response[1] = 2 + code_len; // Length of 0x80 + code
                response[2] = code_tag; // Tag 80
                response[3] = code_len; // Length of code
                memcpy(response + 4, hsm_resp.data, code_len); // Code
                
                *response_len = 4 + code_len;
                
                // Adiciona o Status Word de sucesso
                response[*response_len] = (uint8_t)(OATH_SW_OK >> 8);
                response[*response_len + 1] = (uint8_t)(OATH_SW_OK & 0xFF);
                *response_len += 2;
            } else {
                SET_SW(OATH_SW_COMMAND_NOT_ALLOWED);
            }
            break;

        case OATH_INS_PUT:
            printf("OATH Applet: Recebido comando PUT (Ignorado na simulação).\n");
            SET_SW(OATH_SW_OK);
            break;

        case OATH_INS_LIST:
            printf("OATH Applet: Recebido comando LIST (Ignorado na simulação).\n");
            // Simulação de lista vazia
            SET_SW(OATH_SW_OK);
            break;

        default:
            printf("OATH Applet: Comando INS 0x%02X não suportado.\n", ins);
            SET_SW(OATH_SW_WRONG_P1P2);
            break;
    }
}
