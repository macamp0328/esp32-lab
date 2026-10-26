#pragma once
// The memory tempo: every LOOM_THREAD_S the M5 weaves one thread of what that span was, and the
// TFT holds the last 240 side by side (4 h at a minute each), a cursor overwriting the oldest.
// A thread is three bands in the three hues (m, a, b top to bottom). Its borders run straight
// while the beat agrees and zig-zag thread to thread as it does not; each radio edge frays its
// own border (m-a, a-b; b-m frays the thread's two ends); each clap ties a white knot. Pure;
// host-tested.

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "lab_fx_math.h"
#include "lab_pixel.h"
#include "law.h"

namespace trio {

#pragma pack(push, 1)
struct Thread {
  uint8_t hue[3];   // mean hue of m, a, b (0..255 round the wheel)
  uint8_t r;        // mean beat order R, 0..255
  uint8_t edge[3];  // peak disturbance of edges m-a, a-b, b-m, 0..255
  uint8_t claps;
};
#pragma pack(pop)
static_assert(sizeof(Thread) == 8, "a thread is 8 bytes");

// Gathers one thread's worth of the piece.
class Weft {
 public:
  void hues(const float* h, const bool* present) {
    for (int i = 0; i < 3; ++i) {
      if (!present[i]) continue;
      c_[i] += cosf(kTau * h[i]);
      s_[i] += sinf(kTau * h[i]);
      last_[i] = h[i];
    }
  }
  void order(float r) {
    r_ += r;
    ++n_r_;
  }
  void edges(const float* e) {
    for (int i = 0; i < 3; ++i) peak_[i] = fmaxf(peak_[i], e[i]);
  }
  void clap() {
    if (claps_ < 255) ++claps_;
  }

  // The thread, and a fresh start for the next.
  Thread finish() {
    Thread t;
    for (int i = 0; i < 3; ++i) {
      const float h = c_[i] || s_[i] ? wrap01(atan2f(s_[i], c_[i]) / kTau) : last_[i];
      t.hue[i] = static_cast<uint8_t>(h * 256);
      t.edge[i] = static_cast<uint8_t>(lab::clamp01(peak_[i]) * 255 + 0.5f);
    }
    t.r = static_cast<uint8_t>(lab::clamp01(n_r_ ? r_ / n_r_ : 1) * 255 + 0.5f);
    t.claps = claps_;
    const float keep[3] = {last_[0], last_[1], last_[2]};
    *this = Weft{};
    memcpy(last_, keep, sizeof last_);
    return t;
  }

 private:
  float c_[3] = {}, s_[3] = {}, last_[3] = {};
  float r_ = 0;
  uint32_t n_r_ = 0;
  float peak_[3] = {};
  uint8_t claps_ = 0;
};

// The last kMax threads, oldest overwritten first.
class Loom {
 public:
  static constexpr int kMax = 240;

  // Adds t; returns the column it goes in (0..kMax-1).
  int add(const Thread& t) {
    const int col = static_cast<int>(woven_ % kMax);
    ring_[col] = t;
    ++woven_;
    return col;
  }
  const Thread& at(int col) const { return ring_[col]; }
  uint32_t woven() const { return woven_; }
  int columns() const { return woven_ < kMax ? static_cast<int>(woven_) : kMax; }

 private:
  Thread ring_[kMax] = {};
  uint32_t woven_ = 0;
};

// One thread as a column of h pixels; `index` is its place in the weave (it sets the zig-zag
// parity and the fray's grain, so a thread draws the same every time).
inline void thread_column(const Thread& t, uint32_t index, lab::Pixel* out, int h) {
  const float r = t.r / 255.0f;
  const float zig = (index & 1) ? 1.0f : -1.0f;
  const float amp = h / 9.0f * (1 - r);
  const float margin = h * 0.06f;
  const float b1 = margin + (h - 2 * margin) / 3 + zig * amp;      // m | a
  const float b2 = margin + 2 * (h - 2 * margin) / 3 - zig * amp;  // a | b
  const lab::Pixel band[3] = {lab::hsv(t.hue[0], 210, 255), lab::hsv(t.hue[1], 210, 255),
                              lab::hsv(t.hue[2], 210, 255)};
  for (int y = 0; y < h; ++y) {
    const float fy = y + 0.5f;
    int k = fy < b1 ? 0 : (fy < b2 ? 1 : 2);
    // Fray: near a border, by how disturbed its edge was, pixels take the other band's color.
    const float d1 = fabsf(fy - b1), d2 = fabsf(fy - b2);
    const int near = d1 < d2 ? 0 : 1;
    const float dist = near == 0 ? d1 : d2;
    const float stir = t.edge[near] / 255.0f;  // m-a for the first border, a-b for the second
    const float reach = 1 + stir * h * 0.12f;
    const float grain = lab::hash01(index * 2654435761u ^ static_cast<uint32_t>(y) * 40503u);
    if (stir > 0.02f && dist < reach && grain < 0.5f * stir * (1 - dist / reach) + 0.15f * stir) {
      k = near == 0 ? (k == 0 ? 1 : 0) : (k == 1 ? 2 : 1);
    }
    // A twill: alternate rows a little darker, shifted each thread.
    const float shade = ((static_cast<uint32_t>(y) + index) & 3) < 2 ? 1.0f : 0.78f;
    lab::Pixel c = lab::scale(band[k], shade);
    // The thread's ends: dark, sparked by the b-m edge.
    if (fy < margin || fy > h - margin) {
      const float spark = t.edge[2] / 255.0f;
      c = grain < 0.6f * spark ? lab::scale(band[fy < margin ? 0 : 2], 0.7f) : lab::Pixel(0, 0, 0);
    }
    out[y] = c;
  }
  // Knots: up to four, each a short white bead at a place of its own along the thread.
  const int knots = t.claps < 4 ? t.claps : 4;
  for (int i = 0; i < knots; ++i) {
    const int y0 =
        static_cast<int>(margin + lab::hash01(index * 977u + i * 7919u) * (h - 2 * margin));
    for (int y = y0 - 1; y <= y0 + 1; ++y) {
      if (y >= 0 && y < h) out[y] = lab::Pixel(255, 255, 255);
    }
  }
}

// A column sampled down to n (box average) and written as 4 hex digits of RGB565 each: the
// `weave` event, so a capture can be printed (tools/weave.py). out holds 4 * n + 1 chars.
inline void column_hex(const lab::Pixel* px, int h, int n, char* out) {
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < n; ++i) {
    const int a = i * h / n, b = (i + 1) * h / n;
    uint32_t r = 0, g = 0, bl = 0;
    for (int y = a; y < b; ++y) {
      r += px[y].r;
      g += px[y].g;
      bl += px[y].b;
    }
    const uint32_t m = b > a ? b - a : 1;
    const uint16_t v =
        static_cast<uint16_t>(((r / m) >> 3) << 11 | ((g / m) >> 2) << 5 | (bl / m) >> 3);
    for (int k = 0; k < 4; ++k) out[4 * i + k] = kHex[(v >> (12 - 4 * k)) & 0xf];
  }
  out[4 * n] = 0;
}

}  // namespace trio
