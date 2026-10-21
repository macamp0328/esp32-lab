#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"

namespace {
CRGB leds[NUM_LEDS];
#if MATRIX_W > 0
const lab::Layout layout = lab::Layout::matrix(MATRIX_W, MATRIX_H, MATRIX_SERPENTINE);
#else
const lab::Layout layout = lab::Layout::strip(NUM_LEDS);
#endif
constexpr uint32_t kFpsPeriodMs = 5000;
}  // namespace

void setup() {
  lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});  // applies the power cap
  lab_board::begin();
  lab_board::status_text(LAB_PROJECT);
  lab_board::status_text(LAB_FW);
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  lab_core::Event("leds")
      .num("n", NUM_LEDS)
      .num("pin", LED_DATA_PIN)
      .str("driver", lab_core::led_driver())
      .num("cap_ma", POWER_MAX_MA)
      .emit();
}

void loop() {
  static uint32_t frames = 0;
  static uint32_t window_start = millis();
  const uint32_t now = millis();

  lab_fx::sweep(leds, layout, lab::AudioFrame{}, now);  // red, green, blue: 1 s each
  FastLED.show();
  lab_board::status_led((now / 500) % 2);  // 1 Hz heartbeat on boards with a status LED
  ++frames;

  if (now - window_start >= kFpsPeriodMs) {
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("ma_est", lab::estimate_ma(leds, NUM_LEDS, FastLED.getBrightness()))
        .emit();
    frames = 0;
    window_start = now;
  }
  delay(10);
}
