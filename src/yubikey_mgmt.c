#include "yubikey_mgmt.h"
#include "ccid_engine.h"
#include <stdio.h>
#include <string.h>

// YubiKey Manager AID (A0 00 00 05 27 47 11 17)
const uint8_t YUBIKEY_MGMT_AID[YUBIKEY_MGMT_AID_LEN] = {0xA0, 0x00, 0x00, 0x05, 0x27, 0x47, 0x11, 0x17};

// YubiKey OTP AID (A0 00 00 05 27 20 01 01) - for explicit rejection
const uint8_t YUBIKEY_OTP_AID[YUBIKEY_OTP_AID_LEN] = {0xA0, 0x00, 0x00, 0x05, 0x27, 0x20, 0x01, 0x01};

//--------------------------------------------------------------------+
// FORWARD DECLARATIONS
//--------------------------------------------------------------------+
static bool is_yubikey_proprietary_command(uint8_t ins);
static void yubikey_mgmt_log_alternative_for_command(uint8_t ins);
static bool yubikey_mgmt_validate_apdu_structure(const uint8_t *apdu, uint16_t len);
static void yubikey_mgmt_report_compatibility_status(void);

//--------------------------------------------------------------------+
// YUBIKEY MANAGER COMPATIBILITY LAYER
//--------------------------------------------------------------------+

bool yubikey_mgmt_select(const uint8_t *aid, uint8_t aid_len) {
    // Check for YubiKey Management AID
    if (aid_len == YUBIKEY_MGMT_AID_LEN && 
        memcmp(aid, YUBIKEY_MGMT_AID, YUBIKEY_MGMT_AID_LEN) == 0) {
        printf("YubiKey Mgmt: Management interface selected (limited compatibility)\n");
        return true;
    }
    
    // Check for YubiKey OTP AID - explicitly reject
    if (aid_len == YUBIKEY_OTP_AID_LEN && 
        memcmp(aid, YUBIKEY_OTP_AID, YUBIKEY_OTP_AID_LEN) == 0) {
        printf("YubiKey Mgmt: OTP interface requested - not supported\n");
        return false;
    }
    
    return false;
}

