/**
 * @file webusb_handler.c
 * @brief WebUSB Management Interface for OpenToken
 *
 * This module implements the WebUSB-based management interface that allows
 * browsers to communicate with the OpenToken device for credential management.
 */

#include "pico/bootrom.h"
#include "storage.h"
#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


// WebUSB Management Commands
#define WEBUSB_CMD_GET_VERSION 0x01
#define WEBUSB_CMD_LIST_CREDS 0x02
#define WEBUSB_CMD_DELETE_CRED 0x03
#define WEBUSB_CMD_GET_STATUS 0x04
#define WEBUSB_CMD_RESET_DEVICE 0x05
#define WEBUSB_CMD_REBOOT_BOOTLOADER 0x06
#define WEBUSB_CMD_LIST_OATH 0x10
#define WEBUSB_CMD_DELETE_OATH 0x11

// Response status codes
#define WEBUSB_STATUS_OK 0x00
#define WEBUSB_STATUS_ERROR 0x01
#define WEBUSB_STATUS_NOT_FOUND 0x02
#define WEBUSB_STATUS_UNAUTHORIZED 0x03

// OpenToken version
#define OPENTOKEN_VERSION_MAJOR 1
#define OPENTOKEN_VERSION_MINOR 0
#define OPENTOKEN_VERSION_PATCH 0

// Buffer for WebUSB responses
static uint8_t webusb_response[256];
static uint16_t webusb_response_len;

/**
 * @brief Handle GET_VERSION command
 */
static void handle_get_version(void) {
  webusb_response[0] = WEBUSB_STATUS_OK;
  webusb_response[1] = OPENTOKEN_VERSION_MAJOR;
  webusb_response[2] = OPENTOKEN_VERSION_MINOR;
  webusb_response[3] = OPENTOKEN_VERSION_PATCH;
  webusb_response_len = 4;
}

/**
 * @brief Handle LIST_CREDS command - List FIDO2 Resident Keys
 */
static void handle_list_creds(void) {
  webusb_response[0] = WEBUSB_STATUS_OK;
  uint8_t offset = 1;
  uint8_t count = 0;

  for (uint8_t i = 0; i < STORAGE_FIDO2_MAX_CREDS && offset < 240; i++) {
    storage_fido2_entry_t cred;
    if (storage_load_fido2_cred(i, &cred)) {
      // Format: slot_id (1) + rp_id_hash (8 bytes only, truncated)
      webusb_response[offset++] = i;
      memcpy(&webusb_response[offset], cred.rp_id_hash, 8);
      offset += 8;
      count++;
    }
  }

  // Insert count after status byte
  memmove(&webusb_response[2], &webusb_response[1], offset - 1);
  webusb_response[1] = count;
  webusb_response_len = offset + 1;
}

/**
 * @brief Handle DELETE_CRED command
 * @param slot_id Credential slot to delete
 */
static void handle_delete_cred(uint8_t slot_id) {
  if (slot_id >= STORAGE_FIDO2_MAX_CREDS) {
    webusb_response[0] = WEBUSB_STATUS_NOT_FOUND;
    webusb_response_len = 1;
    return;
  }

  if (storage_delete_fido2_cred(slot_id)) {
    webusb_response[0] = WEBUSB_STATUS_OK;
  } else {
    webusb_response[0] = WEBUSB_STATUS_ERROR;
  }
  webusb_response_len = 1;
}

/**
 * @brief Handle GET_STATUS command
 */
static void handle_get_status(void) {
  webusb_response[0] = WEBUSB_STATUS_OK;

  // Count active FIDO2 credentials
  uint8_t fido2_count = 0;
  for (uint8_t i = 0; i < STORAGE_FIDO2_MAX_CREDS; i++) {
    storage_fido2_entry_t cred;
    if (storage_load_fido2_cred(i, &cred)) {
      fido2_count++;
    }
  }

  // Count active OATH accounts
  uint8_t oath_count = 0;
  for (uint8_t i = 0; i < STORAGE_OATH_MAX_ACCOUNTS; i++) {
    storage_oath_entry_t entry;
    if (storage_load_oath_account(i, &entry)) {
      oath_count++;
    }
  }

  webusb_response[1] = fido2_count;
  webusb_response[2] = STORAGE_FIDO2_MAX_CREDS;
  webusb_response[3] = oath_count;
  webusb_response[4] = STORAGE_OATH_MAX_ACCOUNTS;
  webusb_response_len = 5;
}

/**
 * @brief Handle LIST_OATH command - List OATH accounts
 */
