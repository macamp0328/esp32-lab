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

inline void clear(lab::Pixel* px, const lab::Layout& layout) {
  for (uint16_t i = 0; i < layout.size(); ++i) px[i] = lab::Pixel(0, 0, 0);
}

// Per-channel max, so overlapping lights neither wrap nor wash out.
inline void lighten(lab::Pixel& dst, lab::Pixel c) {
  if (c.r > dst.r) dst.r = c.r;
  if (c.g > dst.g) dst.g = c.g;
  if (c.b > dst.b) dst.b = c.b;
}

// A soft dot at (x, y) in layout space, fading to nothing at `radius`.
inline void splat(lab::Pixel* px, const lab::Layout& layout, float x, float y, float radius,
                  lab::Pixel c) {
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    const float d2 = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
    if (d2 < radius * radius) lighten(px[i], lab::scale(c, 1 - sqrtf(d2) / radius));
  }
}

// 0 -> 1 -> 0 once per unit of u.
inline float tri(float u) { return 1 - fabsf(2 * lab::frac(u) - 1); }

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

// Warp speed: stars stream out of the center, accelerating and brightening toward the edge.
inline void starfield(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                      uint32_t t_ms) {
  constexpr uint32_t kStars = 24;
  const float s = t_ms * 0.001f;
  detail::clear(px, layout);
  for (uint32_t k = 0; k < kStars; ++k) {
    const float a = lab::kTau * lab::hash01(k * 4 + 1);
    const float life =
        lab::frac(s * (0.3f + 0.5f * lab::hash01(k * 4 + 2)) + lab::hash01(k * 4 + 3));
    const float r = 0.72f * life * life;
    const lab::Pixel tint =
        lab::blend(lab::Pixel(140, 170, 255), lab::Pixel(255, 255, 255), lab::hash01(k * 4 + 4));
    const float ca = cosf(a), sa = sinf(a), v = 0.15f + 0.85f * life * sqrtf(life);
    detail::splat(px, layout, 0.5f + r * ca, 0.5f + r * sa, 0.15f, lab::scale(tint, v));
    detail::splat(px, layout, 0.5f + 0.7f * r * ca, 0.5f + 0.7f * r * sa, 0.12f,
                  lab::scale(tint, 0.4f * v));  // streak
  }
}

namespace detail {

constexpr float kRiseS = 0.7f, kBurnS = 1.6f;

// One firework `u` seconds into its cycle: a rocket climbs for kRiseS, then sparks burst from its
// apex, falling and fading for kBurnS. `seed` picks its column, height and color.
inline void firework(lab::Pixel* px, const lab::Layout& layout, uint32_t seed, float u) {
  constexpr int kSparks = 14;
  const float x0 = 0.2f + 0.6f * lab::hash01(seed * 5 + 1);
  const float apex = 0.2f + 0.2f * lab::hash01(seed * 5 + 2);
  const uint8_t hue = static_cast<uint8_t>(lab::hash32(seed * 5 + 3));
  if (u < kRiseS) {
    const float k = 1 - u / kRiseS;
    const float y = 1 - (1 - apex) * (1 - k * k);  // decelerating climb
    splat(px, layout, x0, y, 0.15f, lab::Pixel(255, 140, 40));
    splat(px, layout, x0, y + 0.13f, 0.12f, lab::Pixel(110, 50, 10));
    return;
  }
  const float dt = u - kRiseS;
  if (dt >= kBurnS) return;
  if (dt < 0.15f) {
    splat(px, layout, x0, apex, 0.25f, lab::scale(lab::Pixel(255, 255, 255), 1 - dt / 0.15f));
  }
  const float fade = 1 - (dt / kBurnS) * (dt / kBurnS);  // holds, then drops
  for (int j = 0; j < kSparks; ++j) {
    const float a = lab::kTau * (j + 0.3f * lab::hash01(seed * 31 + j)) / kSparks;
    const float v = 0.3f + 0.12f * lab::hash01(seed * 37 + j);
    const float x = x0 + v * dt * cosf(a);
    const float y = apex + v * dt * sinf(a) + 0.25f * dt * dt;
    splat(px, layout, x, y, 0.15f,
          lab::scale(lab::hsv(static_cast<uint8_t>(hue + j * 4), 220, 255), fade));
  }
}

}  // namespace detail

