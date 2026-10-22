#pragma once
// The benchmark suite: compute, led, dualcore, memory and screen groups, one `bench` event per
// result, framed by `bench_start` and `bench_done`.

#include <FastLED.h>

#include "config.h"
#include "lab_layout.h"

struct Outputs {
  CRGB* buf;                           // BENCH_MAX_LEDS pixels; the showcase uses the first n
  CLEDController* out[BENCH_OUTPUTS];  // [0] is LED_DATA_PIN; the rest only for the parallel test
};

// Adds the parallel test's extra outputs, disabled. Call in setup() before the first show():
// FastLED 3.10.7 times out every show() once an output is added later, or set to 0 LEDs.
void add_outputs(Outputs& o);

// The layout the suite and showcase render at n LEDs: the real 8x8 matrix, 16x16 or 32x32.
const lab::Layout& bench_layout(uint16_t n);

void run_suite(Outputs& o);
