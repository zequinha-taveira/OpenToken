#include "hsm_layer.h"
#include "error_handling.h"
#include "mbedtls_config.h"
#include "storage.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Pico SDK for Hardware Root of Trust
#include "pico/unique_id.h"

// mbedTLS Includes
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md.h"
#include "mbedtls/platform.h"
#include "mbedtls/sha256.h"

// Static Contexts (to avoid stack overflow on small micros)
// In a real generic implem, these should be managed more carefully.
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static bool is_init = false;

// Hardware-backed encryption key derived from RP2350 unique ID
static uint8_t g_derived_storage_key[32] = {0};
static bool g_key_derived = false;

// Derive a unique key for this specific hardware
static void hsm_derive_hardware_key(void) {
  if (g_key_derived)
    return;

  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);

  // Derive key using SHA-256 of the board ID + a fixed salt
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(
      &ctx, (const unsigned char *)"OpenToken-Hardened-Salt-v1", 26);
  mbedtls_sha256_update(&ctx, board_id.id, 8);
  mbedtls_sha256_finish(&ctx, g_derived_storage_key);
  mbedtls_sha256_free(&ctx);

  g_key_derived = true;
  printf("HSM: Hardware-backed storage key derived successfully\n");
}

// Static wrapper prototypes
static bool ensure_init_wrapper(void);
static bool storage_save_hsm_key_wrapper(void *context);

static void ensure_init(void) {
  if (is_init)
    return;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  // Seed with some fixed string + timer if possible (in Pico use standard RNG)
  // Pico SDK pico_mbedtls handles this if using standard platform setup,
  // but here we just do a basic seed.
  const char *pers = "opentoken";
  mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                        (const unsigned char *)pers, strlen(pers));

  // Also derive the hardware key during initialization
  hsm_derive_hardware_key();

  is_init = true;
}

void hsm_init(void) {
  printf("HSM: Initializing cryptographic layer with error handling\n");

  if (!retry_operation((bool (*)(void))ensure_init_wrapper,
                       &RETRY_CONFIG_CRYPTO)) {
    ERROR_REPORT_CRITICAL(ERROR_CRYPTO_RNG_FAILURE,
                          "Failed to initialize cryptographic subsystem");
    return;
  }

  // Initialize storage if not already done
  if (!retry_operation((bool (*)(void))storage_init, &RETRY_CONFIG_STORAGE)) {
    ERROR_REPORT_CRITICAL(ERROR_STORAGE_WRITE_FAILED,
                          "Failed to initialize storage from HSM");
    return;
  }

  printf("HSM: Cryptographic layer initialized successfully\n");
}

// Wrapper for ensure_init to work with retry mechanism
static bool ensure_init_wrapper(void) {
  ensure_init();
  return is_init;
}

// Simple XOR encryption/decryption for private key storage using derived key
static void encrypt_decrypt_key(const uint8_t *input, uint8_t *output) {
  if (!g_key_derived) {
    hsm_derive_hardware_key();
  }
  for (int i = 0; i < 32; i++) {
    output[i] = input[i] ^ g_derived_storage_key[i];
  }
}

// Hash PIN for secure comparison
static void hash_pin(const uint8_t *pin, uint16_t pin_len, uint8_t *hash_out) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0); // SHA-256, not SHA-224
  mbedtls_sha256_update(&ctx, pin, pin_len);
  mbedtls_sha256_finish(&ctx, hash_out);
  mbedtls_sha256_free(&ctx);
}

// Secure PIN verification with retry counter
hsm_pin_result_t hsm_verify_pin_secure(const uint8_t *pin_in,
                                       uint16_t pin_len) {
  printf("HSM: Verifying PIN securely.\n");

  storage_pin_data_t pin_data;
  if (!storage_load_pin_data(&pin_data)) {
    return HSM_PIN_ERROR;
  }

  // Check if PIN is locked
  if (pin_data.retries_remaining == 0) {
    printf("HSM: PIN is locked.\n");
    return HSM_PIN_LOCKED;
  }

  // Hash the input PIN
  uint8_t input_hash[32];
  hash_pin(pin_in, pin_len, input_hash);

  // Compare hashes
  uint8_t diff = 0;
  for (int i = 0; i < 32; i++) {
    diff |= (input_hash[i] ^ pin_data.pin_hash[i]);
  }

  if (diff == 0) {
    // PIN correct - reset retry counter
    pin_data.retries_remaining = HSM_PIN_MAX_RETRIES;
    storage_save_pin_data(&pin_data);
    printf("HSM: PIN verified successfully.\n");
    return HSM_PIN_SUCCESS;
  } else {
    // PIN incorrect - decrement retry counter
    pin_data.retries_remaining--;
    storage_save_pin_data(&pin_data);
    printf("HSM: PIN incorrect. %d retries remaining.\n",
           pin_data.retries_remaining);
    return HSM_PIN_INCORRECT;
  }
}

