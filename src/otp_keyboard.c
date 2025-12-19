#include "bsp/board.h"
#include "tusb.h"
#include <ctype.h>

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
void otp_keyboard_task(void) {
  // Placeholder: Check button state (debounced)
  // If pressed long enough -> Get Default OTP Slot -> Calculate -> Type

  // Example usage (triggered elsewhere):
  // otp_keyboard_type("123456");
}
