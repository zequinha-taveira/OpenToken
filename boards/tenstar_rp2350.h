/*
 * Copyright (c) 2024 OpenToken
 *
 * Board definition for Tenstar RP2350-USB
 */

#ifndef _BOARDS_TENSTAR_RP2350_H
#define _BOARDS_TENSTAR_RP2350_H

// Allows the header to be included for boards that don't exist in the SDK
// #include "boards/pico_key.h"

// For board detection
#define TENSTAR_RP2350 1

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
// This board has a WS2812, not a standard LED, but we can define this to avoid
// build errors if something checks it. However, the led_status.c uses specific
// macros.
#define PICO_DEFAULT_LED_PIN 25 // Unused, but kept for compatibility
#endif

#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 22
#endif

// --- BUTTON ---
// Tenstar RP2350 usually has a user button on GP21 or GP23.
// Defining as GP21 for default, user can query/change.
#ifndef PICO_DEFAULT_USER_BUTTON_PIN
#define PICO_DEFAULT_USER_BUTTON_PIN 21
#endif

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// --- RP2350 Specific ---
#define PICO_RP2350 1

#endif
