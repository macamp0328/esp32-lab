#pragma once
// Two Langton's ants on a torus, each painting its own color. On an empty cell an ant turns right
// and paints it; on a painted cell it turns left and clears it; then it steps forward.

#include "lab_sim_base.h"

namespace lab_sim {

class Ant {
 public:
  static constexpr uint16_t kSteps = 600;  // then the board starts over
  static constexpr uint8_t kAnts = 2;
  struct Walker {
    int x;
    int y;
    uint8_t dir;  // 0 up, 1 right, 2 down, 3 left
  };

  void reset(uint32_t seed, const lab::Layout& layout) {
    grid_ = Grid::of(layout);
    rng_ = Rng(seed);
    start();
  }

  void step(const Input&) {
    if (grid_.cells() == 0) return;
    if (steps_ >= kSteps) {
      start();
      return;
    }
    static const int kDx[] = {0, 1, 0, -1};
    static const int kDy[] = {-1, 0, 1, 0};
    for (uint8_t k = 0; k < kAnts; ++k) {
      Walker& a = ants_[k];
      uint8_t& c = cells_[grid_.at(a.x, a.y)];
      a.dir = static_cast<uint8_t>((a.dir + (c ? 3 : 1)) & 3);
      c = c ? 0 : static_cast<uint8_t>(k + 1);
      a.x = (a.x + kDx[a.dir] + grid_.w) % grid_.w;
      a.y = (a.y + kDy[a.dir] + grid_.h) % grid_.h;
    }
    ++steps_;
  }

  void draw(lab::Pixel* px, const lab::Layout& layout) const {
    static const lab::Pixel kPaint[] = {lab::Pixel(0, 0, 0), lab::Pixel(255, 110, 0),
                                        lab::Pixel(0, 150, 255)};
    paint(px, layout, grid_, [this](int x, int y) {
      for (const Walker& a : ants_) {
        if (a.x == x && a.y == y) return lab::Pixel(255, 255, 255);
      }
      return kPaint[cells_[grid_.at(x, y)]];
    });
  }

  uint8_t cell(int x, int y) const { return cells_[grid_.at(x, y)]; }
  const Walker& ant(uint8_t k) const { return ants_[k]; }

 private:
  void start() {
    memset(cells_, 0, sizeof cells_);
    for (uint8_t k = 0; k < kAnts; ++k) {
      ants_[k] = {grid_.w * (k + 1) / (kAnts + 1), grid_.h / 2,
                  static_cast<uint8_t>(rng_.below(4))};
    }
    steps_ = 0;
  }

  Grid grid_;
  Rng rng_;
  uint8_t cells_[kMaxCells] = {};  // 0 empty, else 1 + the ant that painted it
  Walker ants_[kAnts] = {};
  uint16_t steps_ = 0;
};

}  // namespace lab_sim