uint8_t hsm_get_pin_retries_remaining(void) {
  storage_pin_data_t pin_data;
  if (!storage_load_pin_data(&pin_data)) {
    return 0;
  }
  return pin_data.retries_remaining;
}

bool hsm_reset_pin_counter(const uint8_t *admin_pin, uint16_t admin_pin_len) {
  printf("HSM: Attempting to reset PIN counter with admin PIN.\n");

  storage_pin_data_t pin_data;
  if (!storage_load_pin_data(&pin_data)) {
    return false;
  }

  // Hash the admin PIN
  uint8_t admin_hash[32];
  hash_pin(admin_pin, admin_pin_len, admin_hash);

  // Compare with stored admin PIN hash
  uint8_t diff = 0;
  for (int i = 0; i < 32; i++) {
    diff |= (admin_hash[i] ^ pin_data.admin_pin_hash[i]);
  }

  if (diff == 0) {
    // Admin PIN correct - reset user PIN counter
    pin_data.retries_remaining = HSM_PIN_MAX_RETRIES;
    storage_save_pin_data(&pin_data);
    printf("HSM: PIN counter reset successfully.\n");
    return true;
  }

  printf("HSM: Admin PIN incorrect.\n");
  return false;
}

// Legacy PIN verification - DEPRECATED
bool hsm_verify_pin(const uint8_t *pin_in, uint16_t pin_len) {
  printf("HSM: Using legacy PIN verification (deprecated).\n");
  hsm_pin_result_t result = hsm_verify_pin_secure(pin_in, pin_len);
  return (result == HSM_PIN_SUCCESS);
}

bool hsm_calculate_oath(const uint8_t *challenge_in, uint16_t challenge_len,
                        uint8_t *code_out, uint16_t *code_len) {
  printf("HSM: OATH Calc (mbedTLS HMAC-SHA1).\n");
  // HARDCODED SECRET for Demo: "abba" (0x61 0x62 0x62 0x61)
  // Matches the default manual entry in walkthrough.
  uint8_t secret[] = "abba";

  uint8_t hmac_result[20]; // SHA1 is 20 bytes

  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

  if (mbedtls_md_hmac(md_info, secret, 4, challenge_in, challenge_len,
                      hmac_result) != 0) {
    return false;
  }

  // Truncate
  uint8_t offset = hmac_result[19] & 0x0F;
  uint32_t bin_code = (hmac_result[offset] & 0x7F) << 24 |
                      (hmac_result[offset + 1] & 0xFF) << 16 |
                      (hmac_result[offset + 2] & 0xFF) << 8 |
                      (hmac_result[offset + 3] & 0xFF);

  // Convert to 6 digits? Usually OATH returns the full code (uint32) and app
  // truncates. Spec says: "The R response Data field contains the 4 bytes ...
  // of the truncated value" So we return the big-endian 4-byte integer.

  // However, Yk codes are usually standard TOTP arithmetic.
  // The previous stub was returning raw bytes.
  // Let's pass the 4-byte truncated value.

  code_out[0] = (bin_code >> 24) & 0xFF;
  code_out[1] = (bin_code >> 16) & 0xFF;
  code_out[2] = (bin_code >> 8) & 0xFF;
  code_out[3] = bin_code & 0xFF;
  *code_len = 4;

  return true;
}

