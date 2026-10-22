#pragma once
// Resolves the selected board's header from the LAB_BOARD_* flag in boards/<name>/board.ini.
// `just add-board` appends a branch here.

#if defined(LAB_BOARD_S3MINI)
#include "s3mini/board.h"
#elif defined(LAB_BOARD_M5STICKC_PLUS11)
#include "m5stickc-plus11/board.h"
#else
#error "No LAB_BOARD_* flag: build a board env, e.g. `just build <project> s3mini`"
#endif

#ifdef ARDUINO
#include "lab_layout.h"
#include "lab_pixel.h"
#endif

namespace lab_board {

inline constexpr int kSafePins[] = BOARD_SAFE_PINS;

// True when `pin` may carry LED data or I2S on this board (see boards/<name>/pinout.md).
constexpr bool pin_is_safe(int pin) {
  for (int p : kSafePins) {
    if (p == pin) return true;
  }
  return false;
}

// LED data may also go to the board's own onboard RGB LED, which is wired on the PCB.
constexpr bool led_pin_ok(int pin) {
#ifdef BOARD_ONBOARD_RGB
  if (pin == BOARD_ONBOARD_RGB) return true;
#endif
  return pin_is_safe(pin);
}

#ifdef ARDUINO
// Board power-up: on the M5StickC this enables the AXP192 rails (TFT, Grove 5 V) via M5Unified.
// Call after lab_core::begin() so Serial is already owned by lab_core.
void begin();

// Drives BOARD_STATUS_LED if the board has one (handles active-low); no-op otherwise.
void status_led(bool on);

// Prints a line on the board's display if BOARD_HAS_TFT; no-op otherwise.
void status_text(const char* line);

// Clears the board's display and homes the cursor if BOARD_HAS_TFT; no-op otherwise.
void status_clear();

// Fills the board's display with one color if BOARD_HAS_TFT; no-op otherwise.
void screen_fill(uint8_t r, uint8_t g, uint8_t b);

// Draws the LEDs as a grid of squares (layout columns x rows, full brightness), right-aligned on
// the board's display, if BOARD_HAS_TFT; no-op otherwise.
void status_pixels(const lab::Pixel* px, const lab::Layout& layout);

// Debounced button events since the last call; call once per loop(). All false on boards
// without buttons. Hold fires once, after 500 ms.
struct Buttons {
  bool a_click = false;
  bool a_hold = false;
  bool b_click = false;
  bool b_hold = false;
};
Buttons buttons();

// Acceleration in g on the IMU chip's own axes (projects map them to their frame); false, with
// zeros, on boards without BOARD_HAS_IMU or when the read fails.
bool accel(float* x, float* y, float* z);
#endif

}  // namespace lab_board
