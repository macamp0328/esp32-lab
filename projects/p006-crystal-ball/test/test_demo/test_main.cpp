// The demo rotation: turns in order, crossfades at the end of each, loops.
#include <unity.h>

#include "demo.h"

void setUp() {}
void tearDown() {}

constexpr uint32_t kTurn = 30000, kFade = 3000;

static void test_each_pattern_shows_alone_until_its_fade() {
  const uint32_t times[] = {0, 1, 15000, kTurn - kFade - 1};
  for (uint32_t ms : times) {
    const demo::Slot s = demo::at(ms, 9, kTurn, kFade);
    TEST_ASSERT_EQUAL_UINT8(0, s.from);
    TEST_ASSERT_EQUAL_FLOAT(0, s.mix);
  }
  const demo::Slot third = demo::at(2 * kTurn + 100, 9, kTurn, kFade);
  TEST_ASSERT_EQUAL_UINT8(2, third.from);
  TEST_ASSERT_EQUAL_FLOAT(0, third.mix);
}

static void test_fade_runs_into_the_next_pattern() {
  const demo::Slot mid = demo::at(kTurn - kFade / 2, 9, kTurn, kFade);
  TEST_ASSERT_EQUAL_UINT8(0, mid.from);
  TEST_ASSERT_EQUAL_UINT8(1, mid.to);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.mix);
  const demo::Slot end = demo::at(kTurn - 1, 9, kTurn, kFade);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 1, end.mix);  // all `to` just before its own turn starts
}

static void test_the_last_pattern_fades_back_to_the_first() {
  const demo::Slot s = demo::at(9 * kTurn - 1000, 9, kTurn, kFade);
  TEST_ASSERT_EQUAL_UINT8(8, s.from);
  TEST_ASSERT_EQUAL_UINT8(0, s.to);
  TEST_ASSERT_EQUAL_UINT8(0, demo::at(9 * kTurn, 9, kTurn, kFade).from);
  TEST_ASSERT_EQUAL_UINT8(2, demo::at(UINT32_MAX, 9, kTurn, kFade).from);  // 49.7 days in
}

static void test_odd_settings_stay_in_range() {
  const demo::Slot one = demo::at(kTurn - 10, 1, kTurn, kFade);
  TEST_ASSERT_EQUAL_UINT8(0, one.to);
  const demo::Slot none = demo::at(5000, 0, kTurn, kFade);
  TEST_ASSERT_EQUAL_UINT8(0, none.from);
  TEST_ASSERT_EQUAL_FLOAT(0, demo::at(100, 9, kTurn, 0).mix);
  const demo::Slot long_fade = demo::at(kTurn / 2, 9, kTurn, 2 * kTurn);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, long_fade.mix);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_each_pattern_shows_alone_until_its_fade);
  RUN_TEST(test_fade_runs_into_the_next_pattern);
  RUN_TEST(test_the_last_pattern_fades_back_to_the_first);
  RUN_TEST(test_odd_settings_stay_in_range);
  return UNITY_END();
}