// Generate ECC P-256 keypair and store in secure slot
bool hsm_generate_key_ecc(hsm_key_slot_t slot, hsm_pubkey_t *pubkey_out) {
  ensure_init();
  printf("HSM: Generating ECC P-256 Key for slot %d with error handling...\n",
         slot);

  if (slot >= HSM_KEY_SLOT_MAX) {
    ERROR_REPORT_ERROR(ERROR_CRYPTO_INVALID_KEY, "Invalid key slot: %d", slot);
    return false;
  }

  // Start timeout for cryptographic operation
  if (!timeout_start(DEFAULT_TIMEOUTS.crypto_operation_timeout_ms)) {
    ERROR_REPORT_ERROR(ERROR_TIMEOUT_CRYPTO_OPERATION,
                       "Failed to start crypto timeout");
    return false;
  }

  mbedtls_ecp_keypair key;
  mbedtls_ecp_keypair_init(&key);

  if (mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &key,
                          mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    timeout_reset();
    ERROR_REPORT_ERROR(ERROR_CRYPTO_KEY_GENERATION,
                       "ECC key generation failed for slot %d", slot);
    mbedtls_ecp_keypair_free(&key);
    return false;
  }

  timeout_reset();

  // Extract public key
  storage_hsm_key_t storage_key = {0};
  mbedtls_mpi_write_binary(&key.MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X),
                           storage_key.pub_x, 32);
  mbedtls_mpi_write_binary(&key.MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y),
                           storage_key.pub_y, 32);

  // Extract and encrypt private key before storage
  uint8_t raw_priv[32];
  mbedtls_mpi_write_binary(&key.MBEDTLS_PRIVATE(d), raw_priv, 32);
  encrypt_decrypt_key(raw_priv, storage_key.priv);

  // Clear raw private key from memory immediately
  memset(raw_priv, 0, sizeof(raw_priv));

  storage_key.active = 1;

  // Store encrypted key with retry mechanism
  if (!retry_operation_with_context(
          (bool (*)(void *))storage_save_hsm_key_wrapper, &(struct {
            uint8_t slot;
            const storage_hsm_key_t *key;
          }){slot, &storage_key},
          &RETRY_CONFIG_STORAGE)) {
    ERROR_REPORT_ERROR(ERROR_STORAGE_WRITE_FAILED,
                       "Failed to store key in slot %d", slot);
    mbedtls_ecp_keypair_free(&key);
    // Clear storage key from memory
    memset(&storage_key, 0, sizeof(storage_key));
    return false;
  }

  // Return public key
  if (pubkey_out) {
    memcpy(pubkey_out->x, storage_key.pub_x, 32);
    memcpy(pubkey_out->y, storage_key.pub_y, 32);
  }

  // Clear storage key from memory
  memset(&storage_key, 0, sizeof(storage_key));
  mbedtls_ecp_keypair_free(&key);

  printf("HSM: Key generated and stored securely in slot %d\n", slot);
  return true;
}

// Load public key from secure storage slot
bool hsm_load_pubkey(hsm_key_slot_t slot, hsm_pubkey_t *pubkey_out) {
  if (slot >= HSM_KEY_SLOT_MAX || !pubkey_out) {
    return false;
  }

  storage_hsm_key_t storage_key;
  if (!storage_load_hsm_key(slot, &storage_key)) {
    return false;
  }

  memcpy(pubkey_out->x, storage_key.pub_x, 32);
  memcpy(pubkey_out->y, storage_key.pub_y, 32);

  // Clear storage key from memory
  memset(&storage_key, 0, sizeof(storage_key));
  return true;
}

