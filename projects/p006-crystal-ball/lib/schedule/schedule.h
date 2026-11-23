#pragma once
// When the orb glows. Pure; host-tested in test/test_schedule.

#include <stdint.h>

#include "lab_fx_math.h"

namespace schedule {

constexpr uint32_t kDayS = 86400;

// 0..1 at `now_s` seconds after local midnight: 0 outside on_s..off_s (which may cross midnight),
// rising over the first fade_s and back to 0 over the last. on_s == off_s is always off.
inline float level(uint32_t now_s, uint32_t on_s, uint32_t off_s, uint32_t fade_s) {
  const uint32_t window = (off_s + kDayS - on_s % kDayS) % kDayS;
  const uint32_t elapsed = (now_s + kDayS - on_s % kDayS) % kDayS;
  if (elapsed >= window) return 0;
  if (fade_s == 0) return 1;
  const float up = static_cast<float>(elapsed) / fade_s;
  const float down = static_cast<float>(window - elapsed) / fade_s;
  return lab::smooth(lab::clamp01(up < down ? up : down));
}

// 0..1 for the first total_ms after power-up, fading in and out over fade_ms; 0 afterwards.
inline float boot_level(uint32_t ms, uint32_t total_ms, uint32_t fade_ms) {
  if (ms >= total_ms) return 0;
  if (fade_ms == 0) return 1;
  const float up = static_cast<float>(ms) / fade_ms;
  const float down = static_cast<float>(total_ms - ms) / fade_ms;
  return lab::smooth(lab::clamp01(up < down ? up : down));
}

}  // namespace schedule
