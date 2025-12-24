#include "storage.h"
#include "error_handling.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Pico SDK Headers
#include "hardware/flash.h"
#include "hardware/structs/otp.h" // For OTP access (RP2350 specific if avail, or stub)
#include "hardware/sync.h"
#include "pico/stdlib.h"

// MbedTLS
#include "mbedtls/gcm.h"
#include "mbedtls/platform.h"
#include "mbedtls/sha256.h"

// Pico SDK
#include "pico/unique_id.h"

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024) // Default to 16MB for Tenstar
#endif

// Storage is at the very end of flash
#define STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - STORAGE_SIZE_BYTES)
#define STORAGE_NONCE_SIZE 12
#define STORAGE_TAG_SIZE 16

// Layout of the raw flash data
// [NONCE (12)] [TAG (16)] [ENCRYPTED_DATA (Remainder)]
#define STORAGE_HEADER_SIZE (STORAGE_NONCE_SIZE + STORAGE_TAG_SIZE)
#define STORAGE_PAYLOAD_SIZE (STORAGE_SIZE_BYTES - STORAGE_HEADER_SIZE)

// The Decrypted Cache Structure
typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t version;
  storage_system_t system;
  storage_oath_entry_t oath_entries[STORAGE_OATH_MAX_ACCOUNTS];
  storage_fido2_entry_t fido2_entries[STORAGE_FIDO2_MAX_CREDS];
  storage_hsm_key_t hsm_keys[STORAGE_HSM_MAX_KEYS];
  // Helper to fill the rest with zeros or future usage
  uint8_t _padding[STORAGE_PAYLOAD_SIZE - 8 - sizeof(storage_system_t) -
                   (sizeof(storage_oath_entry_t) * STORAGE_OATH_MAX_ACCOUNTS) -
                   (sizeof(storage_fido2_entry_t) * STORAGE_FIDO2_MAX_CREDS) -
                   (sizeof(storage_hsm_key_t) * STORAGE_HSM_MAX_KEYS)];
} storage_cache_t;

// Compile-time check to ensure cache fits in payload
_Static_assert(sizeof(storage_cache_t) <= STORAGE_PAYLOAD_SIZE,
               "Storage cache exceeds allocated payload size!");

// Global RAM Cache (Decrypted)
static storage_cache_t g_cache;
static bool g_dirty = false;
static bool g_initialized = false;

static void get_master_key(uint8_t *key_out) {
  // Use RP2350 Unique Board ID to derive a device-specific key
  pico_unique_board_id_t id;
  pico_get_unique_board_id(&id);

  // Hash the unique ID with a salt to create the master key
  // In a real production environment, this should incorporate OTP secrets
  // that are only readable by the Secure World.
  const char *salt = "OpenToken-Master-Key-Salt-v1";

  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0); // SHA-256
  mbedtls_sha256_update(&sha_ctx, (const uint8_t *)id.id, 8);
  mbedtls_sha256_update(&sha_ctx, (const uint8_t *)salt, strlen(salt));
  mbedtls_sha256_finish(&sha_ctx, key_out);
  mbedtls_sha256_free(&sha_ctx);
}

static bool decrypt_storage(const uint8_t *src, storage_cache_t *dst) {
  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  uint8_t key[32];
  get_master_key(key);

  int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256);
  if (ret != 0) {
    mbedtls_gcm_free(&ctx);
    return false;
  }

  const uint8_t *nonce = src;
  const uint8_t *tag = src + STORAGE_NONCE_SIZE;
  const uint8_t *ciphertext = src + STORAGE_HEADER_SIZE;

  // Authenticated Decryption
  ret = mbedtls_gcm_auth_decrypt(
      &ctx, STORAGE_PAYLOAD_SIZE, nonce, STORAGE_NONCE_SIZE, NULL, 0, // No AAD
      tag, STORAGE_TAG_SIZE, ciphertext, (uint8_t *)dst);

  mbedtls_gcm_free(&ctx);
  return (ret == 0);
}

