/*
 * OpenToken NATIVO - Official Firmware
 * Copyright (c) 2025 OpenToken Project
 * Licensed under the MIT License. See LICENSE file for details.
 */
#ifndef CCID_ENGINE_H
#define CCID_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

// ISO7816 APDU Structure Constants
#define APDU_MIN_LEN 4
#define APDU_MAX_LEN 261          // Max extended APDU length
#define APDU_RESPONSE_MAX_LEN 258 // Max response data + SW1 SW2

// ISO7816 Status Words
#define SW_SUCCESS 0x9000
#define SW_FILE_NOT_FOUND 0x6A82
#define SW_WRONG_P1P2 0x6A86
#define SW_CLASS_NOT_SUPPORTED 0x6E00
#define SW_INSTRUCTION_NOT_SUPPORTED 0x6D00
#define SW_WRONG_LENGTH 0x6700
#define SW_SECURITY_STATUS_NOT_SATISFIED 0x6982
#define SW_FUNCTION_NOT_SUPPORTED 0x6A81
#define SW_CONDITIONS_NOT_SATISFIED 0x6985

// APDU Command Structure
typedef struct {
  uint8_t cla;         // Class byte
  uint8_t ins;         // Instruction byte
  uint8_t p1;          // Parameter 1
  uint8_t p2;          // Parameter 2
  uint8_t lc;          // Length of command data
  const uint8_t *data; // Command data
  uint8_t le;          // Expected length of response data
  bool has_lc;         // Whether Lc field is present
  bool has_le;         // Whether Le field is present
} apdu_command_t;

// APDU Response Structure
typedef struct {
  uint8_t *data;     // Response data
  uint16_t data_len; // Length of response data
  uint16_t sw;       // Status word (SW1 SW2)
} apdu_response_t;

// Applet Selection State
typedef enum { APPLET_NONE = 0, APPLET_OATH, APPLET_OPENPGP } ccid_applet_t;

// CCID Engine Functions
void ccid_engine_init(void);
void opentoken_process_ccid_apdu(uint8_t const *buffer, uint16_t len,
                                 uint8_t *out_buffer, uint16_t *out_len);

// APDU Processing Functions
bool ccid_parse_apdu(const uint8_t *buffer, uint16_t len, apdu_command_t *cmd);
void ccid_format_response(const apdu_response_t *response, uint8_t *out_buffer,
                          uint16_t *out_len);
void ccid_send_status_word(uint16_t sw, uint8_t *out_buffer, uint16_t *out_len);

// Applet Management Functions
ccid_applet_t ccid_get_selected_applet(void);
void ccid_reset_applet_selection(void);
bool ccid_select_applet_by_aid(const uint8_t *aid, uint8_t aid_len);

#endif // CCID_ENGINE_H
