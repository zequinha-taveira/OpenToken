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

// Global Commit (if utilizing caching, otherwise direct write)
void storage_commit(void);

#endif // STORAGE_H
