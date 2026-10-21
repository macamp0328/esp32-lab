#pragma once
// Shared pieces for stateful simulations: gravity input, a seeded RNG and the cell grid sized from
// a Layout. Each simulation is a struct the caller owns (no globals, no heap) with reset(), step()
// and draw(); host-tested in shared/tests/test/test_sim. Time-only effects stay in lab_fx.

#include <stdint.h>
#include <string.h>

#include "lab_fx_math.h"
#include "lab_layout.h"
#include "lab_pixel.h"

namespace lab_sim {

constexpr uint8_t kMaxW = 16;
constexpr uint8_t kMaxH = 16;
constexpr uint16_t kMaxCells = kMaxW * kMaxH;

// Gravity in matrix space: +x toward higher columns, +y toward higher rows (down). Default: down.
struct Input {
  float gx = 0;
  float gy = 1;
};

// xorshift32; the seed goes through lab::hash32 so neighbouring seeds diverge.
class Rng {
 public:
  explicit Rng(uint32_t seed = 1) : s_(lab::hash32(seed) | 1) {}
  uint32_t next() {
    s_ ^= s_ << 13;
    s_ ^= s_ >> 17;
    s_ ^= s_ << 5;
    return s_;
  }
  uint32_t below(uint32_t n) { return n ? next() % n : 0; }
  float unit() { return (next() >> 8) * (1.0f / 16777216.0f); }

 private:
  uint32_t s_;
};

// The simulation grid: the layout's columns x rows, clamped to kMaxW x kMaxH.
struct Grid {
  uint8_t w = 0;
  uint8_t h = 0;
  static Grid of(const lab::Layout& l) {
    Grid g;
    g.w = static_cast<uint8_t>(l.width() < kMaxW ? l.width() : kMaxW);
    g.h = static_cast<uint8_t>(l.height() < kMaxH ? l.height() : kMaxH);
    return g;
  }
  uint16_t cells() const { return static_cast<uint16_t>(w * h); }
  bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
  uint16_t at(int x, int y) const { return static_cast<uint16_t>(y * w + x); }
};

// Writes every LED of `layout`: color(x, y) inside the grid, black outside it.
template <typename ColorFn>
void paint(lab::Pixel* px, const lab::Layout& layout, const Grid& g, ColorFn color) {
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const int x = layout.column(i), y = layout.row(i);
    px[i] = g.inside(x, y) ? color(x, y) : lab::Pixel(0, 0, 0);
  }
}

}  // namespace lab_sim
