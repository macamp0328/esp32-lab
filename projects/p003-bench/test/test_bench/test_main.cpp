// Host tests for the measurement core in lib/bench (float asserts: Unity is built without double).
#include <unity.h>

#include "bench.h"

void setUp() {}
void tearDown() {}

static void test_accumulator_mean_max_rate() {
  bench::Accumulator acc;
  TEST_ASSERT_EQUAL_UINT32(0, acc.stats().iterations);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0, static_cast<float>(acc.stats().per_s));
  acc.add(100);
  acc.add(300);
  const bench::Stats s = acc.stats();
  TEST_ASSERT_EQUAL_UINT32(2, s.iterations);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 200, static_cast<float>(s.mean_us));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 300, static_cast<float>(s.max_us));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 5000, static_cast<float>(s.per_s));
}

static void test_run_meets_both_minimums() {
  int64_t clock = 0;
  auto now = [&clock] { return clock; };
  uint32_t calls = 0;
  // Each body call takes 50 us of fake time: 20 calls reach 1000 us, so time wins at 300000.
  bench::Stats s = bench::run(now, [&] { clock += 50, ++calls; }, 300000, 20);
  TEST_ASSERT_EQUAL_UINT32(6000, calls);
  TEST_ASSERT_EQUAL_UINT32(calls, s.iterations);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 50, static_cast<float>(s.mean_us));
  calls = 0;
  // Slow body: 100 ms each; the 20-iteration minimum wins.
  s = bench::run(now, [&] { clock += 100000, ++calls; }, 300000, 20);
  TEST_ASSERT_EQUAL_UINT32(20, calls);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10, static_cast<float>(s.per_s));
}

static void test_wire_time_and_bandwidth() {
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 2200, static_cast<float>(bench::wire_us(64)));  // 64 * 30 + 280
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 31000,
                           static_cast<float>(bench::wire_us(1024)));  // ~32 fps on one pin
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 32.26, static_cast<float>(bench::wire_fps(1024)));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 100, static_cast<float>(bench::mb_per_s(16384, 163.84)));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0, static_cast<float>(bench::mb_per_s(10, 0)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_accumulator_mean_max_rate);
  RUN_TEST(test_run_meets_both_minimums);
  RUN_TEST(test_wire_time_and_bandwidth);
  return UNITY_END();
}
