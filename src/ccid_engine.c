#include "ccid_engine.h"
#include "oath_applet.h"
#include "openpgp_applet.h"
#include "yubikey_mgmt.h"
#include "error_handling.h"
#include <stdio.h>
#include <string.h>

// Global state for selected applet
static ccid_applet_t current_applet = APPLET_NONE;



//--------------------------------------------------------------------+
// CCID ENGINE INITIALIZATION
//--------------------------------------------------------------------+
void ccid_engine_init(void) {
    current_applet = APPLET_NONE;
    printf("CCID Engine: Initialized - Protocol-independent APDU routing\n");
}

//--------------------------------------------------------------------+
// ISO7816 APDU PARSING
//--------------------------------------------------------------------+
bool ccid_parse_apdu(const uint8_t *buffer, uint16_t len, apdu_command_t *cmd) {
    if (!buffer || !cmd || len < APDU_MIN_LEN) {
        return false;
    }

    // Clear command structure
    memset(cmd, 0, sizeof(apdu_command_t));

    // Parse mandatory header (CLA INS P1 P2)
    cmd->cla = buffer[0];
    cmd->ins = buffer[1];
    cmd->p1 = buffer[2];
    cmd->p2 = buffer[3];

    // Handle different APDU cases based on length
    if (len == 4) {
        // Case 1: No data, no expected response (CLA INS P1 P2)
        cmd->has_lc = false;
        cmd->has_le = false;
    } else if (len == 5) {
        // Case 2S: No data, expected response length (CLA INS P1 P2 Le)
        cmd->has_lc = false;
        cmd->has_le = true;
        cmd->le = buffer[4];
    } else if (len > 5) {
        // Cases 3 and 4: Command data present
        cmd->lc = buffer[4];
        cmd->has_lc = true;
        
        if (cmd->lc == 0) {
            // Extended length APDU (not commonly used in smartcards)
            return false; // Simplified implementation doesn't support extended APDUs
        }
        
        if (len < 5 + cmd->lc) {
            return false; // Invalid length
        }
        
        cmd->data = buffer + 5;
        
        if (len == 5 + cmd->lc) {
            // Case 3: Command data, no expected response (CLA INS P1 P2 Lc Data)
            cmd->has_le = false;
        } else if (len == 5 + cmd->lc + 1) {
            // Case 4: Command data and expected response (CLA INS P1 P2 Lc Data Le)
            cmd->has_le = true;
            cmd->le = buffer[5 + cmd->lc];
        } else {
            return false; // Invalid APDU structure
        }
    }

    return true;
}

//--------------------------------------------------------------------+
// APDU RESPONSE FORMATTING
//--------------------------------------------------------------------+
void ccid_format_response(const apdu_response_t *response, uint8_t *out_buffer, uint16_t *out_len) {
    if (!response || !out_buffer || !out_len) {
        return;
    }

    *out_len = 0;

    // Copy response data if present
    if (response->data && response->data_len > 0) {
        if (response->data_len > APDU_RESPONSE_MAX_LEN - 2) {
            // Response too long, truncate and set error status
            ccid_send_status_word(SW_WRONG_LENGTH, out_buffer, out_len);
            return;
        }
        memcpy(out_buffer, response->data, response->data_len);
        *out_len = response->data_len;
    }

    // Append status word (SW1 SW2)
    out_buffer[*out_len] = (uint8_t)(response->sw >> 8);
    out_buffer[*out_len + 1] = (uint8_t)(response->sw & 0xFF);
    *out_len += 2;
}

void ccid_send_status_word(uint16_t sw, uint8_t *out_buffer, uint16_t *out_len) {
    if (!out_buffer || !out_len) {
        return;
    }

    out_buffer[0] = (uint8_t)(sw >> 8);
    out_buffer[1] = (uint8_t)(sw & 0xFF);
    *out_len = 2;
}

//--------------------------------------------------------------------+
// APPLET MANAGEMENT
//--------------------------------------------------------------------+
ccid_applet_t ccid_get_selected_applet(void) {
    return current_applet;
}

void ccid_reset_applet_selection(void) {
    current_applet = APPLET_NONE;
}

