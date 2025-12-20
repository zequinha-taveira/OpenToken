#include "bsp/board.h"
#include "hardware/gpio.h"
#include "led_status.h" // For LED feedback
#include "pico/stdlib.h"
#include "tusb.h"
#include <ctype.h>
#include <stdio.h>

// Helper to convert ASCII to HID Scancode (US Layout)
static uint8_t char_to_hid_code(char c, bool *shift) {
  *shift = false;

  // Lowercase
  if (c >= 'a' && c <= 'z') {
    return 0x04 + (c - 'a');
  }

  // Uppercase
  if (c >= 'A' && c <= 'Z') {
    *shift = true;
    return 0x04 + (c - 'A');
  }

  // Numbers
  if (c >= '1' && c <= '9') {
    return 0x1E + (c - '1');
  }
  if (c == '0')
    return 0x27;

  // Special characters (basic set)
  switch (c) {
  case ' ':
    return 0x2C;
  case '-':
    return 0x2D;
  case '=':
    return 0x2E;
  case '[':
    return 0x2F;
  case ']':
    return 0x30;
  case '\\':
    return 0x31;
  case ';':
    return 0x33;
  case '\'':
    return 0x34;
  case ',':
    return 0x36;
  case '.':
    return 0x37;
  case '/':
    return 0x38;
  case '\n':
    return 0x28; // Enter
  case '\t':
    return 0x2B; // Tab
  }

  // Symbols requiring shift (basic set)
  *shift = true;
  switch (c) {
  case '!':
    return 0x1E; // Shift + 1
  case '@':
    return 0x1F; // Shift + 2
  case '#':
    return 0x20; // Shift + 3
  case '$':
    return 0x21; // Shift + 4
  case '%':
    return 0x22; // Shift + 5
  case '^':
    return 0x23; // Shift + 6
  case '&':
    return 0x24; // Shift + 7
  case '*':
    return 0x25; // Shift + 8
  case '(':
    return 0x26; // Shift + 9
  case ')':
    return 0x27; // Shift + 0
  case '_':
    return 0x2D; // Shift + -
  case '+':
    return 0x2E; // Shift + =
  case ':':
    return 0x33; // Shift + ;
  case '"':
    return 0x34; // Shift + '
  case '<':
    return 0x36; // Shift + ,
  case '>':
    return 0x37; // Shift + .
  case '?':
    return 0x38; // Shift + /
  }

  return 0x00; // Unknown/Unsupported
}

// Function to type a string via HID Keyboard interface (Instance 1)
void otp_keyboard_type(const char *text) {
  if (!tud_hid_n_ready(1)) {
    // If USB not ready, wait a bit or drop (blocking here is risky in interrupt
    // context) Ideally, this should be non-blocking state machine. For now,
    // simpler implementation assuming readiness or short polling
    return;
  }

  while (*text) {
    bool shift = false;
    uint8_t keycode = char_to_hid_code(*text, &shift);

    if (keycode != 0) {
      // 1. Press Key
      uint8_t key_input[6] = {0};
      key_input[0] = keycode;

      // Modifier (0x02 = Left Shift)
      uint8_t modifier = shift ? 0x02 : 0x00;

      tud_hid_n_keyboard_report(1, 0, modifier, key_input);

      // Wait for report to be sent (simple delay, not robust for production)
      // In real App, use a task queue.
      sleep_ms(10);

      // 2. Release Key (Send empty report)
      tud_hid_n_keyboard_report(1, 0, 0, NULL);
      sleep_ms(10);
    }
    text++;
  }
}

// Handler for calculating and typing OTP
// This should be called when "Touch" is detected if configured for OTP
#define BUTTON_PIN PICO_DEFAULT_USER_BUTTON_PIN
static uint32_t last_button_press = 0;
static bool button_was_pressed = false;

// Need to include oath_applet.h for calculation
#include "oath_applet.h"

void otp_keyboard_init(void) {
#ifdef BUTTON_PIN
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN); // Assumes generic active-low button (pull to GND)
#endif
}

void otp_keyboard_task(void) {
#ifdef BUTTON_PIN
  // Simple debounce logic
  bool button_pressed = !gpio_get(BUTTON_PIN); // Active Low
  uint32_t now = to_ms_since_boot(get_absolute_time());

  if (button_pressed && !button_was_pressed) {
    if (now - last_button_press >
        1000) { // 1s debounce to prevent accidental double type
      printf("OpenToken: Button pressed! Calculating Real OTP...\n");

      // Blink LED to indicate processing
      led_status_set(LED_COLOR_PURPLE);

      char otp_string[8];
      if (oath_applet_calculate_default(otp_string)) {
        printf("OpenToken: Typing code %s\n", otp_string);
        otp_keyboard_type(otp_string);
        otp_keyboard_type("\n");         // Enter
        led_status_set(LED_COLOR_GREEN); // Success
      } else {
        printf("OpenToken: No OATH account found!\n");
        led_status_set(LED_COLOR_RED); // Error
        sleep_ms(500);
        led_status_set(LED_COLOR_GREEN);
      }

      last_button_press = now;
    }
  }
  button_was_pressed = button_pressed;
#endif
}
