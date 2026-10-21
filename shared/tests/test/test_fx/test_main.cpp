// Effects and color math, on the host Pixel type (CRGB on the board).
#include <math.h>
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
    {"sweep", lab_fx::sweep},
    {"wiring", lab_fx::wiring},
    {"rainbow", lab_fx::rainbow},
    {"plasma", lab_fx::plasma},
    {"ripple", lab_fx::ripple},
    {"spiral", lab_fx::spiral},
    {"radar", lab_fx::radar},
    {"rain", lab_fx::rain},
    {"twinkle", lab_fx::twinkle},
    {"fire", lab_fx::fire},
    {"lava", lab_fx::lava},
    {"bounce", lab_fx::bounce},
    {"heart", lab_fx::heart},
    {"invader", lab_fx::invader},
    {"marquee", marquee},
    {"starfield", lab_fx::starfield},
    {"fireworks", lab_fx::fireworks},
    {"aurora", lab_fx::aurora},
    {"snow", lab_fx::snow},
    {"tunnel", lab_fx::tunnel},
    {"kaleidoscope", lab_fx::kaleidoscope},
    {"helix", lab_fx::helix},
    {"lightning", lab_fx::lightning},
    {"pong", lab_fx::pong},
    {"pacman", lab_fx::pacman},
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

static void test_rainbow_treats_nan_level_as_silence() {
  Pixel quiet[8], nan_px[8];
  const Layout l = Layout::strip(8);
  lab::AudioFrame audio;
  lab_fx::rainbow(quiet, l, audio, 100);
  audio.level = NAN;
  lab_fx::rainbow(nan_px, l, audio, 100);
  for (int i = 0; i < 8; ++i) assert_pixel(quiet[i], nan_px[i]);
}

static void test_rainbow_on_rings_uses_angle() {
  const uint16_t sizes[] = {16, 8};
  const Layout l = Layout::rings(sizes);
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
                            Layout::matrix(8, 8, false), Layout::matrix(8, 8, true, true),
                            Layout::rings(sizes)};
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
  Pixel tall[24];  // column-wired 3x8: the first wired line is a column of 8
  lab_fx::wiring(tall, Layout::matrix(3, 8, true, true), {}, 0);
  assert_pixel(Pixel(0, 255, 0), tall[7]);
  assert_pixel(Pixel(0, 0, 255), tall[8]);
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

static float mean_value(const Pixel* px, const Layout& l, bool (*pick)(const Layout&, uint16_t)) {
  float sum = 0;
  int n = 0;
  for (uint16_t i = 0; i < l.size(); ++i) {
    if (!pick(l, i)) continue;
    sum += value(px[i]);
    ++n;
  }
  return n ? sum / n : 0;
}
static uint8_t max_value(const Pixel* px, const Layout& l, bool (*pick)(const Layout&, uint16_t)) {
  uint8_t best = 0;
  for (uint16_t i = 0; i < l.size(); ++i) {
    if (pick(l, i) && value(px[i]) > best) best = value(px[i]);
  }
  return best;
}
static float dist_from_center(const Layout& l, uint16_t i) {
  const lab::Point p = l.position(i);
  return sqrtf((p.x - 0.5f) * (p.x - 0.5f) + (p.y - 0.5f) * (p.y - 0.5f));
}
static bool near_center(const Layout& l, uint16_t i) { return dist_from_center(l, i) < 0.2f; }
static bool near_rim(const Layout& l, uint16_t i) { return dist_from_center(l, i) > 0.4f; }

static void test_starfield_is_brighter_toward_the_rim() {  // stars brighten as they fly out
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  uint32_t center = 0, rim = 0;
  for (uint32_t t = 0; t < 8000; t += 50) {
    lab_fx::starfield(px, l, {}, t);
    center += max_value(px, l, near_center);
    rim += max_value(px, l, near_rim);
  }
  TEST_ASSERT_TRUE(rim > 3 * center / 2);
}

