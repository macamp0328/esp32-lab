#pragma once
// p002's playlist logic: which effect plays, auto-advance, crossfade, brightness level and which
// set of effects is on, driven by button events and the clock. Pure, so it is host-tested
// (test/test_demo).

#include <stdint.h>

namespace demo {

struct Input {
  bool a_click = false;  // next effect, then stay on it
  bool a_hold = false;   // next effect, back to auto-advance
  bool b_click = false;  // next brightness level
  bool b_hold = false;   // next set (1, 2, ..., then all), from its first effect
};

constexpr uint8_t kAllSets = 0;

struct Config {
  uint8_t count;          // effects in the playlist
  uint32_t auto_ms;       // auto-advance period
  uint32_t fade_ms;       // crossfade length
  const uint8_t* levels;  // brightness levels, cycled by B
  uint8_t level_count;
  const uint8_t* set_of = nullptr;  // each effect's set (1..sets); nullptr: a single set
  uint8_t sets = 1;
};

struct State {
  uint8_t fx = 0;    // effect playing (fading in)
  uint8_t prev = 0;  // effect fading out; == fx before the first change
  uint32_t started_ms = 0;
  uint32_t prev_started_ms = 0;
  uint8_t level = 0;
  bool autoplay = true;
  uint8_t set = 1;  // 1..sets, or kAllSets
};

inline bool in_set(const State& s, const Config& c, uint8_t i) {
  return s.set == kAllSets || !c.set_of || c.set_of[i] == s.set;
}

// The next effect of the current set after `from`, wrapping; `from` when it is the only one.
inline uint8_t next_in_set(const State& s, const Config& c, uint8_t from) {
  for (uint8_t k = 1; k <= c.count; ++k) {
    const uint8_t i = static_cast<uint8_t>((from + k) % c.count);
    if (in_set(s, c, i)) return i;
  }
  return from;
}

inline void go_to(State& s, uint8_t fx, uint32_t now) {
  s.prev = s.fx;
  s.prev_started_ms = s.started_ms;
  s.fx = fx;
  s.started_ms = now;
}

// Applies one loop's input at `now`; true when the effect, level or autoplay changed.
inline bool step(State& s, const Config& c, const Input& in, uint32_t now) {
  bool changed = false;
  if (in.b_hold && c.sets > 1) {
    s.set = s.set == kAllSets ? 1 : (s.set >= c.sets ? kAllSets : static_cast<uint8_t>(s.set + 1));
    go_to(s, next_in_set(s, c, static_cast<uint8_t>(c.count - 1)), now);  // its first effect
    changed = true;
  } else if (in.a_hold || in.a_click) {
    s.autoplay = in.a_hold;
    go_to(s, next_in_set(s, c, s.fx), now);
    changed = true;
  } else if (s.autoplay && now - s.started_ms >= c.auto_ms) {
    go_to(s, next_in_set(s, c, s.fx), now);
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
