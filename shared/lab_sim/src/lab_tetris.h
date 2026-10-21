#pragma once
// Tetris playing itself in a well the size of the layout. Each new piece gets the rotation and
// column with the best score (lines cleared, holes, stack height, bumpiness), slides there while
// falling, and locks; full rows flash, then clear. A topped-out well starts over.

#include "lab_sim_base.h"

namespace lab_sim {

// 4x4 masks per piece and rotation: bit (15 - 4 * row - col). Order: I O T S Z J L.
constexpr uint16_t kTetromino[7][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, {0x6600, 0x6600, 0x6600, 0x6600},
    {0x4E00, 0x4640, 0x0E40, 0x4C40}, {0x6C00, 0x4620, 0x06C0, 0x8C40},
    {0xC600, 0x2640, 0x0C60, 0x4C80}, {0x8E00, 0x6440, 0x0E20, 0x44C0},
    {0x2E00, 0x4460, 0x0E80, 0xC440},
};

// Removes full rows from a well (0 = empty), shifting the rest down; returns how many.
inline uint8_t tetris_collapse(uint8_t* well, const Grid& g) {
  uint8_t cleared = 0;
  int dst = g.h - 1;
  for (int y = g.h - 1; y >= 0; --y) {
    bool full = true;
    for (int x = 0; x < g.w; ++x) full = full && well[g.at(x, y)];
    if (full) {
      ++cleared;
      continue;
    }
    if (dst != y) memcpy(&well[g.at(0, dst)], &well[g.at(0, y)], g.w);
    --dst;
  }
  for (; dst >= 0; --dst) memset(&well[g.at(0, dst)], 0, g.w);
  return cleared;
}

class Tetris {
 public:
  static constexpr uint8_t kFlashSteps = 4;

  void reset(uint32_t seed, const lab::Layout& layout) {
    grid_ = Grid::of(layout);
    rng_ = Rng(seed);
    lines_ = 0;
    resets_ = 0;
    start();
  }

  void step(const Input&) {
    if (grid_.w < 4 || grid_.h < 4) return;
    if (flash_ > 0) {
      if (--flash_ == 0) {
        lines_ += tetris_collapse(well_, grid_);
        flash_rows_ = 0;
      }
      return;
    }
    if (!active_) {
      spawn();
      return;
    }
    if (x_ != target_x_) {
      const int nx = x_ + (target_x_ > x_ ? 1 : -1);
      if (fits(type_, rot_, nx, y_)) x_ = nx;
    }
    if (fits(type_, rot_, x_, y_ + 1)) {
      ++y_;
    } else {
      lock();
    }
  }

  void draw(lab::Pixel* px, const lab::Layout& layout) const {
    static const lab::Pixel kColors[8] = {lab::Pixel(0, 0, 0),     lab::Pixel(0, 220, 255),
                                          lab::Pixel(255, 220, 0), lab::Pixel(170, 0, 255),
                                          lab::Pixel(0, 230, 60),  lab::Pixel(255, 30, 30),
                                          lab::Pixel(30, 60, 255), lab::Pixel(255, 120, 0)};
    paint(px, layout, grid_, [this](int x, int y) {
      if (flash_rows_ & (1u << y)) {
        return (flash_ & 1) ? lab::Pixel(255, 255, 255) : kColors[well_[grid_.at(x, y)]];
      }
      if (active_ && covers(type_, rot_, x_, y_, x, y)) return kColors[type_ + 1];
      return kColors[well_[grid_.at(x, y)]];
    });
  }

  uint8_t cell(int x, int y) const { return well_[grid_.at(x, y)]; }
  void set_cell(int x, int y, uint8_t v) { well_[grid_.at(x, y)] = v; }
  bool active() const { return active_; }
  bool piece_covers(int x, int y) const { return active_ && covers(type_, rot_, x_, y_, x, y); }
  uint32_t lines() const { return lines_; }
  uint32_t resets() const { return resets_; }

 private:
  static bool covers(uint8_t type, uint8_t rot, int px, int py, int x, int y) {
    const int r = y - py, c = x - px;
    if (r < 0 || r > 3 || c < 0 || c > 3) return false;
    return (kTetromino[type][rot] >> (15 - 4 * r - c)) & 1;
  }

