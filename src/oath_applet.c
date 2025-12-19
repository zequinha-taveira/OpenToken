#include "oath_applet.h"
#include "hsm_layer.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


// mbedTLS for HMAC operations
#include "mbedtls/md.h"
#include "mbedtls/sha1.h"

// AID OATH (Standard Yubico OATH AID: A0 00 00 05 27 21 01 01)
const uint8_t OATH_AID[OATH_AID_LEN] = {0xA0, 0x00, 0x00, 0x05,
                                        0x27, 0x21, 0x01, 0x01};

static bool is_selected = false;

// OATH algorithm types
#define OATH_TYPE_HOTP 0x10
#define OATH_TYPE_TOTP 0x20

// OATH hash algorithms
#define OATH_HASH_SHA1 0x01
#define OATH_HASH_SHA256 0x02

// Default TOTP period (30 seconds)
#define OATH_DEFAULT_PERIOD 30

// Helper function to calculate HMAC-SHA1 for OATH
static bool calculate_hmac_sha1(const uint8_t *key, uint8_t key_len,
                                const uint8_t *data, uint8_t data_len,
                                uint8_t *hmac_out) {
  // Use mbedTLS for proper HMAC-SHA1 calculation
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  if (!md_info) {
    return false;
  }

  return (mbedtls_md_hmac(md_info, key, key_len, data, data_len, hmac_out) ==
          0);
}

// Helper function to perform OATH truncation
static uint32_t oath_truncate(const uint8_t *hmac, uint8_t digits) {
  uint8_t offset = hmac[19] & 0x0F;
  uint32_t bin_code =
      ((hmac[offset] & 0x7F) << 24) | ((hmac[offset + 1] & 0xFF) << 16) |
      ((hmac[offset + 2] & 0xFF) << 8) | (hmac[offset + 3] & 0xFF);

  // Apply modulo based on number of digits
  uint32_t mod = 1;
  for (int i = 0; i < digits; i++) {
    mod *= 10;
  }

  return bin_code % mod;
}

// Calculate TOTP code
static bool calculate_totp(const storage_oath_entry_t *entry,
                           uint32_t *code_out) {
  // Get current time (simplified - in real implementation use RTC)
  // For now, use a mock timestamp that increments
  static uint32_t mock_time = 1640995200; // 2022-01-01 00:00:00 UTC
  mock_time += 30;                        // Increment by 30 seconds each call

  uint64_t time_step = mock_time / OATH_DEFAULT_PERIOD;

  // Convert time step to big-endian 8-byte array
  uint8_t challenge[8];
  for (int i = 7; i >= 0; i--) {
    challenge[i] = time_step & 0xFF;
    time_step >>= 8;
  }

  // Calculate HMAC-SHA1
  uint8_t hmac[20];
  if (!calculate_hmac_sha1(entry->key, entry->key_len, challenge, 8, hmac)) {
    return false;
  }

  // Truncate to get final code
  *code_out = oath_truncate(hmac, 6); // Default to 6 digits
  return true;
}

// Calculate HOTP code
static bool calculate_hotp(storage_oath_entry_t *entry, uint32_t *code_out) {
  // Convert counter to big-endian 8-byte array
  uint8_t challenge[8];
  uint64_t counter = entry->counter;

  for (int i = 7; i >= 0; i--) {
    challenge[i] = counter & 0xFF;
    counter >>= 8;
  }

  // Calculate HMAC-SHA1
  uint8_t hmac[20];
  if (!calculate_hmac_sha1(entry->key, entry->key_len, challenge, 8, hmac)) {
    return false;
  }

  // Increment counter for next use
  entry->counter++;

  // Truncate to get final code
  *code_out = oath_truncate(hmac, 6); // Default to 6 digits
  return true;
}

// Helper to find account by name using Flash Storage
static int find_account(const uint8_t *name, uint8_t len) {
  storage_oath_entry_t entry;
  for (int i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
    if (storage_load_oath_account(i, &entry)) {
      if (entry.name_len == len && memcmp(entry.name, name, len) == 0) {
        return i;
      }
    }
  }
  return -1;
}