static void handle_list_oath(void) {
  webusb_response[0] = WEBUSB_STATUS_OK;
  uint8_t offset = 2;
  uint8_t count = 0;

  for (uint8_t i = 0; i < STORAGE_OATH_MAX_ACCOUNTS && offset < 200; i++) {
    storage_oath_entry_t entry;
    if (storage_load_oath_account(i, &entry)) {
      // Format: slot_id (1) + name_len (1) + name (up to 32 bytes)
      webusb_response[offset++] = i;
      uint8_t name_len = (entry.name_len > 32) ? 32 : entry.name_len;
      webusb_response[offset++] = name_len;
      memcpy(&webusb_response[offset], entry.name, name_len);
      offset += name_len;
      count++;
    }
  }

  webusb_response[1] = count;
  webusb_response_len = offset;
}

/**
 * @brief Handle DELETE_OATH command
 * @param slot_id OATH account slot to delete
 */
static void handle_delete_oath(uint8_t slot_id) {
  if (slot_id >= STORAGE_OATH_MAX_ACCOUNTS) {
    webusb_response[0] = WEBUSB_STATUS_NOT_FOUND;
    webusb_response_len = 1;
    return;
  }

  if (storage_delete_oath_account(slot_id)) {
    webusb_response[0] = WEBUSB_STATUS_OK;
  } else {
    webusb_response[0] = WEBUSB_STATUS_ERROR;
  }
  webusb_response_len = 1;
}

/**
 * @brief Handle RESET_DEVICE command - Perform a full secure wipe
 */
static void handle_reset_device(void) {
  printf("WebUSB: Received global reset command\n");
  webusb_response_len = 1;
}

/**
 * @brief Handle REBOOT_BOOTLOADER command - Reboot to BOOTSEL mode
 */
static void handle_reboot_bootloader(void) {
  printf("WebUSB: Rebooting to BOOTSEL mode...\n");
  // Use status byte to acknowledge before rebooting
  webusb_response[0] = WEBUSB_STATUS_OK;
  webusb_response_len = 1;

  // Flush and delay slightly to ensure response is sent
  tud_vendor_write(webusb_response, webusb_response_len);
  tud_vendor_flush();

  // Reboot into BOOTSEL mode (Pico SDK)
  reset_usb_boot(0, 0);
}

/**
 * @brief Process incoming WebUSB command
 * @param buffer Input command buffer
 * @param bufsize Input buffer size
 */
void opentoken_webusb_rx_cb(uint8_t const *buffer, uint16_t bufsize) {
  if (bufsize < 1) {
    webusb_response[0] = WEBUSB_STATUS_ERROR;
    webusb_response_len = 1;
    tud_vendor_write(webusb_response, webusb_response_len);
    return;
  }

  uint8_t cmd = buffer[0];

  switch (cmd) {
  case WEBUSB_CMD_GET_VERSION:
    handle_get_version();
    break;

  case WEBUSB_CMD_LIST_CREDS:
    handle_list_creds();
    break;

  case WEBUSB_CMD_DELETE_CRED:
    if (bufsize >= 2) {
      handle_delete_cred(buffer[1]);
    } else {
      webusb_response[0] = WEBUSB_STATUS_ERROR;
      webusb_response_len = 1;
    }
    break;

  case WEBUSB_CMD_GET_STATUS:
    handle_get_status();
    break;

  case WEBUSB_CMD_LIST_OATH:
    handle_list_oath();
    break;

  case WEBUSB_CMD_DELETE_OATH:
    if (bufsize >= 2) {
      handle_delete_oath(buffer[1]);
    } else {
      webusb_response[0] = WEBUSB_STATUS_ERROR;
      webusb_response_len = 1;
    }
    break;

  case WEBUSB_CMD_RESET_DEVICE:
    handle_reset_device();
    break;

  case WEBUSB_CMD_REBOOT_BOOTLOADER:
    handle_reboot_bootloader();
    return; // Don't send response twice

  default:
    printf("WebUSB: Unknown command 0x%02X\n", cmd);
    webusb_response[0] = WEBUSB_STATUS_ERROR;
    webusb_response_len = 1;
    break;
  }

  // Send response back to host
  tud_vendor_write(webusb_response, webusb_response_len);
  tud_vendor_flush();
}

/**
 * @brief TinyUSB Vendor RX callback - called when data is received
 */
void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize) {
  (void)itf;

  if (bufsize > 0) {
    opentoken_webusb_rx_cb(buffer, bufsize);
  }
}

/**
 * @brief Handle vendor control transfer requests
 * @return true if request was handled
 */
bool opentoken_webusb_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                      tusb_control_request_t const *request) {
  (void)rhport;
  (void)stage;
  (void)request;

  // Additional vendor control requests can be handled here
  // For now, we return false to indicate the request was not handled
  return false;
}