void yubikey_mgmt_process_apdu(const uint8_t *apdu, uint16_t len,
                               uint8_t *response, uint16_t *response_len) {
    if (!apdu || !response || !response_len) {
        if (response_len) *response_len = 0;
        return;
    }
    
    if (len < 4) {
        printf("YubiKey Mgmt: Invalid APDU length (%d bytes, minimum 4 required)\n", len);
        ccid_send_status_word(SW_WRONG_LENGTH, response, response_len);
        return;
    }
    
    uint8_t cla = apdu[0];
    uint8_t ins = apdu[1];
    uint8_t p1 = apdu[2];
    uint8_t p2 = apdu[3];
    
    *response_len = 0;
    
    printf("YubiKey Mgmt: Processing command CLA=0x%02X INS=0x%02X P1=0x%02X P2=0x%02X (len=%d)\n",
           cla, ins, p1, p2, len);
    
    // Validate CLA byte - YubiKey Manager typically uses 0x00
    if (cla != 0x00) {
        printf("YubiKey Mgmt: Unsupported CLA=0x%02X (expected 0x00)\n", cla);
        ccid_send_status_word(SW_CLASS_NOT_SUPPORTED, response, response_len);
        return;
    }
    
    // Handle standard commands that we support
    switch (ins) {
        case YUBIKEY_INS_GET_VERSION: {
            // Validate parameters for GET_VERSION
            if (p1 != 0x00 || p2 != 0x00) {
                printf("YubiKey Mgmt: GET_VERSION - invalid parameters P1=0x%02X P2=0x%02X\n", p1, p2);
                ccid_send_status_word(SW_WRONG_P1P2, response, response_len);
                return;
            }
            
            // Return version and capability information for YubiKey Manager compatibility
            // Format: Major.Minor.Patch + Capabilities
            printf("YubiKey Mgmt: GET_VERSION - returning version and capabilities\n");
            
            // Version information (compatible with YubiKey 5 series)
            response[0] = 5;  // Major version (YubiKey 5 series compatible)
            response[1] = 4;  // Minor version
            response[2] = 3;  // Patch version
            
            // Capability information - only report standard protocol support
            response[3] = OPENTOKEN_SUPPORTED_CAPS;  // Supported capabilities
            response[4] = OPENTOKEN_SUPPORTED_CAPS;  // Enabled capabilities (same as supported)
            
            // Form factor (0x04 = USB-A key, generic)
            response[5] = 0x04;
            
            uint16_t data_len = 6;
            *response_len = data_len;
            ccid_send_status_word(SW_SUCCESS, response + data_len, response_len);
            *response_len = data_len + 2; // Data + status word
            break;
        }
        
        case YUBIKEY_INS_GET_SERIAL: {
            // Validate parameters for GET_SERIAL
            if (p1 != 0x00 || p2 != 0x00) {
                printf("YubiKey Mgmt: GET_SERIAL - invalid parameters P1=0x%02X P2=0x%02X\n", p1, p2);
                ccid_send_status_word(SW_WRONG_P1P2, response, response_len);
                return;
            }
            
            // Return a generic serial number
            // YubiKey Manager uses this for device identification
            printf("YubiKey Mgmt: GET_SERIAL - returning generic serial\n");
            uint32_t serial = 0x00000001; // Generic serial number
            response[0] = (serial >> 24) & 0xFF;
            response[1] = (serial >> 16) & 0xFF;
            response[2] = (serial >> 8) & 0xFF;
            response[3] = serial & 0xFF;
            uint16_t data_len = 4;
            *response_len = data_len;
            ccid_send_status_word(SW_SUCCESS, response + data_len, response_len);
            *response_len = data_len + 2; // Data + status word
            break;
        }
        
        // Proprietary commands that we explicitly reject with appropriate errors
        case YUBIKEY_INS_SET_MODE:
            printf("YubiKey Mgmt: SET_MODE - proprietary mode switching not supported\n");
            printf("YubiKey Mgmt: Recommendation: Use standard CCID/HID interfaces instead\n");
            yubikey_mgmt_log_alternative_for_command(ins);
            ccid_send_status_word(SW_FUNCTION_NOT_SUPPORTED, response, response_len);
            break;
            
        case YUBIKEY_INS_RESET:
            printf("YubiKey Mgmt: RESET - proprietary device reset not supported\n");
            printf("YubiKey Mgmt: Recommendation: Use OATH RESET command for clearing OATH accounts\n");
            yubikey_mgmt_log_alternative_for_command(ins);
            ccid_send_status_word(SW_FUNCTION_NOT_SUPPORTED, response, response_len);
            break;
            
        case YUBIKEY_INS_SET_DEVICE_INFO:
            printf("YubiKey Mgmt: SET_DEVICE_INFO - proprietary device configuration not supported\n");
            printf("YubiKey Mgmt: Recommendation: Device operates with fixed, standard configuration\n");
            yubikey_mgmt_log_alternative_for_command(ins);
            ccid_send_status_word(SW_FUNCTION_NOT_SUPPORTED, response, response_len);
            break;
            
        case YUBIKEY_INS_OTP_NDEF:
            printf("YubiKey Mgmt: OTP_NDEF - YubiOTP functionality not supported\n");
            printf("YubiKey Mgmt: Recommendation: Use OATH TOTP/HOTP for time-based codes\n");
            yubikey_mgmt_log_alternative_for_command(ins);
            ccid_send_status_word(SW_FUNCTION_NOT_SUPPORTED, response, response_len);
            break;
            
        case YUBIKEY_INS_API_REQUEST:
            printf("YubiKey Mgmt: API_REQUEST - proprietary API not supported\n");
            printf("YubiKey Mgmt: Recommendation: Use standard FIDO2, OATH, or OpenPGP protocols\n");
            yubikey_mgmt_log_alternative_for_command(ins);
            ccid_send_status_word(SW_FUNCTION_NOT_SUPPORTED, response, response_len);
            break;
        
        default:
            // Unknown command - provide helpful error message
            printf("YubiKey Mgmt: Unknown command INS=0x%02X - not supported\n", ins);
            if (is_yubikey_proprietary_command(ins)) {
                printf("YubiKey Mgmt: This appears to be a proprietary YubiKey command\n");
                printf("YubiKey Mgmt: OpenToken supports only standard protocols\n");
            }
            ccid_send_status_word(SW_INSTRUCTION_NOT_SUPPORTED, response, response_len);
            break;
    }
}

//--------------------------------------------------------------------+
// YUBIKEY MANAGER DETECTION AND COMPATIBILITY HELPERS
//--------------------------------------------------------------------+

