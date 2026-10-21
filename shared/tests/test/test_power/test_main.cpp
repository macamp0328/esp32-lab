// Power estimator: FastLED's WS2812 model (R 16, G 11, B 15 mA per full channel, 1 mA idle).
#include <unity.h>

#include "lab_pixel.h"
#include "lab_power.h"

using lab::Pixel;

void setUp() {}
void tearDown() {}

static void test_dark_pixels_draw_idle_current() {
  Pixel px[64];
  TEST_ASSERT_EQUAL_UINT32(64, lab::estimate_ma(px, 64));
}

static void test_full_white() {
  Pixel px[64];
  for (auto& p : px) p = Pixel(255, 255, 255);
  TEST_ASSERT_EQUAL_UINT32(43, lab::estimate_ma(px, 1));
  TEST_ASSERT_EQUAL_UINT32(64 * 42 + 64, lab::estimate_ma(px, 64));
}

static void test_brightness_scales_color_not_idle() {
  Pixel px[10];
  for (auto& p : px) p = Pixel(255, 0, 0);
  TEST_ASSERT_EQUAL_UINT32(170, lab::estimate_ma(px, 10));
  TEST_ASSERT_EQUAL_UINT32(80 + 10, lab::estimate_ma(px, 10, 128));
  TEST_ASSERT_EQUAL_UINT32(10, lab::estimate_ma(px, 10, 0));
}

static void test_brightness_for_cap() {
  Pixel px[64];
  for (auto& p : px) p = Pixel(255, 255, 255);
  const uint8_t b = lab::brightness_for_cap(px, 64, 255, 800);
  TEST_ASSERT_TRUE(lab::estimate_ma(px, 64, b) <= 800);
  TEST_ASSERT_TRUE(lab::estimate_ma(px, 64, b + 1) > 800);
  TEST_ASSERT_EQUAL_UINT8(40, lab::brightness_for_cap(px, 1, 40, 800));  // under cap: keep
  TEST_ASSERT_EQUAL_UINT8(0, lab::brightness_for_cap(px, 64, 255, 50));  // idle alone exceeds
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_dark_pixels_draw_idle_current);
  RUN_TEST(test_full_white);
  RUN_TEST(test_brightness_scales_color_not_idle);
  RUN_TEST(test_brightness_for_cap);
  return UNITY_END();
}
