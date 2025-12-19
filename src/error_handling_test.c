#include "error_handling.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Test functions for error handling system validation

// Test function that always fails (for retry testing)
static bool test_always_fails(void) {
    printf("Test: Operation that always fails\n");
    return false;
}

// Test function that succeeds after 2 attempts
static int test_attempt_counter = 0;
static bool test_succeeds_after_retries(void) {
    test_attempt_counter++;
    printf("Test: Attempt %d\n", test_attempt_counter);
    
    if (test_attempt_counter >= 2) {
        test_attempt_counter = 0; // Reset for next test
        return true;
    }
    return false;
}

// Test function with context
static bool test_with_context(void* context) {
    int* value = (int*)context;
    printf("Test: Context value = %d\n", *value);
    (*value)++;
    return (*value >= 3);
}

// Test USB stability system
void test_usb_stability(void) {
    printf("\n=== Testing USB Stability System ===\n");
    
    // Test state transitions
    usb_stability_update_state(USB_STATE_CONNECTING);
    sleep_ms(100);
    usb_stability_update_state(USB_STATE_CONNECTED);
    
    printf("USB Stability: Is stable = %s\n", 
           usb_stability_is_stable() ? "true" : "false");
    
    // Test rapid state changes (should trigger stability mode)
    for (int i = 0; i < 5; i++) {
        usb_stability_update_state(USB_STATE_DISCONNECTED);
        usb_stability_update_state(USB_STATE_CONNECTING);
        sleep_ms(50); // Rapid changes
    }
    
    // Test reconnection handling
    bool reconnect_result = usb_stability_handle_reconnection();
    printf("USB Stability: Reconnection result = %s\n", 
           reconnect_result ? "success" : "failed");
}

// Test retry mechanism
void test_retry_mechanism(void) {
    printf("\n=== Testing Retry Mechanism ===\n");
    
    // Test retry with function that always fails
    bool result1 = retry_operation(test_always_fails, &RETRY_CONFIG_PROTOCOL);
    printf("Retry Test 1 (always fails): %s\n", result1 ? "PASS" : "FAIL (expected)");
    
    // Test retry with function that succeeds after attempts
    bool result2 = retry_operation(test_succeeds_after_retries, &RETRY_CONFIG_PROTOCOL);
    printf("Retry Test 2 (succeeds after retries): %s\n", result2 ? "PASS" : "FAIL");
    
    // Test retry with context
    int context_value = 0;
    bool result3 = retry_operation_with_context(test_with_context, &context_value, &RETRY_CONFIG_PROTOCOL);
    printf("Retry Test 3 (with context): %s (final value = %d)\n", 
           result3 ? "PASS" : "FAIL", context_value);
}

// Test timeout system
void test_timeout_system(void) {
    printf("\n=== Testing Timeout System ===\n");
    
    // Test timeout start and check
    timeout_start(1000); // 1 second timeout
    printf("Timeout: Started 1000ms timeout\n");
    
    sleep_ms(500);
    bool expired1 = timeout_check();
    printf("Timeout: After 500ms, expired = %s\n", expired1 ? "true" : "false");
    
    sleep_ms(600);
    bool expired2 = timeout_check();
    printf("Timeout: After 1100ms total, expired = %s\n", expired2 ? "true" : "false");
    
    // Test timeout reset
    timeout_reset();
    bool expired3 = timeout_check();
    printf("Timeout: After reset, expired = %s\n", expired3 ? "true" : "false");
}

// Test error reporting system
void test_error_reporting(void) {
    printf("\n=== Testing Error Reporting System ===\n");
    
    // Test different error severities
    ERROR_REPORT_INFO(ERROR_USB_ENUMERATION_FAILED, "Test info message");
    ERROR_REPORT_WARNING(ERROR_PROTOCOL_MALFORMED_PACKET, "Test warning message");
    ERROR_REPORT_ERROR(ERROR_CRYPTO_KEY_GENERATION, "Test error message");
    
    // Test error with parameters
    ERROR_REPORT_WARNING(ERROR_STORAGE_FULL, "Storage %d%% full", 95);
    
    printf("Error Reporting: All test messages sent\n");
}

// Test protocol error responses
void test_protocol_error_responses(void) {
    printf("\n=== Testing Protocol Error Responses ===\n");
    
    // Test CTAP2 error response
    protocol_send_error_response_ctap2(0x12345678, 0x01);
    
    // Test CCID error response
    uint8_t buffer[64];
    uint16_t len;
    protocol_send_error_response_ccid(buffer, &len, 0x6A82);
    printf("Protocol: CCID error response length = %d bytes\n", len);
}

// Test system health monitoring
void test_system_health(void) {
    printf("\n=== Testing System Health Monitoring ===\n");
    
    bool healthy1 = system_is_healthy();
    printf("System Health: Initial state = %s\n", healthy1 ? "healthy" : "unhealthy");
    
    system_health_check();
    
    bool healthy2 = system_is_healthy();
    printf("System Health: After check = %s\n", healthy2 ? "healthy" : "unhealthy");
}

// Main test function
void run_error_handling_tests(void) {
    printf("\n========================================\n");
    printf("OpenToken Error Handling System Tests\n");
    printf("========================================\n");
    
    // Initialize error handling system
    error_handling_init();
    
    // Run all tests
    test_error_reporting();
    test_retry_mechanism();
    test_timeout_system();
    test_usb_stability();
    test_protocol_error_responses();
    test_system_health();
    
    printf("\n========================================\n");
    printf("Error Handling Tests Completed\n");
    printf("========================================\n");
}