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
#endif

}  // namespace lab_board
