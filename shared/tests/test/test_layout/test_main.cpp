// Layout mappers: wiring order <-> column/row, ring/angle, normalized positions.
#include <unity.h>

#include "lab_layout.h"

using lab::Layout;

void setUp() {}
void tearDown() {}

static void test_strip_is_a_single_row() {
  const Layout l = Layout::strip(8);
  TEST_ASSERT_EQUAL_UINT16(8, l.size());
  TEST_ASSERT_EQUAL_UINT16(3, l.index(3, 0));
  TEST_ASSERT_EQUAL_UINT16(8, l.index(8, 0));  // out of range -> size()
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.0625f, l.position(0).x);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.5f, l.position(5).y);
}

static void test_serpentine_matrix_reverses_odd_rows() {
  const Layout l = Layout::matrix(4, 2, true);
  TEST_ASSERT_EQUAL_UINT16(0, l.index(0, 0));
  TEST_ASSERT_EQUAL_UINT16(3, l.index(3, 0));
  TEST_ASSERT_EQUAL_UINT16(7, l.index(0, 1));  // second row runs right to left
  TEST_ASSERT_EQUAL_UINT16(4, l.index(3, 1));
  TEST_ASSERT_EQUAL_UINT16(8, l.index(0, 2));
}

static void test_progressive_matrix_keeps_direction() {
  const Layout l = Layout::matrix(4, 2, false);
  TEST_ASSERT_EQUAL_UINT16(4, l.index(0, 1));
  TEST_ASSERT_EQUAL_UINT16(7, l.index(3, 1));
}

static void test_column_wired_matrix_runs_down_columns() {
  const Layout l = Layout::matrix(4, 2, true, true);
  TEST_ASSERT_EQUAL_UINT16(0, l.index(0, 0));
  TEST_ASSERT_EQUAL_UINT16(1, l.index(0, 1));  // down column 0
  TEST_ASSERT_EQUAL_UINT16(2, l.index(1, 1));  // column 1 runs bottom to top
  TEST_ASSERT_EQUAL_UINT16(3, l.index(1, 0));
  TEST_ASSERT_EQUAL_UINT16(7, l.index(3, 0));
  TEST_ASSERT_EQUAL_UINT16(2, Layout::matrix(4, 2, false, true).index(1, 0));
  // matrix-8x8-01 as measured: 0 top-left, 7 bottom-left, 8 beside it, 63 top-right
  const Layout m = Layout::matrix(8, 8, true, true);
  TEST_ASSERT_EQUAL_UINT16(7, m.index(0, 7));
  TEST_ASSERT_EQUAL_UINT16(8, m.index(1, 7));
  TEST_ASSERT_EQUAL_UINT16(63, m.index(7, 0));
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.1875f, m.position(8).x);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.9375f, m.position(8).y);
}

static void test_matrix_index_and_column_row_round_trip() {
  for (int serp = 0; serp < 2; ++serp) {
    for (int cols = 0; cols < 2; ++cols) {
      const Layout l = Layout::matrix(8, 4, serp, cols);
      for (uint16_t i = 0; i < l.size(); ++i) {
        TEST_ASSERT_EQUAL_UINT16(i, l.index(l.column(i), l.row(i)));
        TEST_ASSERT_TRUE(l.column(i) < 8 && l.row(i) < 4);
      }
    }
  }
  const Layout l = Layout::matrix(8, 8, true);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.0625f, l.position(15).x);  // LED 15 is column 0 of row 1
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.1875f, l.position(15).y);
}

static void test_rings_split_by_wiring_order() {
  const uint16_t sizes[] = {16, 8};
  const Layout l = Layout::rings(sizes);
  TEST_ASSERT_EQUAL_UINT16(24, l.size());
  TEST_ASSERT_EQUAL_UINT8(0, l.ring(15));
  TEST_ASSERT_EQUAL_UINT8(1, l.ring(16));
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.25f, l.angle(4));
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.0f, l.angle(16));
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.5f, l.angle(20));
  TEST_ASSERT_EQUAL_UINT16(24, l.index(0, 0));  // no column/row on rings
}

static void test_ring_radius_follows_led_count() {
  const uint16_t sizes[] = {16, 8};
  const Layout l = Layout::rings(sizes);
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0f, l.position(0).x);    // outer ring, angle 0
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.75f, l.position(16).x);  // inner ring, half the radius
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.5f, l.position(16).y);
}

static void test_empty_layouts_do_not_divide_by_zero() {
  const Layout none;
  const Layout strip0 = Layout::strip(0);
  const Layout matrix0 = Layout::matrix(0, 4, true);
  const Layout* layouts[] = {&none, &strip0, &matrix0};
  for (const Layout* l : layouts) {
    TEST_ASSERT_EQUAL_UINT16(0, l->size());
    TEST_ASSERT_EQUAL_UINT16(0, l->row(0));
    TEST_ASSERT_EQUAL_UINT16(0, l->column(0));
    TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.5f, l->position(0).x);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_strip_is_a_single_row);
  RUN_TEST(test_serpentine_matrix_reverses_odd_rows);
  RUN_TEST(test_progressive_matrix_keeps_direction);
  RUN_TEST(test_column_wired_matrix_runs_down_columns);
  RUN_TEST(test_matrix_index_and_column_row_round_trip);
  RUN_TEST(test_rings_split_by_wiring_order);
  RUN_TEST(test_ring_radius_follows_led_count);
  RUN_TEST(test_empty_layouts_do_not_divide_by_zero);
  return UNITY_END();
}