// Fireworks: two launch slots half a cycle apart, each rising and bursting every 2.6 s.
inline void fireworks(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                      uint32_t t_ms) {
  constexpr uint32_t kCycleMs = 2600;
  detail::clear(px, layout);
  for (uint32_t slot = 0; slot < 2; ++slot) {
    const uint32_t t = t_ms + slot * kCycleMs / 2;
    detail::firework(px, layout, (t / kCycleMs) * 2 + slot, (t % kCycleMs) * 0.001f);
  }
}

// Aurora: green curtains waving on noise, brightest at their lower edge, turning teal and violet
// as they fade upward in vertical rays.
inline void aurora(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    const float edge = 0.72f + 0.44f * (lab::noise(1.6f * p.x + 0.15f * s, 0.2f * s, 3) - 0.5f);
    const float above = edge - p.y;
    const float rays = 0.3f + 0.7f * lab::noise(7 * p.x + 0.5f * s, 0.6f * s, 9);
    const float v = above >= 0 ? expf(-2.2f * above) * rays : expf(12 * above);
    const uint8_t hue = 96 + static_cast<uint8_t>(85 * lab::clamp01(1.3f * above));
    px[i] = lab::hsv(hue, 230, static_cast<uint8_t>(255 * v));
  }
}

namespace detail {

// Snowflake k at time s: it falls at its own speed and sways round its column.
inline void snowflake(uint32_t k, float s, float* x, float* y) {
  const float speed = 0.12f + 0.14f * lab::hash01(k * 4 + 1);
  *y = 1.2f * lab::frac(s * speed + lab::hash01(k * 4 + 2)) - 0.1f;
  *x = lab::hash01(k * 4 + 3) + 0.05f * sinf(1.3f * s + k);
}

}  // namespace detail

// Snow drifting down onto a white bottom row.
inline void snow(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&, uint32_t t_ms) {
  constexpr uint32_t kFlakes = 14;
  const float s = t_ms * 0.001f;
  detail::clear(px, layout);
  for (uint32_t k = 0; k < kFlakes; ++k) {
    float x, y;
    detail::snowflake(k, s, &x, &y);
    detail::splat(px, layout, x, y, 0.14f,
                  lab::scale(lab::Pixel(210, 225, 255), 0.7f + 0.3f * lab::hash01(k * 4 + 4)));
  }
  if (layout.height() < 2) return;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    if (layout.row(i) == layout.height() - 1) detail::lighten(px[i], lab::Pixel(110, 110, 140));
  }
}

// Tunnel: spiral hue rings rushing toward you and slowly turning; the far center is dark.
inline void tunnel(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const detail::Polar q = detail::polar(layout, i);
    const float d = q.d > 0.04f ? q.d : 0.04f;
    const float depth = 0.25f / d + 1.2f * s;
    float ring = 0.5f + 0.5f * cosf(lab::kTau * (depth + q.a + 0.15f * s));
    ring *= ring;
    const float v = (0.15f + 0.85f * ring) * lab::clamp01(3 * d);
    px[i] = lab::hsv(static_cast<uint8_t>(static_cast<uint32_t>(depth * 30) + t_ms / 40), 255,
                     static_cast<uint8_t>(255 * v));
  }
}

// Kaleidoscope: drifting noise folded into 8-way mirror symmetry about the center.
inline void kaleidoscope(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                         uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    float fx = fabsf(p.x - 0.5f), fy = fabsf(p.y - 0.5f);
    if (fy > fx) {
      const float swap = fx;
      fx = fy;
      fy = swap;
    }
    const float n = lab::noise(5 * fx + 0.4f * s, 5 * fy + 0.3f * s, 0.2f * s);
    const uint8_t hue = static_cast<uint8_t>(static_cast<uint32_t>(n * 380) + t_ms / 30);
    const float v = 0.15f + 0.85f * lab::smooth(lab::clamp01(1.6f * n - 0.3f));
    px[i] = lab::hsv(hue, 240, static_cast<uint8_t>(255 * v));
  }
}

