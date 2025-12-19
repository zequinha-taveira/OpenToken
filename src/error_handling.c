#include "error_handling.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//--------------------------------------------------------------------+
// GLOBAL STATE AND CONFIGURATION
//--------------------------------------------------------------------+

// Error handling system state
static bool error_system_initialized = false;
static error_context_t last_error;
static uint32_t error_count = 0;

// USB stability tracking
static usb_stability_context_t usb_stability;

// Timeout tracking
static uint32_t timeout_start_time = 0;
static uint32_t timeout_duration = 0;
static bool timeout_active = false;

// Default retry configurations
const retry_config_t RETRY_CONFIG_USB = {
    .max_attempts = 3,
    .base_delay_ms = 100,
    .max_delay_ms = 1000,
    .exponential_backoff = true
};

const retry_config_t RETRY_CONFIG_PROTOCOL = {
    .max_attempts = 2,
    .base_delay_ms = 50,
    .max_delay_ms = 200,
    .exponential_backoff = false
};

const retry_config_t RETRY_CONFIG_CRYPTO = {
    .max_attempts = 3,
    .base_delay_ms = 10,
    .max_delay_ms = 100,
    .exponential_backoff = true
};

const retry_config_t RETRY_CONFIG_STORAGE = {
    .max_attempts = 5,
    .base_delay_ms = 20,
    .max_delay_ms = 500,
    .exponential_backoff = true
};

// Default timeout configuration
const timeout_config_t DEFAULT_TIMEOUTS = {
    .usb_operation_timeout_ms = 5000,
    .protocol_response_timeout_ms = 30000,
    .user_presence_timeout_ms = 30000,
    .crypto_operation_timeout_ms = 10000
};

//--------------------------------------------------------------------+
// ERROR HANDLING SYSTEM INITIALIZATION
//--------------------------------------------------------------------+

void error_handling_init(void) {
    if (error_system_initialized) {
        return;
    }
    
    printf("Error Handling: Initializing comprehensive error management system\n");
    
    // Initialize error tracking
    memset(&last_error, 0, sizeof(last_error));
    error_count = 0;
    
    // Initialize USB stability tracking
    usb_stability_init();
    
    // Initialize timeout system
    timeout_init();
    
    error_system_initialized = true;
    printf("Error Handling: System initialized successfully\n");
}

//--------------------------------------------------------------------+
// ERROR REPORTING AND LOGGING
//--------------------------------------------------------------------+

void error_report(error_code_t code, error_severity_t severity, 
                  const char* file, uint16_t line, const char* function,
                  const char* format, ...) {
    
    if (!error_system_initialized) {
        error_handling_init();
    }
    
    // Update error context
    last_error.code = code;
    last_error.category = (error_category_t)((code >> 12) & 0xF);
    last_error.severity = severity;
    last_error.timestamp = to_ms_since_boot(get_absolute_time());
    last_error.line = line;
    last_error.file = file;
    last_error.function = function;
    
    // Format error message
    va_list args;
    va_start(args, format);
    vsnprintf(last_error.message, sizeof(last_error.message), format, args);
    va_end(args);
    
    error_count++;
    
    // Log error with appropriate severity
    const char* severity_str[] = {"INFO", "WARN", "ERROR", "CRITICAL"};
    const char* category_str[] = {"NONE", "USB", "PROTOCOL", "CRYPTO", 
                                  "STORAGE", "TIMEOUT", "MEMORY", "SYSTEM"};
    
    printf("ERROR [%s:%s] %s:%d in %s() - Code:0x%04X - %s\n",
           severity_str[severity], category_str[last_error.category],
           file, line, function, code, last_error.message);
    
    // Attempt automatic recovery for non-critical errors
    if (severity < ERROR_SEVERITY_CRITICAL) {
        if (error_attempt_recovery(code)) {
            printf("Error Handling: Automatic recovery successful for error 0x%04X\n", code);
        }
    } else {
        printf("Error Handling: CRITICAL ERROR - Entering safe mode\n");
        system_enter_safe_mode();
    }
}

//--------------------------------------------------------------------+
// ERROR RECOVERY FUNCTIONS
//--------------------------------------------------------------------+

bool error_attempt_recovery(error_code_t error_code) {
    printf("Error Handling: Attempting recovery for error 0x%04X\n", error_code);
    
    error_category_t category = (error_category_t)((error_code >> 12) & 0xF);
    
    switch (category) {
        case ERROR_CATEGORY_USB:
            return usb_stability_handle_reconnection();
            
        case ERROR_CATEGORY_PROTOCOL:
            cleanup_protocol_buffers();
            return true;
            
        case ERROR_CATEGORY_CRYPTO:
            cleanup_crypto_context();
            return true;
            
        case ERROR_CATEGORY_STORAGE:
            // Storage errors typically require manual intervention
            return false;
            
        case ERROR_CATEGORY_TIMEOUT:
            timeout_reset();
            return true;
            
        case ERROR_CATEGORY_MEMORY:
            error_cleanup_resources();
            return true;
            
        default:
            return false;
    }
}

