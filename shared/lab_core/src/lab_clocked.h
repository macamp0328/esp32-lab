#pragma once
// Clocked LEDs (APA102 and kin) on plain hardware SPI, using only the two pins named.

#include <FastLED.h>

namespace lab_core {

// Use this, not FastLED.addLeds<APA102, ...>: on the S3 that takes the LCD peripheral, which also
// drives GPIO 0 and 21 and cannot clock at 1 MHz (docs/lessons.md 2026-11-22).
template <ESPIChipsets CHIP, uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder ORDER, uint32_t RATE_HZ>
CLEDController& add_clocked_leds(CRGB* leds, int count) {
  using Chip = CFastLED::ClockedChipsetHelper<CHIP, DATA_PIN, CLOCK_PIN>;
  static_assert(Chip::IS_VALID, "LED_TYPE is not a clocked chipset FastLED knows");
  using Controller =
      typename Chip::template CONTROLLER_CLASS_WITH_ORDER_AND_FREQ<ORDER, RATE_HZ>::ControllerType;
  static Controller controller;
  return FastLED.addLeds(&controller, leds, count);
}

}  // namespace lab_core
