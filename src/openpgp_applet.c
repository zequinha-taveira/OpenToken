#include "openpgp_applet.h"
#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// AID OpenPGP (Exemplo: D2 76 00 01 24 01 01)
const uint8_t OPENPGP_AID[OPENPGP_AID_LEN] = {0xD2, 0x76, 0x00, 0x01,
                                              0x24, 0x01, 0x01};

static bool is_selected = false;

// Helper macro for creating status word response (Appending)
#define SET_SW(sw)                                                             \
  do {                                                                         \
    response[*response_len] = (uint8_t)((sw) >> 8);                            \
    response[*response_len + 1] = (uint8_t)((sw) & 0xFF);                      \
    *response_len += 2;                                                        \
  } while (0)

if (!is_selected) {
  *response_len = 0;
  SET_SW(OPENPGP_SW_FILE_NOT_FOUND);
  return;
}

*response_len = 0;

switch (ins) {
case OPENPGP_INS_VERIFY:
  // P2 indica o PIN a ser verificado (0x81: PIN 1, 0x82: PIN Admin)
  printf("OpenPGP Applet: Recebido comando VERIFY (PIN 0x%02X).\n", p2);

  // Simulação de chamada ao HSM para verificar o PIN
  // Na implementação real, o PIN seria extraído do campo Data
  bool success = hsm_verify_pin(data, lc);
  ;

  if (success) {
    SET_SW(OPENPGP_SW_OK);
  } else {
    // Simulação de tentativas restantes (ex: 3 tentativas)
    SET_SW(OPENPGP_SW_VERIFICATION_FAILED | 0x03);
  }
  break;

case OPENPGP_INS_PSO:
  // P1 P2 indicam a operação (e.g., 0x9E 0x9A para Assinatura)
  printf(
      "OpenPGP Applet: Recebido comando PSO (Perform Security Operation).\n");

  // Simulação de chamada ao HSM para assinatura
  uint8_t signature_out[128];
  uint16_t signature_len = 0;
  bool success_pso = hsm_sign_ecc(data, lc, signature_out, &signature_len);
  ;

  if (success_pso) {
    // Retorna a assinatura (simulada) e o Status Word de sucesso
    memcpy(response, signature_out, signature_len);
    *response_len = signature_len;
    SET_SW(OPENPGP_SW_OK);
  } else {
    SET_SW(OPENPGP_SW_SECURITY_STATUS_NOT_SATISFIED);
  }
  break;

case OPENPGP_INS_GET_DATA:
  // Usado para obter metadados do cartão (e.g., AID, chaves públicas)
  printf("OpenPGP Applet: Recebido comando GET DATA P1=%02X P2=%02X.\n", p1,
         p2);

  uint16_t tag = (p1 << 8) | p2;
  if (tag == 0x004F) { // AID
    memcpy(response, OPENPGP_AID, OPENPGP_AID_LEN);
    *response_len = OPENPGP_AID_LEN;
    SET_SW(OPENPGP_SW_OK);
  } else if (tag == 0x0065) { // Cardholder Related Data
    // Tag 5B Name, 5F2B Sex, 5F35 Lang
    // Return empty/ minimal
    response[0] = 0x00; // Minimal
    *response_len = 1;
    SET_SW(OPENPGP_SW_OK);
  } else if (tag == 0x005E) { // Login Data
    // Payload: Login String
    char *login = "opentoken_user";
    memcpy(response, login, strlen(login));
    *response_len = strlen(login);
    SET_SW(OPENPGP_SW_OK);
  } else if (tag == 0x5F52) { // Historical Bytes
    // Status: Operational
    uint8_t hist[] = {0x00, 0x80, 0x00}; // Dummy
    memcpy(response, hist, 3);
    *response_len = 3;
    SET_SW(OPENPGP_SW_OK);
  } else {
    SET_SW(OPENPGP_SW_FILE_NOT_FOUND); // Not found
  }
  break;

default:
  printf("OpenPGP Applet: Comando INS 0x%02X não suportado.\n", ins);
  SET_SW(OPENPGP_SW_WRONG_P1P2);
  break;
}
