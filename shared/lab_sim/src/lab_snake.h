#pragma once
// Snake playing itself inside the walls. Each step it takes the shortest path to the apple, but
// only if, having eaten, its head could still reach its tail; otherwise it follows its tail the
// long way round, which sweeps the board until the apple is safe. It starts over when it is boxed
// in, fills the board, or goes kStallPerCell steps per cell without eating.

#include "lab_sim_base.h"

namespace lab_sim {

class Snake {
 public:
  static constexpr uint16_t kStallPerCell = 4;

  void reset(uint32_t seed, const lab::Layout& layout) {
    grid_ = Grid::of(layout);
    rng_ = Rng(seed);
    restarts_ = 0;
    eaten_ = 0;
    start();
  }

  void step(const Input&) {
    if (grid_.w < 3 || grid_.h < 2) return;
    const int next = apple_ < 0 ? -1 : choose();  // no apple: the board is full, a win
    if (next < 0 || ++hungry_ > kStallPerCell * grid_.cells()) {
      ++restarts_;
      start();
      return;
    }
    const bool eat = next == apple_;
    const uint16_t len = eat ? len_ + 1 : len_;
    if (!eat) occ_[body_[len_ - 1]] = 0;
    for (uint16_t k = len - 1; k > 0; --k) body_[k] = body_[k - 1];
    body_[0] = static_cast<uint16_t>(next);
    occ_[next] = 1;
    len_ = len;
    if (eat) {
      ++eaten_;
      hungry_ = 0;
      place_apple();
    }
  }

  void draw(lab::Pixel* px, const lab::Layout& layout) const {
    paint(px, layout, grid_, [this](int x, int y) {
      const uint16_t c = grid_.at(x, y);
      if (c == apple_) return lab::Pixel(255, 20, 20);
      if (!occ_[c]) return lab::Pixel(0, 0, 0);
      if (c == body_[0]) return lab::Pixel(200, 255, 120);
      uint16_t k = 1;
      while (k < len_ && body_[k] != c) ++k;
      return lab::hsv(static_cast<uint8_t>(96 - 40 * k / len_), 255,
                      static_cast<uint8_t>(230 - 150 * k / len_));
    });
  }

  uint16_t length() const { return len_; }
  uint16_t segment(uint16_t k) const { return body_[k]; }
  int apple() const { return apple_; }
  uint32_t restarts() const { return restarts_; }
  uint32_t eaten() const { return eaten_; }

 private:
  // Shortest path over free cells from `from` to `to` (`to` may be occupied); fills prev.
  bool bfs(const uint8_t* occ, uint16_t from, uint16_t to, int16_t* prev) const {
    static const int kDx[] = {1, 0, -1, 0};
    static const int kDy[] = {0, 1, 0, -1};
    for (uint16_t i = 0; i < grid_.cells(); ++i) prev[i] = -1;
    uint16_t queue[kMaxCells];
    uint16_t head = 0, tail = 0;
    queue[tail++] = from;
    prev[from] = static_cast<int16_t>(from);
    while (head < tail) {
      const uint16_t c = queue[head++];
      if (c == to) return true;
      const int x = c % grid_.w, y = c / grid_.w;
      for (int d = 0; d < 4; ++d) {
        const int nx = x + kDx[d], ny = y + kDy[d];
        if (!grid_.inside(nx, ny)) continue;
        const uint16_t n = grid_.at(nx, ny);
        if (prev[n] != -1 || (occ[n] && n != to)) continue;
        prev[n] = static_cast<int16_t>(c);
        queue[tail++] = n;
      }
    }
    return false;
  }

  // The cell after `from` on the path bfs() found to `to`.
  static uint16_t first_step(const int16_t* prev, uint16_t from, uint16_t to) {
    uint16_t c = to;
    while (static_cast<uint16_t>(prev[c]) != from) c = static_cast<uint16_t>(prev[c]);
    return c;
  }

