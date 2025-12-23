/*
 * OpenToken Secure World Entry
 * Copyright (c) 2025 OpenToken Project
 */

#include "error_handling.h"
#include "hsm_layer.h"
#include "storage.h"


// Forward declaration for OTP Keyboard (User Presence)
// TODO: Move to a proper header
extern void otp_keyboard_init(void);

// Configuration for retries
static const retry_config_t RETRY_CONFIG_STORAGE_SEC = {.max_attempts = 3,
                                                        .base_delay_ms = 100,
                                                        .max_delay_ms = 500,
                                                        .exponential_backoff =
                                                            true};

static const retry_config_t RETRY_CONFIG_CRYPTO_SEC = {.max_attempts = 3,
                                                       .base_delay_ms = 50,
                                                       .max_delay_ms = 150,
                                                       .exponential_backoff =
                                                           false};

void secure_world_init(void) {
  // Initialize secure storage first
  if (!retry_operation((bool (*)(void))storage_init,
                       &RETRY_CONFIG_STORAGE_SEC)) {
    ERROR_REPORT_CRITICAL(ERROR_STORAGE_WRITE_FAILED,
                          "Secure Storage init failed");
    system_enter_safe_mode();
  }

  // Initialize HSM (Keys, Crypto)
  if (!retry_operation((bool (*)(void))hsm_init, &RETRY_CONFIG_CRYPTO_SEC)) {
    ERROR_REPORT_CRITICAL(ERROR_CRYPTO_KEY_GENERATION,
                          "Secure HSM init failed");
    system_enter_safe_mode();
  }

  // Initialize Secure User Presence (Button)
  otp_keyboard_init();
}
