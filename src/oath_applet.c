#include "oath_applet.h"
#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// AID OATH (Exemplo: A0 00 00 05 27 21 01 01 - Yubico OATH)
const uint8_t OATH_AID[OATH_AID_LEN] = {0xA0, 0x00, 0x00, 0x05,
                                        0x27, 0x21, 0x01, 0x01};

static bool is_selected = false;

#include "storage.h"

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
    printf("OATH Applet: Selecionado.\n");
    // Ensure storage is init
    storage_init();
    return true;
  }
  is_selected = false;
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
  case OATH_INS_PUT: {
    printf("OATH Applet: PUT Command.\n");
    // Parse TLV: 71 Name, 73 Key, 75 Prop
    uint16_t offset = 0;
    uint8_t name_buf[64];
    uint8_t name_l = 0;
    uint8_t key_buf[64];
    uint8_t key_l = 0;
    uint8_t prop = 0;

    while (offset < lc_val) {
      uint8_t tag = data[offset++];
      uint8_t len = data[offset++];
      if (tag == OATH_TAG_NAME && len < 64) {
        memcpy(name_buf, data + offset, len);
        name_l = len;
      } else if (tag == OATH_TAG_KEY && len < 64) {
        memcpy(key_buf, data + offset, len);
        key_l = len;
      } else if (tag == OATH_TAG_PROPERTY && len == 1) {
        prop = data[offset];
      }
      offset += len;
    }

    if (name_l > 0) {
      // Check if exists
      int idx = find_account(name_buf, name_l);
      if (idx == -1) {
        idx = find_free_slot();
      }

      if (idx != -1) {
        storage_oath_entry_t new_entry;
        memset(&new_entry, 0, sizeof(new_entry));
        memcpy(new_entry.name, name_buf, name_l);
        new_entry.name_len = name_l;
        memcpy(new_entry.key, key_buf, key_l);
        new_entry.key_len = key_l;
        new_entry.prop = prop;
        new_entry.active = 1;

        storage_save_oath_account(idx, &new_entry);
        printf("OATH Applet: Account '%s' saved to Flash index %d.\n", name_buf,
               idx);
      } else {
        SET_SW(OATH_SW_COMMAND_NOT_ALLOWED); // Storage Full
        return;
      }
    }
    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_LIST: {
    printf("OATH Applet: LIST Command.\n");
    storage_oath_entry_t entry;

    // Response: Sequence of [72] [Len] [71] [NameLen] [Name] [75] [1] [Prop]
    for (int i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
      if (storage_load_oath_account(i, &entry)) {
        uint8_t *ptr = response + *response_len;
        uint8_t total_len = 2 + entry.name_len + 3; // 71 L Name + 75 1 Prop

        *ptr++ = OATH_TAG_NAME_LIST; // 72
        *ptr++ = total_len;          // L

        *ptr++ = OATH_TAG_NAME; // 71
        *ptr++ = entry.name_len;
        memcpy(ptr, entry.name, entry.name_len);
        ptr += entry.name_len;

        *ptr++ = OATH_TAG_PROPERTY; // 75
        *ptr++ = 1;
        *ptr++ = entry.prop;

        *response_len += (2 + total_len);
      }
    }
    SET_SW(OATH_SW_OK);
    break;
  }

  case OATH_INS_CALCULATE: {
    printf("OATH Applet: CALCULATE Command.\n");
    // Parse Name from data (Tag 71)
    uint16_t offset = 0;
    uint8_t name_buf[64];
    uint8_t name_l = 0;
    uint8_t challenge_buf[8];
    uint8_t chal_len = 0;

    while (offset < lc_val) {
      uint8_t tag = data[offset++];
      uint8_t len = data[offset++];
      if (tag == OATH_TAG_NAME && len < 64) {
        memcpy(name_buf, data + offset, len);
        name_l = len;
      } else if (tag == OATH_TAG_CHALLENGE && len <= 8) {
        memcpy(challenge_buf, data + offset, len);
        chal_len = len;
      }
      offset += len;
    }

    if (name_l > 0) {
      int idx = find_account(name_buf, name_l);
      if (idx >= 0) {
        // Found account.
        // Calculate based on Challenge + Secret
        // Retrieve secret
        storage_oath_entry_t entry;
        storage_load_oath_account(idx, &entry);

        // Mock Calculate (or real if we have HMAC)
        // We use the challenge and mix it to create a code
        uint32_t code = 0;
        if (chal_len > 0) {
          // Simple dynamic code logic: (Challenge[last] ^ Key[0]) * 123456
          code = (challenge_buf[chal_len - 1] ^ entry.key[0]) * 123456;
          code = code % 1000000;
        }

        uint8_t digits = 6;

        // 76 05 [Digits] [Code]
        response[0] = OATH_TAG_RESPONSE_VAL; // 76
        response[1] = 5;
        response[2] = digits;
        response[3] = (code >> 24) & 0xFF;
        response[4] = (code >> 16) & 0xFF;
        response[5] = (code >> 8) & 0xFF;
        response[6] = code & 0xFF;

        *response_len = 7;
        SET_SW(OATH_SW_OK);
      } else {
        SET_SW(OATH_SW_FILE_NOT_FOUND);
      }
    } else {
      SET_SW(OATH_SW_WRONG_P1P2);
    }
    break;
  }

  case OATH_INS_DELETE:
    // TODO: Remove account
    SET_SW(OATH_SW_OK);
    break;

  default:
    SET_SW(OATH_SW_WRONG_P1P2);
    break;
  }
}
