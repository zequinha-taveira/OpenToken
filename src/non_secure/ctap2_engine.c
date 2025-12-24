#include "ctap2_engine.h"
#include "cbor_utils.h"
#include "ccid_engine.h"
#include "error_handling.h"
#include "hsm_layer.h"
#include "led_status.h"
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

// CTAPHID Flags
#define CTAPHID_INIT_FLAG 0x80

// Fixed Broadcast CID
const uint32_t CID_BROADCAST = 0xFFFFFFFF;

// COSE Algorithms
#define COSE_ALG_ES256 -7
#define COSE_KTY_EC2 2
#define COSE_CRV_P256 1

// AuthenticatorData flags
#define AUTHDATA_FLAG_UP 0x01 // User Present
#define AUTHDATA_FLAG_UV 0x04 // User Verified
#define AUTHDATA_FLAG_AT 0x40 // Attested credential data included
#define AUTHDATA_FLAG_ED 0x80 // Extension data included

// Global CTAP2 context
static ctap2_context_t g_ctap2_ctx;

// CTAP2 Engine initialization
void ctap2_engine_init(void) {
  printf("CTAP2: Initializing engine\n");
  memset(&g_ctap2_ctx, 0, sizeof(g_ctap2_ctx));
  g_ctap2_ctx.state = CTAP2_STATE_IDLE;
}

