#pragma once
// The space between the boards: how disturbed each radio link is right now. A hand or a body in
// the gap shadows and scatters the signal, so the RSSI leaves its slow baseline and jitters. Each
// link also learns its own calm (a minute's average of that disturbance), so a room that stays
// busy is slowly forgiven. Pure; host-tested in test/test_trio.

#include <math.h>
#include <stdint.h>

#include "lab_fx_math.h"

namespace trio {

// One board hearing one other: RSSI per packet, in dBm.
class Link {
 public:
  static constexpr float kFastS = 0.3f;    // now
  static constexpr float kSlowS = 20.0f;   // the link's baseline
  static constexpr float kJitS = 0.5f;     // packet-to-packet scatter
  static constexpr float kCalmS = 60.0f;   // what disturbance is usual here
  static constexpr uint32_t kWarmup = 40;  // packets (2 s at 20 Hz) before it reports anything

  void push(float rssi, float dt_s) {
    if (n_++ == 0) {
      fast_ = slow_ = rssi;
      return;
    }
    jit_ += mix(dt_s, kJitS) * (fabsf(rssi - fast_) - jit_);
    fast_ += mix(dt_s, kFastS) * (rssi - fast_);
    slow_ += mix(dt_s, kSlowS) * (rssi - slow_);
    if (n_ == kWarmup) {
      calm_ = dev();
    } else if (n_ > kWarmup) {
      calm_ += mix(dt_s, kCalmS) * (dev() - calm_);
    }
  }

  // dB away from calm: the baseline step plus the scatter.
  float dev() const { return fabsf(fast_ - slow_) + jit_; }

  // 0..1: dev above what is usual for this link, over span_db.
  float stir(float floor_db, float span_db) const {
    if (n_ < kWarmup) return 0;
    return lab::clamp01((dev() - calm_ - floor_db) / span_db);
  }

  float fast() const { return fast_; }
  float slow() const { return slow_; }
  float jit() const { return jit_; }
  float calm() const { return calm_; }
  uint32_t packets() const { return n_; }

 private:
  static float mix(float dt_s, float tau_s) { return 1 - expf(-dt_s / tau_s); }

  float fast_ = 0, slow_ = 0, jit_ = 0, calm_ = 0;
  uint32_t n_ = 0;
};

// Shown values rise fast and fall slowly, so a passing hand leaves a short trail.
inline float follow(float shown, float target, float dt_s, float rise_s, float fall_s) {
  const float tau = target > shown ? rise_s : fall_s;
  return shown + (target - shown) * (1 - expf(-dt_s / tau));
}

// Edge index for two corners: 0 m-a, 1 a-b, 2 b-m (draw.h's kEdge).
inline int edge_of(int i, int j) {
  if (i > j) {
    const int t = i;
    i = j;
    j = t;
  }
  if (i == 0) return j == 1 ? 0 : 2;
  return 1;
}

}  // namespace trio
