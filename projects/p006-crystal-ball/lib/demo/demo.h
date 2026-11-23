#pragma once
// Which pattern the demo shows when. Pure; host-tested in test/test_demo.

#include <stdint.h>

#include "lab_fx_math.h"

namespace demo {

// Pattern `from` is showing; in the last fade of its turn it crossfades into `to` (mix 0 -> 1).
struct Slot {
  uint8_t from;
  uint8_t to;
  float mix;
};

// `count` patterns take turns of turn_ms each, in order, looping; fade_ms is clipped to turn_ms.
inline Slot at(uint32_t ms, uint8_t count, uint32_t turn_ms, uint32_t fade_ms) {
  if (count == 0 || turn_ms == 0) return {0, 0, 0};
  if (fade_ms > turn_ms) fade_ms = turn_ms;
  const uint32_t turn = ms / turn_ms, into = ms % turn_ms;
  const uint8_t from = turn % count, to = (turn + 1) % count;
  if (fade_ms == 0 || into < turn_ms - fade_ms) return {from, from, 0};
  return {from, to, lab::smooth(static_cast<float>(into - (turn_ms - fade_ms)) / fade_ms)};
}

}  // namespace demo
