#pragma once
// The pixel type effects write: FastLED's CRGB on the board; a plain {r,g,b} on the host, so
// effects and mappers are unit-tested natively (FastLED 3.10.7 does not build for PlatformIO
// native). Color math lives here, not in FastLED, so host and board run the same code.

#include <stdint.h>

#ifdef ARDUINO
#include <FastLED.h>
namespace lab {
using Pixel = CRGB;
}  // namespace lab
#else
namespace lab {
struct Pixel {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  constexpr Pixel() = default;
  constexpr Pixel(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  constexpr bool operator==(const Pixel& o) const { return r == o.r && g == o.g && b == o.b; }
};
}  // namespace lab
#endif

namespace lab {

// HSV to RGB in six linear sectors (hue 0 red, 85 green, 170 blue).
inline Pixel hsv(uint8_t h, uint8_t s, uint8_t v) {
  const uint16_t h6 = static_cast<uint16_t>(h) * 6;  // 0..1530
  const uint8_t sector = h6 >> 8;
  const int f = h6 & 0xff;
  const uint8_t p = v * (255 - s) / 255;
  const uint8_t q = v * (255 - s * f / 255) / 255;
  const uint8_t t = v * (255 - s * (255 - f) / 255) / 255;
  switch (sector) {
    case 0:
      return Pixel(v, t, p);
    case 1:
      return Pixel(q, v, p);
    case 2:
      return Pixel(p, v, t);
    case 3:
      return Pixel(p, q, v);
    case 4:
      return Pixel(t, p, v);
    default:
      return Pixel(v, p, q);
  }
}

}  // namespace lab
