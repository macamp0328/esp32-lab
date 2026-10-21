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

}  // namespace lab_board
