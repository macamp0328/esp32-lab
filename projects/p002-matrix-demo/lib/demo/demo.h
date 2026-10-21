#pragma once
// p002's playlist logic: which effect plays, auto-advance, crossfade and brightness level,
// driven by button events and the clock. Pure, so it is host-tested (test/test_demo).

#include <stdint.h>

namespace demo {

struct Input {
  bool a_click = false;  // next effect, then stay on it
  bool a_hold = false;   // next effect, back to auto-advance
  bool b_click = false;  // next brightness level
};

struct Config {
  uint8_t count;          // effects in the playlist
  uint32_t auto_ms;       // auto-advance period
  uint32_t fade_ms;       // crossfade length
  const uint8_t* levels;  // brightness levels, cycled by B
  uint8_t level_count;
};

struct State {
  uint8_t fx = 0;    // effect playing (fading in)
  uint8_t prev = 0;  // effect fading out; == fx before the first change
  uint32_t started_ms = 0;
  uint32_t prev_started_ms = 0;
  uint8_t level = 0;
  bool autoplay = true;
};

inline void go_next(State& s, const Config& c, uint32_t now) {
  s.prev = s.fx;
  s.prev_started_ms = s.started_ms;
  s.fx = static_cast<uint8_t>((s.fx + 1) % c.count);
  s.started_ms = now;
}

// Applies one loop's input at `now`; true when the effect, level or autoplay changed.
inline bool step(State& s, const Config& c, const Input& in, uint32_t now) {
  bool changed = false;
  if (in.a_hold || in.a_click) {
    s.autoplay = in.a_hold;
    go_next(s, c, now);
    changed = true;
  } else if (s.autoplay && now - s.started_ms >= c.auto_ms) {
    go_next(s, c, now);
    changed = true;
  }
  if (in.b_click) {
    s.level = static_cast<uint8_t>((s.level + 1) % c.level_count);
    changed = true;
  }
  return changed;
}

// Weight of the new effect: 0 right after a change, 1 once the crossfade is over.
inline float fade(const State& s, const Config& c, uint32_t now) {
  if (s.fx == s.prev || c.fade_ms == 0) return 1;
  const uint32_t in = now - s.started_ms;
  return in >= c.fade_ms ? 1 : static_cast<float>(in) / c.fade_ms;
}

inline uint8_t brightness(const State& s, const Config& c) { return c.levels[s.level]; }

}  // namespace demo
