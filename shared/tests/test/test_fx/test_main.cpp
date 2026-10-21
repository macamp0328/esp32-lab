// Effects and color math, on the host Pixel type (CRGB on the board).
#include <unity.h>

#include "lab_font.h"
#include "lab_fx.h"
#include "lab_power.h"

using lab::Layout;
using lab::Pixel;

void setUp() {}
void tearDown() {}

static void assert_pixel(Pixel want, Pixel got) {
  TEST_ASSERT_EQUAL_UINT8(want.r, got.r);
  TEST_ASSERT_EQUAL_UINT8(want.g, got.g);
  TEST_ASSERT_EQUAL_UINT8(want.b, got.b);
}

static const Pixel kBlack(0, 0, 0);
static bool lit(Pixel p) { return !(p == kBlack); }
static uint8_t value(Pixel p) {
  const uint8_t rg = p.r > p.g ? p.r : p.g;
  return rg > p.b ? rg : p.b;
}

static void marquee(Pixel* px, const Layout& l, const lab::AudioFrame&, uint32_t t) {
  lab_fx::text(px, l, "HI 42!", t);
}

struct Named {
  const char* name;
  lab_fx::Fn fn;
};
static const Named kEffects[] = {
    {"sweep", lab_fx::sweep},   {"wiring", lab_fx::wiring},   {"rainbow", lab_fx::rainbow},
    {"plasma", lab_fx::plasma}, {"ripple", lab_fx::ripple},   {"spiral", lab_fx::spiral},
    {"radar", lab_fx::radar},   {"rain", lab_fx::rain},       {"twinkle", lab_fx::twinkle},
    {"fire", lab_fx::fire},     {"lava", lab_fx::lava},       {"bounce", lab_fx::bounce},
    {"heart", lab_fx::heart},   {"invader", lab_fx::invader}, {"marquee", marquee},
};

static void test_hsv_primaries_and_gray() {
  assert_pixel(Pixel(255, 0, 0), lab::hsv(0, 255, 255));
  const Pixel g = lab::hsv(85, 255, 255);
  TEST_ASSERT_TRUE(g.g == 255 && g.r <= 2 && g.b == 0);
  const Pixel b = lab::hsv(170, 255, 255);
  TEST_ASSERT_TRUE(b.b == 255 && b.r == 0 && b.g <= 4);
  assert_pixel(Pixel(100, 100, 100), lab::hsv(42, 0, 100));
}

static void test_math_helpers() {
  for (uint32_t i = 0; i < 1000; ++i) {
    const float h = lab::hash01(i);
    TEST_ASSERT_TRUE(h >= 0 && h < 1);
  }
  TEST_ASSERT_NOT_EQUAL(lab::hash32(1), lab::hash32(2));
  float prev = lab::noise(0.3f, 1.7f, 2.0f);
  for (int k = 1; k <= 200; ++k) {  // smooth: tiny steps give tiny changes, always in 0..1
    const float n = lab::noise(0.3f + k * 0.01f, 1.7f, 2.0f);
    TEST_ASSERT_TRUE(n >= 0 && n <= 1);
    TEST_ASSERT_FLOAT_WITHIN(0.06f, prev, n);
    prev = n;
  }
  assert_pixel(Pixel(10, 20, 30), lab::blend(Pixel(10, 20, 30), Pixel(200, 0, 90), 0));
  assert_pixel(Pixel(200, 0, 90), lab::blend(Pixel(10, 20, 30), Pixel(200, 0, 90), 1));
  assert_pixel(Pixel(100, 50, 0), lab::scale(Pixel(200, 100, 0), 0.5f));
  assert_pixel(kBlack, lab::heat(0));
  assert_pixel(Pixel(255, 255, 255), lab::heat(1));
}

static void test_font_lookup() {
  const uint8_t* l = lab::glyph('L');
  TEST_ASSERT_EQUAL_HEX8(0x7F, l[0]);                              // full left column
  for (int c = 1; c < 5; ++c) TEST_ASSERT_EQUAL_HEX8(0x40, l[c]);  // bottom row only
  TEST_ASSERT_EQUAL_PTR(lab::glyph('E'), lab::glyph('e'));
  for (int c = 0; c < 5; ++c) TEST_ASSERT_EQUAL_HEX8(0, lab::glyph('~')[c]);
}

