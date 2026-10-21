#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "lab_board.h"
#include "lab_core.h"

#if !defined(POWER_VOLTS) || !defined(POWER_MAX_MA)
#error "config.h must define POWER_VOLTS and POWER_MAX_MA"
#endif
#if POWER_MAX_MA > 2000 && !defined(POWER_README_TABLE)
#error "POWER_MAX_MA > 2000 needs a README power table naming supply and fuse; then define it"
#endif
static_assert(lab_board::led_pin_ok(LED_DATA_PIN), "LED_DATA_PIN is not in BOARD_SAFE_PINS");

namespace {
CRGB leds[NUM_LEDS];
constexpr uint32_t kFpsPeriodMs = 5000;
// Red, green, blue for 1 s each: what a human sees also confirms the LED's color order.
const CRGB kSweep[] = {CRGB::Red, CRGB::Green, CRGB::Blue};
}  // namespace

void setup() {
  lab_core::begin(LAB_PROJECT, LAB_FW);
  lab_board::begin();
  lab_board::status_text(LAB_PROJECT);
  lab_board::status_text(LAB_FW);
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(POWER_VOLTS, POWER_MAX_MA);
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

  fill_solid(leds, NUM_LEDS, kSweep[(now / 1000) % 3]);
  FastLED.show();
  lab_board::status_led((now / 500) % 2);  // 1 Hz heartbeat on boards with a status LED
  ++frames;

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