static void test_firework_rises_bursts_high_and_burns_out() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  for (uint32_t seed = 0; seed < 20; ++seed) {
    lab_fx::detail::clear(px, l);
    lab_fx::detail::firework(px, l, seed, 0.05f);  // just launched: only the bottom rows
    for (uint16_t i = 0; i < 64; ++i) {
      if (lit(px[i])) TEST_ASSERT_TRUE(l.row(i) >= 5);
    }
    lab_fx::detail::clear(px, l);
    lab_fx::detail::firework(px, l, seed, lab_fx::detail::kRiseS + 0.05f);
    TEST_ASSERT_TRUE(brightest_row(px, l) <= 3);  // the burst is in the top half
    lab_fx::detail::clear(px, l);
    lab_fx::detail::firework(px, l, seed, lab_fx::detail::kRiseS + lab_fx::detail::kBurnS);
    for (const Pixel& p : px) TEST_ASSERT_FALSE(lit(p));
  }
}

static void test_aurora_is_green_to_violet_and_moves() {
  Pixel px[64], later[64];
  const Layout l = Layout::matrix(8, 8, true);
  for (uint32_t t = 0; t < 20000; t += 500) {
    lab_fx::aurora(px, l, {}, t);
    for (const Pixel& p : px) {
      if (value(p) > 20) TEST_ASSERT_TRUE(p.g + p.b > 2 * p.r);
    }
  }
  lab_fx::aurora(px, l, {}, 1000);
  lab_fx::aurora(later, l, {}, 3000);
  int changed = 0;
  for (int i = 0; i < 64; ++i) changed += !(px[i] == later[i]);
  TEST_ASSERT_TRUE(changed > 16);
}

static void test_snow_falls_onto_a_white_floor() {
  for (uint32_t k = 0; k < 14; ++k) {
    for (float s = 0; s < 20; s += 0.37f) {
      float x0, y0, x1, y1;
      lab_fx::detail::snowflake(k, s, &x0, &y0);
      lab_fx::detail::snowflake(k, s + 0.1f, &x1, &y1);
      TEST_ASSERT_TRUE(y1 > y0 || y0 > 0.9f);   // down, unless it wrapped to the top
      TEST_ASSERT_FLOAT_WITHIN(0.02f, x0, x1);  // sways, never jumps sideways
    }
  }
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true, true);
  lab_fx::snow(px, l, {}, 1234);
  for (uint16_t x = 0; x < 8; ++x) TEST_ASSERT_TRUE(value(px[l.index(x, 7)]) >= 110);
}

static void test_tunnel_center_is_dark() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  float center = 0, rim = 0;
  for (uint32_t t = 0; t < 5000; t += 100) {
    lab_fx::tunnel(px, l, {}, t);
    center += mean_value(px, l, near_center);
    rim += mean_value(px, l, near_rim);
  }
  TEST_ASSERT_TRUE(rim > 2 * center);
}

static void test_kaleidoscope_has_eight_way_symmetry() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, false);
  for (uint32_t t = 0; t < 10000; t += 1300) {
    lab_fx::kaleidoscope(px, l, {}, t);
    for (uint16_t y = 0; y < 8; ++y) {
      for (uint16_t x = 0; x < 8; ++x) {
        const Pixel p = px[l.index(x, y)];
        assert_pixel(p, px[l.index(7 - x, y)]);
        assert_pixel(p, px[l.index(x, 7 - y)]);
        assert_pixel(p, px[l.index(y, x)]);
      }
    }
  }
}

static void test_helix_shows_both_strands_and_turns() {
  Pixel px[64], later[64];
  const Layout l = Layout::matrix(8, 8, true);
  for (uint32_t t = 0; t < 6000; t += 400) {
    lab_fx::helix(px, l, {}, t);
    int rows = 0;
    for (uint16_t y = 0; y < 8; ++y) {
      bool cyan = false, magenta = false;
      for (uint16_t x = 0; x < 8; ++x) {
        const Pixel p = px[l.index(x, y)];
        cyan = cyan || p.g > p.r + 40;
        magenta = magenta || p.r > p.g + 40;
      }
      rows += cyan && magenta;
    }
    TEST_ASSERT_TRUE(rows >= 5);
  }
  lab_fx::helix(px, l, {}, 0);
  lab_fx::helix(later, l, {}, 300);
  TEST_ASSERT_FALSE(px[l.index(2, 0)] == later[l.index(2, 0)]);
}