bool ccid_select_applet_by_aid(const uint8_t *aid, uint8_t aid_len) {
    if (!aid || aid_len == 0) {
        return false;
    }

    // Try YubiKey Manager compatibility layer first
    if (yubikey_mgmt_select(aid, aid_len)) {
        current_applet = APPLET_YUBIKEY_MGMT;
        printf("CCID Engine: YubiKey Manager compatibility layer selected\n");
        return true;
    }

    // Try OATH applet selection
    if (oath_applet_select(aid, aid_len)) {
        current_applet = APPLET_OATH;
        printf("CCID Engine: OATH applet selected\n");
        return true;
    }

    // Try OpenPGP applet selection
    if (openpgp_applet_select(aid, aid_len)) {
        current_applet = APPLET_OPENPGP;
        printf("CCID Engine: OpenPGP applet selected\n");
        return true;
    }

    // No applet matched the AID
    current_applet = APPLET_NONE;
    return false;
}

//--------------------------------------------------------------------+
// MAIN APDU PROCESSING FUNCTION
//--------------------------------------------------------------------+
void opentoken_process_ccid_apdu(uint8_t const *buffer, uint16_t len,
                                 uint8_t *out_buffer, uint16_t *out_len) {
    apdu_command_t cmd;
    apdu_response_t response;

    // Input validation with comprehensive error handling
    if (!buffer || !out_buffer || !out_len) {
        ERROR_REPORT_ERROR(ERROR_PROTOCOL_INVALID_COMMAND, 
                          "Null parameters in CCID APDU processing");
        if (out_buffer && out_len) {
            protocol_send_error_response_ccid(out_buffer, out_len, SW_CONDITIONS_NOT_SATISFIED);
        }
        return;
    }
    
    if (len > APDU_MAX_LEN) {
        ERROR_REPORT_ERROR(ERROR_PROTOCOL_BUFFER_OVERFLOW, 
                          "APDU too large: %d bytes", len);
        protocol_send_error_response_ccid(out_buffer, out_len, SW_WRONG_LENGTH);
        return;
    }

    printf("CCID Engine: Processing APDU (%d bytes)\n", len);

    // Initialize response
    response.data = NULL;
    response.data_len = 0;
    response.sw = SW_SUCCESS;

    // Parse APDU command with timeout protection
    if (!timeout_start(DEFAULT_TIMEOUTS.protocol_response_timeout_ms)) {
        ERROR_REPORT_ERROR(ERROR_TIMEOUT_PROTOCOL_RESPONSE, 
                          "Failed to start APDU processing timeout");
        protocol_send_error_response_ccid(out_buffer, out_len, SW_CONDITIONS_NOT_SATISFIED);
        return;
    }
    
    if (!ccid_parse_apdu(buffer, len, &cmd)) {
        timeout_reset();
        ERROR_REPORT_ERROR(ERROR_PROTOCOL_MALFORMED_PACKET, 
                          "Invalid APDU format, length: %d", len);
        protocol_send_error_response_ccid(out_buffer, out_len, SW_WRONG_LENGTH);
        return;
    }
    
    timeout_reset();

    printf("CCID Engine: CLA=0x%02X INS=0x%02X P1=0x%02X P2=0x%02X\n", 
           cmd.cla, cmd.ins, cmd.p1, cmd.p2);

    // Handle SELECT command (ISO7816 standard)
    if (cmd.cla == 0x00 && cmd.ins == 0xA4 && cmd.p1 == 0x04 && cmd.p2 == 0x00) {
        // SELECT by AID command
        printf("CCID Engine: SELECT AID command\n");
        
        if (!cmd.has_lc || !cmd.data) {
            ccid_send_status_word(SW_WRONG_LENGTH, out_buffer, out_len);
            return;
        }

        if (ccid_select_applet_by_aid(cmd.data, cmd.lc)) {
            ccid_send_status_word(SW_SUCCESS, out_buffer, out_len);
        } else {
            ccid_send_status_word(SW_FILE_NOT_FOUND, out_buffer, out_len);
        }
        return;
    }

    // Route to selected applet with error handling and timeout protection
    if (!timeout_start(DEFAULT_TIMEOUTS.protocol_response_timeout_ms)) {
        ERROR_REPORT_ERROR(ERROR_TIMEOUT_PROTOCOL_RESPONSE, 
                          "Failed to start applet processing timeout");
        protocol_send_error_response_ccid(out_buffer, out_len, SW_CONDITIONS_NOT_SATISFIED);
        return;
    }
    
    switch (current_applet) {
        case APPLET_YUBIKEY_MGMT:
            printf("CCID Engine: Routing to YubiKey Manager compatibility layer\n");
            if (!retry_operation_with_context(
                    (bool (*)(void*))yubikey_mgmt_process_apdu_wrapper, 
                    &(struct {const uint8_t* buf; uint16_t len; uint8_t* out; uint16_t* out_len;}){buffer, len, out_buffer, out_len},
                    &RETRY_CONFIG_PROTOCOL)) {
                ERROR_REPORT_ERROR(ERROR_PROTOCOL_SEQUENCE_ERROR, 
                                  "YubiKey Manager APDU processing failed");
                protocol_send_error_response_ccid(out_buffer, out_len, SW_CONDITIONS_NOT_SATISFIED);
            }
            break;

        case APPLET_OATH:
            printf("CCID Engine: Routing to OATH applet\n");
            if (!retry_operation_with_context(
                    (bool (*)(void*))oath_applet_process_apdu_wrapper,
                    &(struct {const uint8_t* buf; uint16_t len; uint8_t* out; uint16_t* out_len;}){buffer, len, out_buffer, out_len},
                    &RETRY_CONFIG_PROTOCOL)) {
                ERROR_REPORT_ERROR(ERROR_PROTOCOL_SEQUENCE_ERROR, 
                                  "OATH applet APDU processing failed");
                protocol_send_error_response_ccid(out_buffer, out_len, SW_CONDITIONS_NOT_SATISFIED);
            }
            break;

        case APPLET_OPENPGP:
            printf("CCID Engine: Routing to OpenPGP applet\n");
            if (!retry_operation_with_context(
                    (bool (*)(void*))openpgp_applet_process_apdu_wrapper,
                    &(struct {const uint8_t* buf; uint16_t len; uint8_t* out; uint16_t* out_len;}){buffer, len, out_buffer, out_len},
                    &RETRY_CONFIG_PROTOCOL)) {
                ERROR_REPORT_ERROR(ERROR_PROTOCOL_SEQUENCE_ERROR, 
                                  "OpenPGP applet APDU processing failed");
                protocol_send_error_response_ccid(out_buffer, out_len, SW_CONDITIONS_NOT_SATISFIED);
            }
            break;

        case APPLET_NONE:
        default:
            ERROR_REPORT_WARNING(ERROR_PROTOCOL_INVALID_COMMAND, 
                                "No applet selected for APDU processing");
            protocol_send_error_response_ccid(out_buffer, out_len, SW_CLASS_NOT_SUPPORTED);
            break;
    }
    
    timeout_reset();
    printf("CCID Engine: Response generated (%d bytes)\n", *out_len);
}

// Wrapper functions for retry mechanism compatibility
static bool yubikey_mgmt_process_apdu_wrapper(void* context) {
    struct {const uint8_t* buf; uint16_t len; uint8_t* out; uint16_t* out_len;} *ctx = context;
    yubikey_mgmt_process_apdu(ctx->buf, ctx->len, ctx->out, ctx->out_len);
    return true; // YubiKey manager always succeeds or handles errors internally
}

static bool oath_applet_process_apdu_wrapper(void* context) {
    struct {const uint8_t* buf; uint16_t len; uint8_t* out; uint16_t* out_len;} *ctx = context;
    oath_applet_process_apdu(ctx->buf, ctx->len, ctx->out, ctx->out_len);
    return true; // OATH applet always succeeds or handles errors internally
}

static bool openpgp_applet_process_apdu_wrapper(void* context) {
    struct {const uint8_t* buf; uint16_t len; uint8_t* out; uint16_t* out_len;} *ctx = context;
    openpgp_applet_process_apdu(ctx->buf, ctx->len, ctx->out, ctx->out_len);
    return true; // OpenPGP applet always succeeds or handles errors internally
}