static bool encrypt_storage(const storage_cache_t *src, uint8_t *dst) {
  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  uint8_t key[32];
  get_master_key(key);

  int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256);
  if (ret != 0) {
    mbedtls_gcm_free(&ctx);
    return false;
  }

  uint8_t *nonce = dst;
  uint8_t *tag = dst + STORAGE_NONCE_SIZE;
  uint8_t *ciphertext = dst + STORAGE_HEADER_SIZE;

  // Generate Nonce (Random)
  for (int i = 0; i < STORAGE_NONCE_SIZE; i++) {
    nonce[i] = (uint8_t)(rand() % 256); // Better RNG needed in production
  }

  ret = mbedtls_gcm_crypt_and_tag(
      &ctx, MBEDTLS_GCM_ENCRYPT, STORAGE_PAYLOAD_SIZE, nonce,
      STORAGE_NONCE_SIZE, NULL, 0, (const uint8_t *)src, ciphertext,
      STORAGE_TAG_SIZE, tag);

  mbedtls_gcm_free(&ctx);
  return (ret == 0);
}

// ----------------------------------------------------------------------------
// Core Storage API
// ----------------------------------------------------------------------------

void storage_init(void) {
  if (g_initialized)
    return;

  printf("Storage: Initializing Encrypted Storage...\n");

  const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + STORAGE_OFFSET);

  // Attempt to decrypt
  if (decrypt_storage(flash_ptr, &g_cache)) {
    // Decryption success, check Magic
    if (g_cache.magic == STORAGE_MAGIC) {
      printf("Storage: Loaded and Decrypted Successfully.\n");
      g_initialized = true;
      return;
    } else {
      printf("Storage: Valid crypto but invalid magic (First boot?). "
             "Formatting...\n");
    }
  } else {
    printf("Storage: Decryption failed (Integrity or Key mismatch). "
           "Formatting...\n");
  }

  // Format / Reset
  memset(&g_cache, 0, sizeof(storage_cache_t));
  g_cache.magic = STORAGE_MAGIC;
  g_cache.version = STORAGE_VERSION;

  // Defaults
  g_cache.system.retries_remaining = 3;
  // PIN hashes would be set by user later

  g_dirty = true;
  g_initialized = true;
  storage_commit();
}

void storage_commit(void) {
  if (!g_dirty)
    return;

  // Buffer for the Encrypted Blob (needs 32KB RAM)
  // We allocate on heap to avoid stack overflow, assuming ample heap on RP2350
  uint8_t *chk_buffer = malloc(STORAGE_SIZE_BYTES);
  if (!chk_buffer) {
    ERROR_REPORT_ERROR(ERROR_OUT_OF_MEMORY,
                       "Failed to allocate buffer for storage commit");
    return;
  }

  printf("Storage: Encrypting and Committing...\n");

  if (!encrypt_storage(&g_cache, chk_buffer)) {
    free(chk_buffer);
    ERROR_REPORT_ERROR(ERROR_CRYPTO_FAILURE, "Storage encryption failed");
    return;
  }

  // Write to Flash
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(STORAGE_OFFSET, STORAGE_SIZE_BYTES);
  flash_range_program(STORAGE_OFFSET, chk_buffer, STORAGE_SIZE_BYTES);
  restore_interrupts(ints);

  free(chk_buffer);
  g_dirty = false;
  printf("Storage: Commit Complete.\n");
}

bool storage_reset_device(void) {
  memset(&g_cache, 0, sizeof(storage_cache_t));
  g_cache.magic = STORAGE_MAGIC;
  g_cache.version = STORAGE_VERSION;
  g_cache.system.retries_remaining = 3;
  g_dirty = true;
  storage_commit();
  return true;
}

// ----------------------------------------------------------------------------
// Get/Set Implementations
// ----------------------------------------------------------------------------

// OATH
bool storage_load_oath_account(uint8_t index, storage_oath_entry_t *out_entry) {
  if (index >= STORAGE_OATH_MAX_ACCOUNTS)
    return false;
  if (g_cache.oath_entries[index].active != 1)
    return false;
  memcpy(out_entry, &g_cache.oath_entries[index], sizeof(storage_oath_entry_t));
  return true;
}

bool storage_save_oath_account(uint8_t index,
                               const storage_oath_entry_t *entry) {
  if (index >= STORAGE_OATH_MAX_ACCOUNTS)
    return false;
  memcpy(&g_cache.oath_entries[index], entry, sizeof(storage_oath_entry_t));
  g_cache.oath_entries[index].active = 1;
  g_dirty = true;
  storage_commit();
  return true;
}