static bool bolt_in_rows(const Pixel* px, const Layout& l, uint16_t from, uint16_t to) {
  for (uint16_t i = 0; i < l.size(); ++i) {
    if (l.row(i) >= from && l.row(i) <= to && px[i].b > 200) return true;
  }
  return false;
}

static void test_lightning_bolt_grows_downward() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  for (uint32_t c = 0; c < 6; ++c) {
    const uint32_t strike = c * lab_fx::detail::kStormMs + lab_fx::detail::strike_ms(c);
    lab_fx::lightning(px, l, {}, strike - 50);
    TEST_ASSERT_FALSE(bolt_in_rows(px, l, 0, 7));
    lab_fx::lightning(px, l, {}, strike + 20);
    TEST_ASSERT_TRUE(bolt_in_rows(px, l, 0, 1));
    TEST_ASSERT_FALSE(bolt_in_rows(px, l, 4, 7));
    lab_fx::lightning(px, l, {}, strike + 100);
    TEST_ASSERT_TRUE(bolt_in_rows(px, l, 7, 7));
  }
}

static void test_pong_paddles_always_meet_the_ball() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true);
  const Pixel ball(255, 255, 255);
  int returns = 0;
  for (uint32_t t = 0; t < 30000; t += 10) {
    lab_fx::pong(px, l, {}, t);
    for (uint16_t y = 0; y < 8; ++y) {
      if (px[l.index(1, y)] == ball) {
        TEST_ASSERT_TRUE(lit(px[l.index(0, y)]));
        ++returns;
      }
      if (px[l.index(6, y)] == ball) {
        TEST_ASSERT_TRUE(lit(px[l.index(7, y)]));
        ++returns;
      }
    }
  }
  TEST_ASSERT_TRUE(returns > 100);
}

static void test_pacman_eats_dots_then_chases_the_blue_ghost() {
  Pixel px[64];
  const Layout l = Layout::matrix(8, 8, true, true);
  lab_fx::pacman(px, l, {}, 1180);  // first half: Pac-Man at columns 1..6
  bool yellow = false;
  for (const Pixel& p : px) yellow = yellow || (p.r > 200 && p.g > 200 && p.b < 50);
  TEST_ASSERT_TRUE(yellow);
  TEST_ASSERT_TRUE(lit(px[l.index(7, 4)]));          // the power pellet ahead of him
  const uint32_t back = (8 + 15) * 1000 / 6 + 1500;  // second half: the scared ghost
  lab_fx::pacman(px, l, {}, back);
  bool blue = false;
  for (const Pixel& p : px) blue = blue || (p.b > 200 && p.r < 100);
  TEST_ASSERT_TRUE(blue);
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
  RUN_TEST(test_rainbow_treats_nan_level_as_silence);
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
  RUN_TEST(test_starfield_is_brighter_toward_the_rim);
  RUN_TEST(test_firework_rises_bursts_high_and_burns_out);
  RUN_TEST(test_aurora_is_green_to_violet_and_moves);
  RUN_TEST(test_snow_falls_onto_a_white_floor);
  RUN_TEST(test_tunnel_center_is_dark);
  RUN_TEST(test_kaleidoscope_has_eight_way_symmetry);
  RUN_TEST(test_helix_shows_both_strands_and_turns);
  RUN_TEST(test_lightning_bolt_grows_downward);
  RUN_TEST(test_pong_paddles_always_meet_the_ball);
  RUN_TEST(test_pacman_eats_dots_then_chases_the_blue_ghost);
  return UNITY_END();
}