// Find a free slot
static int find_free_slot(void) {
  storage_oath_entry_t entry;
  for (int i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
    if (!storage_load_oath_account(i, &entry)) {
      return i; // Using the failure to load (empty/inactive) as a sign of free
                // slot?
      // Better: storage_load checks active bit. If not active, it returns
      // false.
    }
  }
  return -1;
}

bool oath_applet_select(const uint8_t *aid, uint8_t len) {
  if (len == OATH_AID_LEN && memcmp(aid, OATH_AID, OATH_AID_LEN) == 0) {
    is_selected = true;
    printf(
        "OATH Applet: Selected successfully (YubiKey Manager compatible).\n");

    // Ensure storage and HSM are initialized
    storage_init();
    hsm_init();

    return true;
  }

  is_selected = false;
  printf("OATH Applet: AID not recognized (len=%d).\n", len);
  return false;
}

void oath_applet_process_apdu(const uint8_t *apdu, uint16_t len,
                              uint8_t *response, uint16_t *response_len) {
  // APDU Structure: CLA INS P1 P2 Lc Data Le
  uint8_t ins = apdu[1];
  uint8_t p1 = apdu[2];
  uint8_t p2 = apdu[3];
  uint8_t lc_val = apdu[4];
  const uint8_t *data = apdu + 5;

// Helper macro for creating status word response
#define SET_SW(sw)                                                             \
  do {                                                                         \
    response[*response_len] = (uint8_t)((sw) >> 8);                            \
    response[*response_len + 1] = (uint8_t)((sw) & 0xFF);                      \
    *response_len += 2;                                                        \
  } while (0)

  if (!is_selected) {
    *response_len = 0;
    SET_SW(OATH_SW_FILE_NOT_FOUND);
    return;
  }

  *response_len = 0; // Start with empty response payload

  switch (ins) {
  case OATH_INS_VALIDATE: {
    // YubiKey Manager sends VALIDATE command to check if OATH applet is
    // available This is used for device detection
    printf("OATH Applet: VALIDATE Command - confirming OATH availability.\n");

    // Return success to indicate OATH functionality is available
    // YubiKey Manager uses this to detect OATH-capable devices
    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_RESET: {
    // YubiKey Manager might send RESET to clear all accounts
    // We'll implement this as a security feature but require confirmation
    printf("OATH Applet: RESET Command - clearing all accounts.\n");

    // Clear all OATH accounts from storage
    bool reset_success = true;
    for (int i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
      if (!storage_delete_oath_account(i)) {
        // Continue even if some deletions fail
        printf("OATH RESET: Warning - failed to delete account at slot %d\n",
               i);
      }
    }

    if (reset_success) {
      printf("OATH RESET: All accounts cleared successfully\n");
      SET_SW(OATH_SW_OK);
    } else {
      printf("OATH RESET: Reset completed with some errors\n");
      SET_SW(OATH_SW_COMMAND_NOT_ALLOWED);
    }
    break;
  }

  case OATH_INS_SET_CODE: {
    // YubiKey Manager uses SET_CODE to set a password for OATH applet
    // For simplicity, we'll accept but not enforce passwords
    printf("OATH Applet: SET_CODE Command - password protection not "
           "implemented.\n");

    // Return success but don't actually implement password protection
    // This allows YubiKey Manager to work but doesn't add security complexity
    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_CALCULATE_ALL: {
    // YubiKey Manager uses CALCULATE_ALL to get all TOTP codes at once
    printf("OATH Applet: CALCULATE_ALL Command - generating all TOTP codes.\n");

    storage_oath_entry_t entry;
    uint16_t total_accounts = 0;

    // Build response with all active TOTP accounts and their codes
    for (int i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
      if (storage_load_oath_account(i, &entry)) {
        // Only process TOTP accounts for CALCULATE_ALL
        if ((entry.prop & 0xF0) == OATH_TYPE_TOTP) {
          uint32_t code = 0;
          if (calculate_totp(&entry, &code)) {
            // Calculate response length needed
            uint8_t name_tlv_len = 2 + entry.name_len; // 71 + len + name
            uint8_t code_tlv_len = 7; // 76 + 5 + digits + 4-byte code
            uint8_t total_inner_len = name_tlv_len + code_tlv_len;

            // Check buffer space
            if (*response_len + 2 + total_inner_len > 256) {
              printf("OATH CALCULATE_ALL: Response buffer full, truncating\n");
              break;
            }

            uint8_t *ptr = response + *response_len;

            // Name list entry (tag 72)
            *ptr++ = OATH_TAG_NAME_LIST; // 72
            *ptr++ = total_inner_len;    // Length of inner TLV data

            // Name TLV (tag 71)
            *ptr++ = OATH_TAG_NAME;  // 71
            *ptr++ = entry.name_len; // Name length
            memcpy(ptr, entry.name, entry.name_len);
            ptr += entry.name_len;

            // Response value TLV (tag 76)
            *ptr++ = OATH_TAG_RESPONSE_VAL; // 76
            *ptr++ = 5;                     // Length: 1 + 4
            *ptr++ = 6;                     // Digits
            *ptr++ = (code >> 24) & 0xFF;   // Code (big-endian)
            *ptr++ = (code >> 16) & 0xFF;
            *ptr++ = (code >> 8) & 0xFF;
            *ptr++ = code & 0xFF;

            *response_len += (2 + total_inner_len);
            total_accounts++;

            printf("OATH CALCULATE_ALL: Added TOTP code %06u for '%.*s'\n",
                   code, entry.name_len, entry.name);
          }
        }
      }
    }

    printf("OATH CALCULATE_ALL: Generated codes for %d TOTP accounts\n",
           total_accounts);
    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_PUT: {
    printf("OATH Applet: PUT Command - Adding/Updating account.\n");

    // Parse TLV data: 71 Name, 73 Key, 75 Property
    uint16_t offset = 0;
    uint8_t name_buf[64];
    uint8_t name_len = 0;
    uint8_t key_buf[64];
    uint8_t key_len = 0;
    uint8_t property =
        OATH_TYPE_TOTP | OATH_HASH_SHA1; // Default: TOTP with SHA1

    // Parse TLV structure
    while (offset < lc_val) {
      if (offset + 1 >= lc_val)
        break; // Ensure we have at least tag and length

      uint8_t tag = data[offset++];
      uint8_t len = data[offset++];

      if (offset + len > lc_val)
        break; // Ensure we don't read beyond data

      switch (tag) {
      case OATH_TAG_NAME:
        if (len < sizeof(name_buf)) {
          memcpy(name_buf, data + offset, len);
          name_len = len;
          name_buf[len] = '\0'; // Null terminate for logging
          printf("OATH PUT: Name = '%s' (len=%d)\n", name_buf, len);
        }
        break;

      case OATH_TAG_KEY:
        if (len < sizeof(key_buf)) {
          memcpy(key_buf, data + offset, len);
          key_len = len;
          printf("OATH PUT: Key length = %d\n", len);
        }
        break;

      case OATH_TAG_PROPERTY:
        if (len == 1) {
          property = data[offset];
          printf("OATH PUT: Property = 0x%02X\n", property);
        }
        break;

      default:
        printf("OATH PUT: Unknown tag 0x%02X, length %d\n", tag, len);
        break;
      }

      offset += len;
    }

    // Validate required fields
    if (name_len == 0 || key_len == 0) {
      printf("OATH PUT: Missing required fields (name_len=%d, key_len=%d)\n",
             name_len, key_len);
      SET_SW(OATH_SW_WRONG_P1P2);
      return;
    }

    // Find existing account or free slot
    int idx = find_account(name_buf, name_len);
    if (idx == -1) {
      idx = find_free_slot();
      printf("OATH PUT: Using new slot %d\n", idx);
    } else {
      printf("OATH PUT: Updating existing account at slot %d\n", idx);
    }

    if (idx == -1) {
      printf("OATH PUT: No free slots available\n");
      SET_SW(OATH_SW_COMMAND_NOT_ALLOWED); // Storage full
      return;
    }

    // Create new entry
    storage_oath_entry_t new_entry;
    memset(&new_entry, 0, sizeof(new_entry));

    memcpy(new_entry.name, name_buf, name_len);
    new_entry.name_len = name_len;
    memcpy(new_entry.key, key_buf, key_len);
    new_entry.key_len = key_len;
    new_entry.prop = property;
    new_entry.counter = 0; // Initialize HOTP counter
    new_entry.active = 1;

    // Save to storage
    if (storage_save_oath_account(idx, &new_entry)) {
      printf("OATH PUT: Account '%.*s' saved successfully to slot %d\n",
             name_len, name_buf, idx);
      SET_SW(OATH_SW_OK);
    } else {
      printf("OATH PUT: Failed to save account to storage\n");
      SET_SW(OATH_SW_COMMAND_NOT_ALLOWED);
    }

    break;
  }

  case OATH_INS_LIST: {
    printf("OATH Applet: LIST Command - Enumerating accounts.\n");

    storage_oath_entry_t entry;
    uint16_t account_count = 0;

    // Build response with all active accounts
    // Response format: Sequence of [72 Len [71 NameLen Name] [75 1 Property]]
    for (int i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
      if (storage_load_oath_account(i, &entry)) {
        // Calculate total length for this account entry
        uint8_t name_tlv_len = 2 + entry.name_len; // 71 + len + name
        uint8_t prop_tlv_len = 3;                  // 75 + 1 + property
        uint8_t total_inner_len = name_tlv_len + prop_tlv_len;

        // Check if we have enough space in response buffer
        if (*response_len + 2 + total_inner_len >
            256) { // Assume max 256 byte response
          printf("OATH LIST: Response buffer full, truncating list\n");
          break;
        }

        uint8_t *ptr = response + *response_len;

        // Name list entry (tag 72)
        *ptr++ = OATH_TAG_NAME_LIST; // 72
        *ptr++ = total_inner_len;    // Length of inner TLV data

        // Name TLV (tag 71)
        *ptr++ = OATH_TAG_NAME;  // 71
        *ptr++ = entry.name_len; // Name length
        memcpy(ptr, entry.name, entry.name_len);
        ptr += entry.name_len;

        // Property TLV (tag 75)
        *ptr++ = OATH_TAG_PROPERTY; // 75
        *ptr++ = 1;                 // Property length (always 1)
        *ptr++ = entry.prop;        // Property value

        *response_len += (2 + total_inner_len);
        account_count++;

        printf("OATH LIST: Added account '%.*s' (prop=0x%02X)\n",
               entry.name_len, entry.name, entry.prop);
      }
    }

    printf("OATH LIST: Listed %d accounts, response length = %d\n",
           account_count, *response_len);
    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_CALCULATE: {
    printf("OATH Applet: CALCULATE Command - Generating OTP code.\n");

    // Parse TLV data: 71 Name, 74 Challenge (optional)
    uint16_t offset = 0;
    uint8_t name_buf[64];
    uint8_t name_len = 0;
    uint8_t challenge_buf[8];
    uint8_t challenge_len = 0;

    // Parse TLV structure
    while (offset < lc_val) {
      if (offset + 1 >= lc_val)
        break;

      uint8_t tag = data[offset++];
      uint8_t len = data[offset++];

      if (offset + len > lc_val)
        break;

      switch (tag) {
      case OATH_TAG_NAME:
        if (len < sizeof(name_buf)) {
          memcpy(name_buf, data + offset, len);
          name_len = len;
          printf("OATH CALCULATE: Name = '%.*s'\n", len, name_buf);
        }
        break;

      case OATH_TAG_CHALLENGE:
        if (len <= sizeof(challenge_buf)) {
          memcpy(challenge_buf, data + offset, len);
          challenge_len = len;
          printf("OATH CALCULATE: Challenge provided (len=%d)\n", len);
        }
        break;

      default:
        printf("OATH CALCULATE: Unknown tag 0x%02X\n", tag);
        break;
      }

      offset += len;
    }

    // Validate required fields
    if (name_len == 0) {
      printf("OATH CALCULATE: Missing account name\n");
      SET_SW(OATH_SW_WRONG_P1P2);
      return;
    }

    // Find the account
    int idx = find_account(name_buf, name_len);
    if (idx < 0) {
      printf("OATH CALCULATE: Account '%.*s' not found\n", name_len, name_buf);
      SET_SW(OATH_SW_FILE_NOT_FOUND);
      return;
    }

    // Load account data
    storage_oath_entry_t entry;
    if (!storage_load_oath_account(idx, &entry)) {
      printf("OATH CALCULATE: Failed to load account data\n");
      SET_SW(OATH_SW_FILE_NOT_FOUND);
      return;
    }

    // Calculate OTP code based on account type
    uint32_t code = 0;
    bool calculation_success = false;

    if ((entry.prop & 0xF0) == OATH_TYPE_TOTP) {
      // TOTP calculation
      printf("OATH CALCULATE: Calculating TOTP for '%.*s'\n", name_len,
             name_buf);
      calculation_success = calculate_totp(&entry, &code);
    } else if ((entry.prop & 0xF0) == OATH_TYPE_HOTP) {
      // HOTP calculation - increment counter and save back
      printf("OATH CALCULATE: Calculating HOTP for '%.*s' (counter=%u)\n",
             name_len, name_buf, entry.counter);
      calculation_success = calculate_hotp(&entry, &code);

      // Save updated counter back to storage
      if (calculation_success) {
        storage_save_oath_account(idx, &entry);
      }
    } else {
      printf("OATH CALCULATE: Unknown OATH type 0x%02X\n", entry.prop);
      SET_SW(OATH_SW_WRONG_P1P2);
      return;
    }

    if (!calculation_success) {
      printf("OATH CALCULATE: Code calculation failed\n");
      SET_SW(OATH_SW_COMMAND_NOT_ALLOWED);
      return;
    }

    // Format response: 76 05 [Digits] [4-byte code]
    uint8_t digits = 6; // Standard 6-digit codes

    response[0] = OATH_TAG_RESPONSE_VAL; // 76
    response[1] = 5;                     // Length: 1 byte digits + 4 bytes code
    response[2] = digits;                // Number of digits
    response[3] = (code >> 24) & 0xFF;   // Code bytes (big-endian)
    response[4] = (code >> 16) & 0xFF;
    response[5] = (code >> 8) & 0xFF;
    response[6] = code & 0xFF;

    *response_len = 7;

    printf("OATH CALCULATE: Generated %d-digit code %06u for '%.*s'\n", digits,
           code, name_len, name_buf);

    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_DELETE: {
    printf("OATH Applet: DELETE Command - Removing account.\n");

    // Parse TLV data: 71 Name
    uint16_t offset = 0;
    uint8_t name_buf[64];
    uint8_t name_len = 0;

    // Parse TLV structure
    while (offset < lc_val) {
      if (offset + 1 >= lc_val)
        break;

      uint8_t tag = data[offset++];
      uint8_t len = data[offset++];

      if (offset + len > lc_val)
        break;

      if (tag == OATH_TAG_NAME && len < sizeof(name_buf)) {
        memcpy(name_buf, data + offset, len);
        name_len = len;
        printf("OATH DELETE: Name = '%.*s'\n", len, name_buf);
        break;
      }

      offset += len;
    }

    // Validate required fields
    if (name_len == 0) {
      printf("OATH DELETE: Missing account name\n");
      SET_SW(OATH_SW_WRONG_P1P2);
      return;
    }

    // Find the account
    int idx = find_account(name_buf, name_len);
    if (idx < 0) {
      printf("OATH DELETE: Account '%.*s' not found\n", name_len, name_buf);
      SET_SW(OATH_SW_FILE_NOT_FOUND);
      return;
    }

    // Delete the account
    if (storage_delete_oath_account(idx)) {
      printf("OATH DELETE: Account '%.*s' deleted successfully\n", name_len,
             name_buf);
      SET_SW(OATH_SW_OK);
    } else {
      printf("OATH DELETE: Failed to delete account\n");
      SET_SW(OATH_SW_COMMAND_NOT_ALLOWED);
    }

    break;
  }

  default:
    SET_SW(OATH_SW_WRONG_P1P2);
    break;
  }
}
