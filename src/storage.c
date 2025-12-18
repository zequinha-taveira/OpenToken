#include "storage.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // For proper compilation
#include <string.h>


// Pico SDK Headers
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

// Check Flash usage.
// We will use the *last* sector of flash.
// Default PICO_FLASH_SIZE_BYTES is often 2MB (2 * 1024 * 1024)
// FLASH_SECTOR_SIZE is 4096 (4KB)

// Determine offset. If PICO_FLASH_SIZE_BYTES is not defined, assume 2MB.
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#define STORAGE_FLAG_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define STORAGE_MAGIC 0xDEADBEEF

typedef struct {
  uint8_t pin[32];
  uint8_t pin_len;
} storage_system_t;

// Layout:
// Header (Magic + Version): 8 bytes
// OATH Entries: 8 * sizeof(entry)

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t version;
  storage_oath_entry_t oath_entries[STORAGE_OATH_MAX_ACCOUNTS];
  storage_fido2_entry_t fido2_entries[STORAGE_FIDO2_MAX_CREDS];
  storage_system_t system;
  // Future expansion...
  uint8_t _padding[FLASH_SECTOR_SIZE - 8 -
                   (sizeof(storage_oath_entry_t) * STORAGE_OATH_MAX_ACCOUNTS) -
                   (sizeof(storage_fido2_entry_t) * STORAGE_FIDO2_MAX_CREDS)];
} storage_flash_layout_t;

// Verify size
_Static_assert(sizeof(storage_flash_layout_t) == FLASH_SECTOR_SIZE,
               "Storage struct must match sector size");

static storage_flash_layout_t g_cache;
static bool g_dirty = false;

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

void storage_init(void) {
  // Read flash into cache
  const uint8_t *flash_target_contents =
      (const uint8_t *)(XIP_BASE + STORAGE_FLAG_OFFSET);
  memcpy(&g_cache, flash_target_contents, sizeof(storage_flash_layout_t));

  if (g_cache.magic != STORAGE_MAGIC) {
    printf("Storage: No valid storage found (Magic: %08X). formatting...\n",
           g_cache.magic);
    memset(&g_cache, 0, sizeof(storage_flash_layout_t));
    g_cache.magic = STORAGE_MAGIC;
    g_cache.version = 1;
    g_dirty = true;
    storage_commit();
  } else {
    printf("Storage: Valid storage loaded.\n");
  }
}

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

void storage_commit(void) {
  if (!g_dirty)
    return;

  printf("Storage: Committing to Flash...\n");
  // Disable interrupts to prevent crash during flash write (XIP is unavailable)
  uint32_t ints = save_and_disable_interrupts();

  flash_range_erase(STORAGE_FLAG_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(STORAGE_FLAG_OFFSET, (const uint8_t *)&g_cache,
                      FLASH_SECTOR_SIZE);

  restore_interrupts(ints);
  g_dirty = false;
  printf("Storage: Commit done.\n");
}
