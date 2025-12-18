#include "ctap2_engine.h"
#include "cbor_utils.h"
#include "hsm_layer.h"
#include "storage.h"
#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>
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

// COSE Algorithms
#define COSE_ALG_ES256 -7
#define COSE_KTY_EC2 2
#define COSE_CRV_P256 1

// Helper to encode ECC Public Key as COSE Map
static bool ctap_encode_cose_key(cbor_encoder_t *enc, const hsm_pubkey_t *pub) {
  if (!cbor_encode_map_start(enc, 5))
    return false;

  // Key Type: 1: 2 (EC2)
  cbor_encode_int(enc, 1);
  cbor_encode_int(enc, COSE_KTY_EC2);

  // Algorithm: 3: -7 (ES256)
  cbor_encode_int(enc, 3);
  cbor_encode_int(enc, COSE_ALG_ES256);

  // Curve: -1: 1 (P-256)
  cbor_encode_int(enc, -1);
  cbor_encode_int(enc, COSE_CRV_P256);

  // X: -2: bstr(32)
  cbor_encode_int(enc, -2);
  cbor_encode_bstr(enc, pub->x, 32);

  // Y: -3: bstr(32)
  cbor_encode_int(enc, -3);
  cbor_encode_bstr(enc, pub->y, 32);

  return true;
}

// Helper to build FIDO2 authData
static uint16_t ctap_build_authdata(uint8_t *out, const uint8_t *rp_id_hash,
                                    uint8_t flags, uint32_t counter,
                                    const uint8_t *cred_id,
                                    uint16_t cred_id_len,
                                    const hsm_pubkey_t *pub) {
  uint16_t offset = 0;

  // 1. RP ID Hash (32)
  memcpy(out + offset, rp_id_hash, 32);
  offset += 32;

  // 2. Flags (1)
  out[offset++] = flags;

  // 3. Counter (4) - Big Endian
  out[offset++] = (counter >> 24) & 0xFF;
  out[offset++] = (counter >> 16) & 0xFF;
  out[offset++] = (counter >> 8) & 0xFF;
  out[offset++] = counter & 0xFF;

  // 4. Attested Credential Data (if AT flag set)
  if (flags & 0x40) {
    // AAGUID (16) - All zeros for none attestation
    memset(out + offset, 0, 16);
    offset += 16;

    // L (2) - Credential ID Length
    out[offset++] = (cred_id_len >> 8) & 0xFF;
    out[offset++] = cred_id_len & 0xFF;

    // Credential ID
    memcpy(out + offset, cred_id, cred_id_len);
    offset += cred_id_len;

    // COSE Key
    cbor_encoder_t enc;
    cbor_encoder_init(&enc, out + offset, 256);
    if (ctap_encode_cose_key(&enc, pub)) {
      offset += enc.offset;
    }
  }

  return offset;
}