bool storage_delete_oath_account(uint8_t index) {
  if (index >= STORAGE_OATH_MAX_ACCOUNTS)
    return false;
  memset(&g_cache.oath_entries[index], 0, sizeof(storage_oath_entry_t));
  g_dirty = true;
  storage_commit();
  return true;
}

// FIDO2
bool storage_load_fido2_cred(uint8_t index, storage_fido2_entry_t *out_entry) {
  if (index >= STORAGE_FIDO2_MAX_CREDS)
    return false;
  if (g_cache.fido2_entries[index].active != 1)
    return false;
  memcpy(out_entry, &g_cache.fido2_entries[index],
         sizeof(storage_fido2_entry_t));
  return true;
}

bool storage_save_fido2_cred(uint8_t index,
                             const storage_fido2_entry_t *entry) {
  if (index >= STORAGE_FIDO2_MAX_CREDS)
    return false;
  memcpy(&g_cache.fido2_entries[index], entry, sizeof(storage_fido2_entry_t));
  g_cache.fido2_entries[index].active = 1;
  g_dirty = true;
  storage_commit();
  return true;
}

bool storage_delete_fido2_cred(uint8_t index) {
  if (index >= STORAGE_FIDO2_MAX_CREDS)
    return false;
  memset(&g_cache.fido2_entries[index], 0, sizeof(storage_fido2_entry_t));
  g_dirty = true;
  storage_commit();
  return true;
}

bool storage_find_fido2_cred_by_rp(const uint8_t *rp_id_hash,
                                   storage_fido2_entry_t *out_entry,
                                   uint8_t *index_out) {
  for (uint8_t i = 0; i < STORAGE_FIDO2_MAX_CREDS; i++) {
    if (g_cache.fido2_entries[i].active == 1 &&
        memcmp(g_cache.fido2_entries[i].rp_id_hash, rp_id_hash, 32) == 0) {
      if (out_entry)
        memcpy(out_entry, &g_cache.fido2_entries[i],
               sizeof(storage_fido2_entry_t));
      if (index_out)
        *index_out = i;
      return true;
    }
  }
  return false;
}

uint8_t storage_find_fido2_creds_all_by_rp(const uint8_t *rp_id_hash,
                                           uint8_t *indices_out,
                                           uint8_t max_indices) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < STORAGE_FIDO2_MAX_CREDS && count < max_indices; i++) {
    if (g_cache.fido2_entries[i].active == 1 &&
        memcmp(g_cache.fido2_entries[i].rp_id_hash, rp_id_hash, 32) == 0) {
      if (indices_out) {
        indices_out[count] = i;
      }
      count++;
    }
  }
  return count;
}

// HSM
bool storage_load_hsm_key(uint8_t slot, storage_hsm_key_t *out_key) {
  if (slot >= STORAGE_HSM_MAX_KEYS)
    return false;
  if (g_cache.hsm_keys[slot].active != 1)
    return false;
  memcpy(out_key, &g_cache.hsm_keys[slot], sizeof(storage_hsm_key_t));
  return true;
}

bool storage_save_hsm_key(uint8_t slot, const storage_hsm_key_t *key) {
  if (slot >= STORAGE_HSM_MAX_KEYS)
    return false;
  memcpy(&g_cache.hsm_keys[slot], key, sizeof(storage_hsm_key_t));
  g_cache.hsm_keys[slot].active = 1;
  g_dirty = true;
  storage_commit();
  return true;
}

bool storage_delete_hsm_key(uint8_t slot) {
  if (slot >= STORAGE_HSM_MAX_KEYS)
    return false;
  memset(&g_cache.hsm_keys[slot], 0, sizeof(storage_hsm_key_t));
  g_dirty = true;
  storage_commit();
  return true;
}

// System
bool storage_load_pin_data(storage_system_t *out_data) {
  memcpy(out_data, &g_cache.system, sizeof(storage_system_t));
  return true;
}

bool storage_save_pin_data(const storage_system_t *data) {
  memcpy(&g_cache.system, data, sizeof(storage_system_t));
  g_dirty = true;
  storage_commit();
  return true;
}
