// Host tests for the serial contract this project emits (lab_core/lab_json.h).
// Add this project's own pure-logic tests next to these.
#include <math.h>
#include <string.h>
#include <unity.h>

#include "lab_json.h"

void setUp() {}
void tearDown() {}

static void test_boot_line_matches_contract() {
  char buf[160];
  lab::JsonLine line(buf, sizeof buf, 1250, "boot");
  line.str("proj", "p000-blink").str("fw", "a1b2c3d").str("fastled", "3.10.7");
  TEST_ASSERT_EQUAL_STRING(
      "{\"t\":1250,\"ev\":\"boot\",\"proj\":\"p000-blink\",\"fw\":\"a1b2c3d\","
      "\"fastled\":\"3.10.7\"}",
      line.finish());
  TEST_ASSERT_TRUE(line.ok());
}

static void test_numbers_bools_and_nonfinite() {
  char buf[128];
  lab::JsonLine line(buf, sizeof buf, 6320, "fps");
  line.num("fps", 118).num("neg", -3).real("bass", 0.714).real("bad", NAN).flag("ok", true);
  TEST_ASSERT_EQUAL_STRING(
      "{\"t\":6320,\"ev\":\"fps\",\"fps\":118,\"neg\":-3,\"bass\":0.71,\"bad\":null,\"ok\":true}",
      line.finish());
}

static void test_strings_are_escaped() {
  char buf[96];
  lab::JsonLine line(buf, sizeof buf, 0, "err");
  line.str("code", "a\"b\\c\nd\x01");
  TEST_ASSERT_EQUAL_STRING("{\"t\":0,\"ev\":\"err\",\"code\":\"a\\\"b\\\\c\\nd\\u0001\"}",
                           line.finish());
}

static void test_overflow_never_writes_past_capacity() {
  char buf[24];
  memset(buf, 'X', sizeof buf);
  lab::JsonLine line(buf, 16, 1, "leds");
  line.num("n", 64).str("driver", "RMT");
  line.finish();
  TEST_ASSERT_FALSE(line.ok());
  TEST_ASSERT_TRUE(strlen(buf) < 16);
  for (size_t i = 16; i < sizeof buf; ++i) TEST_ASSERT_EQUAL_INT('X', buf[i]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_boot_line_matches_contract);
  RUN_TEST(test_numbers_bools_and_nonfinite);
  RUN_TEST(test_strings_are_escaped);
  RUN_TEST(test_overflow_never_writes_past_capacity);
  return UNITY_END();
}