// Helper to encode ECC Public Key as COSE Map
static bool ctap_encode_cose_key(cbor_encoder_t *enc, const hsm_pubkey_t *pub) {
  if (!cbor_encode_map_start(enc, 5))
    return false;

  // Key Type: 1: 2 (EC2)
  if (!cbor_encode_int(enc, 1) || !cbor_encode_int(enc, COSE_KTY_EC2))
    return false;

  // Algorithm: 3: -7 (ES256)
  if (!cbor_encode_int(enc, 3) || !cbor_encode_int(enc, COSE_ALG_ES256))
    return false;

  // Curve: -1: 1 (P-256)
  if (!cbor_encode_int(enc, -1) || !cbor_encode_int(enc, COSE_CRV_P256))
    return false;

  // X: -2: bstr(32)
  if (!cbor_encode_int(enc, -2) || !cbor_encode_bstr(enc, pub->x, 32))
    return false;

  // Y: -3: bstr(32)
  if (!cbor_encode_int(enc, -3) || !cbor_encode_bstr(enc, pub->y, 32))
    return false;

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
  if (flags & AUTHDATA_FLAG_AT) {
    // AAGUID (16) - All zeros for none attestation
    memset(out + offset, 0, 16);
    offset += 16;

    // L (2) - Credential ID Length (Big Endian)
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

// User presence verification with timeout handling
bool ctap2_verify_user_presence(void) {
  printf("CTAP2: Waiting for user presence (button press/touch)...\n");

  // Flash Blue to indicate User Presence request
  led_status_set(LED_COLOR_BLUE);

  // Start timeout for user presence
  if (!timeout_start(DEFAULT_TIMEOUTS.user_presence_timeout_ms)) {
    ERROR_REPORT_ERROR(ERROR_TIMEOUT_USER_PRESENCE,
                       "Failed to start user presence timeout");
    return false;
  }

  // NOTE: In a real RP2350 implementation, this would poll a GPIO
  // for a button press or check a capacitive touch sensor.
  // For the OpenToken demo, we simulate a successful interaction.

  printf("CTAP2: User presence detected.\n");
  led_status_set(LED_COLOR_GREEN); // Revert to idle after touch
  timeout_reset();
  return true;
}

// User verification (simplified - always false for now as no PIN is set)
bool ctap2_verify_user_verification(void) {
  printf("CTAP2: User verification check\n");
  // In a real implementation, this would verify PIN or biometric
  return false;
}

// Generate a deterministic credential ID based on RP and user info
uint8_t ctap2_generate_credential_id(const uint8_t *rp_id_hash,
                                     const uint8_t *user_id,
                                     uint16_t user_id_len, uint8_t *cred_id_out,
                                     uint16_t *cred_id_len_out) {
  // Simple credential ID generation - in production this should be more secure
  // Format: "OT-" + first 8 bytes of RP hash + first 4 bytes of user ID +
  // counter
  static uint32_t cred_counter = 1;

  uint16_t offset = 0;

  // Prefix
  memcpy(cred_id_out + offset, "OT-", 3);
  offset += 3;

  // RP ID hash (first 8 bytes)
  memcpy(cred_id_out + offset, rp_id_hash, 8);
  offset += 8;

  // User ID (first 4 bytes or less)
  uint16_t user_copy_len = (user_id_len < 4) ? user_id_len : 4;
  memcpy(cred_id_out + offset, user_id, user_copy_len);
  offset += user_copy_len;

  // Counter (4 bytes, big endian)
  cred_id_out[offset++] = (cred_counter >> 24) & 0xFF;
  cred_id_out[offset++] = (cred_counter >> 16) & 0xFF;
  cred_id_out[offset++] = (cred_counter >> 8) & 0xFF;
  cred_id_out[offset++] = cred_counter & 0xFF;

  *cred_id_len_out = offset;
  cred_counter++;

  return CTAP2_OK;
}

// Helper to send fragmented CTAPHID response with error handling
static bool ctap_send_response(uint32_t cid, uint8_t cmd, const uint8_t *data,
                               uint16_t len) {
  if (len > 7609) { // Maximum CTAPHID message size
    ERROR_REPORT_ERROR(ERROR_PROTOCOL_BUFFER_OVERFLOW,
                       "Response too large: %d bytes", len);
    protocol_send_error_response_ctap2(cid, CTAP2_ERR_REQUEST_TOO_LARGE);
    return false;
  }

  uint8_t report[64];
  memset(report, 0, 64);

  // Convert CID to little endian for USB HID
  report[0] = cid & 0xFF;
  report[1] = (cid >> 8) & 0xFF;
  report[2] = (cid >> 16) & 0xFF;
  report[3] = (cid >> 24) & 0xFF;

  report[4] = cmd;
  report[5] = (len >> 8) & 0xFF;
  report[6] = len & 0xFF;

  uint16_t sent = 0;
  uint16_t to_send = (len < 57) ? len : 57;
  memcpy(report + 7, data, to_send);

  // Send with timeout protection
  if (!timeout_start(DEFAULT_TIMEOUTS.usb_operation_timeout_ms)) {
    ERROR_REPORT_ERROR(ERROR_TIMEOUT_USB_OPERATION,
                       "Failed to start USB timeout");
    return false;
  }

  if (!tud_hid_report(0, report, 64)) {
    timeout_reset();
    ERROR_REPORT_ERROR(ERROR_USB_ENDPOINT_ERROR, "Failed to send HID report");
    return false;
  }
  timeout_reset();

  sent += to_send;

  uint8_t seq = 0;
  while (sent < len) {
    memset(report, 0, 64);
    report[0] = cid & 0xFF;
    report[1] = (cid >> 8) & 0xFF;
    report[2] = (cid >> 16) & 0xFF;
    report[3] = (cid >> 24) & 0xFF;
    report[4] = seq++; // Sequence 0x00 ... 0x7F
    to_send = (len - sent < 59) ? (len - sent) : 59;
    memcpy(report + 5, data + sent, to_send);

    if (!timeout_start(DEFAULT_TIMEOUTS.usb_operation_timeout_ms)) {
      ERROR_REPORT_ERROR(ERROR_TIMEOUT_USB_OPERATION,
                         "Failed to start USB timeout");
      return false;
    }

    if (!tud_hid_report(0, report, 64)) {
      timeout_reset();
      ERROR_REPORT_ERROR(ERROR_USB_ENDPOINT_ERROR,
                         "Failed to send continuation HID report");
      return false;
    }
    timeout_reset();

    sent += to_send;
  }

  return true;
}

// CTAP2 GetInfo command handler
uint8_t ctap2_handle_get_info(uint8_t *response, uint16_t *response_len) {
  printf("CTAP2: Handling GetInfo\n");

  cbor_encoder_t enc;
  cbor_encoder_init(&enc, response, 512);

  // Status: OK
  if (!cbor_encode_uint(&enc, CTAP2_OK))
    return CTAP2_ERR_PROCESSING;

  // Response map with 4 entries
  if (!cbor_encode_map_start(&enc, 4))
    return CTAP2_ERR_PROCESSING;

  // 1. versions (0x01)
  if (!cbor_encode_uint(&enc, 0x01))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_array_start(&enc, 2))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_tstr(&enc, "FIDO_2_0"))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_tstr(&enc, "FIDO_2_1"))
    return CTAP2_ERR_PROCESSING;

  // 2. extensions (0x02) - empty array for now
  if (!cbor_encode_uint(&enc, 0x02))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_array_start(&enc, 0))
    return CTAP2_ERR_PROCESSING;

  // 3. aaguid (0x03) - 16 zero bytes
  if (!cbor_encode_uint(&enc, 0x03))
    return CTAP2_ERR_PROCESSING;
  uint8_t aaguid[16] = {0};
  if (!cbor_encode_bstr(&enc, aaguid, 16))
    return CTAP2_ERR_PROCESSING;

  // 4. options (0x04)
  if (!cbor_encode_uint(&enc, 0x04))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_map_start(&enc, 3))
    return CTAP2_ERR_PROCESSING;

  // rk (resident key) support
  if (!cbor_encode_tstr(&enc, "rk"))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_uint(&enc, 1)) // true
    return CTAP2_ERR_PROCESSING;

  // up (user presence) support
  if (!cbor_encode_tstr(&enc, "up"))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_uint(&enc, 1)) // true
    return CTAP2_ERR_PROCESSING;

  // plat (platform device) - false
  if (!cbor_encode_tstr(&enc, "plat"))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_uint(&enc, 0)) // false
    return CTAP2_ERR_PROCESSING;

  *response_len = enc.offset;
  return CTAP2_OK;
}