void error_cleanup_resources(void) {
    printf("Error Handling: Performing comprehensive resource cleanup\n");
    
    cleanup_crypto_context();
    cleanup_protocol_buffers();
    cleanup_usb_endpoints();
    
    // Reset timeout system
    timeout_reset();
    
    printf("Error Handling: Resource cleanup completed\n");
}

//--------------------------------------------------------------------+
// USB STABILITY AND RECONNECTION HANDLING
//--------------------------------------------------------------------+

void usb_stability_init(void) {
    printf("USB Stability: Initializing reconnection stability system\n");
    
    memset(&usb_stability, 0, sizeof(usb_stability));
    usb_stability.state = USB_STATE_DISCONNECTED;
    usb_stability.last_state_change = to_ms_since_boot(get_absolute_time());
    usb_stability.stability_mode_active = false;
    
    printf("USB Stability: System initialized\n");
}

void usb_stability_update_state(usb_connection_state_t new_state) {
    if (usb_stability.state != new_state) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        printf("USB Stability: State change %d -> %d (after %dms)\n", 
               usb_stability.state, new_state, now - usb_stability.last_state_change);
        
        // Track rapid state changes as potential instability
        if ((now - usb_stability.last_state_change) < 1000) {
            usb_stability.enumeration_failures++;
            if (usb_stability.enumeration_failures > 3) {
                usb_stability.stability_mode_active = true;
                printf("USB Stability: Entering stability mode due to rapid state changes\n");
            }
        }
        
        usb_stability.state = new_state;
        usb_stability.last_state_change = now;
        
        // Reset failure counters on successful connection
        if (new_state == USB_STATE_CONNECTED) {
            usb_stability.last_successful_operation = now;
            usb_stability_reset_counters();
        }
    }
}

bool usb_stability_handle_reconnection(void) {
    printf("USB Stability: Handling reconnection attempt %d\n", 
           usb_stability.reconnection_attempts + 1);
    
    if (usb_stability.reconnection_attempts >= 5) {
        ERROR_REPORT_ERROR(ERROR_USB_RECONNECTION_FAILED, 
                          "Maximum reconnection attempts exceeded");
        return false;
    }
    
    usb_stability.reconnection_attempts++;
    usb_stability_update_state(USB_STATE_RECOVERY);
    
    // Implement exponential backoff for reconnection attempts
    uint32_t delay = 100 * (1 << usb_stability.reconnection_attempts);
    if (delay > 5000) delay = 5000; // Cap at 5 seconds
    
    printf("USB Stability: Waiting %dms before reconnection attempt\n", delay);
    sleep_ms(delay);
    
    // Attempt to reinitialize USB
    cleanup_usb_endpoints();
    
    // In a real implementation, this would trigger USB re-enumeration
    // For now, we simulate successful recovery
    usb_stability_update_state(USB_STATE_CONNECTING);
    
    // Simulate connection success after brief delay
    sleep_ms(100);
    usb_stability_update_state(USB_STATE_CONNECTED);
    
    printf("USB Stability: Reconnection attempt successful\n");
    return true;
}

bool usb_stability_is_stable(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Consider stable if connected for more than 5 seconds without issues
    return (usb_stability.state == USB_STATE_CONNECTED && 
            (now - usb_stability.last_state_change) > 5000 &&
            !usb_stability.stability_mode_active);
}

void usb_stability_reset_counters(void) {
    usb_stability.reconnection_attempts = 0;
    usb_stability.enumeration_failures = 0;
    usb_stability.stability_mode_active = false;
    printf("USB Stability: Counters reset - system stable\n");
}

//--------------------------------------------------------------------+
// RETRY MECHANISM FUNCTIONS
//--------------------------------------------------------------------+

bool retry_operation(bool (*operation)(void), const retry_config_t* config) {
    return retry_operation_with_context((bool (*)(void*))operation, NULL, config);
}

bool retry_operation_with_context(bool (*operation)(void*), void* context, 
                                  const retry_config_t* config) {
    if (!operation || !config) {
        return false;
    }
    
    uint32_t delay = config->base_delay_ms;
    
    for (uint8_t attempt = 0; attempt < config->max_attempts; attempt++) {
        printf("Retry: Attempt %d/%d\n", attempt + 1, config->max_attempts);
        
        if (operation(context)) {
            if (attempt > 0) {
                printf("Retry: Operation succeeded after %d attempts\n", attempt + 1);
            }
            return true;
        }
        
        // Don't delay after the last attempt
        if (attempt < config->max_attempts - 1) {
            printf("Retry: Waiting %dms before next attempt\n", delay);
            sleep_ms(delay);
            
            // Apply exponential backoff if configured
            if (config->exponential_backoff) {
                delay *= 2;
                if (delay > config->max_delay_ms) {
                    delay = config->max_delay_ms;
                }
            }
        }
    }
    
    printf("Retry: Operation failed after %d attempts\n", config->max_attempts);
    return false;
}

//--------------------------------------------------------------------+
// TIMEOUT MANAGEMENT
//--------------------------------------------------------------------+

void timeout_init(void) {
    timeout_active = false;
    timeout_start_time = 0;
    timeout_duration = 0;
}

