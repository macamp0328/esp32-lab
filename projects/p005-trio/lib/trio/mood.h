#pragma once
// The law at the mood tempo: each board's hue drifts slowly round the color wheel and nudges
// toward (calm room) or away from (stirred room) the others' hues. The room decides the sign: a
// slow average of how disturbed the three links are, plus a kick per clap. Agreement is one
// shared color; dissent for three is a triad, a third of the wheel apart. Pure; host-tested.

#include <math.h>
#include <stdint.h>

#include "law.h"

namespace trio {

struct MoodConfig {
  float lap_s = 7200;        // the palette circles the wheel about this often
  float detune = 0.15f;      // each board's drift differs from the lap by up to this share
  float k_calm = 2.0e-3f;    // 1/s: pull toward agreement in a quiet room
  float k_storm = 8.0e-3f;   // 1/s: push toward a triad in a stirred one
  float calm_below = 0.12f;  // room levels: agreement below, full dissent above stir_above
  float stir_above = 0.3f;
  float room_s = 120;       // how long the room remembers
  float clap_kick = 0.12f;  // how much one clap stirs the room
};

// The coupling for a room stirred 0..1: +k_calm when quiet, -k_storm once stirred, with a smooth
// turn between calm_below and stir_above (where the hues drift free near zero).
inline float mood_k(float room, const MoodConfig& c) {
  float u = (room - c.calm_below) / (c.stir_above - c.calm_below);
  u = u < 0 ? 0 : (u > 1 ? 1 : u);
  u = u * u * (3 - 2 * u);
  return c.k_calm - (c.k_calm + c.k_storm) * u;
}

// How stirred the room has been lately, 0..1.
struct Room {
  float value = 0;

  void push(float stir_now, float dt_s, const MoodConfig& c) {
    value += (stir_now - value) * (1 - expf(-dt_s / c.room_s));
  }
  void clap(const MoodConfig& c) { value = fminf(1, value + c.clap_kick); }
};

struct Hue {
  float h = 0;   // 0..1 round the wheel
  float nu = 0;  // natural drift, turns per second

  // Drift from a 0..1 seed (the MAC), so each board has its own.
  void set_drift(float seed01, const MoodConfig& c) {
    nu = (1 + c.detune * (2 * seed01 - 1)) / c.lap_s;
  }

  void step(float dt_s, float k, const float* others, int n) {
    h = wrap01(h + (nu + k * pull(h, others, n)) * dt_s);
  }
};

// The color a corner shows: its hue fanned out by up to `fan` turns (m in the middle, a below,
// b above) as far as the three agree (r_hue, their order), so agreement reads as an analogous
// palette rather than one flat color, and a triad stays a true triad.
inline float shown_hue(float h, int slot, float r_hue, float fan) {
  static const float kFan[3] = {0, -1, 1};
  return wrap01(h + kFan[slot < 0 ? 0 : slot] * fan * r_hue);
}

}  // namespace trio