// DNA: two strands (cyan, magenta) winding round a vertical axis, the nearer one brighter, with
// dim rungs between them on every other row.
inline void helix(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                  uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    const float th = lab::kTau * 0.8f * p.y + 2.2f * s;
    const float xa = 0.5f + 0.33f * sinf(th), xb = 1 - xa;
    const float za = cosf(th);  // +1 = in front
    auto strand = [&p](float xs, float z) {
      return lab::clamp01(1.3f - fabsf(p.x - xs) / 0.13f) * (0.3f + 0.35f * (z + 1));
    };
    lab::Pixel c = lab::scale(lab::Pixel(0, 220, 255), strand(xa, za));
    detail::lighten(c, lab::scale(lab::Pixel(255, 40, 200), strand(xb, -za)));
    if ((layout.row(i) & 1) == 0 && p.x > fminf(xa, xb) && p.x < fmaxf(xa, xb)) {
      detail::lighten(c, lab::Pixel(30, 30, 40));
    }
    px[i] = c;
  }
}

namespace detail {

constexpr uint32_t kStormMs = 3200;

// When storm cycle c strikes, in ms after the cycle starts.
inline uint32_t strike_ms(uint32_t c) { return 400 + lab::hash32(c * 7 + 1) % 1800; }

// The bolt's column (0..1) at height y (0 top .. 1 bottom) in storm cycle c: a hashed random walk.
inline float bolt_x(uint32_t c, float y) {
  float x = 0.25f + 0.5f * lab::hash01(c * 7 + 2);
  const int steps = static_cast<int>(y * 8);
  for (int k = 0; k < steps && k < 8; ++k) x += 0.22f * (lab::hash01(c * 131 + k) - 0.5f);
  return x;
}

}  // namespace detail

// Lightning: dim clouds churn along the top; once per 3.2 s a bolt forks down in 80 ms, the sky
// flashes twice and the bolt fades.
inline void lightning(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                      uint32_t t_ms) {
  const float s = t_ms * 0.001f;
  const uint32_t c = t_ms / detail::kStormMs;
  const float since =
      (static_cast<int32_t>(t_ms % detail::kStormMs) - static_cast<int32_t>(detail::strike_ms(c))) *
      0.001f;  // seconds since the strike; negative before it
  float flash = 0, bolt = 0, reach = -1;
  if (since >= 0) {
    flash =
        lab::clamp01(expf(-12 * since) + (since > 0.12f ? 0.6f * expf(-14 * (since - 0.12f)) : 0));
    bolt = since < 0.6f ? lab::clamp01(1.5f - 2.5f * since) : 0;
    reach = since / 0.08f;
  }
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const lab::Point p = layout.position(i);
    const float cloud =
        p.y < 0.4f
            ? (1 - p.y / 0.4f) * (0.3f + 0.7f * lab::noise(3 * p.x + 0.3f * s, 3 * p.y, 0.4f * s))
            : 0;
    lab::Pixel out = lab::scale(lab::Pixel(70, 70, 110), cloud);
    detail::lighten(out, lab::scale(lab::Pixel(90, 90, 140), 0.6f * flash));
    if (bolt > 0 && p.y <= reach && fabsf(p.x - detail::bolt_x(c, p.y)) < 0.08f) {
      detail::lighten(out, lab::scale(lab::Pixel(230, 230, 255), bolt));
    }
    px[i] = out;
  }
}