// CTAP2 MakeCredential command handler
uint8_t ctap2_handle_make_credential(const uint8_t *cbor_data,
                                     uint16_t cbor_len, uint8_t *response,
                                     uint16_t *response_len) {
  printf("CTAP2: Handling MakeCredential\n");

  cbor_decoder_t dec;
  cbor_decoder_init(&dec, cbor_data, cbor_len);

  uint8_t client_data_hash[32] = {0};
  uint8_t rp_id_hash[32] = {0};
  uint8_t user_id[64] = {0};
  uint16_t user_id_len = 0;
  bool rk_required = false;
  bool uv_required = false;

  // Parse CBOR map
  uint32_t map_pairs;
  if (!cbor_decode_map_start(&dec, &map_pairs)) {
    return CTAP2_ERR_INVALID_CBOR;
  }

  for (uint32_t i = 0; i < map_pairs; i++) {
    uint32_t key;
    if (!cbor_decode_uint(&dec, &key)) {
      cbor_skip_item(&dec);
      cbor_skip_item(&dec);
      continue;
    }

    switch (key) {
    case 1: { // clientDataHash
      const uint8_t *hash_data;
      uint16_t hash_len;
      if (cbor_decode_bstr(&dec, &hash_data, &hash_len) && hash_len == 32) {
        memcpy(client_data_hash, hash_data, 32);
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    case 2: { // rp
      uint32_t rp_pairs;
      if (cbor_decode_map_start(&dec, &rp_pairs)) {
        for (uint32_t j = 0; j < rp_pairs; j++) {
          const char *rp_key;
          uint16_t rp_key_len;
          if (cbor_decode_tstr(&dec, &rp_key, &rp_key_len)) {
            if (rp_key_len == 2 && memcmp(rp_key, "id", 2) == 0) {
              const char *rp_id;
              uint16_t rp_id_len;
              if (cbor_decode_tstr(&dec, &rp_id, &rp_id_len)) {
                // Hash the RP ID to create rp_id_hash
                // For simplicity, just copy first 32 chars or pad with zeros
                memset(rp_id_hash, 0, 32);
                uint16_t copy_len = (rp_id_len < 32) ? rp_id_len : 32;
                memcpy(rp_id_hash, rp_id, copy_len);
              } else {
                cbor_skip_item(&dec);
              }
            } else {
              cbor_skip_item(&dec);
            }
          } else {
            cbor_skip_item(&dec);
            cbor_skip_item(&dec);
          }
        }
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    case 3: { // user
      uint32_t user_pairs;
      if (cbor_decode_map_start(&dec, &user_pairs)) {
        for (uint32_t j = 0; j < user_pairs; j++) {
          const char *user_key;
          uint16_t user_key_len;
          if (cbor_decode_tstr(&dec, &user_key, &user_key_len)) {
            if (user_key_len == 2 && memcmp(user_key, "id", 2) == 0) {
              const uint8_t *uid_data;
              uint16_t uid_len;
              if (cbor_decode_bstr(&dec, &uid_data, &uid_len)) {
                user_id_len = (uid_len < 64) ? uid_len : 64;
                memcpy(user_id, uid_data, user_id_len);
              } else {
                cbor_skip_item(&dec);
              }
            } else {
              cbor_skip_item(&dec);
            }
          } else {
            cbor_skip_item(&dec);
            cbor_skip_item(&dec);
          }
        }
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    case 7: { // options
      uint32_t opt_pairs;
      if (cbor_decode_map_start(&dec, &opt_pairs)) {
        for (uint32_t j = 0; j < opt_pairs; j++) {
          const char *opt_key;
          uint16_t opt_key_len;
          if (cbor_decode_tstr(&dec, &opt_key, &opt_key_len)) {
            if (opt_key_len == 2 && memcmp(opt_key, "rk", 2) == 0) {
              uint32_t rk_val;
              if (cbor_decode_uint(&dec, &rk_val)) {
                rk_required = (rk_val != 0);
              } else {
                cbor_skip_item(&dec);
              }
            } else if (opt_key_len == 2 && memcmp(opt_key, "uv", 2) == 0) {
              uint32_t uv_val;
              if (cbor_decode_uint(&dec, &uv_val)) {
                uv_required = (uv_val != 0);
              } else {
                cbor_skip_item(&dec);
              }
            } else {
              cbor_skip_item(&dec);
            }
          } else {
            cbor_skip_item(&dec);
            cbor_skip_item(&dec);
          }
        }
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    default:
      cbor_skip_item(&dec);
      break;
    }
  }

  // Verify user presence if required
  if (!ctap2_verify_user_presence()) {
    return CTAP2_ERR_USER_ACTION_TIMEOUT;
  }

  // Verify user verification if required
  if (uv_required && !ctap2_verify_user_verification()) {
    return CTAP2_ERR_PIN_REQUIRED;
  }

  // Generate new key pair
  hsm_keypair_t keypair;
  if (!hsm_generate_key_ecc_legacy(&keypair)) {
    return CTAP2_ERR_PROCESSING;
  }

  // Generate credential ID
  uint8_t cred_id[64];
  uint16_t cred_id_len;
  uint8_t result = ctap2_generate_credential_id(
      rp_id_hash, user_id, user_id_len, cred_id, &cred_id_len);
  if (result != CTAP2_OK) {
    return result;
  }

  // Store credential if resident key is required
  if (rk_required) {
    storage_fido2_entry_t cred;
    memset(&cred, 0, sizeof(cred));
    memcpy(cred.rp_id_hash, rp_id_hash, 32);
    memcpy(cred.user_id, user_id, user_id_len);
    cred.user_id_len = user_id_len;
    memcpy(cred.cred_id, cred_id, cred_id_len);
    memcpy(cred.priv_key, keypair.priv, 32);
    cred.sign_count = 0;
    cred.active = 1;

    // Find empty slot
    bool stored = false;
    for (uint8_t slot = 0; slot < STORAGE_FIDO2_MAX_CREDS; slot++) {
      storage_fido2_entry_t existing;
      if (!storage_load_fido2_cred(slot, &existing) || !existing.active) {
        if (storage_save_fido2_cred(slot, &cred)) {
          stored = true;
          break;
        }
      }
    }

    if (!stored) {
      return CTAP2_ERR_KEY_STORE_FULL;
    }
  }

  // Build authenticator data
  uint8_t auth_data[512];
  uint8_t flags = AUTHDATA_FLAG_UP | AUTHDATA_FLAG_AT;
  if (uv_required && ctap2_verify_user_verification()) {
    flags |= AUTHDATA_FLAG_UV;
  }

  uint16_t auth_data_len = ctap_build_authdata(
      auth_data, rp_id_hash, flags, 0, cred_id, cred_id_len, &keypair.pub);

  // Build response
  cbor_encoder_t enc;
  cbor_encoder_init(&enc, response, 1024);

  // Pulse Blue for success/activity
  led_status_set(LED_COLOR_BLUE);
  sleep_ms(10);
  led_status_set(LED_COLOR_GREEN);

  // Status: OK
  if (!cbor_encode_uint(&enc, CTAP2_OK))
    return CTAP2_ERR_PROCESSING;

  // Response map with 3 entries
  if (!cbor_encode_map_start(&enc, 3))
    return CTAP2_ERR_PROCESSING;

  // 1. fmt (0x01) - attestation format
  if (!cbor_encode_uint(&enc, 0x01))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_tstr(&enc, "none"))
    return CTAP2_ERR_PROCESSING;

  // 2. authData (0x02)
  if (!cbor_encode_uint(&enc, 0x02))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_bstr(&enc, auth_data, auth_data_len))
    return CTAP2_ERR_PROCESSING;

  // 3. attStmt (0x03) - empty map for "none" attestation
  if (!cbor_encode_uint(&enc, 0x03))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_map_start(&enc, 0))
    return CTAP2_ERR_PROCESSING;

  *response_len = enc.offset;
  return CTAP2_OK;
}

// CTAP2 GetAssertion command handler
uint8_t ctap2_handle_get_assertion(const uint8_t *cbor_data, uint16_t cbor_len,
                                   uint8_t *response, uint16_t *response_len) {
  printf("CTAP2: Handling GetAssertion\n");

  cbor_decoder_t dec;
  cbor_decoder_init(&dec, cbor_data, cbor_len);

  uint8_t client_data_hash[32] = {0};
  uint8_t rp_id_hash[32] = {0};
  bool uv_required = false;

  // Parse CBOR map
  uint32_t map_pairs;
  if (!cbor_decode_map_start(&dec, &map_pairs)) {
    return CTAP2_ERR_INVALID_CBOR;
  }

  for (uint32_t i = 0; i < map_pairs; i++) {
    uint32_t key;
    if (!cbor_decode_uint(&dec, &key)) {
      cbor_skip_item(&dec);
      cbor_skip_item(&dec);
      continue;
    }

    switch (key) {
    case 1: { // rpId
      const char *rp_id;
      uint16_t rp_id_len;
      if (cbor_decode_tstr(&dec, &rp_id, &rp_id_len)) {
        // Hash the RP ID
        memset(rp_id_hash, 0, 32);
        uint16_t copy_len = (rp_id_len < 32) ? rp_id_len : 32;
        memcpy(rp_id_hash, rp_id, copy_len);
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    case 2: { // clientDataHash
      const uint8_t *hash_data;
      uint16_t hash_len;
      if (cbor_decode_bstr(&dec, &hash_data, &hash_len) && hash_len == 32) {
        memcpy(client_data_hash, hash_data, 32);
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    case 5: { // options
      uint32_t opt_pairs;
      if (cbor_decode_map_start(&dec, &opt_pairs)) {
        for (uint32_t j = 0; j < opt_pairs; j++) {
          const char *opt_key;
          uint16_t opt_key_len;
          if (cbor_decode_tstr(&dec, &opt_key, &opt_key_len)) {
            if (opt_key_len == 2 && memcmp(opt_key, "uv", 2) == 0) {
              uint32_t uv_val;
              if (cbor_decode_uint(&dec, &uv_val)) {
                uv_required = (uv_val != 0);
              } else {
                cbor_skip_item(&dec);
              }
            } else {
              cbor_skip_item(&dec);
            }
          } else {
            cbor_skip_item(&dec);
            cbor_skip_item(&dec);
          }
        }
      } else {
        cbor_skip_item(&dec);
      }
      break;
    }
    default:
      cbor_skip_item(&dec);
      break;
    }
  }

  // Case 3: allowList (not implemented in this snippet, but normally goes here)
  // For now, we focus on Resident Keys (Case 2: empty allowList)

  uint8_t cred_indices[STORAGE_FIDO2_MAX_CREDS];
  uint8_t cred_count = storage_find_fido2_creds_all_by_rp(
      rp_id_hash, cred_indices, STORAGE_FIDO2_MAX_CREDS);

  if (cred_count == 0) {
    return CTAP2_ERR_NO_CREDENTIALS;
  }

  // If multiple credentials exist, in a full implementation we would support
  // getNextAssertion. For now, we take the first one or the one matching
  // allowList. Since allowList parsing is omitted for brevity, we take the
  // first found.
  storage_fido2_entry_t cred;
  uint8_t cred_index = cred_indices[0];
  if (!storage_load_fido2_cred(cred_index, &cred)) {
    return CTAP2_ERR_NO_CREDENTIALS;
  }

  // Verify user presence
  if (!ctap2_verify_user_presence()) {
    return CTAP2_ERR_USER_ACTION_TIMEOUT;
  }

  // Verify user verification if required
  if (uv_required && !ctap2_verify_user_verification()) {
    return CTAP2_ERR_PIN_REQUIRED;
  }

  // Increment signature counter
  cred.sign_count++;
  storage_save_fido2_cred(cred_index, &cred);

  // Build authenticator data
  uint8_t auth_data[256];
  uint8_t flags = AUTHDATA_FLAG_UP;
  if (uv_required && ctap2_verify_user_verification()) {
    flags |= AUTHDATA_FLAG_UV;
  }

  uint16_t auth_data_len = ctap_build_authdata(auth_data, rp_id_hash, flags,
                                               cred.sign_count, NULL, 0, NULL);

  // Create signature base (authData + clientDataHash)
  uint8_t sign_data[256 + 32];
  memcpy(sign_data, auth_data, auth_data_len);
  memcpy(sign_data + auth_data_len, client_data_hash, 32);

  // Sign the data
  uint8_t signature[64];
  uint16_t sig_len;
  if (!hsm_sign_ecc(cred.priv_key, sign_data, auth_data_len + 32, signature,
                    &sig_len)) {
    return CTAP2_ERR_PROCESSING;
  }

  // Build response
  cbor_encoder_t enc;
  cbor_encoder_init(&enc, response, 1024);

  // Pulse Blue for success/activity
  led_status_set(LED_COLOR_BLUE);
  sleep_ms(10);
  led_status_set(LED_COLOR_GREEN);

  // Status: OK
  if (!cbor_encode_uint(&enc, CTAP2_OK))
    return CTAP2_ERR_PROCESSING;

  // Response map with 3 entries
  if (!cbor_encode_map_start(&enc, 3))
    return CTAP2_ERR_PROCESSING;

  // 1. credential (0x01)
  if (!cbor_encode_uint(&enc, 0x01))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_map_start(&enc, 2))
    return CTAP2_ERR_PROCESSING;

  // credential.type
  if (!cbor_encode_tstr(&enc, "type"))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_tstr(&enc, "public-key"))
    return CTAP2_ERR_PROCESSING;

  // credential.id
  if (!cbor_encode_tstr(&enc, "id"))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_bstr(&enc, cred.cred_id, 32))
    return CTAP2_ERR_PROCESSING;

  // 2. authData (0x02)
  if (!cbor_encode_uint(&enc, 0x02))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_bstr(&enc, auth_data, auth_data_len))
    return CTAP2_ERR_PROCESSING;

  // 3. signature (0x03)
  if (!cbor_encode_uint(&enc, 0x03))
    return CTAP2_ERR_PROCESSING;
  if (!cbor_encode_bstr(&enc, signature, sig_len))
    return CTAP2_ERR_PROCESSING;

  *response_len = enc.offset;
  return CTAP2_OK;
}

// Main CTAP2 processing function
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len) {
  // Input validation with comprehensive error handling
  if (!buffer) {
    ERROR_REPORT_ERROR(ERROR_PROTOCOL_INVALID_COMMAND,
                       "Null buffer in CTAP2 command");
    return;
  }

  if (len < 7) {
    ERROR_REPORT_ERROR(ERROR_PROTOCOL_MALFORMED_PACKET,
                       "CTAP2 packet too short: %d bytes", len);
    return;
  }

  if (len > 7609) { // Maximum CTAPHID message size
    ERROR_REPORT_ERROR(ERROR_PROTOCOL_BUFFER_OVERFLOW,
                       "CTAP2 packet too large: %d bytes", len);
    return;
  }

  // Extract CID (little endian from USB HID)
  uint32_t cid =
      buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
  uint8_t cmd = buffer[4];
  uint16_t payload_len = (buffer[5] << 8) | buffer[6];

  printf("CTAP2: CID=%08X CMD=%02X PayLen=%d\n", cid, cmd, payload_len);

  // Validate payload length
  if (payload_len > len - 7) {
    ERROR_REPORT_ERROR(ERROR_PROTOCOL_MALFORMED_PACKET,
                       "Invalid payload length: %d > %d", payload_len, len - 7);
    protocol_send_error_response_ctap2(cid, CTAP2_ERR_INVALID_LENGTH);
    return;
  }

  // Update context with error checking
  g_ctap2_ctx.current_cid = cid;
  g_ctap2_ctx.state = CTAP2_STATE_PROCESSING;

  if (cmd == (CTAPHID_CMD_INIT | CTAPHID_INIT_FLAG)) {
    // CTAPHID_INIT command
    if (payload_len < 8) {
      // ... error handling ...
      return;
    }
    // ... existing init logic ...

    uint8_t resp[17];
    memcpy(resp, buffer + 7, 8); // Echo nonce

    // Generate new CID (simple increment for demo)
    static uint32_t next_cid = 0x12345678;
    uint32_t new_cid = next_cid++;

    resp[8] = new_cid & 0xFF;
    resp[9] = (new_cid >> 8) & 0xFF;
    resp[10] = (new_cid >> 16) & 0xFF;
    resp[11] = (new_cid >> 24) & 0xFF;

    resp[12] = 2; // Protocol Version
    resp[13] = 1; // Major Version
    resp[14] = 0; // Minor Version
    resp[15] = 0; // Build Version
    resp[16] = 0; // Capabilities

    ctap_send_response(cid, cmd, resp, 17);
    g_ctap2_ctx.state = CTAP2_STATE_IDLE;
  } else if (cmd == (CTAPHID_CMD_APDU_TUNNEL | CTAPHID_INIT_FLAG)) {
    // APDU Tunneling Command (0x70)
    uint8_t apdu_response[APDU_RESPONSE_MAX_LEN];
    uint16_t apdu_resp_len = 0;

    opentoken_process_ccid_apdu(buffer + 7, payload_len, apdu_response,
                                &apdu_resp_len);
    ctap_send_response(cid, cmd & 0x7F, apdu_response, apdu_resp_len);
    g_ctap2_ctx.state = CTAP2_STATE_IDLE;
  } else if (cmd == (CTAPHID_CMD_PING | CTAPHID_INIT_FLAG)) {
    // CTAPHID_PING command - echo payload
    ctap_send_response(cid, cmd, buffer + 7, payload_len);
    g_ctap2_ctx.state = CTAP2_STATE_IDLE;
  } else if (cmd == (CTAPHID_CMD_MSG | CTAPHID_INIT_FLAG) ||
             cmd == (CTAPHID_CMD_CBOR | CTAPHID_INIT_FLAG)) {
    // CTAP2 command
    if (len < 8) {
      printf("CTAP2: Invalid CTAP2 command length\n");
      g_ctap2_ctx.state = CTAP2_STATE_ERROR;
      return;
    }

    uint8_t ctap_method = buffer[7];
    uint8_t response[1024];
    uint16_t response_len = 0;
    uint8_t status = CTAP2_ERR_INVALID_COMMAND;

    g_ctap2_ctx.current_command = ctap_method;

    switch (ctap_method) {
    case CTAP2_GET_INFO:
      status = ctap2_handle_get_info(response, &response_len);
      break;

    case CTAP2_MAKE_CREDENTIAL:
      status = ctap2_handle_make_credential(buffer + 8, payload_len - 1,
                                            response, &response_len);
      break;

    case CTAP2_GET_ASSERTION:
      status = ctap2_handle_get_assertion(buffer + 8, payload_len - 1, response,
                                          &response_len);
      break;

    default:
      printf("CTAP2: Unsupported method 0x%02X\n", ctap_method);
      status = CTAP2_ERR_INVALID_COMMAND;
      break;
    }

    // Send response or error with proper error handling
    if (status == CTAP2_OK && response_len > 0) {
      if (!ctap_send_response(cid, cmd, response, response_len)) {
        ERROR_REPORT_ERROR(ERROR_PROTOCOL_SEQUENCE_ERROR,
                           "Failed to send CTAP2 response");
        g_ctap2_ctx.state = CTAP2_STATE_ERROR;
        return;
      }
    } else {
      uint8_t error_resp[1] = {status};
      if (!ctap_send_response(cid, cmd, error_resp, 1)) {
        ERROR_REPORT_ERROR(ERROR_PROTOCOL_SEQUENCE_ERROR,
                           "Failed to send CTAP2 error response");
        g_ctap2_ctx.state = CTAP2_STATE_ERROR;
        return;
      }
    }

    g_ctap2_ctx.state = CTAP2_STATE_IDLE;
  } else {
    ERROR_REPORT_WARNING(ERROR_PROTOCOL_UNSUPPORTED_VERSION,
                         "Unknown CTAP2 command: 0x%02X", cmd);
    protocol_send_error_response_ctap2(cid, CTAP2_ERR_INVALID_COMMAND);
    g_ctap2_ctx.state = CTAP2_STATE_ERROR;
  }
}
