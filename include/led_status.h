#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <stdbool.h>
#include <stdint.h>


// Color definitions (GRB format for WS2812 usually, or RGB)
// We will abstract this.

typedef enum {
  LED_COLOR_OFF = 0,
  LED_COLOR_GREEN,  // Idle / Success
  LED_COLOR_BLUE,   // FIDO2 / U2F
  LED_COLOR_YELLOW, // OATH
  LED_COLOR_RED,    // Error
  LED_COLOR_PURPLE, // OpenPGP
  LED_COLOR_WHITE   // Boot / Init
} led_color_t;

void led_status_init(void);
void led_status_set(led_color_t color);
void led_status_signal_activity(led_color_t color); // Short blink
void led_status_update_loop(void); // Call in main loop for non-blocking effects

#endif // LED_STATUS_H