static void test_sweep_is_red_green_blue_one_second_each() {
  Pixel px[4];
  const Layout l = Layout::strip(4);
  lab_fx::sweep(px, l, {}, 0);
  assert_pixel(Pixel(255, 0, 0), px[3]);
  lab_fx::sweep(px, l, {}, 1500);
  assert_pixel(Pixel(0, 255, 0), px[0]);
  lab_fx::sweep(px, l, {}, 2999);
  assert_pixel(Pixel(0, 0, 255), px[2]);
  lab_fx::sweep(px, l, {}, 3000);
  assert_pixel(Pixel(255, 0, 0), px[1]);
}

static void test_rainbow_hue_follows_position_and_time() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::rainbow(px, l, {}, 0);
  TEST_ASSERT_TRUE(px[0].r > px[0].g && px[0].r > px[0].b);  // column 0: red
  assert_pixel(px[0], px[15]);  // same column (serpentine row 1), same color
  TEST_ASSERT_FALSE(px[0] == px[7]);
  Pixel later[64];
  lab_fx::rainbow(later, l, {}, 2000);
  TEST_ASSERT_FALSE(px[0] == later[0]);
}

static void test_rainbow_louder_is_brighter() {
  Pixel quiet[8], loud[8];
  const Layout l = Layout::strip(8);
  lab::AudioFrame audio;
  lab_fx::rainbow(quiet, l, audio, 100);
  audio.level = 1;
  lab_fx::rainbow(loud, l, audio, 100);
  TEST_ASSERT_TRUE(lab::estimate_ma(loud, 8) > lab::estimate_ma(quiet, 8));
}

static void test_rainbow_on_rings_uses_angle() {
  const uint16_t sizes[] = {16, 8};
  const Layout l = Layout::rings(sizes, 2);
  Pixel px[24];
  lab_fx::rainbow(px, l, {}, 0);
  assert_pixel(px[0], px[16]);  // angle 0 on both rings
  assert_pixel(px[8], px[20]);  // angle 0.5 on both rings
}

// Every effect on every layout kind: same frame for the same t, no write past size(), and some
// frame in 10 s lights something.
static void test_every_effect_is_deterministic_in_bounds_and_lit() {
  const uint16_t sizes[] = {12, 8};
  const Layout layouts[] = {Layout::strip(10), Layout::matrix(8, 8, true),
                            Layout::matrix(8, 8, false), Layout::rings(sizes, 2)};
  constexpr uint16_t kGuard = 4;
  const Pixel sentinel(1, 2, 3);
  for (const Named& fx : kEffects) {
    for (const Layout& l : layouts) {
      Pixel px[64 + kGuard], again[64 + kGuard];
      bool any = false;
      for (uint32_t t = 0; t <= 10000; t += 250) {
        for (Pixel& p : px) p = sentinel;
        fx.fn(px, l, {}, t);
        fx.fn(again, l, {}, t);
        for (uint16_t i = 0; i < l.size(); ++i) {
          TEST_ASSERT_TRUE_MESSAGE(px[i] == again[i], fx.name);
          any = any || lit(px[i]);
        }
        for (uint16_t i = l.size(); i < l.size() + kGuard; ++i) {
          TEST_ASSERT_TRUE_MESSAGE(px[i] == sentinel, fx.name);
        }
      }
      TEST_ASSERT_TRUE_MESSAGE(any, fx.name);
    }
  }
}

static void test_wiring_marks_first_rows_and_last_led() {
  Pixel px[64];
  lab_fx::wiring(px, Layout::matrix(8, 8, true), {}, 0);
  assert_pixel(Pixel(255, 0, 0), px[0]);
  assert_pixel(Pixel(0, 255, 0), px[7]);
  assert_pixel(Pixel(0, 0, 255), px[8]);
  assert_pixel(Pixel(255, 255, 255), px[63]);
  int on = 0;
  for (const Pixel& p : px) on += lit(p);
  TEST_ASSERT_EQUAL_INT(4, on);
}