  int choose() {
    int16_t prev[kMaxCells];
    const uint16_t head = body_[0], tail = body_[len_ - 1];
    uint8_t occ[kMaxCells];
    memcpy(occ, occ_, grid_.cells());
    occ[tail] = 0;  // the tail moves away unless we eat
    if (bfs(occ, head, static_cast<uint16_t>(apple_), prev) && safe_after(prev)) {
      return first_step(prev, head, static_cast<uint16_t>(apple_));
    }
    // No safe meal: of the moves that keep the tail in reach, take the one with the longest way
    // back to it (ties broken at random, so the snake never settles into a loop).
    static const int kDx[] = {1, 0, -1, 0};
    static const int kDy[] = {0, 1, 0, -1};
    const uint32_t spin = rng_.next();
    int best = -1, best_len = -1, any = -1;
    for (uint32_t j = 0; j < 4; ++j) {
      const int d = static_cast<int>((j + spin) & 3);
      const int nx = head % grid_.w + kDx[d], ny = head / grid_.w + kDy[d];
      if (!grid_.inside(nx, ny) || occ[grid_.at(nx, ny)]) continue;
      const uint16_t n = grid_.at(nx, ny);
      if (any < 0) any = n;
      if (n == apple_) continue;
      uint8_t moved[kMaxCells];
      memcpy(moved, occ, grid_.cells());
      moved[n] = 1;
      const uint16_t new_tail = body_[len_ - 2];
      if (!bfs(moved, n, new_tail, prev)) continue;
      int steps = 0;
      for (uint16_t c = new_tail; c != n; c = static_cast<uint16_t>(prev[c])) ++steps;
      if (steps > best_len) {
        best_len = steps;
        best = n;
      }
    }
    return best >= 0 ? best : any;
  }

  // Follows the path to the apple on a copy of the snake, then asks: can the head reach the tail?
  bool safe_after(const int16_t* prev) const {
    uint16_t path[kMaxCells];
    uint16_t steps = 0;
    for (uint16_t c = static_cast<uint16_t>(apple_); c != body_[0];
         c = static_cast<uint16_t>(prev[c])) {
      path[steps++] = c;
    }
    uint16_t body[kMaxCells];
    uint8_t occ[kMaxCells];
    memcpy(body, body_, len_ * sizeof body[0]);
    memcpy(occ, occ_, grid_.cells());
    uint16_t len = len_;
    for (int s = steps - 1; s >= 0; --s) {
      const bool eat = s == 0;
      if (!eat) occ[body[len - 1]] = 0;
      if (eat) ++len;
      for (uint16_t k = len - 1; k > 0; --k) body[k] = body[k - 1];
      body[0] = path[s];
      occ[path[s]] = 1;
    }
    if (len >= grid_.cells()) return true;  // it fills the board: a win
    int16_t scratch[kMaxCells];
    return bfs(occ, body[0], body[len - 1], scratch);
  }

  void start() {
    memset(occ_, 0, sizeof occ_);
    hungry_ = 0;
    len_ = 3;
    for (uint16_t k = 0; k < len_; ++k) {
      body_[k] = grid_.at(grid_.w / 2 - static_cast<int>(k) + 1, grid_.h / 2);
      occ_[body_[k]] = 1;
    }
    place_apple();
  }

  void place_apple() {
    const uint16_t free_cells = static_cast<uint16_t>(grid_.cells() - len_);
    if (free_cells == 0) {
      apple_ = -1;
      return;
    }
    uint16_t pick = static_cast<uint16_t>(rng_.below(free_cells));
    for (uint16_t c = 0; c < grid_.cells(); ++c) {
      if (occ_[c]) continue;
      if (pick-- == 0) {
        apple_ = c;
        return;
      }
    }
  }

  Grid grid_;
  Rng rng_;
  uint16_t body_[kMaxCells] = {};  // 0 = head
  uint8_t occ_[kMaxCells] = {};
  uint16_t len_ = 0;
  int apple_ = -1;
  uint32_t restarts_ = 0;
  uint32_t eaten_ = 0;
  uint32_t hungry_ = 0;  // steps since the last apple
};

}  // namespace lab_sim
