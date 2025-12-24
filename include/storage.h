#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stdint.h>

// Initialize storage (load cache if needed)
void storage_init(void);

// Storage Constants
#define STORAGE_SIZE_BYTES (32 * 1024) // 32KB Encrypted Storage
#define STORAGE_MAGIC 0x53454352       // "SECR"
#define STORAGE_VERSION 2

// OATH Storage
#define STORAGE_OATH_MAX_ACCOUNTS 50

typedef struct {
  uint8_t name[64];
  uint8_t name_len;
  uint8_t key[64];
  uint8_t key_len;
  uint8_t type;     // 1=TOTP, 2=HOTP
  uint8_t digits;   // 6 or 8
  uint8_t active;   // 1 if active
  uint32_t counter; // For HOTP
} storage_oath_entry_t;

bool storage_load_oath_account(uint8_t index, storage_oath_entry_t *out_entry);
bool storage_save_oath_account(uint8_t index,
                               const storage_oath_entry_t *entry);
bool storage_delete_oath_account(uint8_t index);

// FIDO2 / WebAuthn Storage (Resident Keys)
#define STORAGE_FIDO2_MAX_CREDS 50

typedef struct {
  uint8_t rp_id_hash[32];
  uint8_t user_id[64];
  uint8_t user_id_len;
  uint8_t cred_id[64]; // Handle
  uint8_t cred_id_len;
  uint8_t priv_key[32]; // Private Scalar
  uint32_t sign_count;
  uint8_t active;
  uint8_t flags;
} storage_fido2_entry_t;

bool storage_load_fido2_cred(uint8_t index, storage_fido2_entry_t *out_entry);
bool storage_save_fido2_cred(uint8_t index, const storage_fido2_entry_t *entry);
bool storage_delete_fido2_cred(uint8_t index);
bool storage_find_fido2_cred_by_rp(const uint8_t *rp_id_hash,
                                   storage_fido2_entry_t *out_entry,
                                   uint8_t *index_out);
uint8_t storage_find_fido2_creds_all_by_rp(const uint8_t *rp_id_hash,
                                           uint8_t *indices_out,
                                           uint8_t max_indices);

// HSM Key Storage
#define STORAGE_HSM_MAX_KEYS 4

typedef struct {
  uint8_t pub_x[32]; // Or RSA Modulus part
  uint8_t pub_y[32];
  uint8_t priv[128]; // Encrypted private key material (supports larger keys)
  uint8_t type;      // 0=ECC, 1=RSA
  uint8_t active;    // 1 if used
  uint8_t fingerprint[20];
} storage_hsm_key_t;

bool storage_load_hsm_key(uint8_t slot, storage_hsm_key_t *out_key);
bool storage_save_hsm_key(uint8_t slot, const storage_hsm_key_t *key);
bool storage_delete_hsm_key(uint8_t slot);

// System / PIN storage
typedef struct {
  uint8_t retries_remaining;
  uint8_t pin_hash[32];
  uint8_t admin_pin_hash[32];
  uint32_t global_counter;
} storage_system_t;

bool storage_load_pin_data(storage_system_t *out_data);
bool storage_save_pin_data(const storage_system_t *data);

// IO
void storage_commit(void);
bool storage_reset_device(void);

#endif // STORAGE_H
