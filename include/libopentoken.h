/*
 * libopentoken - NATIVO Host Library (C)
 * Copyright (c) 2025 OpenToken Project
 * Licensed under the MIT License.
 */

#ifndef LIBOPENTOKEN_H
#define LIBOPENTOKEN_H

#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

// OpenToken NATIVO Device Context
typedef struct {
  uint16_t vid;
  uint16_t pid;
  char serial[32];
  void *transport_handle;
} ot_device_t;

// Library Initialization
bool ot_init(void);

// Device Discovery
int ot_list_devices(ot_device_t *devices, int max_count);

// Core Operations
bool ot_open(ot_device_t *device);
void ot_close(ot_device_t *device);

// OATH Operations
bool ot_oath_calculate(ot_device_t *device, const char *name, char *code_out);

// CTAP2 Operations
bool ot_ctap2_get_info(ot_device_t *device, uint8_t *buffer_out, size_t *len);

#ifdef __cplusplus
}
#endif

#endif // LIBOPENTOKEN_H
