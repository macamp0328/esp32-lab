// The glow schedule: 18:00 to 22:00, fades, windows across midnight, the boot glow.
#include <unity.h>

#include "schedule.h"

using schedule::boot_level;
using schedule::level;

constexpr uint32_t kOn = 18 * 3600, kOff = 22 * 3600, kFade = 300;
constexpr uint32_t at(uint32_t h, uint32_t m, uint32_t s = 0) { return h * 3600 + m * 60 + s; }

void setUp() {}
void tearDown() {}

static void test_dark_outside_the_window() {
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(0, 0), kOn, kOff, kFade));
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(17, 59, 59), kOn, kOff, kFade));
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(18, 0), kOn, kOff, kFade));  // the fade starts at 0
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(22, 0), kOn, kOff, kFade));
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(23, 30), kOn, kOff, kFade));
}

static void test_full_between_the_fades() {
  TEST_ASSERT_EQUAL_FLOAT(1, level(at(18, 5), kOn, kOff, kFade));
  TEST_ASSERT_EQUAL_FLOAT(1, level(at(20, 0), kOn, kOff, kFade));
  TEST_ASSERT_EQUAL_FLOAT(1, level(at(21, 55), kOn, kOff, kFade));
}

static void test_fades_are_symmetric_and_monotonic() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, level(at(18, 2, 30), kOn, kOff, kFade));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, level(at(21, 57, 30), kOn, kOff, kFade));
  float prev = 0;
  for (uint32_t s = 0; s <= kFade; s += 5) {
    const float up = level(kOn + s, kOn, kOff, kFade);
    TEST_ASSERT_TRUE(up >= prev);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, up, level(kOff - s, kOn, kOff, kFade));
    prev = up;
  }
  TEST_ASSERT_EQUAL_FLOAT(1, prev);
}

static void test_window_may_cross_midnight() {
  const uint32_t on = at(22, 0), off = at(2, 0);
  TEST_ASSERT_EQUAL_FLOAT(1, level(at(23, 0), on, off, kFade));
  TEST_ASSERT_EQUAL_FLOAT(1, level(at(1, 0), on, off, kFade));
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(3, 0), on, off, kFade));
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(21, 0), on, off, kFade));
}

static void test_edge_settings_never_divide_by_zero() {
  TEST_ASSERT_EQUAL_FLOAT(1, level(at(18, 0), kOn, kOff, 0));
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(12, 0), kOn, kOn, kFade));  // on == off: never
  TEST_ASSERT_EQUAL_FLOAT(0, level(at(18, 5), kOn, kOn, kFade));
  // a window shorter than two fades peaks at its middle
  const float mid = level(kOn + 120, kOn, kOn + 240, kFade);
  TEST_ASSERT_TRUE(mid > 0 && mid < 1);
}

static void test_boot_glow_fades_in_and_ends() {
  TEST_ASSERT_EQUAL_FLOAT(0, boot_level(0, 60000, 2000));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, boot_level(1000, 60000, 2000));
  TEST_ASSERT_EQUAL_FLOAT(1, boot_level(30000, 60000, 2000));
  TEST_ASSERT_EQUAL_FLOAT(0, boot_level(60000, 60000, 2000));
  TEST_ASSERT_EQUAL_FLOAT(0, boot_level(4000000000U, 60000, 2000));
  TEST_ASSERT_EQUAL_FLOAT(0, boot_level(1000, 0, 2000));  // boot glow switched off
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_dark_outside_the_window);
  RUN_TEST(test_full_between_the_fades);
  RUN_TEST(test_fades_are_symmetric_and_monotonic);
  RUN_TEST(test_window_may_cross_midnight);
  RUN_TEST(test_edge_settings_never_divide_by_zero);
  RUN_TEST(test_boot_glow_fades_in_and_ends);
  return UNITY_END();
}