// Helper to send fragmented CTAPHID response
static void ctap_send_response(uint32_t cid, uint8_t cmd, const uint8_t *data,
                               uint16_t len) {
  uint8_t report[64];
  memset(report, 0, 64);
  memcpy(report, &cid, 4);
  report[4] = cmd;
  report[5] = (len >> 8) & 0xFF;
  report[6] = len & 0xFF;

  uint16_t sent = 0;
  uint16_t to_send = (len < 57) ? len : 57;
  memcpy(report + 7, data, to_send);
  tud_hid_report(0, report, 64);
  sent += to_send;

  uint8_t seq = 0;
  while (sent < len) {
    memset(report, 0, 64);
    memcpy(report, &cid, 4);
    report[4] = seq++; // Sequence 0x00 ... 0x7F
    to_send = (len - sent < 59) ? (len - sent) : 59;
    memcpy(report + 5, data + sent, to_send);
    tud_hid_report(0, report, 64);
    sent += to_send;
  }
}

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
    uint8_t resp[17];
    memcpy(resp, buffer + 7, 8); // Nonce
    uint32_t new_cid = 0x1D2D3D4D;
    memcpy(resp + 8, &new_cid, 4);
    resp[12] = 2; // Protocol Version
    resp[13] = 1; // Major
    resp[14] = 0; // Minor
    resp[15] = 0; // Build
    resp[16] = 0; // Caps
    ctap_send_response(cid, cmd, resp, 17);

  } else if (cmd == (CTAPHID_CMD_PING | 0x80)) {
    ctap_send_response(cid, cmd, buffer + 7, payload_len);

  } else if (cmd == (CTAPHID_CMD_MSG | 0x80) ||
             cmd == (CTAPHID_CMD_CBOR | 0x80)) {

    if (len < 8)
      return;
    uint8_t ctap_method = buffer[7];
    uint8_t status = 0x00;

    if (ctap_method == CTAP2_GET_INFO) {
      uint8_t static_resp[] = {
          0x00,                                                     // OK
          0xA2,                                                     // Map(2)
          0x01, 0x81, 0x68, 'F', 'I', 'D', 'O', '_', '2', '_', '1', // versions
          0x03, 0x50, 0,    0,   0,   0,   0,   0,   0,   0,   0,
          0,    0,    0,    0,   0,   0,   0 // aaguid
      };
      ctap_send_response(cid, cmd, static_resp, sizeof(static_resp));
      return;

    } else if (ctap_method == CTAP2_MAKE_CREDENTIAL) {
      printf("CTAP2: MakeCredential (REAL)\n");
      cbor_decoder_t dec;
      cbor_decoder_init(&dec, buffer + 8, payload_len - 1);
      uint8_t client_hash[32];
      memset(client_hash, 0, 32);
      uint32_t pairs;
      if (cbor_decode_map_start(&dec, &pairs)) {
        for (uint32_t i = 0; i < pairs; i++) {
          uint32_t key;
          if (cbor_decode_uint(&dec, &key)) {
            if (key == 1) { // clientDataHash
              const uint8_t *h;
              uint16_t hlen;
              if (cbor_decode_bstr(&dec, &h, &hlen) && hlen == 32)
                memcpy(client_hash, h, 32);
            } else
              cbor_skip_item(&dec);
          } else
            cbor_skip_item(&dec);
        }
      }

      hsm_keypair_t keypair;
      if (hsm_generate_key_ecc(&keypair)) {
        storage_fido2_entry_t cred;
        memset(&cred, 0, sizeof(cred));
        memcpy(cred.priv_key, keypair.priv, 32);
        memcpy(cred.cred_id, "OT-CRED-01", 10);
        memcpy(cred.rp_id_hash, client_hash, 32);
        cred.active = 1;
        storage_save_fido2_cred(0, &cred);

        uint8_t ad[256];
        uint16_t ad_len =
            ctap_build_authdata(ad, client_hash, 0x41, 0,
                                (uint8_t *)"OT-CRED-01", 10, &keypair.pub);

        cbor_encoder_t enc;
        uint8_t out[512];
        cbor_encoder_init(&enc, out, sizeof(out));
        cbor_encode_uint(&enc, 0); // OK
        cbor_encode_map_start(&enc, 3);
        cbor_encode_uint(&enc, 1);
        cbor_encode_tstr(&enc, "none");
        cbor_encode_uint(&enc, 2);
        cbor_encode_map_start(&enc, 0);
        cbor_encode_uint(&enc, 3);
        cbor_encode_bstr(&enc, ad, ad_len);

        ctap_send_response(cid, cmd, out, enc.offset);
        return;
      }
      status = 0x01;

    } else if (ctap_method == CTAP2_GET_ASSERTION) {
      printf("CTAP2: GetAssertion (REAL)\n");
      storage_fido2_entry_t cred;
      if (storage_load_fido2_cred(0, &cred)) {
        uint8_t ad[37];
        uint16_t ad_len =
            ctap_build_authdata(ad, cred.rp_id_hash, 0x01, 0, NULL, 0, NULL);
        uint8_t sign_base[37 + 32];
        memcpy(sign_base, ad, 37);
        memset(sign_base + 37, 0xDD, 32); // Mock hash from request
        uint8_t sig[64];
        uint16_t slen;
        hsm_sign_ecc(cred.priv_key, sign_base, sizeof(sign_base), sig, &slen);

        cbor_encoder_t enc;
        uint8_t out[256];
        cbor_encoder_init(&enc, out, sizeof(out));
        cbor_encode_uint(&enc, 0); // OK
        cbor_encode_map_start(&enc, 3);
        cbor_encode_uint(&enc, 1);
        cbor_encode_map_start(&enc, 0);
        cbor_encode_uint(&enc, 2);
        cbor_encode_bstr(&enc, ad, ad_len);
        cbor_encode_uint(&enc, 3);
        cbor_encode_bstr(&enc, sig, slen);

        ctap_send_response(cid, cmd, out, enc.offset);
        return;
      }
      status = 0x2E;
    }

    uint8_t err[1] = {status ? status : 0x01};
    ctap_send_response(cid, cmd, err, 1);
  } else {
    // Unknown
  }
}
}
