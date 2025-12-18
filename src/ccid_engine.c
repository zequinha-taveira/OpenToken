#include "ccid_engine.h"
#include "hsm_layer.h"
#include "oath_applet.h"
#include "openpgp_applet.h"
#include <stdio.h>
#include <string.h>

// Variável global para rastrear o applet selecionado
typedef enum { APPLET_NONE, APPLET_OATH, APPLET_OPENPGP } ccid_applet_t;

static ccid_applet_t current_applet = APPLET_NONE;

// Implementação da função de processamento de comandos CCID APDU
void opentoken_process_ccid_apdu(uint8_t const *buffer, uint16_t len,
                                 uint8_t *out_buffer, uint16_t *out_len) {
  printf("CCID Engine: Recebido APDU com %d bytes.\n", len);

  // APDU Structure: CLA INS P1 P2 Lc Data Le
  uint8_t cla = buffer[0];
  uint8_t ins = buffer[1];
  uint8_t p1 = buffer[2];
  uint8_t p2 = buffer[3];
  uint8_t lc = buffer[4];
  const uint8_t *data = buffer + 5;

  // Inicializa o tamanho da resposta
  *out_len = 0;

  if (cla == 0x00 && ins == 0xA4 && p1 == 0x04 &&
      p2 == 0x00) { // SELECT command (by AID)
    printf("CCID Engine: Comando SELECT AID recebido.\n");

    if (oath_applet_select(data, lc)) {
      current_applet = APPLET_OATH;
      out_buffer[0] = 0x90;
      out_buffer[1] = 0x00; // Success
      *out_len = 2;
    } else if (openpgp_applet_select(data, lc)) {
      current_applet = APPLET_OPENPGP;
      out_buffer[0] = 0x90;
      out_buffer[1] = 0x00; // Success
      *out_len = 2;
    } else {
      current_applet = APPLET_NONE;
      out_buffer[0] = 0x6A;
      out_buffer[1] = 0x82; // File Not Found
      *out_len = 2;
    }
  } else {
    // Roteamento para o applet selecionado
    switch (current_applet) {
    case APPLET_OATH:
      oath_applet_process_apdu(buffer, len, out_buffer, out_len);
      break;
    case APPLET_OPENPGP:
      openpgp_applet_process_apdu(buffer, len, out_buffer, out_len);
      break;
    case APPLET_NONE:
    default:
      printf("CCID Engine: Nenhum applet selecionado. APDU ignorado.\n");
      out_buffer[0] = 0x6E;
      out_buffer[1] = 0x00; // Class not supported
      *out_len = 2;
      break;
    }
  }

  printf("CCID Engine: Resposta APDU gerada (Len: %d).\n", *out_len);
}
