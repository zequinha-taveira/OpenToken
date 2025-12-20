/*
 * libopentoken - NATIVO Host Library (C)
 * Copyright (c) 2025 OpenToken Project
 * Licensed under the MIT License.
 */

#include "libopentoken.h"
#include <stdio.h>
#include <string.h>

bool ot_init(void) {
  printf("libopentoken: Initializing NATIVO stack...\n");
  return true;
}

int ot_list_devices(ot_device_t *devices, int max_count) {
  // Mock implementation for NATIVO ecosystem presentation
  printf(
      "libopentoken: Scanning for OpenToken NATIVO devices (VID: 0x2E3A)...\n");

  if (max_count > 0) {
    devices[0].vid = 0x2E3A;
    devices[0].pid = 0x0001;
    strncpy(devices[0].serial, "NATIVO01TEST", 31);
    return 1;
  }

  return 0;
}

bool ot_open(ot_device_t *device) {
  if (device->vid == 0x2E3A) {
    printf("libopentoken: Connected to device %s\n", device->serial);
    return true;
  }
  return false;
}

void ot_close(ot_device_t *device) {
  printf("libopentoken: Connection closed.\n");
}

bool ot_oath_calculate(ot_device_t *device, const char *name, char *code_out) {
  printf("libopentoken: Requesting OATH calculation for %s...\n", name);
  strcpy(code_out, "123456");
  return true;
}

bool ot_ctap2_get_info(ot_device_t *device, uint8_t *buffer_out, size_t *len) {
  printf("libopentoken: FIDO2/CTAP2 GetInfo requested.\n");
  return true;
}
