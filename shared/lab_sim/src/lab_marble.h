#pragma once
// A marble rolling under gravity with rolling friction, bouncing off the walls and leaving a short
// fading trail. Positions are in cells (cell centers at 0.5, 1.5, ...); one step is 1/60 s.

#include <math.h>

#include "lab_sim_base.h"

namespace lab_sim {

class Marble {
 public:
  static constexpr float kDt = 1.0f / 60;
  static constexpr float kAccel = 40;       // cells/s^2 at 1 g of tilt
  static constexpr float kFriction = 0.9f;  // velocity lost per second, as a fraction
  static constexpr float kBounce = 0.6f;    // speed kept off a wall
  static constexpr uint8_t kTrail = 6;

  void reset(uint32_t seed, const lab::Layout& layout) {
    grid_ = Grid::of(layout);
    rng_ = Rng(seed);
    x_ = grid_.w * 0.5f;
    y_ = grid_.h * 0.5f;
    vx_ = 6 * (rng_.unit() - 0.5f);
    vy_ = 6 * (rng_.unit() - 0.5f);
    for (uint8_t k = 0; k < kTrail; ++k) {
      tx_[k] = x_;
      ty_[k] = y_;
    }
  }

  void step(const Input& in) {
    if (grid_.cells() == 0) return;
    vx_ += in.gx * kAccel * kDt;
    vy_ += in.gy * kAccel * kDt;
    const float keep = 1 - kFriction * kDt;
    vx_ *= keep;
    vy_ *= keep;
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;
    bounce(&x_, &vx_, grid_.w);
    bounce(&y_, &vy_, grid_.h);
    for (uint8_t k = kTrail - 1; k > 0; --k) {
      tx_[k] = tx_[k - 1];
      ty_[k] = ty_[k - 1];
    }
    tx_[0] = x_;
    ty_[0] = y_;
  }

  void draw(lab::Pixel* px, const lab::Layout& layout) const {
    paint(px, layout, grid_, [this](int cx, int cy) {
      const float x = cx + 0.5f, y = cy + 0.5f;
      float v = 0;
      for (uint8_t k = 0; k < kTrail; k += 2) {
        const float d = sqrtf((x - tx_[k]) * (x - tx_[k]) + (y - ty_[k]) * (y - ty_[k]));
        const float vk =
            lab::clamp01(1.25f - d) * (k == 0 ? 1 : 0.35f * (1 - k / static_cast<float>(kTrail)));
        if (vk > v) v = vk;
      }
      return lab::scale(lab::Pixel(255, 220, 120), v);
    });
  }

  float x() const { return x_; }
  float y() const { return y_; }
  float speed() const { return sqrtf(vx_ * vx_ + vy_ * vy_); }
  void place(float x, float y, float vx, float vy) {
    x_ = x;
    y_ = y;
    vx_ = vx;
    vy_ = vy;
  }

 private:
  static void bounce(float* p, float* v, uint8_t size) {
    const float lo = 0.5f, hi = size - 0.5f;
    if (*p < lo) {
      *p = lo + (lo - *p);
      *v = -*v * kBounce;
    } else if (*p > hi) {
      *p = hi - (*p - hi);
      *v = -*v * kBounce;
    }
    if (*p < lo || *p > hi) *p = *p < lo ? lo : hi;  // a size-1 axis, or a huge step
  }

  Grid grid_;
  Rng rng_;
  float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
  float tx_[kTrail] = {};
  float ty_[kTrail] = {};
};

}  // namespace lab_sim
