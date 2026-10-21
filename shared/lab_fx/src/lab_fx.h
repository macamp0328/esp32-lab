#pragma once
// Effects: (pixels, layout, audio, time) -> pixels. Pure, so each is host-tested; on the board
// lab::Pixel is CRGB. Add new effects here with a test in shared/tests/test/test_fx.
// Effects are functions of time only (no state), so any frame can be rendered in any order.

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "lab_audio_frame.h"
#include "lab_font.h"
#include "lab_fx_math.h"
#include "lab_layout.h"
#include "lab_pixel.h"

namespace lab_fx {

using Fn = void (*)(lab::Pixel*, const lab::Layout&, const lab::AudioFrame&, uint32_t);

// Every LED red, green, then blue for 1 s each: the bring-up and HIL-check effect.
inline void sweep(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                  uint32_t t_ms) {
  static const lab::Pixel kColors[] = {lab::Pixel(255, 0, 0), lab::Pixel(0, 255, 0),
                                       lab::Pixel(0, 0, 255)};
  const lab::Pixel c = kColors[(t_ms / 1000) % 3];
  for (uint16_t i = 0; i < layout.size(); ++i) px[i] = c;
}

// Bring-up probe in wiring order, not layout order: LED 0 red, last LED of the first wired line
// green, first LED of the second line blue, last LED white. Green at the end of the top row means
// rows (else `by_columns`); blue next to green means serpentine, next to red progressive; white
// proves the whole chain.
inline void wiring(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&, uint32_t) {
  const uint16_t n = layout.size();
  for (uint16_t i = 0; i < n; ++i) px[i] = lab::Pixel(0, 0, 0);
  if (n == 0) return;
  const uint16_t w = layout.by_columns() ? layout.height() : layout.width();
  px[n - 1] = lab::Pixel(255, 255, 255);
  if (w < n) px[w] = lab::Pixel(0, 0, 255);
  px[w - 1] = lab::Pixel(0, 255, 0);
  px[0] = lab::Pixel(255, 0, 0);
}

// Hue follows position (x on strips and matrices, angle on rings) and drifts one full turn
// every ~4 s; louder audio is brighter.
inline void rainbow(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame& audio,
                    uint32_t t_ms) {
  const uint8_t drift = static_cast<uint8_t>(t_ms / 16);
  const float level = !(audio.level > 0) ? 0 : (audio.level > 1 ? 1 : audio.level);  // NaN -> 0
  const uint8_t value = 160 + static_cast<uint8_t>(95 * level);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const uint8_t hue = static_cast<uint8_t>(layout.angle(i) * 255) + drift;
    px[i] = lab::hsv(hue, 255, value);
  }
}

namespace detail {

// Distance from the layout's center (0..~0.7) and angle round it (0..1).
struct Polar {
  float d;
  float a;
};

inline Polar polar(const lab::Layout& layout, uint16_t i) {
  const lab::Point p = layout.position(i);
  const float dx = p.x - 0.5f, dy = p.y - 0.5f;
  return {sqrtf(dx * dx + dy * dy), lab::frac(atan2f(dy, dx) / lab::kTau)};
}

// An 8x8 bitmap (one byte per row, bit 7 = left column) stretched over the layout.
inline bool bitmap8(const uint8_t* rows, const lab::Layout& layout, uint16_t i) {
  const lab::Point p = layout.position(i);
  auto cell = [](float v) { return v <= 0 ? 0 : (v >= 0.875f ? 7 : static_cast<int>(v * 8)); };
  return (rows[cell(p.y)] >> (7 - cell(p.x))) & 1;
}

}  // namespace detail

// Sine-sum plasma in full hue.
inline void plasma(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  const float cx = 0.5f + 0.3f * sinf(s * 0.4f), cy = 0.5f + 0.3f * cosf(s * 0.3f);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    const float dx = p.x - cx, dy = p.y - cy;
    const float v = sinf(lab::kTau * p.x + s) + sinf(lab::kTau * 0.8f * p.y - 1.3f * s) +
                    sinf(lab::kTau * 0.6f * (p.x + p.y) + 0.7f * s) +
                    sinf(lab::kTau * 1.2f * sqrtf(dx * dx + dy * dy) - 1.1f * s);  // -4..4
    px[i] =
        lab::hsv(static_cast<uint8_t>(static_cast<uint32_t>((v + 4) * 32) + t_ms / 40), 255, 255);
  }
}

