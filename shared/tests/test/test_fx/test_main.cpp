// Effects and color math, on the host Pixel type (CRGB on the board).
#include <math.h>
#include <unity.h>

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

static void test_hsv_primaries_and_gray() {
  assert_pixel(Pixel(255, 0, 0), lab::hsv(0, 255, 255));
  const Pixel g = lab::hsv(85, 255, 255);
  TEST_ASSERT_TRUE(g.g == 255 && g.r <= 2 && g.b == 0);
  const Pixel b = lab::hsv(170, 255, 255);
  TEST_ASSERT_TRUE(b.b == 255 && b.r == 0 && b.g <= 4);
  assert_pixel(Pixel(100, 100, 100), lab::hsv(42, 0, 100));
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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_hsv_primaries_and_gray);
  RUN_TEST(test_sweep_is_red_green_blue_one_second_each);
  RUN_TEST(test_rainbow_hue_follows_position_and_time);
  RUN_TEST(test_rainbow_louder_is_brighter);
  RUN_TEST(test_rainbow_on_rings_uses_angle);
  RUN_TEST(test_rainbow_treats_nan_level_as_silence);
  return UNITY_END();
}