static void test_plasma_varies_in_space_and_time() {
  Pixel a[64], b[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::plasma(a, l, {}, 1000);
  lab_fx::plasma(b, l, {}, 1500);
  TEST_ASSERT_FALSE(a[0] == a[l.index(4, 4)]);
  TEST_ASSERT_FALSE(a[0] == b[0]);
}

static void test_ripple_is_mirror_symmetric_and_moves() {
  Pixel px[64], later[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::ripple(px, l, {}, 1234);
  for (uint16_t y = 0; y < 8; ++y) {
    for (uint16_t x = 0; x < 8; ++x) {
      assert_pixel(px[l.index(x, y)], px[l.index(7 - x, y)]);
      assert_pixel(px[l.index(x, y)], px[l.index(x, 7 - y)]);
    }
  }
  lab_fx::ripple(later, l, {}, 1534);
  TEST_ASSERT_FALSE(px[l.index(0, 0)] == later[l.index(0, 0)]);
}

static void test_spiral_turns_and_differs_round_the_center() {
  Pixel px[64], later[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::spiral(px, l, {}, 0);
  TEST_ASSERT_FALSE(px[l.index(2, 2)] == px[l.index(5, 5)]);  // same radius, opposite angle
  lab_fx::spiral(later, l, {}, 500);
  TEST_ASSERT_FALSE(px[l.index(2, 2)] == later[l.index(2, 2)]);
}

static void test_radar_beam_sweeps_round() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  const uint16_t right = l.index(7, 3), left = l.index(0, 4);
  lab_fx::radar(px, l, {}, 0);  // beam pointing right
  TEST_ASSERT_TRUE(px[right].g > px[left].g + 100);
  lab_fx::radar(px, l, {}, 1000);  // half a turn later: pointing left
  TEST_ASSERT_TRUE(px[left].g > px[right].g + 100);
}

static void test_rain_has_one_head_per_column_with_the_tail_above() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  const Pixel head(180, 255, 180);
  for (uint32_t t = 0; t < 5000; t += 37) {
    lab_fx::rain(px, l, {}, t);
    for (uint16_t x = 0; x < 8; ++x) {
      int head_row = -1;
      for (uint16_t y = 0; y < 8; ++y) {
        if (px[l.index(x, y)] == head) {
          TEST_ASSERT_EQUAL_INT(-1, head_row);
          head_row = y;
        }
      }
      if (head_row < 0) continue;
      for (uint16_t y = head_row + 1; y < 8; ++y) TEST_ASSERT_FALSE(lit(px[l.index(x, y)]));
    }
  }
}

static void test_twinkle_lights_a_fraction_of_the_stars() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  int on = 0, frames = 0;
  for (uint32_t t = 0; t < 10000; t += 100, ++frames) {
    lab_fx::twinkle(px, l, {}, t);
    for (const Pixel& p : px) on += lit(p);
  }
  const float share = static_cast<float>(on) / (frames * 64);
  TEST_ASSERT_TRUE(share > 0.1f && share < 0.6f);
}

static void test_fire_is_hotter_at_the_bottom() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  uint32_t top = 0, bottom = 0;
  for (uint32_t t = 0; t < 5000; t += 100) {
    lab_fx::fire(px, l, {}, t);
    for (uint16_t x = 0; x < 8; ++x) {
      top += lab::estimate_ma(&px[l.index(x, 0)], 1);
      bottom += lab::estimate_ma(&px[l.index(x, 7)], 1);
    }
  }
  TEST_ASSERT_TRUE(bottom > 4 * top);
}

static void test_lava_has_blobs_on_a_background_and_drifts() {
  Pixel px[64], later[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::lava(px, l, {}, 2000);
  bool hot = false, cold = false;
  for (const Pixel& p : px) {
    hot = hot || p.r > 200;
    cold = cold || (p.b > p.r && p.g == 0);
  }
  TEST_ASSERT_TRUE(hot && cold);
  lab_fx::lava(later, l, {}, 4000);
  int changed = 0;
  for (int i = 0; i < 64; ++i) changed += !(px[i] == later[i]);
  TEST_ASSERT_TRUE(changed > 8);
}

static int brightest_row(const Pixel* px, const Layout& l) {
  int best = 0;
  uint8_t best_v = 0;
  for (uint16_t i = 0; i < l.size(); ++i) {
    if (value(px[i]) > best_v) {
      best_v = value(px[i]);
      best = l.row(i);
    }
  }
  return best;
}

static void test_bounce_hits_the_floor_and_peaks_at_the_top() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  for (uint32_t n = 1; n <= 4; ++n) {
    lab_fx::bounce(px, l, {}, n * 1200);
    TEST_ASSERT_EQUAL_INT(7, brightest_row(px, l));
    lab_fx::bounce(px, l, {}, n * 1200 + 600);
    TEST_ASSERT_EQUAL_INT(0, brightest_row(px, l));
  }
}

