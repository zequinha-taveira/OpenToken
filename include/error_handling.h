#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <stdbool.h>
#include <stdint.h>

// Error handling system for OpenToken
// Provides centralized error management, recovery, and stability features

//--------------------------------------------------------------------+
// ERROR CODES AND CATEGORIES
//--------------------------------------------------------------------+

// Error categories for systematic error handling
typedef enum {
  ERROR_CATEGORY_NONE = 0,
  ERROR_CATEGORY_USB,
  ERROR_CATEGORY_PROTOCOL,
  ERROR_CATEGORY_CRYPTO,
  ERROR_CATEGORY_STORAGE,
  ERROR_CATEGORY_TIMEOUT,
  ERROR_CATEGORY_MEMORY,
  ERROR_CATEGORY_SYSTEM
} error_category_t;

// Specific error codes within categories
typedef enum {
  // USB errors (0x1000-0x1FFF)
  ERROR_USB_ENUMERATION_FAILED = 0x1001,
  ERROR_USB_DESCRIPTOR_INVALID = 0x1002,
  ERROR_USB_ENDPOINT_ERROR = 0x1003,
  ERROR_USB_POWER_MANAGEMENT = 0x1004,
  ERROR_USB_RECONNECTION_FAILED = 0x1005,

  // Protocol errors (0x2000-0x2FFF)
  ERROR_PROTOCOL_INVALID_COMMAND = 0x2001,
  ERROR_PROTOCOL_MALFORMED_PACKET = 0x2002,
  ERROR_PROTOCOL_UNSUPPORTED_VERSION = 0x2003,
  ERROR_PROTOCOL_SEQUENCE_ERROR = 0x2004,
  ERROR_PROTOCOL_BUFFER_OVERFLOW = 0x2005,

  // Cryptographic errors (0x3000-0x3FFF)
  ERROR_CRYPTO_KEY_GENERATION = 0x3001,
  ERROR_CRYPTO_SIGNATURE_FAILED = 0x3002,
  ERROR_CRYPTO_VERIFICATION_FAILED = 0x3003,
  ERROR_CRYPTO_RNG_FAILURE = 0x3004,
  ERROR_CRYPTO_INVALID_KEY = 0x3005,
  ERROR_CRYPTO_FAILURE = 0x3006,

  // Storage errors (0x4000-0x4FFF)
  ERROR_STORAGE_WRITE_FAILED = 0x4001,
  ERROR_STORAGE_READ_FAILED = 0x4002,
  ERROR_STORAGE_CORRUPTION = 0x4003,
  ERROR_STORAGE_FULL = 0x4004,
  ERROR_STORAGE_FLASH_ERROR = 0x4005,

  // Timeout errors (0x5000-0x5FFF)
  ERROR_TIMEOUT_USER_PRESENCE = 0x5001,
  ERROR_TIMEOUT_PROTOCOL_RESPONSE = 0x5002,
  ERROR_TIMEOUT_USB_OPERATION = 0x5003,
  ERROR_TIMEOUT_CRYPTO_OPERATION = 0x5004,

  // Memory errors (0x6000-0x6FFF)
  ERROR_MEMORY_ALLOCATION = 0x6001,
  ERROR_MEMORY_CORRUPTION = 0x6002,
  ERROR_MEMORY_BUFFER_OVERFLOW = 0x6003,
  ERROR_OUT_OF_MEMORY = 0x6004,

  // System errors (0x7000-0x7FFF)
  ERROR_SYSTEM_INITIALIZATION = 0x7001,
  ERROR_SYSTEM_WATCHDOG = 0x7002,
  ERROR_SYSTEM_CRITICAL_FAILURE = 0x7003
} error_code_t;

// Error severity levels
typedef enum {
  ERROR_SEVERITY_INFO = 0,
  ERROR_SEVERITY_WARNING = 1,
  ERROR_SEVERITY_ERROR = 2,
  ERROR_SEVERITY_CRITICAL = 3
} error_severity_t;

// Error context structure
typedef struct {
  error_code_t code;
  error_category_t category;
  error_severity_t severity;
  uint32_t timestamp;
  uint16_t line;
  const char *file;
  const char *function;
  char message[128];
} error_context_t;

