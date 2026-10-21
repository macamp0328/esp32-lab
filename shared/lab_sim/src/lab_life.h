#pragma once
// Conway's Game of Life on a torus, cells colored by age. A board that dies out or starts
// repeating lingers for kLinger generations, then reseeds itself.

#include "lab_sim_base.h"

namespace lab_sim {

// One generation on a torus. `in`/`out` hold cell ages (0 = dead): survivors age, births are 1.
inline void life_next(const uint8_t* in, uint8_t* out, const Grid& g) {
  for (int y = 0; y < g.h; ++y) {
    for (int x = 0; x < g.w; ++x) {
      int n = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0) continue;
          n += in[g.at((x + dx + g.w) % g.w, (y + dy + g.h) % g.h)] > 0;
        }
      }
      const uint8_t age = in[g.at(x, y)];
      if (age) {
        out[g.at(x, y)] = (n == 2 || n == 3) ? static_cast<uint8_t>(age < 255 ? age + 1 : 255) : 0;
      } else {
        out[g.at(x, y)] = n == 3 ? 1 : 0;
      }
    }
  }
}

class Life {
 public:
  static constexpr uint8_t kLinger = 12;  // generations a dead or repeating board stays up
  static constexpr uint8_t kHistory = 16;

  void reset(uint32_t seed, const lab::Layout& layout) {
    grid_ = Grid::of(layout);
    rng_ = Rng(seed);
    reseeds_ = 0;
    reseed();
  }

  void step(const Input&) {
    if (grid_.cells() == 0) return;
    if (stale_ >= kLinger) {
      reseed();
      ++reseeds_;
      return;
    }
    uint8_t next[kMaxCells];
    life_next(age_, next, grid_);
    memcpy(age_, next, grid_.cells());
    ++generation_;
    const uint32_t print = fingerprint();
    bool repeat = false;
    for (uint8_t k = 0; k < kHistory; ++k) repeat = repeat || history_[k] == print;
    history_[generation_ % kHistory] = print;
    if (repeat || population() == 0) ++stale_;
  }

  void draw(lab::Pixel* px, const lab::Layout& layout) const {
    paint(px, layout, grid_, [this](int x, int y) {
      const uint8_t a = age_[grid_.at(x, y)];
      if (a == 0) return lab::Pixel(0, 0, 0);
      if (a == 1) return lab::Pixel(170, 255, 220);                                  // newborn
      return lab::hsv(static_cast<uint8_t>(120 + (a < 24 ? a : 24) * 4), 255, 230);  // older: bluer
    });
  }

  bool alive(int x, int y) const { return age_[grid_.at(x, y)] > 0; }
  void set(int x, int y, bool on) { age_[grid_.at(x, y)] = on ? 1 : 0; }
  void clear() {
    memset(age_, 0, sizeof age_);
    restart_history();
  }
  uint16_t population() const {
    uint16_t n = 0;
    for (uint16_t i = 0; i < grid_.cells(); ++i) n += age_[i] > 0;
    return n;
  }
  uint32_t generation() const { return generation_; }
  uint32_t reseeds() const { return reseeds_; }

 private:
  void reseed() {
    for (uint16_t i = 0; i < grid_.cells(); ++i) age_[i] = rng_.unit() < 0.35f ? 1 : 0;
    restart_history();
  }
  void restart_history() {
    memset(history_, 0, sizeof history_);
    generation_ = 0;
    stale_ = 0;
  }
  uint32_t fingerprint() const {
    uint32_t h = 0x9e3779b9U;
    for (uint16_t i = 0; i < grid_.cells(); ++i) h = lab::hash32(h ^ (age_[i] > 0 ? i + 1 : 0));
    return h | 1;  // never 0, the empty history slot
  }

  Grid grid_;
  Rng rng_;
  uint8_t age_[kMaxCells] = {};
  uint32_t history_[kHistory] = {};
  uint32_t generation_ = 0;
  uint32_t reseeds_ = 0;
  uint8_t stale_ = 0;
};

}  // namespace lab_sim