// Rings expanding from the center; each ring keeps its hue as it travels.
inline void ripple(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const float phase = 2.5f * detail::polar(layout, i).d - 0.8f * s;
    float v = 0.5f + 0.5f * cosf(lab::kTau * phase);
    v *= v;
    px[i] = lab::hsv(static_cast<uint8_t>(lab::frac(phase / 8) * 255), 255,
                     static_cast<uint8_t>(255 * v * v));
  }
}

// Two arms twisting round the center and rotating, hue following the angle.
inline void spiral(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const detail::Polar q = detail::polar(layout, i);
    const float arm = 0.5f + 0.5f * cosf(lab::kTau * (2 * q.a + 3 * q.d - 0.5f * s));
    const uint8_t hue = static_cast<uint8_t>(lab::frac(q.a + q.d - 0.1f * s) * 255);
    px[i] = lab::hsv(hue, 255, static_cast<uint8_t>(255 * (0.2f + 0.8f * arm * arm)));
  }
}

// A green beam sweeping one turn every 2 s with a fading trail; amber blips light as it passes.
inline void radar(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                  uint32_t t_ms) {
  const float turns = t_ms * 0.0005f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const detail::Polar q = detail::polar(layout, i);
    const float swept = turns - q.a + 1;   // > 0; the integer part counts beam passes
    const float since = lab::frac(swept);  // turns since the beam last passed
    lab::Pixel c = lab::scale(lab::Pixel(30, 255, 60), expf(-10 * since));
    const uint32_t pass = static_cast<uint32_t>(swept);
    if (q.d > 0.15f && lab::hash01(i * 2654435761U ^ lab::hash32(pass)) > 0.92f) {
      c = lab::blend(c, lab::Pixel(255, 150, 0), expf(-2.5f * since));
    }
    px[i] = c;
  }
}

// Digital rain: one falling head per column at its own speed, with a green tail above it.
inline void rain(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&, uint32_t t_ms) {
  constexpr float kTail = 5;
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const uint32_t c = layout.column(i);
    const float speed = 5 + 7 * lab::hash01(c * 3 + 1);  // rows per second
    const float period = layout.height() + kTail + 3 + 6 * lab::hash01(c * 3 + 2);  // with gap
    const float head = floorf(fmodf(s * speed + period * lab::hash01(c * 3 + 3), period));
    const float behind = head - layout.row(i);  // rows above the head
    if (behind == 0) {
      px[i] = lab::Pixel(180, 255, 180);
    } else if (behind > 0 && behind < kTail) {
      const float k = 1 - behind / kTail;
      px[i] = lab::scale(lab::Pixel(0, 255, 40), k * k);
    } else {
      px[i] = lab::Pixel(0, 0, 0);
    }
  }
}

// Stars fading in and out at random: warm white, cool white and gold.
inline void twinkle(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                    uint32_t t_ms) {
  static const lab::Pixel kStars[] = {lab::Pixel(255, 190, 110), lab::Pixel(150, 190, 255),
                                      lab::Pixel(255, 140, 20)};
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const float cycles = s / (1.2f + 2.4f * lab::hash01(i * 2 + 1)) + lab::hash01(i * 2 + 2);
    const uint32_t seed = lab::hash32(i * 977U ^ lab::hash32(static_cast<uint32_t>(cycles)));
    if (lab::hash01(seed) > 0.4f) {
      px[i] = lab::Pixel(0, 0, 0);
      continue;
    }
    const float env = sinf(3.1415927f * lab::frac(cycles));
    px[i] = lab::scale(kStars[lab::hash32(seed + 1) % 3], env * env);
  }
}

// Flames rising from the bottom row: two octaves of noise scrolling upward push tongues of heat
// up from a hot base; black/red/yellow.
inline void fire(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&, uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    const float n = 0.65f * lab::noise(3 * p.x, 2.5f * p.y + 1.8f * s, 0.5f * s) +
                    0.35f * lab::noise(7 * p.x, 6 * p.y + 4.5f * s, 1.3f * s + 7);
    const float h = 1.3f * p.y - 0.4f + 1.2f * (n - 0.5f);
    px[i] = lab::heat(h < 0.72f ? h : 0.72f);  // stop at yellow; white is too hot for 8x8
  }
}

// Lava lamp: three metaballs drifting on Lissajous paths, orange on deep purple.
inline void lava(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&, uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  float bx[3], by[3];
  for (int k = 0; k < 3; ++k) {
    bx[k] = 0.5f + 0.32f * sinf(s * (0.31f + 0.13f * k) + 2.1f * k);
    by[k] = 0.5f + 0.32f * sinf(s * (0.23f + 0.17f * k) + 1.3f * k + 1);
  }
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    float f = 0;
    for (int k = 0; k < 3; ++k) {
      const float dx = p.x - bx[k], dy = p.y - by[k];
      f += 0.035f / (dx * dx + dy * dy + 0.002f);
    }
    const lab::Pixel hot =
        lab::blend(lab::Pixel(255, 40, 0), lab::Pixel(255, 170, 0), (f - 1.2f) * 0.5f);
    px[i] = lab::blend(lab::Pixel(25, 0, 45), hot, (f - 0.7f) * 1.5f);
  }
}

