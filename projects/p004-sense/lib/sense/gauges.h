#pragma once
// p004's gauges: one picture per sensor on a matrix, plus the clap ripple. Pure, so they are
// host-tested (test/test_sense). Row 0 is the top; gauges fill from the bottom.

#include <math.h>
#include <stdint.h>

#include "lab_fx_math.h"
#include "lab_layout.h"
#include "lab_pixel.h"

namespace gauge {

inline void clear(lab::Pixel* px, const lab::Layout& layout) {
  for (uint16_t i = 0; i < layout.size(); ++i) px[i] = lab::Pixel(0, 0, 0);
}

// Channel-wise sum, clipped at 255.
inline lab::Pixel add(lab::Pixel a, lab::Pixel b) {
  auto sat = [](int v) { return static_cast<uint8_t>(v > 255 ? 255 : v); };
  return lab::Pixel(sat(a.r + b.r), sat(a.g + b.g), sat(a.b + b.b));
}

// Fills the bottom `amount` (0..1) of column x, colored from `bottom` to `top` by height. The
// partly filled cell at the top is dimmed in proportion.
inline void column(lab::Pixel* px, const lab::Layout& layout, int x, float amount,
                   lab::Pixel bottom, lab::Pixel top) {
  const int h = layout.height();
  if (x < 0 || x >= layout.width() || h == 0) return;
  const float cells = lab::clamp01(amount) * h;
  for (int k = 0; k < h; ++k) {  // k cells up from the bottom
    const float lit = lab::clamp01(cells - k);
    const lab::Pixel c = lab::blend(bottom, top, h > 1 ? static_cast<float>(k) / (h - 1) : 0);
    px[layout.index(x, h - 1 - k)] = lab::scale(c, lit);
  }
}

// Fills the left `amount` (0..1) of row y with `color`; the rest of the row goes dark.
inline void bar(lab::Pixel* px, const lab::Layout& layout, int y, float amount, lab::Pixel color) {
  const int w = layout.width();
  if (y < 0 || y >= layout.height()) return;
  const float cells = lab::clamp01(amount) * w;
  for (int x = 0; x < w; ++x) px[layout.index(x, y)] = lab::scale(color, lab::clamp01(cells - x));
}

// Thermometer: heat (0..1 of the scale) fills the columns blue to red, with a shimmer on top; the
// right column is the power chip's heat (0..1) in amber.
inline void thermo(lab::Pixel* px, const lab::Layout& layout, float heat, float chip,
                   uint32_t t_ms) {
  clear(px, layout);
  const int w = layout.width();
  for (int x = 0; x + 1 < w; ++x) {
    const float wobble = 0.03f * sinf(t_ms * 0.004f + x * 1.7f);
    column(px, layout, x, heat > 0.01f ? heat + wobble : 0, lab::Pixel(0, 40, 255),
           lab::Pixel(255, 20, 0));
  }
  column(px, layout, w - 1, chip, lab::Pixel(60, 25, 0), lab::Pixel(255, 110, 0));
}

// VU history: one column per step, newest on the right; levels[i] 0..1, a clap is a white column.
// `newest` indexes the latest of the n entries (a ring).
inline void vu(lab::Pixel* px, const lab::Layout& layout, const float* levels, const bool* claps,
               int n, int newest) {
  clear(px, layout);
  const int w = layout.width();
  for (int x = 0; x < w && x < n; ++x) {
    const int i = ((newest - (w - 1 - x)) % n + n) % n;
    if (claps[i]) {
      column(px, layout, x, 1, lab::Pixel(255, 255, 255), lab::Pixel(255, 255, 255));
    } else {
      column(px, layout, x, levels[i], lab::Pixel(0, 255, 30), lab::Pixel(255, 0, 0));
    }
  }
}

// Power meter, over whatever is already drawn: the bottom row is the measured LED current, the
// row above it the estimate, both as fractions of full scale.
inline void meter(lab::Pixel* px, const lab::Layout& layout, float measured, float estimate) {
  const int h = layout.height();
  bar(px, layout, h - 1, measured, lab::Pixel(255, 120, 0));
  if (h > 1) bar(px, layout, h - 2, estimate, lab::Pixel(0, 90, 255));
}

// Battery: charge (0..1) fills the middle columns from the bottom, green, or red when low; the
// outer columns are a dim casing. While charging, bubbles rise through the charge at `flow`
// (0..1, the charge current) cells per 100 ms.
inline void battery(lab::Pixel* px, const lab::Layout& layout, float charge, bool charging,
                    float flow, bool low, uint32_t t_ms) {
  clear(px, layout);
  const int w = layout.width(), h = layout.height();
  const lab::Pixel body = low ? lab::Pixel(255, 0, 0) : lab::Pixel(0, 200, 40);
  for (int x = 0; x < w; ++x) {
    if (x == 0 || x == w - 1) {
      column(px, layout, x, 1, lab::Pixel(12, 12, 12), lab::Pixel(12, 12, 12));
      continue;
    }
    column(px, layout, x, charge, lab::scale(body, 0.5f), body);
    if (!charging || h == 0) continue;
    const float rise = t_ms * 0.01f * (0.3f + flow) + lab::hash01(x) * h;
    const int k = static_cast<int>(rise) % h;  // cells up from the bottom
    if (k < lab::clamp01(charge) * h) {
      const uint16_t i = layout.index(x, h - 1 - k);
      px[i] = add(px[i], lab::Pixel(120, 160, 120));
    }
  }
}

// A ring that expands from the centre for ms_total after a clap, added over the picture.
inline void ripple(lab::Pixel* px, const lab::Layout& layout, uint32_t age_ms, uint32_t ms_total) {
  if (age_ms >= ms_total || ms_total == 0) return;
  const float life = static_cast<float>(age_ms) / ms_total;
  const float cx = (layout.width() - 1) / 2.0f, cy = (layout.height() - 1) / 2.0f;
  const float r = life * (cx > cy ? cx : cy) * 1.5f;
  const float v = 1 - life;
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const float dx = layout.column(i) - cx, dy = layout.row(i) - cy;
    const float k = lab::clamp01(1 - fabsf(sqrtf(dx * dx + dy * dy) - r)) * v;
    px[i] = add(px[i], lab::scale(lab::Pixel(200, 220, 255), k));
  }
}

// Every lit pixel becomes `color`, scaled by how bright it was (for text in one color).
inline void recolor(lab::Pixel* px, const lab::Layout& layout, lab::Pixel color) {
  for (uint16_t i = 0; i < layout.size(); ++i) {
    const uint8_t m = px[i].r > px[i].g ? (px[i].r > px[i].b ? px[i].r : px[i].b)
                                        : (px[i].g > px[i].b ? px[i].g : px[i].b);
    px[i] = lab::scale(color, m / 255.0f);
  }
}

}  // namespace gauge
