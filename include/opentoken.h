/*
 * OpenToken NATIVO - Official Firmware
 * Copyright (c) 2025 OpenToken Project
 * Licensed under the MIT License. See LICENSE file for details.
 */
#ifndef OPENTOKEN_H
#define OPENTOKEN_H

#include "tusb_config.h" // Inclui as definições de configuração do TinyUSB
#include <stdint.h>

// USB Vendor/Product IDs - Using generic/test IDs for interoperability
// These should be standard IDs that don't claim to be specific brands
#define OPENTOKEN_VID 0x1209 // pid.codes - Open Source Hardware VID
#define OPENTOKEN_PID 0x0001 // Generic security token PID

// Funções de processamento de comandos (declaradas no main.c ou em outros
// arquivos)
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len);
void opentoken_process_ccid_apdu(uint8_t const *buffer, uint16_t len,
                                 uint8_t *out_buffer, uint16_t *out_len);

// WebUSB management interface callback
#include "tusb.h"
bool opentoken_webusb_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                      tusb_control_request_t const *request);

#endif // OPENTOKEN_H
