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
      // (Keep existing GetInfo logic)
      printf("CTAP2: GetInfo\n");
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
      response[4] = cmd;
      uint16_t rlen = sizeof(static_resp);
      response[5] = (rlen >> 8) & 0xFF;
      response[6] = rlen & 0xFF;
      memcpy(response + 7, static_resp, rlen);
      tud_hid_report(0, response, 64);
      printf("CTAP2: Sent GetInfo Response.\n");
      return;

    } else if (ctap_method == CTAP2_MAKE_CREDENTIAL) {
      printf("CTAP2: MakeCredential Request\n");
      // 1. Generate Key
      hsm_keypair_t keypair;
      if (!hsm_generate_key_ecc(&keypair)) {
        status = 0x01; // Invalid CMD
      } else {
        // 2. Save Cred (Slot 0 for demo)
        storage_fido2_entry_t cred;
        memset(&cred, 0, sizeof(cred));
        memcpy(cred.priv_key, keypair.priv, 32);
        memcpy(cred.cred_id, "OPEN-TOKEN-CRED-01", 18);
        cred.active = 1;
        storage_save_fido2_cred(0, &cred);

        // 3. Response: Status(0) + Attestation Object
        uint8_t att[] = {0x00,                              // OK
                         0xA3,                              // Map(3)
                         0x01, 0x64, 'n',  'o',  'n',  'e', // fmt: none
                         0x02, 0xA0,                        // attStmt: {}
                         0x03, 0x58, 0x24, // authData (36 bytes mock)
                         0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        response[4] = cmd;
        uint16_t rlen = sizeof(att);
        response[5] = (rlen >> 8) & 0xFF;
        response[6] = rlen & 0xFF;
        memcpy(response + 7, att, rlen);
        tud_hid_report(0, response, 64);
        printf("CTAP2: Sent MakeCredential Response.\n");
        return;
      }
    } else if (ctap_method == CTAP2_GET_ASSERTION) {
      printf("CTAP2: GetAssertion Request\n");
      // Use Slot 0
      storage_fido2_entry_t cred;
      if (storage_load_fido2_cred(0, &cred)) {
        uint8_t ass[] = {0x00,             // OK
                         0xA2,             // Map(2)
                         0x01, 0xA0,       // Credential: {}
                         0x02, 0x58, 0x20, // authData (32 bytes)
                         0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                         0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
        response[4] = cmd;
        uint16_t rlen = sizeof(ass);
        response[5] = (rlen >> 8) & 0xFF;
        response[6] = rlen & 0xFF;
        memcpy(response + 7, ass, rlen);
        tud_hid_report(0, response, 64);
        printf("CTAP2: Sent GetAssertion Response.\n");
        return;
      } else {
        status = 0x2E; // No Credentials
      }
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
