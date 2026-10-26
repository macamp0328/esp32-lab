#pragma once
// The matrix is the space between the three: each corner flashes in its board's color, every
// flash sends a pulse along both of its edges that reaches the neighbor a third of a beat later
// (in waltz it arrives as the neighbor fires: a baton pass), and each edge glows with the radio
// disturbance of its link. Pure; host-tested.

#include <math.h>

#include "lab_fx.h"
#include "trio.h"

namespace trio {

struct Corner {
  float x, y;
};
// An equilateral triangle on the 8x8 grid (pixel centres run 0.0625..0.9375), m on top.
constexpr Corner kCorner[kSlots] = {{0.5f, 0.18f}, {0.0625f, 0.9375f}, {0.9375f, 0.9375f}};
// Edges: 0 m-a, 1 a-b, 2 b-m.
constexpr uint8_t kEdge[3][2] = {{kM, kA}, {kA, kB}, {kB, kM}};

struct View {
  bool present[kSlots] = {};
  float since_s[kSlots] = {};  // since each one's last flash
  lab::Pixel color[kSlots] = {};
  float stir[3] = {};  // each edge's radio disturbance, 0..1
  float flash_s = 0.14f;
  float travel_s = 0.33f;  // a pulse's time along one edge
};

inline float flash(float since_s, float flash_s) { return expf(-since_s / flash_s); }

inline void draw_space(lab::Pixel* px, const lab::Layout& layout, const View& v) {
  using lab_fx::detail::splat;
  lab_fx::detail::clear(px, layout);
  const lab::Pixel calm(0, 40, 48), stirred(200, 150, 255);
  for (int e = 0; e < 3; ++e) {
    const int i = kEdge[e][0], j = kEdge[e][1];
    if (!v.present[i] || !v.present[j]) continue;
    const float s = lab::clamp01(v.stir[e]);
    const lab::Pixel c = lab::scale(lab::blend(calm, stirred, s), 0.35f + 0.65f * s);
    for (int k = 1; k < 8; ++k) {
      const float u = k / 8.0f;
      splat(px, layout, kCorner[i].x + u * (kCorner[j].x - kCorner[i].x),
            kCorner[i].y + u * (kCorner[j].y - kCorner[i].y), 0.1f, c);
    }
  }
  for (int i = 0; i < kSlots; ++i) {
    if (!v.present[i]) continue;
    const float u = v.since_s[i] / v.travel_s;
    if (u >= 1) continue;
    for (int j = 0; j < kSlots; ++j) {
      if (j == i || !v.present[j]) continue;
      splat(px, layout, kCorner[i].x + u * (kCorner[j].x - kCorner[i].x),
            kCorner[i].y + u * (kCorner[j].y - kCorner[i].y), 0.14f,
            lab::scale(v.color[i], 0.8f * (1 - u)));
    }
  }
  for (int i = 0; i < kSlots; ++i) {
    if (!v.present[i]) continue;
    const float f = flash(v.since_s[i], v.flash_s);
    splat(px, layout, kCorner[i].x, kCorner[i].y, 0.2f + 0.12f * f,
          lab::scale(v.color[i], 0.1f + 0.9f * f));
  }
}

// An S3's one pixel: its own flash in its own color over a faint glow.
inline lab::Pixel draw_body(lab::Pixel color, float since_s, float flash_s, float glow) {
  return lab::scale(color, glow + (1 - glow) * flash(since_s, flash_s));
}

}  // namespace trio
