// Host tests for the playlist logic in lib/demo.
#include <unity.h>

#include "demo.h"

void setUp() {}
void tearDown() {}

static const uint8_t kLevels[] = {8, 20, 40, 80};
static const demo::Config kCfg{3, 15000, 600, kLevels, 4};

static void test_auto_advances_every_period_and_wraps() {
  demo::State s;
  TEST_ASSERT_FALSE(demo::step(s, kCfg, {}, 14999));
  TEST_ASSERT_TRUE(demo::step(s, kCfg, {}, 15000));
  TEST_ASSERT_EQUAL_UINT8(1, s.fx);
  TEST_ASSERT_EQUAL_UINT32(15000, s.started_ms);
  demo::step(s, kCfg, {}, 30000);
  demo::step(s, kCfg, {}, 45000);
  TEST_ASSERT_EQUAL_UINT8(0, s.fx);
  TEST_ASSERT_EQUAL_UINT8(2, s.prev);
}

static void test_click_a_skips_and_holds_until_a_hold() {
  demo::State s;
  TEST_ASSERT_TRUE(demo::step(s, kCfg, {true, false, false}, 1000));
  TEST_ASSERT_EQUAL_UINT8(1, s.fx);
  TEST_ASSERT_FALSE(s.autoplay);
  TEST_ASSERT_FALSE(demo::step(s, kCfg, {}, 60000));  // no auto-advance while held
  TEST_ASSERT_EQUAL_UINT8(1, s.fx);
  TEST_ASSERT_TRUE(demo::step(s, kCfg, {false, true, false}, 61000));
  TEST_ASSERT_EQUAL_UINT8(2, s.fx);
  TEST_ASSERT_TRUE(s.autoplay);
  demo::step(s, kCfg, {}, 76000);
  TEST_ASSERT_EQUAL_UINT8(0, s.fx);
}

static void test_b_cycles_brightness_levels() {
  demo::State s;
  s.level = 2;
  TEST_ASSERT_EQUAL_UINT8(40, demo::brightness(s, kCfg));
  TEST_ASSERT_TRUE(demo::step(s, kCfg, {false, false, true}, 10));
  TEST_ASSERT_EQUAL_UINT8(80, demo::brightness(s, kCfg));
  demo::step(s, kCfg, {false, false, true}, 20);
  TEST_ASSERT_EQUAL_UINT8(8, demo::brightness(s, kCfg));
  TEST_ASSERT_EQUAL_UINT8(0, s.fx);  // brightness never changes the effect
}

static void test_crossfade_runs_zero_to_one() {
  demo::State s;
  TEST_ASSERT_EQUAL_FLOAT(1, demo::fade(s, kCfg, 0));  // nothing to fade from at boot
  demo::step(s, kCfg, {true, false, false}, 5000);
  TEST_ASSERT_EQUAL_FLOAT(0, demo::fade(s, kCfg, 5000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, demo::fade(s, kCfg, 5300));
  TEST_ASSERT_EQUAL_FLOAT(1, demo::fade(s, kCfg, 5600));
  TEST_ASSERT_EQUAL_UINT32(0, s.prev_started_ms);  // the old effect keeps its own clock
}

static void test_survives_millis_wraparound() {
  demo::State s;
  s.started_ms = 0xFFFFFF00U;
  TEST_ASSERT_FALSE(demo::step(s, kCfg, {}, 0x00000100U));  // 512 ms later
  TEST_ASSERT_TRUE(demo::step(s, kCfg, {}, 0xFFFFFF00U + 15000));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_auto_advances_every_period_and_wraps);
  RUN_TEST(test_click_a_skips_and_holds_until_a_hold);
  RUN_TEST(test_b_cycles_brightness_levels);
  RUN_TEST(test_crossfade_runs_zero_to_one);
  RUN_TEST(test_survives_millis_wraparound);
  return UNITY_END();
}
