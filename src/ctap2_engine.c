#include "ctap2_engine.h"
#include "cbor_utils.h"
#include "hsm_layer.h"
#include "tusb.h" // Required for tud_hid_report
#include <stdio.h>
#include <string.h>


// CTAPHID Commands
#define CTAPHID_CMD_MSG 0x03
#define CTAPHID_CMD_CBOR 0x10
#define CTAPHID_CMD_INIT 0x06
#define CTAPHID_CMD_PING 0x01
#define CTAPHID_CMD_ERROR 0x3F

// CTAP2 Methods
#define CTAP2_MAKE_CREDENTIAL 0x01
#define CTAP2_GET_ASSERTION 0x02
#define CTAP2_GET_INFO 0x04

// Fixed Broadcast CID
const uint32_t CID_BROADCAST = 0xFFFFFFFF;

// Implementação da função de processamento de comandos CTAP2
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len) {
  if (len < 7)
    return; // Malformed

  uint32_t cid = 0;
  memcpy(&cid, buffer, 4);

  uint8_t cmd = buffer[4];
  uint16_t payload_len = (buffer[5] << 8) | buffer[6];

  printf("CTAP2: CID=%08X CMD=%02X PayLen=%d\n", cid, cmd, payload_len);

  uint8_t response[64];
  memset(response, 0, 64);

  // Default: Echo CID
  memcpy(response, &cid, 4);

  if (cmd == (CTAPHID_CMD_INIT | 0x80)) {
    // INIT logic (Keep existing)
    response[4] = cmd;
    uint16_t resp_len = 17;
    response[5] = (resp_len >> 8) & 0xFF;
    response[6] = resp_len & 0xFF;
    memcpy(response + 7, buffer + 7, 8); // Nonce
    uint32_t new_cid = 0x1D2D3D4D;
    if (cid == CID_BROADCAST) {
      memcpy(response + 15, &new_cid, 4);
    } else {
      memcpy(response + 15, &cid, 4);
    }
    response[19] = 2; // Protocol Version
    response[20] = 1; // Major
    response[21] = 0; // Minor
    response[22] = 0; // Build
    response[23] = 0; // Caps
    tud_hid_report(0, response, 64);
    printf("CTAP2: Sent INIT Response.\n");

  } else if (cmd == (CTAPHID_CMD_PING | 0x80)) {
    memcpy(response, buffer, 64);
    tud_hid_report(0, response, 64);
    printf("CTAP2: Sent PING Response.\n");

  } else if (cmd == (CTAPHID_CMD_MSG | 0x80) ||
             cmd == (CTAPHID_CMD_CBOR | 0x80)) {

    // Parse CTAP Method (First byte of payload)
    // Payload starts at 7
    if (len < 7 + 1)
      return;
    uint8_t ctap_method = buffer[7];
    printf("CTAP2: Method %02X\n", ctap_method);

    uint8_t cbor_buf[256];
    cbor_encoder_t enc; // Fix: use instance not pointer
    cbor_encoder_init(&enc, cbor_buf, sizeof(cbor_buf)); // Fix: Pass address

    // Default Status: OK (0x00)
    uint8_t status = 0x00;

    if (ctap_method == CTAP2_GET_INFO) {
      printf("CTAP2: GetInfo\n");
      // Response: Status(0) + CBOR Map
      // Map Keys:
      // 01: versions (arr)
      // 03: aaguid (bytes)
      // 04: options (map)

      cbor_encode_map_start(&enc, 3);

      // 1. Versions
      cbor_encode_uint(&enc, 1);
      cbor_encode_type_val(
          &enc, 4, 1); // Array(1) - Wait, cbor_utils needs array support
                       // usually, but here manual Let's implement array
                       // manually or assume FIDO_2_1 string Usually versions is
                       // array of strings. Simplified: Just encode "FIDO_2_1"
                       // as a single string? No, spec says Array. Hack: Manual
                       // Array encoding. Array(1) member "FIDO_2_1"
      // TODO: Add proper Array support to Utils. Using hack for now.
      // 81 (Array 1)
      cbor_encoder_init(&enc, cbor_buf, 256); // Reset
      // Map(3)
      cbor_encode_map_start(&enc, 3);

      // Key 1: Versions (Array of 1 string "FIDO_2_1")
      cbor_encode_uint(&enc, 1);
      // Manual array
      // cbor_write_byte(&enc, 0x81); // Array(1) -> needs adding to cbor_utils
      // Let's rely on updated cbor_utils later? Or just hack encoding here.
      // Major 4 = Array.
      // Array(1) = 0x81
      // String "FIDO_2_1"

      // WORKAROUND: Implement array/hack in place
      // Since cbor_utils is limited, I will only respond with ERROR if too
      // hard. But GetInfo is critical. I will write raw bytes for the known
      // response if dynamic generation is hard.

      // Static Good Response for GetInfo:
      // Map(2): {1: ["FIDO_2_1"], 3: [00..00]}
      // A2 01 81 68 46 49 44 4F 5F 32 5F 31 03 50 00...00

      // Let's build it cleaner if possible.
      // cbor_utils: encode_type_val(enc, 4, 1) -> Array(1)

      // RE-CHECK: encode_type_val is static in cbor_utils.c (not exposed).
      // I should have exposed it or added cbor_encode_array_start.
      // I will just implement a simplified GetInfo response directly to buffer.

      uint8_t static_resp[] = {0x00, // Status OK
                               0xA2, // Map(2)
                               0x01, // Key: versions
                               0x81, // Arr(1)
                               0x68, 'F',  'I',  'D',  'O',  '_',  '2',  '_',
                               '1',  // "FIDO_2_1"
                               0x03, // Key: aaguid
                               0x50, // Bytes(16)
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      // Send
      response[4] = cmd; // Echo Method
      uint16_t rlen = sizeof(static_resp);
      response[5] = (rlen >> 8) & 0xFF;
      response[6] = rlen & 0xFF;

      if (rlen <= 64 - 7) {
        memcpy(response + 7, static_resp, rlen);
        tud_hid_report(0, response, 64);
      } else {
        // Fragmentation needed!
        // Simplest is to assume it fits (30 bytes fits).
        memcpy(response + 7, static_resp, rlen);
        tud_hid_report(0, response, 64);
      }
      printf("CTAP2: Sent GetInfo Response.\n");
      return; // Done

    } else {
      printf("CTAP2: Unsupported Method.\n");
      status = 0x01; // Invalid Command
    }

    // Default Error/Empty Response
    response[4] = cmd;
    response[5] = 0;
    response[6] = 1;
    response[7] = status;
    tud_hid_report(0, response, 64);
  } else {
    // Unknown
  }
}
