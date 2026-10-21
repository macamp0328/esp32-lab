#pragma once
// Where each LED sits, so effects are written once for strips, serpentine matrices and rings.
// Index = wiring order. Positions are normalized to 0..1. Pure; host-tested.

#include <math.h>
#include <stddef.h>
#include <stdint.h>

namespace lab {

struct Point {
  float x;
  float y;
};

class Layout {
 public:
  enum class Kind : uint8_t { kStrip, kMatrix, kRings };
  static constexpr uint8_t kMaxRings = 8;

  static Layout strip(uint16_t n) {
    Layout l;
    l.kind_ = Kind::kStrip;
    l.size_ = l.w_ = n;
    l.h_ = 1;
    return l;
  }

  // Row 0 is the first wired row; serpentine rows alternate direction.
  static Layout matrix(uint16_t w, uint16_t h, bool serpentine) {
    Layout l;
    l.kind_ = Kind::kMatrix;
    l.w_ = w;
    l.h_ = h;
    l.size_ = w * h;
    l.serpentine_ = serpentine;
    return l;
  }

  // Concentric rings in wiring order; a ring's radius is proportional to its LED count.
  // Takes the array itself so more than kMaxRings rings is a compile error, not a silent cut.
  template <size_t N>
  static Layout rings(const uint16_t (&sizes)[N]) {
    static_assert(N >= 1 && N <= kMaxRings, "Layout::rings takes 1..kMaxRings rings");
    Layout l;
    l.kind_ = Kind::kRings;
    l.ring_count_ = N;
    for (uint8_t r = 0; r < l.ring_count_; ++r) {
      l.rings_[r] = sizes[r];
      l.size_ += sizes[r];
      if (sizes[r] > l.max_ring_) l.max_ring_ = sizes[r];
    }
    l.w_ = l.size_;
    l.h_ = 1;
    return l;
  }

  Kind kind() const { return kind_; }
  uint16_t size() const { return size_; }
  uint16_t width() const { return w_; }
  uint16_t height() const { return h_; }
  uint8_t ring_count() const { return ring_count_; }

  // Strip/matrix: LED at column x, row y; size() when out of range or on rings.
  uint16_t index(uint16_t x, uint16_t y) const {
    if (kind_ == Kind::kRings || x >= w_ || y >= h_) return size_;
    const uint16_t col = (serpentine_ && (y & 1)) ? w_ - 1 - x : x;
    return y * w_ + col;
  }

  // Empty layouts (0 LEDs) answer 0 everywhere instead of dividing by zero.
  uint16_t row(uint16_t i) const { return (kind_ == Kind::kRings || w_ == 0) ? 0 : i / w_; }
  uint16_t column(uint16_t i) const {
    if (kind_ == Kind::kRings || w_ == 0) return kind_ == Kind::kRings ? i : 0;
    const uint16_t c = i % w_;
    return (serpentine_ && (row(i) & 1)) ? w_ - 1 - c : c;
  }

  // Rings: which ring (0 = first wired); strip/matrix: 0.
  uint8_t ring(uint16_t i) const {
    if (kind_ != Kind::kRings || ring_count_ == 0) return 0;
    for (uint8_t r = 0; r < ring_count_; ++r) {
      if (i < rings_[r]) return r;
      i -= rings_[r];
    }
    return ring_count_ - 1;
  }

  // Rings: 0..1 around the LED's ring from its first LED; strip/matrix: normalized x.
  float angle(uint16_t i) const {
    if (kind_ != Kind::kRings) return position(i).x;
    for (uint8_t r = 0; r < ring_count_; ++r) {
      if (i < rings_[r]) return static_cast<float>(i) / rings_[r];
      i -= rings_[r];
    }
    return 0;
  }

  Point position(uint16_t i) const {
    if (size_ == 0) return {0.5f, 0.5f};
    if (kind_ == Kind::kRings) {
      const float radius = 0.5f * rings_[ring(i)] / max_ring_;
      const float a = 6.2831853f * angle(i);
      return {0.5f + radius * cosf(a), 0.5f + radius * sinf(a)};
    }
    return {(column(i) + 0.5f) / w_, (row(i) + 0.5f) / h_};
  }

 private:
  Kind kind_ = Kind::kStrip;
  uint16_t size_ = 0;
  uint16_t w_ = 0;
  uint16_t h_ = 0;
  bool serpentine_ = false;
  uint8_t ring_count_ = 0;
  uint16_t rings_[kMaxRings] = {};
  uint16_t max_ring_ = 0;
};

}  // namespace lab