//--------------------------------------------------------------------+
// RETRY AND TIMEOUT CONFIGURATION
//--------------------------------------------------------------------+

// Retry configuration for different operations
typedef struct {
  uint8_t max_attempts;
  uint32_t base_delay_ms;
  uint32_t max_delay_ms;
  bool exponential_backoff;
} retry_config_t;

// Timeout configuration
typedef struct {
  uint32_t usb_operation_timeout_ms;
  uint32_t protocol_response_timeout_ms;
  uint32_t user_presence_timeout_ms;
  uint32_t crypto_operation_timeout_ms;
} timeout_config_t;

//--------------------------------------------------------------------+
// USB STABILITY AND RECONNECTION
//--------------------------------------------------------------------+

// USB connection state tracking
typedef enum {
  USB_STATE_DISCONNECTED = 0,
  USB_STATE_CONNECTING,
  USB_STATE_CONNECTED,
  USB_STATE_SUSPENDED,
  USB_STATE_ERROR,
  USB_STATE_RECOVERY
} usb_connection_state_t;

// USB stability context
typedef struct {
  usb_connection_state_t state;
  uint32_t last_state_change;
  uint8_t reconnection_attempts;
  uint8_t enumeration_failures;
  bool stability_mode_active;
  uint32_t last_successful_operation;
} usb_stability_context_t;

//--------------------------------------------------------------------+
// FUNCTION DECLARATIONS
//--------------------------------------------------------------------+

// Error handling system initialization
void error_handling_init(void);

// Error reporting and logging
void error_report(error_code_t code, error_severity_t severity,
                  const char *file, uint16_t line, const char *function,
                  const char *format, ...);

// Error recovery functions
bool error_attempt_recovery(error_code_t error_code);
void error_cleanup_resources(void);

// USB stability functions
void usb_stability_init(void);
void usb_stability_update_state(usb_connection_state_t new_state);
bool usb_stability_handle_reconnection(void);
bool usb_stability_is_stable(void);
void usb_stability_reset_counters(void);

// Retry mechanism functions
bool retry_operation(bool (*operation)(void), const retry_config_t *config);
bool retry_operation_with_context(bool (*operation)(void *), void *context,
                                  const retry_config_t *config);

// Timeout management
void timeout_init(void);
bool timeout_start(uint32_t timeout_ms);
bool timeout_check(void);
void timeout_reset(void);

// Protocol error response functions
void protocol_send_error_response_ctap2(uint32_t cid, uint8_t error_code);
void protocol_send_error_response_ccid(uint8_t *buffer, uint16_t *len,
                                       uint16_t sw);

// Memory and resource cleanup
void cleanup_crypto_context(void);
void cleanup_protocol_buffers(void);
void cleanup_usb_endpoints(void);

// System health monitoring
void system_health_check(void);
bool system_is_healthy(void);
void system_enter_safe_mode(void);

// Macros for convenient error reporting
#define ERROR_REPORT(code, severity, ...)                                      \
  error_report(code, severity, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define ERROR_REPORT_CRITICAL(code, ...)                                       \
  ERROR_REPORT(code, ERROR_SEVERITY_CRITICAL, __VA_ARGS__)

#define ERROR_REPORT_ERROR(code, ...)                                          \
  ERROR_REPORT(code, ERROR_SEVERITY_ERROR, __VA_ARGS__)

#define ERROR_REPORT_WARNING(code, ...)                                        \
  ERROR_REPORT(code, ERROR_SEVERITY_WARNING, __VA_ARGS__)

#define ERROR_REPORT_INFO(code, ...)                                           \
  ERROR_REPORT(code, ERROR_SEVERITY_INFO, __VA_ARGS__)

// Default retry configurations
extern const retry_config_t RETRY_CONFIG_USB;
extern const retry_config_t RETRY_CONFIG_PROTOCOL;
extern const retry_config_t RETRY_CONFIG_CRYPTO;
extern const retry_config_t RETRY_CONFIG_STORAGE;

// Default timeout configuration
extern const timeout_config_t DEFAULT_TIMEOUTS;

#endif // ERROR_HANDLING_H