// Helper function to check if a command is a known YubiKey proprietary command
static bool is_yubikey_proprietary_command(uint8_t ins) {
    switch (ins) {
        case YUBIKEY_INS_SET_MODE:
        case YUBIKEY_INS_RESET:
        case YUBIKEY_INS_SET_DEVICE_INFO:
        case YUBIKEY_INS_OTP_NDEF:
        case YUBIKEY_INS_API_REQUEST:
            return true;
        default:
            return false;
    }
}

// Helper function to log alternative approaches for unsupported commands
static void yubikey_mgmt_log_alternative_for_command(uint8_t ins) {
    switch (ins) {
        case YUBIKEY_INS_SET_MODE:
            printf("YubiKey Mgmt: Alternative: Device automatically supports HID (FIDO2) and CCID (OATH/OpenPGP)\n");
            break;
        case YUBIKEY_INS_RESET:
            printf("YubiKey Mgmt: Alternative: Select OATH applet and use OATH RESET command\n");
            break;
        case YUBIKEY_INS_SET_DEVICE_INFO:
            printf("YubiKey Mgmt: Alternative: Device configuration is fixed for maximum compatibility\n");
            break;
        case YUBIKEY_INS_OTP_NDEF:
            printf("YubiKey Mgmt: Alternative: Use OATH applet for TOTP/HOTP generation\n");
            break;
        case YUBIKEY_INS_API_REQUEST:
            printf("YubiKey Mgmt: Alternative: Use standard CCID APDUs or HID FIDO2 commands\n");
            break;
        default:
            printf("YubiKey Mgmt: Alternative: Check OpenToken documentation for standard protocol usage\n");
            break;
    }
}

// Enhanced error reporting for YubiKey Manager compatibility
static void yubikey_mgmt_report_compatibility_status(void) {
    printf("=== YubiKey Manager Compatibility Status ===\n");
    printf("✓ Device Detection: Supported via standard CCID interface\n");
    printf("✓ OATH Operations: Fully supported (PUT, LIST, CALCULATE, DELETE)\n");
    printf("✓ FIDO2/WebAuthn: Supported via HID interface\n");
    printf("✓ OpenPGP: Supported via CCID interface\n");
    printf("✗ YubiOTP: Not supported (proprietary protocol)\n");
    printf("✗ Mode Switching: Not supported (use standard interfaces)\n");
    printf("✗ Firmware Updates: Not supported (proprietary protocol)\n");
    printf("✗ Device Configuration: Not supported (proprietary protocol)\n");
    printf("============================================\n");
}

// Function to handle USB reconnection stability
void yubikey_mgmt_handle_reconnection(void) {
    printf("YubiKey Mgmt: Handling USB reconnection - resetting state\n");
    
    // Reset any internal state if needed
    // For now, we don't maintain persistent state in the compatibility layer
    // but this function provides a hook for future enhancements
    
    printf("YubiKey Mgmt: Reconnection handling complete\n");
}

// Function to validate APDU structure for YubiKey Manager commands
static bool yubikey_mgmt_validate_apdu_structure(const uint8_t *apdu, uint16_t len) {
    if (!apdu || len < 4) {
        return false;
    }
    
    // Basic APDU structure validation
    uint8_t cla = apdu[0];
    uint8_t ins = apdu[1];
    
    // YubiKey Manager typically uses CLA 0x00
    if (cla != 0x00) {
        return false;
    }
    
    // Check if this is a known YubiKey command
    switch (ins) {
        case YUBIKEY_INS_GET_VERSION:
        case YUBIKEY_INS_GET_SERIAL:
        case YUBIKEY_INS_SET_MODE:
        case YUBIKEY_INS_RESET:
        case YUBIKEY_INS_SET_DEVICE_INFO:
        case YUBIKEY_INS_OTP_NDEF:
        case YUBIKEY_INS_API_REQUEST:
            return true;
        default:
            return false;
    }
}

// Function to be called during device initialization for logging
void yubikey_mgmt_init_compatibility_layer(void) {
    printf("YubiKey Manager Compatibility Layer: Initialized\n");
    printf("Supporting standard OATH operations through CCID interface\n");
    printf("Rejecting proprietary YubiKey commands with appropriate errors\n");
    
    // Report compatibility status for debugging
    yubikey_mgmt_report_compatibility_status();
}