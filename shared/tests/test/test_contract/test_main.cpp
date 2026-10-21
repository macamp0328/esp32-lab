// Serial contract builders: the exact boot/err lines lab_core sends.
#include <string.h>
#include <unity.h>

#include "lab_contract.h"

void setUp() {}
void tearDown() {}

static void test_boot_line_carries_every_required_field() {
  char buf[256];
  const char* line =
      lab::boot_line(buf, sizeof buf, 7, {"p000-blink", "a1b2c3d", "s3mini", "3.10.7", "3.3.12"});
  TEST_ASSERT_EQUAL_STRING(
      "{\"t\":7,\"ev\":\"boot\",\"proj\":\"p000-blink\",\"fw\":\"a1b2c3d\",\"board\":\"s3mini\","
      "\"fastled\":\"3.10.7\",\"core\":\"3.3.12\"}",
      line);
}

static void test_boot_line_that_does_not_fit_becomes_an_error() {
  char buf[64];
  const char* line =
      lab::boot_line(buf, sizeof buf, 7,
                     {"p000-a-very-long-project-name", "a1b2c3d", "s3mini", "3.10.7", "3.3.12"});
  TEST_ASSERT_EQUAL_STRING("{\"t\":7,\"ev\":\"err\",\"code\":\"boot_overflow\"}", line);
}

static void test_error_line_falls_back_to_log_overflow() {
  char buf[56];  // fits the 49-char fallback line, not the long code below
  TEST_ASSERT_EQUAL_STRING("{\"t\":1,\"ev\":\"err\",\"code\":\"wifi_timeout\"}",
                           lab::error_line(buf, sizeof buf, 1, "wifi_timeout"));
  const char* line = lab::error_line(buf, sizeof buf, 4000000000u,
                                     "a_code_much_longer_than_this_little_buffer_allows");
  TEST_ASSERT_EQUAL_STRING("{\"t\":4000000000,\"ev\":\"err\",\"code\":\"log_overflow\"}", line);
  TEST_ASSERT_EQUAL_CHAR('}', line[strlen(line) - 1]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_boot_line_carries_every_required_field);
  RUN_TEST(test_boot_line_that_does_not_fit_becomes_an_error);
  RUN_TEST(test_error_line_falls_back_to_log_overflow);
  return UNITY_END();
}
