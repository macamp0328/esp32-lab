#pragma once
// Falling sand. A spout on the edge opposite gravity pours colored grains; each step every grain
// moves one cell along gravity (quantized to 8 directions), sliding diagonally when blocked. Once
// the grid is nearly full the far edge opens and the grains drain out, then pouring restarts.

#include <math.h>

#include "lab_sim_base.h"

namespace lab_sim {

class Sand {
 public:
  static constexpr float kFullShare = 0.8f;  // start draining at this share of cells

  void reset(uint32_t seed, const lab::Layout& layout) {
    grid_ = Grid::of(layout);
    rng_ = Rng(seed);
    memset(cells_, 0, sizeof cells_);
    count_ = 0;
    draining_ = false;
    spout_ = 0.5f;
    hue_ = static_cast<uint8_t>(rng_.next());
    tick_ = 0;
  }

  void step(const Input& in) {
    if (grid_.cells() == 0) return;
    int dx, dy;
    direction(in, &dx, &dy);
    fall(dx, dy);
    if (draining_) {
      drain(dx, dy);
      if (count_ == 0) draining_ = false;
    } else if (pour && (tick_++ & 1) == 0) {
      spawn(dx, dy);
      if (count_ >= kFullShare * grid_.cells()) draining_ = true;
    }
  }

  void draw(lab::Pixel* px, const lab::Layout& layout) const {
    paint(px, layout, grid_, [this](int x, int y) {
      const uint8_t c = cells_[grid_.at(x, y)];
      return c ? lab::hsv(static_cast<uint8_t>(c - 1), 210, 255) : lab::Pixel(0, 0, 0);
    });
  }

  // Gravity quantized to one of 8 directions; nearly no tilt means straight down.
  static void direction(const Input& in, int* dx, int* dy) {
    if (in.gx * in.gx + in.gy * in.gy < 0.04f) {
      *dx = 0;
      *dy = 1;
      return;
    }
    const float a = atan2f(in.gy, in.gx);  // 0 = +x
    const int k = static_cast<int>(floorf(a / 0.7853982f + 0.5f)) & 7;
    static const int kDx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int kDy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    *dx = kDx[k];
    *dy = kDy[k];
  }

  bool pour = true;  // tests switch the spout off
  uint8_t cell(int x, int y) const { return cells_[grid_.at(x, y)]; }
  void add(int x, int y, uint8_t hue) {
    if (!cells_[grid_.at(x, y)]) ++count_;
    cells_[grid_.at(x, y)] = static_cast<uint8_t>(hue < 255 ? hue + 1 : 255);
  }
  uint16_t count() const { return count_; }
  bool draining() const { return draining_; }

 private:
  bool free_at(int x, int y) const { return grid_.inside(x, y) && !cells_[grid_.at(x, y)]; }

  // Grains nearest the downhill corner move first, so each moves at most one cell per step.
  void fall(int dx, int dy) {
    const int x0 = dx > 0 ? grid_.w - 1 : 0, xs = dx > 0 ? -1 : 1;
    const int y0 = dy > 0 ? grid_.h - 1 : 0, ys = dy > 0 ? -1 : 1;
    // (dx, dy) turned 45 degrees each way on the 8-direction compass, for sliding.
    auto sgn = [](int v) { return (v > 0) - (v < 0); };
    const int ax = sgn(dx - dy), ay = sgn(dx + dy);
    const int bx = sgn(dx + dy), by = sgn(dy - dx);
    for (int j = 0, y = y0; j < grid_.h; ++j, y += ys) {
      for (int i = 0, x = x0; i < grid_.w; ++i, x += xs) {
        const uint16_t c = grid_.at(x, y);
        if (!cells_[c]) continue;
        int tx = x + dx, ty = y + dy;
        if (!free_at(tx, ty)) {
          const bool a_first = rng_.next() & 1;
          const int fx = x + (a_first ? ax : bx), fy = y + (a_first ? ay : by);
          const int sx = x + (a_first ? bx : ax), sy = y + (a_first ? by : ay);
          if (free_at(fx, fy)) {
            tx = fx;
            ty = fy;
          } else if (free_at(sx, sy)) {
            tx = sx;
            ty = sy;
          } else {
            continue;
          }
        }
        cells_[grid_.at(tx, ty)] = cells_[c];
        cells_[c] = 0;
      }
    }
  }

  void spawn(int dx, int dy) {
    spout_ += 0.08f * (rng_.unit() - 0.5f);
    spout_ = spout_ < 0.1f ? 0.1f : (spout_ > 0.9f ? 0.9f : spout_);
    hue_ = static_cast<uint8_t>(hue_ + 1);
    int x, y;
    if (dy != 0) {
      y = dy > 0 ? 0 : grid_.h - 1;
      x = static_cast<int>(spout_ * grid_.w);
    } else {
      x = dx > 0 ? 0 : grid_.w - 1;
      y = static_cast<int>(spout_ * grid_.h);
    }
    if (free_at(x, y)) add(x, y, hue_);
  }

  // The downhill edge is open: grains on it fall out.
  void drain(int dx, int dy) {
    for (int y = 0; y < grid_.h; ++y) {
      for (int x = 0; x < grid_.w; ++x) {
        if (!cells_[grid_.at(x, y)] || grid_.inside(x + dx, y + dy)) continue;
        cells_[grid_.at(x, y)] = 0;
        --count_;
      }
    }
  }

  Grid grid_;
  Rng rng_;
  uint8_t cells_[kMaxCells] = {};  // 0 empty, else 1 + hue
  uint16_t count_ = 0;
  bool draining_ = false;
  float spout_ = 0.5f;
  uint8_t hue_ = 0;
  uint32_t tick_ = 0;
};

}  // namespace lab_sim
