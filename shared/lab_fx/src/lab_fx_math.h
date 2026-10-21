#pragma once
// Small math for effects: integer hash, smooth value noise, color scale/blend, heat palette.
// Pure and single-precision (the ESP32 FPU); host-tested in shared/tests/test/test_fx.

#include <math.h>
#include <stdint.h>

#include "lab_pixel.h"

namespace lab {

constexpr float kTau = 6.2831853f;

// lowbias32: well mixed, so hash32(i) is usable as per-LED randomness.
constexpr uint32_t hash32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

// hash32 mapped to 0..1.
inline float hash01(uint32_t x) { return (hash32(x) >> 8) * (1.0f / 16777216.0f); }

inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float frac(float v) { return v - floorf(v); }
inline float smooth(float t) { return t * t * (3 - 2 * t); }

// Value noise in 3D, 0..1: random values on the integer lattice, smoothstepped in between.
inline float noise(float x, float y, float z) {
  const float fx = floorf(x), fy = floorf(y), fz = floorf(z);
  const uint32_t ix = static_cast<uint32_t>(static_cast<int32_t>(fx));
  const uint32_t iy = static_cast<uint32_t>(static_cast<int32_t>(fy));
  const uint32_t iz = static_cast<uint32_t>(static_cast<int32_t>(fz));
  const float u = smooth(x - fx), v = smooth(y - fy), w = smooth(z - fz);
  auto at = [&](uint32_t dx, uint32_t dy, uint32_t dz) {
    return hash01((ix + dx) * 73856093U ^ (iy + dy) * 19349663U ^ (iz + dz) * 83492791U);
  };
  auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
  const float y0 = lerp(lerp(at(0, 0, 0), at(1, 0, 0), u), lerp(at(0, 1, 0), at(1, 1, 0), u), v);
  const float y1 = lerp(lerp(at(0, 0, 1), at(1, 0, 1), u), lerp(at(0, 1, 1), at(1, 1, 1), u), v);
  return lerp(y0, y1, w);
}

// Color times k (0..1).
inline Pixel scale(Pixel c, float k) {
  k = clamp01(k);
  return Pixel(static_cast<uint8_t>(c.r * k + 0.5f), static_cast<uint8_t>(c.g * k + 0.5f),
               static_cast<uint8_t>(c.b * k + 0.5f));
}

// a at t = 0, b at t = 1.
inline Pixel blend(Pixel a, Pixel b, float t) {
  t = clamp01(t);
  auto mix = [t](uint8_t x, uint8_t y) { return static_cast<uint8_t>(x + (y - x) * t + 0.5f); };
  return Pixel(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b));
}

// Black -> red -> yellow -> white for heat 0..1.
inline Pixel heat(float h) {
  const float t = clamp01(h) * 3;
  if (t < 1) return Pixel(static_cast<uint8_t>(255 * t), 0, 0);
  if (t < 2) return Pixel(255, static_cast<uint8_t>(255 * (t - 1)), 0);
  return Pixel(255, 255, static_cast<uint8_t>(255 * (t - 2)));
}

}  // namespace lab
