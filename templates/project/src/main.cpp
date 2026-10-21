#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "lab_core.h"

#if !defined(POWER_VOLTS) || !defined(POWER_MAX_MA)
#error "config.h must define POWER_VOLTS and POWER_MAX_MA"
#endif
#if POWER_MAX_MA > 2000 && !defined(POWER_README_TABLE)
#error "POWER_MAX_MA > 2000 needs a README power table naming supply and fuse; then define it"
#endif
static_assert(lab_board::pin_is_safe(LED_DATA_PIN), "LED_DATA_PIN is not in BOARD_SAFE_PINS");

namespace {
CRGB leds[NUM_LEDS];
constexpr uint32_t kFpsPeriodMs = 5000;
}  // namespace

void setup() {
  lab_core::begin(LAB_PROJECT, LAB_FW);
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(POWER_VOLTS, POWER_MAX_MA);
  FastLED.setBrightness(64);
  lab_core::Event("leds")
      .num("n", NUM_LEDS)
      .num("pin", LED_DATA_PIN)
      .str("driver", "RMT")
      .num("cap_ma", POWER_MAX_MA)
      .emit();
}

void loop() {
  static uint8_t hue = 0;
  static uint32_t frames = 0;
  static uint32_t window_start = millis();

  fill_rainbow(leds, NUM_LEDS, hue++, 255 / NUM_LEDS);
  FastLED.show();
  ++frames;

  const uint32_t now = millis();
  if (now - window_start >= kFpsPeriodMs) {
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .emit();
    frames = 0;
    window_start = now;
  }
  delay(10);
}