  // Cells above the well (y < 0) are allowed; walls, floor and the stack are not.
  bool fits(uint8_t type, uint8_t rot, int px, int py, const uint8_t* well = nullptr) const {
    if (!well) well = well_;
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (!((kTetromino[type][rot] >> (15 - 4 * r - c)) & 1)) continue;
        const int x = px + c, y = py + r;
        if (x < 0 || x >= grid_.w || y >= grid_.h) return false;
        if (y >= 0 && well[grid_.at(x, y)]) return false;
      }
    }
    return true;
  }

  static int top_row(uint8_t type, uint8_t rot) {
    for (int r = 0; r < 4; ++r) {
      if ((kTetromino[type][rot] >> (12 - 4 * r)) & 0xF) return r;
    }
    return 0;
  }

  // Score of the well after dropping the piece at (rot, px); -1e9 when it does not fit.
  float score(uint8_t type, uint8_t rot, int px) const {
    int py = -top_row(type, rot);
    if (!fits(type, rot, px, py)) return -1e9f;
    while (fits(type, rot, px, py + 1)) ++py;
    uint8_t well[kMaxCells];
    memcpy(well, well_, grid_.cells());
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (!covers(type, rot, px, py, px + c, py + r)) continue;
        if (py + r < 0) return -1e9f;  // would lock above the well
        well[grid_.at(px + c, py + r)] = 1;
      }
    }
    const uint8_t lines = tetris_collapse(well, grid_);
    int height = 0, holes = 0, bump = 0, prev = -1;
    for (int x = 0; x < grid_.w; ++x) {
      int top = grid_.h;
      for (int y = 0; y < grid_.h; ++y) {
        if (well[grid_.at(x, y)]) {
          if (top == grid_.h) top = y;
        } else if (top < y) {
          ++holes;
        }
      }
      const int h = grid_.h - top;
      height += h;
      if (prev >= 0) bump += h > prev ? h - prev : prev - h;
      prev = h;
    }
    return 0.76f * lines - 0.51f * height - 0.36f * holes - 0.18f * bump;
  }

  void spawn() {
    type_ = static_cast<uint8_t>(rng_.below(7));
    float best = -1e9f;
    target_x_ = (grid_.w - 4) / 2;
    rot_ = 0;
    for (uint8_t rot = 0; rot < 4; ++rot) {
      for (int px = -3; px < grid_.w; ++px) {
        const float s = score(type_, rot, px);
        if (s > best) {
          best = s;
          rot_ = rot;
          target_x_ = px;
        }
      }
    }
    x_ = (grid_.w - 4) / 2;
    y_ = -top_row(type_, rot_);
    if (!fits(type_, rot_, x_, y_)) x_ = target_x_;
    if (best <= -1e9f || !fits(type_, rot_, x_, y_)) {
      ++resets_;
      start();
      return;
    }
    active_ = true;
  }

  void lock() {
    active_ = false;
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (!covers(type_, rot_, x_, y_, x_ + c, y_ + r)) continue;
        if (y_ + r < 0) {  // topped out
          ++resets_;
          start();
          return;
        }
        well_[grid_.at(x_ + c, y_ + r)] = static_cast<uint8_t>(type_ + 1);
      }
    }
    for (int y = 0; y < grid_.h; ++y) {
      bool full = true;
      for (int x = 0; x < grid_.w; ++x) full = full && well_[grid_.at(x, y)];
      if (full) flash_rows_ |= 1u << y;
    }
    if (flash_rows_) flash_ = kFlashSteps;
  }

  void start() {
    memset(well_, 0, sizeof well_);
    active_ = false;
    flash_ = 0;
    flash_rows_ = 0;
  }

  Grid grid_;
  Rng rng_;
  uint8_t well_[kMaxCells] = {};  // 0 empty, else 1 + piece type
  uint8_t type_ = 0;
  uint8_t rot_ = 0;
  int x_ = 0;
  int y_ = 0;
  int target_x_ = 0;
  bool active_ = false;
  uint8_t flash_ = 0;
  uint32_t flash_rows_ = 0;
  uint32_t lines_ = 0;
  uint32_t resets_ = 0;
};

}  // namespace lab_sim