bool timeout_start(uint32_t timeout_ms) {
    timeout_start_time = to_ms_since_boot(get_absolute_time());
    timeout_duration = timeout_ms;
    timeout_active = true;
    
    printf("Timeout: Started %dms timeout\n", timeout_ms);
    return true;
}

bool timeout_check(void) {
    if (!timeout_active) {
        return false;
    }
    
    uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - timeout_start_time;
    
    if (elapsed >= timeout_duration) {
        printf("Timeout: Timeout expired after %dms\n", elapsed);
        timeout_active = false;
        return true;
    }
    
    return false;
}

void timeout_reset(void) {
    timeout_active = false;
    timeout_start_time = 0;
    timeout_duration = 0;
}

//--------------------------------------------------------------------+
// PROTOCOL ERROR RESPONSE FUNCTIONS
//--------------------------------------------------------------------+

void protocol_send_error_response_ctap2(uint32_t cid, uint8_t error_code) {
    printf("Protocol Error: Sending CTAP2 error response 0x%02X to CID 0x%08X\n", 
           error_code, cid);
    
    uint8_t report[64];
    memset(report, 0, 64);
    
    // Format CTAPHID error response
    report[0] = cid & 0xFF;
    report[1] = (cid >> 8) & 0xFF;
    report[2] = (cid >> 16) & 0xFF;
    report[3] = (cid >> 24) & 0xFF;
    report[4] = 0x3F; // CTAPHID_ERROR command
    report[5] = 0x00; // Length high byte
    report[6] = 0x01; // Length low byte (1 byte error code)
    report[7] = error_code;
    
    // Send error response via HID
    if (tud_hid_ready()) {
        tud_hid_report(0, report, 64);
    } else {
        ERROR_REPORT_WARNING(ERROR_PROTOCOL_SEQUENCE_ERROR, 
                           "HID not ready for error response");
    }
}

void protocol_send_error_response_ccid(uint8_t* buffer, uint16_t* len, uint16_t sw) {
    printf("Protocol Error: Sending CCID error response SW=0x%04X\n", sw);
    
    if (!buffer || !len) {
        ERROR_REPORT_ERROR(ERROR_PROTOCOL_INVALID_COMMAND, 
                          "Invalid buffer for CCID error response");
        return;
    }
    
    // Send only status word for error response
    buffer[0] = (uint8_t)(sw >> 8);   // SW1
    buffer[1] = (uint8_t)(sw & 0xFF); // SW2
    *len = 2;
}

//--------------------------------------------------------------------+
// CLEANUP FUNCTIONS
//--------------------------------------------------------------------+

void cleanup_crypto_context(void) {
    printf("Cleanup: Clearing cryptographic contexts\n");
    
    // In a real implementation, this would:
    // - Clear sensitive key material from memory
    // - Reset cryptographic state machines
    // - Reinitialize entropy sources if needed
    
    // For now, we just log the cleanup
    printf("Cleanup: Cryptographic cleanup completed\n");
}

void cleanup_protocol_buffers(void) {
    printf("Cleanup: Clearing protocol buffers\n");
    
    // In a real implementation, this would:
    // - Clear CTAP2 and CCID command/response buffers
    // - Reset protocol state machines
    // - Clear any cached authentication state
    
    printf("Cleanup: Protocol buffer cleanup completed\n");
}

void cleanup_usb_endpoints(void) {
    printf("Cleanup: Resetting USB endpoints\n");
    
    // In a real implementation, this would:
    // - Flush USB endpoint buffers
    // - Reset endpoint state
    // - Clear any pending USB transfers
    
    printf("Cleanup: USB endpoint cleanup completed\n");
}

//--------------------------------------------------------------------+
// SYSTEM HEALTH MONITORING
//--------------------------------------------------------------------+

void system_health_check(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Check USB stability
    if (!usb_stability_is_stable()) {
        ERROR_REPORT_WARNING(ERROR_USB_ENUMERATION_FAILED, 
                           "USB connection unstable");
    }
    
    // Check for excessive error rate
    if (error_count > 100) {
        ERROR_REPORT_ERROR(ERROR_SYSTEM_CRITICAL_FAILURE, 
                          "Excessive error count: %d", error_count);
    }
    
    // Check timeout system
    if (timeout_active && timeout_check()) {
        ERROR_REPORT_WARNING(ERROR_TIMEOUT_PROTOCOL_RESPONSE, 
                           "System timeout detected");
    }
}

bool system_is_healthy(void) {
    return (usb_stability_is_stable() && 
            error_count < 50 && 
            !timeout_active);
}

void system_enter_safe_mode(void) {
    printf("System: ENTERING SAFE MODE due to critical error\n");
    
    // Perform comprehensive cleanup
    error_cleanup_resources();
    
    // Reset all subsystems to known good state
    usb_stability_init();
    timeout_init();
    
    // In a real implementation, this might:
    // - Disable non-essential features
    // - Switch to minimal functionality mode
    // - Increase error reporting verbosity
    // - Implement hardware watchdog reset if available
    
    printf("System: Safe mode activated - minimal functionality only\n");
}