// Sign hash using private key from secure slot (private key never leaves HSM)
bool hsm_sign_ecc_slot(hsm_key_slot_t slot, const uint8_t *hash_in,
                       uint16_t hash_len, uint8_t *signature_out,
                       uint16_t *signature_len) {
  ensure_init();
  printf("HSM: Signing with key from slot %d...\n", slot);

  if (slot >= HSM_KEY_SLOT_MAX) {
    printf("HSM: Invalid key slot %d\n", slot);
    return false;
  }

  // Load encrypted key from storage
  storage_hsm_key_t storage_key;
  if (!storage_load_hsm_key(slot, &storage_key)) {
    printf("HSM: No key found in slot %d\n", slot);
    return false;
  }

  // Decrypt private key temporarily for signing
  uint8_t raw_priv[32];
  encrypt_decrypt_key(storage_key.priv, raw_priv);

  // Clear storage key from memory immediately
  memset(&storage_key, 0, sizeof(storage_key));

  // Perform signing operation
  mbedtls_ecp_keypair key;
  mbedtls_ecp_keypair_init(&key);
  mbedtls_ecp_group_load(&key.MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1);
  mbedtls_mpi_read_binary(&key.MBEDTLS_PRIVATE(d), raw_priv, 32);

  // Clear raw private key from memory immediately after loading
  memset(raw_priv, 0, sizeof(raw_priv));

  mbedtls_mpi r, s;
  mbedtls_mpi_init(&r);
  mbedtls_mpi_init(&s);

  bool success = false;
  if (mbedtls_ecdsa_sign(&key.MBEDTLS_PRIVATE(grp), &r, &s,
                         &key.MBEDTLS_PRIVATE(d), hash_in, hash_len,
                         mbedtls_ctr_drbg_random, &ctr_drbg) == 0) {
    mbedtls_mpi_write_binary(&r, signature_out, 32);
    mbedtls_mpi_write_binary(&s, signature_out + 32, 32);
    *signature_len = 64;
    success = true;
    printf("HSM: Signature generated successfully\n");
  } else {
    printf("HSM: Signature generation failed\n");
  }

  // Clean up all cryptographic material
  mbedtls_mpi_free(&r);
  mbedtls_mpi_free(&s);
  mbedtls_ecp_keypair_free(&key);

  return success;
}

// Key management functions
bool hsm_key_exists(hsm_key_slot_t slot) {
  if (slot >= HSM_KEY_SLOT_MAX) {
    return false;
  }

  storage_hsm_key_t storage_key;
  return storage_load_hsm_key(slot, &storage_key);
}

bool hsm_delete_key(hsm_key_slot_t slot) {
  if (slot >= HSM_KEY_SLOT_MAX) {
    return false;
  }

  printf("HSM: Deleting key from slot %d\n", slot);
  return storage_delete_hsm_key(slot);
}

// Legacy function for backward compatibility - DEPRECATED
bool hsm_generate_key_ecc_legacy(hsm_keypair_t *keypair_out) {
  printf("HSM: Using legacy key generation (deprecated) - keys not stored "
         "securely\n");
  ensure_init();

  mbedtls_ecp_keypair key;
  mbedtls_ecp_keypair_init(&key);

  if (mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &key,
                          mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    printf("HSM: Key Gen Failed\n");
    mbedtls_ecp_keypair_free(&key);
    return false;
  }

  mbedtls_mpi_write_binary(&key.MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X),
                           keypair_out->pub.x, 32);
  mbedtls_mpi_write_binary(&key.MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y),
                           keypair_out->pub.y, 32);
  mbedtls_mpi_write_binary(&key.MBEDTLS_PRIVATE(d), keypair_out->priv, 32);

  mbedtls_ecp_keypair_free(&key);
  return true;
}

// Legacy signing function - DEPRECATED (exposes private key)
bool hsm_sign_ecc(const uint8_t *priv_key, const uint8_t *hash_in,
                  uint16_t hash_len, uint8_t *signature_out,
                  uint16_t *signature_len) {
  printf("HSM: Using legacy signing (deprecated) - private key exposed\n");
  ensure_init();

  mbedtls_ecp_keypair key;
  mbedtls_ecp_keypair_init(&key);
  mbedtls_ecp_group_load(&key.MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1);
  mbedtls_mpi_read_binary(&key.MBEDTLS_PRIVATE(d), priv_key, 32);

  mbedtls_mpi r, s;
  mbedtls_mpi_init(&r);
  mbedtls_mpi_init(&s);

  if (mbedtls_ecdsa_sign(&key.MBEDTLS_PRIVATE(grp), &r, &s,
                         &key.MBEDTLS_PRIVATE(d), hash_in, hash_len,
                         mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_ecp_keypair_free(&key);
    return false;
  }

  mbedtls_mpi_write_binary(&r, signature_out, 32);
  mbedtls_mpi_write_binary(&s, signature_out + 32, 32);
  *signature_len = 64;

  mbedtls_mpi_free(&r);
  mbedtls_mpi_free(&s);
  mbedtls_ecp_keypair_free(&key);

  return true;
}

// Wrapper functions for retry mechanism compatibility
static bool storage_save_hsm_key_wrapper(void *context) {
  struct {
    uint8_t slot;
    const storage_hsm_key_t *key;
  } *ctx = context;
  return storage_save_hsm_key(ctx->slot, ctx->key);
}