#pragma once
// LED power estimate, same model as FastLED's power manager (WS2812 at 5 V): a full channel draws
// red 16 mA, green 11 mA, blue 15 mA, plus 1 mA per LED idle. Pure; works on CRGB or any {r,g,b}.

#include <stddef.h>
#include <stdint.h>

namespace lab {

constexpr uint32_t kRedMa = 16, kGreenMa = 11, kBlueMa = 15, kIdleMa = 1;

// Estimated mA for `n` pixels shown at global `brightness` (0..255).
template <typename Px>
uint32_t estimate_ma(const Px* px, size_t n, uint8_t brightness = 255) {
  uint64_t weighted = 0;  // sum of channel * mA, in 1/255 mA units
  for (size_t i = 0; i < n; ++i) {
    weighted += px[i].r * kRedMa + px[i].g * kGreenMa + px[i].b * kBlueMa;
  }
  return static_cast<uint32_t>(weighted * brightness / (255u * 255u)) + kIdleMa * n;
}

// Highest brightness <= `wanted` whose estimate stays within `max_ma`.
template <typename Px>
uint8_t brightness_for_cap(const Px* px, size_t n, uint8_t wanted, uint32_t max_ma) {
  const uint32_t at_full = estimate_ma(px, n, 255) - kIdleMa * n;
  const uint32_t idle = kIdleMa * n;
  if (max_ma <= idle) return 0;
  if (at_full == 0) return wanted;
  const uint32_t limit = static_cast<uint32_t>((uint64_t)(max_ma - idle) * 255 / at_full);
  return static_cast<uint8_t>(limit < wanted ? limit : wanted);
}

}  // namespace lab