// Pong: paddles in the first and last column always return a ball bouncing round the court; each
// paddle follows the ball a little late.
inline void pong(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&, uint32_t t_ms) {
  const int w = layout.width(), h = layout.height();
  const float s = t_ms * 0.001f;
  auto ball_row = [h](float t) { return static_cast<int>(detail::tri(t / 2.9f) * (h - 1) + 0.5f); };
  auto paddle = [&](float lag) {
    const int c = ball_row(s - lag);
    return c > h - 2 ? (h - 2 < 1 ? 1 : h - 2) : (c < 1 ? 1 : c);
  };
  const int bx = w < 3 ? 0 : 1 + static_cast<int>(detail::tri(s / 2.0f) * (w - 3) + 0.5f);
  const int by = ball_row(s);
  const int left = paddle(0.08f), right = paddle(0.06f);
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const int col = layout.column(i), row = layout.row(i);
    lab::Pixel c(0, 0, 0);
    if (col == bx && row == by) {
      c = lab::Pixel(255, 255, 255);
    } else if (col == 0 && row >= left - 1 && row <= left + 1) {
      c = lab::Pixel(0, 200, 255);
    } else if (col == w - 1 && row >= right - 1 && row <= right + 1) {
      c = lab::Pixel(255, 60, 160);
    }
    px[i] = c;
  }
}

// Pac-Man eats a row of dots with a ghost on his tail, swallows the power pellet, then chases the
// blue ghost back across the screen. Maze walls run along the top and bottom rows.
inline void pacman(lab::Pixel* px, const lab::Layout& layout, const lab::AudioFrame&,
                   uint32_t t_ms) {
  static const uint8_t kOpen[6] = {0x1E, 0x3E, 0x38, 0x38, 0x3E, 0x1E};  // 6x6, bit 5 = left
  static const uint8_t kShut[6] = {0x1E, 0x3F, 0x3F, 0x3F, 0x3F, 0x1E};
  static const uint8_t kGhost[2][6] = {{0x1E, 0x3F, 0x3F, 0x3F, 0x3F, 0x2D},
                                       {0x1E, 0x3F, 0x3F, 0x3F, 0x3F, 0x33}};
  constexpr float kSpeed = 6;  // columns per second
  const int w = layout.width(), h = layout.height();
  const int y0 = (h - 6) / 2;
  const float out_s = (w + 15) / kSpeed, back_s = (w + 18) / kSpeed;
  const float u = fmodf(t_ms * 0.001f, out_s + back_s);
  const bool chased = u < out_s;  // first half: the ghost chases Pac-Man to the right
  int pac_x, ghost_x;
  if (chased) {
    pac_x = static_cast<int>(floorf(kSpeed * u)) - 6;
    ghost_x = pac_x - 9;
  } else {
    ghost_x = w + 3 - static_cast<int>(floorf(kSpeed * (u - out_s)));
    pac_x = ghost_x + 9;
  }
  const uint8_t* pac = (t_ms / 120) % 2 ? kShut : kOpen;
  const uint8_t* ghost = kGhost[(t_ms / 250) % 2];
  const bool blink = !chased && u - out_s > 1.8f && (t_ms / 125) % 2;
  const lab::Pixel ghost_c = chased ? lab::Pixel(255, 0, 0)
                                    : (blink ? lab::Pixel(255, 255, 255) : lab::Pixel(40, 40, 255));
  auto hit = [](const uint8_t* rows, int sx, int sy, bool mirror) {
    if (sx < 0 || sx > 5 || sy < 0 || sy > 5) return false;
    return ((rows[sy] >> (mirror ? sx : 5 - sx)) & 1) != 0;
  };
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const int col = layout.column(i), row = layout.row(i);
    lab::Pixel c(0, 0, 0);
    if (h >= 8 && (row == 0 || row == h - 1)) c = lab::Pixel(20, 20, 120);
    if (chased && row == y0 + 3) {
      if (col >= w - 2 && pac_x + 4 < w - 2) {
        c = lab::scale(lab::Pixel(255, 180, 150), 0.6f + 0.4f * ((t_ms / 200) % 2));
      } else if (col % 2 == 0 && col > pac_x + 4 && col < w - 2) {
        c = lab::Pixel(160, 120, 90);
      }
    }
    if (hit(ghost, col - ghost_x, row - y0, false)) {
      const bool eye = row - y0 == 2 && (col - ghost_x == 1 || col - ghost_x == 4);
      c = eye ? lab::Pixel(255, 255, 255) : ghost_c;
    }
    if (hit(pac, col - pac_x, row - y0, !chased)) c = lab::Pixel(255, 220, 0);
    px[i] = c;
  }
}

}  // namespace lab_fx
