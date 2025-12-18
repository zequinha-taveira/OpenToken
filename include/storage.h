#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stdint.h>

// Initialize storage (load cache if needed)
void storage_init(void);

// OATH Storage
// Simple slot-based storage
#define STORAGE_OATH_MAX_ACCOUNTS 8

typedef struct {
  uint8_t name[64];
  uint8_t name_len;
  uint8_t key[64];
  uint8_t key_len;
  uint8_t prop;
  uint8_t active;   // 1 if active, 0 if empty
  uint32_t counter; // For HOTP
} storage_oath_entry_t;

bool storage_load_oath_account(uint8_t index, storage_oath_entry_t *out_entry);
bool storage_save_oath_account(uint8_t index,
                               const storage_oath_entry_t *entry);
bool storage_delete_oath_account(uint8_t index);

// FIDO2 / WebAuthn Storage (Resident Keys)
#define STORAGE_FIDO2_MAX_CREDS 4

typedef struct {
  uint8_t rp_id_hash[32];
  uint8_t user_id[32];
  uint8_t user_id_len;
  uint8_t cred_id[32];
  uint8_t priv_key[32];
  uint32_t sign_count;
  uint8_t active;
} storage_fido2_entry_t;

bool storage_load_fido2_cred(uint8_t index, storage_fido2_entry_t *out_entry);
bool storage_save_fido2_cred(uint8_t index, const storage_fido2_entry_t *entry);
bool storage_delete_fido2_cred(uint8_t index);
bool storage_find_fido2_cred_by_rp(const uint8_t *rp_id_hash,
                                   storage_fido2_entry_t *out_entry,
                                   uint8_t *index_out);

// Global Commit (if utilizing caching, otherwise direct write)
void storage_commit(void);

#endif // STORAGE_H