static void test_heart_shape_and_beat() {
  Pixel lub[64], rest[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::heart(lub, l, {}, 80);
  lab_fx::heart(rest, l, {}, 650);
  const uint16_t corners[] = {l.index(0, 0), l.index(7, 0), l.index(0, 7), l.index(7, 7)};
  for (uint16_t c : corners) TEST_ASSERT_FALSE(lit(lub[c]));
  const Pixel mid = lub[l.index(3, 3)];
  TEST_ASSERT_TRUE(mid.r > 200 && mid.r > 2 * mid.g && mid.r > 2 * mid.b);
  TEST_ASSERT_TRUE(lab::estimate_ma(lub, 64) > lab::estimate_ma(rest, 64));
}

static void test_invader_walks_and_is_symmetric() {
  Pixel a[64], b[64], c[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::invader(a, l, {}, 0);
  lab_fx::invader(b, l, {}, 500);
  lab_fx::invader(c, l, {}, 1000);
  bool differs = false;
  for (uint16_t y = 0; y < 8; ++y) {
    for (uint16_t x = 0; x < 8; ++x) {
      const uint16_t i = l.index(x, y);
      TEST_ASSERT_EQUAL(lit(a[i]), lit(a[l.index(7 - x, y)]));
      TEST_ASSERT_EQUAL(lit(a[i]), lit(c[i]));
      differs = differs || lit(a[i]) != lit(b[i]);
    }
  }
  TEST_ASSERT_TRUE(differs);
}

static void test_text_enters_from_the_right_and_matches_the_font() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  lab_fx::text(px, l, "EX", 0, 8);
  for (const Pixel& p : px) TEST_ASSERT_FALSE(lit(p));  // still off the right edge
  lab_fx::text(px, l, "EX", 1000, 8);                   // 8 columns in: 'E' at columns 0..4
  const uint8_t* e = lab::glyph('E');
  for (uint16_t x = 0; x < 5; ++x) {
    for (uint16_t y = 0; y < 7; ++y) {
      TEST_ASSERT_EQUAL(((e[x] >> y) & 1) != 0, lit(px[l.index(x, y)]));
    }
  }
  for (uint16_t y = 0; y < 8; ++y) TEST_ASSERT_FALSE(lit(px[l.index(5, y)]));  // letter gap
  for (uint16_t x = 0; x < 8; ++x) TEST_ASSERT_FALSE(lit(px[l.index(x, 7)]));  // below the font
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_hsv_primaries_and_gray);
  RUN_TEST(test_math_helpers);
  RUN_TEST(test_font_lookup);
  RUN_TEST(test_sweep_is_red_green_blue_one_second_each);
  RUN_TEST(test_rainbow_hue_follows_position_and_time);
  RUN_TEST(test_rainbow_louder_is_brighter);
  RUN_TEST(test_rainbow_on_rings_uses_angle);
  RUN_TEST(test_every_effect_is_deterministic_in_bounds_and_lit);
  RUN_TEST(test_wiring_marks_first_rows_and_last_led);
  RUN_TEST(test_plasma_varies_in_space_and_time);
  RUN_TEST(test_ripple_is_mirror_symmetric_and_moves);
  RUN_TEST(test_spiral_turns_and_differs_round_the_center);
  RUN_TEST(test_radar_beam_sweeps_round);
  RUN_TEST(test_rain_has_one_head_per_column_with_the_tail_above);
  RUN_TEST(test_twinkle_lights_a_fraction_of_the_stars);
  RUN_TEST(test_fire_is_hotter_at_the_bottom);
  RUN_TEST(test_lava_has_blobs_on_a_background_and_drifts);
  RUN_TEST(test_bounce_hits_the_floor_and_peaks_at_the_top);
  RUN_TEST(test_heart_shape_and_beat);
  RUN_TEST(test_invader_walks_and_is_symmetric);
  RUN_TEST(test_text_enters_from_the_right_and_matches_the_font);
  return UNITY_END();
}
