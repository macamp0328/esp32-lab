#pragma once
// Effects: (pixels, layout, audio, time) -> pixels. Pure, so each is host-tested; on the board
// lab::Pixel is CRGB. Add new effects here with a test in shared/tests/test/test_fx.

#include <stdint.h>

#include "lab_audio_frame.h"
#include "lab_layout.h"
#include "lab_pixel.h"

namespace lab_fx {

// Every LED red, green, then blue for 1 s each: the bring-up and HIL-check effect.
inline void sweep(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                  uint32_t t_ms) {
  static const lab::Pixel kColors[] = {lab::Pixel(255, 0, 0), lab::Pixel(0, 255, 0),
                                       lab::Pixel(0, 0, 255)};
  const lab::Pixel c = kColors[(t_ms / 1000) % 3];
  for (uint16_t i = 0; i < layout.size(); ++i) px[i] = c;
}

// Hue follows position (x on strips and matrices, angle on rings) and drifts one full turn
// every ~4 s; louder audio is brighter.
inline void rainbow(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame& audio,
                    uint32_t t_ms) {
  const uint8_t drift = static_cast<uint8_t>(t_ms / 16);
  const float level = audio.level < 0 ? 0 : (audio.level > 1 ? 1 : audio.level);
  const uint8_t value = 160 + static_cast<uint8_t>(95 * level);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const uint8_t hue = static_cast<uint8_t>(layout.angle(i) * 255) + drift;
    px[i] = lab::hsv(hue, 255, value);
  }
}

}  // namespace lab_fx