// A ball bouncing under gravity with a short trail; it changes hue on every bounce.
inline void bounce(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  constexpr uint32_t kPeriodMs = 1200, kTrailMs = 35;
  constexpr float kR = 0.06f;  // ball center stays this far from the edges
  auto ball = [](uint32_t t, float* x, float* y) {
    const float u = (t % kPeriodMs) / static_cast<float>(kPeriodMs);
    const float across = lab::frac(t / 3100.0f);
    *x = kR + (1 - 2 * kR) * (1 - fabsf(2 * across - 1));
    *y = 1 - kR - 4 * u * (1 - u) * (1 - 2 * kR);
  };
  float bx[5], by[5];
  for (uint32_t k = 0; k < 5; ++k)
    ball(t_ms >= k * kTrailMs ? t_ms - k * kTrailMs : 0, &bx[k], &by[k]);
  const uint8_t hue = static_cast<uint8_t>(t_ms / kPeriodMs * 47);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    float v = 0;
    for (int k = 0; k < 5; ++k) {
      const float d = sqrtf((p.x - bx[k]) * (p.x - bx[k]) + (p.y - by[k]) * (p.y - by[k]));
      const float vk = lab::clamp01(1 - (d - 0.06f) / 0.1f) * (k == 0 ? 1 : 0.45f * (1 - k / 5.0f));
      if (vk > v) v = vk;
    }
    px[i] = lab::hsv(hue, 255, static_cast<uint8_t>(255 * v));
  }
}

// A heart beating lub-dub once a second.
inline void heart(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                  uint32_t t_ms) {
  static const uint8_t kHeart[8] = {0x00, 0x66, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00};
  const float u = (t_ms % 1000) * 0.001f;
  auto pulse = [u](float center) {
    const float z = (u - center) / 0.06f;
    return expf(-z * z);
  };
  const float beat = lab::clamp01(pulse(0.08f) + 0.7f * pulse(0.32f));
  const lab::Pixel c = lab::scale(lab::blend(lab::Pixel(200, 0, 10), lab::Pixel(255, 40, 70), beat),
                                  0.35f + 0.65f * beat);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    px[i] = detail::bitmap8(kHeart, layout, i) ? c : lab::Pixel(0, 0, 0);
  }
}

// A space invader walking in place (two frames, 0.5 s each), slowly changing color.
inline void invader(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                    uint32_t t_ms) {
  static const uint8_t kFrames[2][8] = {{0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x5A, 0xA5},
                                        {0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x5A, 0x81, 0x42}};
  const uint8_t* rows = kFrames[(t_ms / 500) % 2];
  const lab::Pixel c = lab::hsv(static_cast<uint8_t>(t_ms / 40), 200, 255);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    px[i] = detail::bitmap8(rows, layout, i) ? c : lab::Pixel(0, 0, 0);
  }
}

// Scrolls `s` right to left in the 5x7 font at `cols_per_s`, hue sweeping along the text. Rows
// beyond the layout are clipped, so a strip shows the font's middle row.
inline void text(lab::Pixel* px, const lab::Layout& layout, const char* s, uint32_t t_ms,
                 uint32_t cols_per_s = 8) {
  constexpr int kPitch = lab::kGlyphW + 1;
  const int w = layout.width();
  const int text_cols = static_cast<int>(strlen(s)) * kPitch;
  if (text_cols + w == 0) return;
  const int offset = static_cast<int>(static_cast<uint64_t>(t_ms) * cols_per_s / 1000 %
                                      static_cast<uint64_t>(text_cols + w));
  const int y0 = (static_cast<int>(layout.height()) - lab::kGlyphH) / 2;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const int tc = layout.column(i) + offset - w;  // text column under this LED
    const int y = layout.row(i) - y0;
    px[i] = lab::Pixel(0, 0, 0);
    if (tc < 0 || tc >= text_cols || y < 0 || y >= lab::kGlyphH || tc % kPitch == lab::kGlyphW) {
      continue;
    }
    if ((lab::glyph(s[tc / kPitch])[tc % kPitch] >> y) & 1) {
      px[i] = lab::hsv(static_cast<uint8_t>(tc * 6 + t_ms / 20), 255, 255);
    }
  }
}

}  // namespace lab_fx
