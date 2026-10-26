#pragma once
// The law, at every tempo: each of the three nudges toward (k > 0) or away from (k < 0) the
// others on a circle (Kuramoto). For three, agreement is one point and the only stable dissent
// is exactly a third of a turn apart. Pure; host-tested in test/test_trio.

#include <math.h>
#include <stdint.h>

namespace trio {

constexpr float kTau = 6.2831853f;

// Signed shortest step from a to b on the unit circle, in [-0.5, 0.5).
inline float wrap_diff(float a, float b) {
  const float d = b - a;
  return d - floorf(d + 0.5f);
}

inline float wrap01(float x) { return x - floorf(x); }

// The law's pull on x from the others: sum of sin 2pi(x_j - x).
inline float pull(float x, const float* others, int n) {
  float p = 0;
  for (int j = 0; j < n; ++j) p += sinf(kTau * wrap_diff(x, others[j]));
  return p;
}

// Order R: 1 when all agree, 0 for three evenly spaced.
inline float order(const float* x, int n) {
  if (n <= 0) return 0;
  float c = 0, s = 0;
  for (int i = 0; i < n; ++i) {
    c += cosf(kTau * x[i]);
    s += sinf(kTau * x[i]);
  }
  return sqrtf(c * c + s * s) / n;
}

// Largest shortest distance between any two (0..0.5 of a turn).
inline float spread(const float* x, int n) {
  float worst = 0;
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) worst = fmaxf(worst, fabsf(wrap_diff(x[i], x[j])));
  }
  return worst;
}

// Smallest gap between neighbors around the circle; a third of a turn when three are evenly
// spaced, 0 when two coincide.
inline float min_gap(const float* x, int n) {
  if (n < 2) return 0;
  float best = 1;
  for (int i = 0; i < n; ++i) {
    float next = 1;  // distance forward to the nearest other
    for (int j = 0; j < n; ++j) {
      if (j != i) next = fminf(next, wrap01(x[j] - x[i]));
    }
    best = fminf(best, next);
  }
  return best;
}

// A flash oscillator: phase 0..1 at its own natural rate, flashing on the wrap.
struct Osc {
  float phase = 0;
  float period_s = 1;
  float rate = 1;  // cycles per second after the last step, shared with the others

  // Advances dt_s under coupling k toward/away from the others' phases; true if it flashed.
  bool step(float dt_s, float k, const float* others, int n) {
    const float natural = 1.0f / period_s;
    rate = fmaxf(natural + k * pull(phase, others, n), 0.2f * natural);  // never stalls
    phase += rate * dt_s;
    if (phase < 1) return false;
    phase = wrap01(phase);
    return true;
  }

  // Flashes now, as when a clap is heard.
  void fire() { phase = 0; }

  // Seconds since the last flash.
  float since_s() const { return phase / (rate > 0 ? rate : 1.0f / period_s); }
};

}  // namespace trio
