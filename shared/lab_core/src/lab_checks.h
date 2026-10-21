#pragma once
// Compile-time project rules. Include in src/main.cpp right after config.h.

#include "lab_board.h"

#if !defined(POWER_VOLTS) || !defined(POWER_MAX_MA)
#error "config.h must define POWER_VOLTS and POWER_MAX_MA"
#endif
#if POWER_MAX_MA > 2000 && !defined(POWER_README_TABLE)
#error "POWER_MAX_MA > 2000 needs a README power table naming supply and fuse; then define it"
#endif
#if MATRIX_W > 0 && MATRIX_W * MATRIX_H != NUM_LEDS
#error "MATRIX_W * MATRIX_H must equal NUM_LEDS"
#endif
static_assert(lab_board::led_pin_ok(LED_DATA_PIN), "LED_DATA_PIN is not in BOARD_SAFE_PINS");
