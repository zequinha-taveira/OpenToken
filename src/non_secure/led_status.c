#include "led_status.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

// Hardware Configuration
#ifndef WS2812_PIN
#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
#define WS2812_PIN 22
#endif
#endif
#define IS_RGBW false

static PIO g_pio = pio0;
static uint g_sm = 0;
static uint32_t g_last_update = 0;
static led_color_t g_current_color = LED_COLOR_OFF;
static led_color_t g_base_color = LED_COLOR_GREEN; // Default idle

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

static void put_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(g_pio, g_sm, pixel_grb << 8u);
}

void led_status_init(void) {
  // Initialize PIO
  uint offset = pio_add_program(g_pio, &ws2812_program);
  ws2812_program_init(g_pio, g_sm, offset, WS2812_PIN, 800000, IS_RGBW);

  // Set initial color (White for boot)
  led_status_set(LED_COLOR_WHITE);
}

static uint32_t get_pixel_value(led_color_t color) {
  // BRIGHTNESS SCALING (16 out of 255 - dim for standard usage)
  const uint8_t B = 16;

  switch (color) {
  case LED_COLOR_OFF:
    return urgb_u32(0, 0, 0);
  case LED_COLOR_GREEN:
    return urgb_u32(0, B, 0);
  case LED_COLOR_BLUE:
    return urgb_u32(0, 0, B);
  case LED_COLOR_YELLOW:
    return urgb_u32(B, B, 0); // Red + Green
  case LED_COLOR_RED:
    return urgb_u32(B, 0, 0);
  case LED_COLOR_PURPLE:
    return urgb_u32(B, 0, B); // Red + Blue
  case LED_COLOR_WHITE:
    return urgb_u32(B, B, B);
  default:
    return urgb_u32(0, 0, 0);
  }
}

void led_status_set(led_color_t color) {
  g_current_color = color;
  put_pixel(get_pixel_value(color));
}

// Simple direct set for now
void led_status_signal_activity(led_color_t color) {
  led_status_set(color);
  // In a complex RTOS/loop, we'd set a timer to revert.
  // Here we just set it. The caller or main loop might reset it.
}

void led_status_update_loop(void) {
  // Placeholder for breathing/blinking effects
}
