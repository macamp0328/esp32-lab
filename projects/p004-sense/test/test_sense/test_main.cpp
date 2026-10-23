// p004's pure logic (lib/sense): sensor smoothing, time, the ambient mood, the scene picker and
// the gauges, on an 8x8 matrix wired like the real one.
#include <math.h>
#include <string.h>
#include <unity.h>

#include "gauges.h"
#include "lab_json.h"
#include "sense.h"

using lab::Pixel;

namespace {
const lab::Layout kMatrix = lab::Layout::matrix(8, 8, true, true);
Pixel px[64];

bool lit(int x, int y) {
  const Pixel& p = px[kMatrix.index(x, y)];
  return p.r || p.g || p.b;
}

int lit_count() {
  int n = 0;
  for (const Pixel& p : px) n += p.r || p.g || p.b;
  return n;
}

sense::Time at(int y, int mo, int d, int h, int mi, int s) {
  sense::Time t;
  t.year = y;
  t.month = mo;
  t.day = d;
  t.hour = h;
  t.minute = mi;
  t.second = s;
  return t;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_ema_seeds_then_converges() {
  sense::Ema e{1000};
  TEST_ASSERT_EQUAL_FLOAT(20, e.push(20, 250));
  for (int i = 0; i < 20; ++i) e.push(30, 250);  // 5 time constants
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 30, e.value);
}

static void test_warmth_settles_then_tracks_changes() {
  sense::Warmth w;
  w.push(58, 250);                               // a noisy first sample
  for (int i = 0; i < 20; ++i) w.push(55, 250);  // 5 s: the average follows while settling
  TEST_ASSERT_EQUAL_FLOAT(0, w.delta_c());
  for (int i = 0; i < 420; ++i) w.push(55, 250);  // 105 s at rest: no bias from the first sample
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55, w.now_c());
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0, w.delta_c());
  for (int i = 0; i < 120; ++i) w.push(53, 250);  // 2 C cooler for 30 s
  TEST_ASSERT_TRUE(w.delta_c() < -1.0f);
  for (int i = 0; i < 2400; ++i) w.push(53, 250);  // 10 more minutes: adapted
  TEST_ASSERT_TRUE(fabsf(w.delta_c()) < 0.05f);

  sense::Warmth drift;
  for (int i = 0; i < 2400; ++i) drift.push(30 + i * 0.25f * 0.2f / 60, 250);  // 0.2 C per minute
  TEST_ASSERT_TRUE(drift.delta_c() < 0.25f);
}

static void test_led_current_from_the_usb_input() {
  TEST_ASSERT_EQUAL_FLOAT(150, sense::led_ma(250, 0, 100));
  TEST_ASSERT_EQUAL_FLOAT(100, sense::led_ma(250, 50, 100));   // charging takes its share
  TEST_ASSERT_EQUAL_FLOAT(150, sense::led_ma(250, -30, 100));  // discharge is not input current
  TEST_ASSERT_EQUAL_FLOAT(0, sense::led_ma(80, 0, 100));
}

static void test_battery_level_is_monotonic_and_clamped() {
  TEST_ASSERT_EQUAL_FLOAT(0, sense::battery_level(3.0f));
  TEST_ASSERT_EQUAL_FLOAT(1, sense::battery_level(4.2f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, sense::battery_level(3.8f));
  float prev = -1;
  for (float v = 3.2f; v < 4.3f; v += 0.01f) {
    const float l = sense::battery_level(v);
    TEST_ASSERT_TRUE(l >= prev);
    prev = l;
  }
}

static void test_daylight_dims_and_warms_at_night() {
  const sense::Daylight noon = sense::daylight(12 * 60);
  TEST_ASSERT_EQUAL_FLOAT(1, noon.scale);
  TEST_ASSERT_EQUAL_FLOAT(0, noon.warm);
  const sense::Daylight night = sense::daylight(2 * 60);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.35f, night.scale);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.7f, night.warm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.35f, sense::daylight(7 * 60).warm);   // dawn, halfway
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.35f, sense::daylight(20 * 60).warm);  // dusk, halfway
  TEST_ASSERT_EQUAL_FLOAT(night.warm, sense::daylight(-60).warm);          // 23:00
}

static void test_warm_filter_keeps_dark_dark() {
  TEST_ASSERT_TRUE(sense::warm_filter(Pixel(200, 200, 200), 0) == Pixel(200, 200, 200));
  const Pixel w = sense::warm_filter(Pixel(200, 200, 200), 1);
  TEST_ASSERT_EQUAL_UINT8(200, w.r);
  TEST_ASSERT_TRUE(w.g < 120 && w.b < 40);
  TEST_ASSERT_TRUE(sense::warm_filter(Pixel(0, 0, 0), 1) == Pixel(0, 0, 0));
}

static void test_calendar_round_trips() {
  TEST_ASSERT_EQUAL_INT32(0, sense::days_from_civil(1970, 1, 1));
  const sense::Time cases[] = {at(2000, 1, 1, 0, 0, 0), at(2026, 10, 23, 14, 5, 9),
                               at(2028, 2, 29, 23, 59, 59), at(2099, 12, 31, 12, 0, 0)};
  for (const sense::Time& t : cases) {
    const sense::Time r = sense::from_seconds(sense::seconds(t));
    TEST_ASSERT_EQUAL_UINT16(t.year, r.year);
    TEST_ASSERT_EQUAL_UINT8(t.month, r.month);
    TEST_ASSERT_EQUAL_UINT8(t.day, r.day);
    TEST_ASSERT_EQUAL_UINT8(t.hour, r.hour);
    TEST_ASSERT_EQUAL_UINT8(t.minute, r.minute);
    TEST_ASSERT_EQUAL_UINT8(t.second, r.second);
  }
  TEST_ASSERT_EQUAL_INT(14 * 60 + 5, sense::minute_of_day(cases[1]));
}

static void test_build_time_parses_and_moves_to_local() {
  sense::Time t;
  TEST_ASSERT_TRUE(sense::build_time("Oct 23 2026", "14:05:09", -240, &t));
  TEST_ASSERT_EQUAL_UINT16(2026, t.year);
  TEST_ASSERT_EQUAL_UINT8(23, t.day);
  TEST_ASSERT_EQUAL_UINT8(10, t.hour);
  TEST_ASSERT_EQUAL_UINT8(9, t.second);
  TEST_ASSERT_TRUE(sense::build_time("Jan  1 2027", "02:00:00", -240, &t));  // one-digit day
  TEST_ASSERT_EQUAL_UINT16(2026, t.year);
  TEST_ASSERT_EQUAL_UINT8(12, t.month);
  TEST_ASSERT_EQUAL_UINT8(31, t.day);
  TEST_ASSERT_EQUAL_UINT8(22, t.hour);
  TEST_ASSERT_FALSE(sense::build_time("Foo 23 2026", "14:05:09", 0, &t));
  TEST_ASSERT_FALSE(sense::build_time("Oct 23 2026", "14:05", 0, &t));
  TEST_ASSERT_FALSE(sense::build_time("Oct 23 2026", "25:00:00", 0, &t));
}

static void test_clock_is_set_only_when_wrong() {
  const sense::Time build = at(2026, 10, 23, 10, 5, 0);
  TEST_ASSERT_TRUE(sense::clock_needs_set(true, at(2026, 10, 24, 0, 0, 0), build));    // lost
  TEST_ASSERT_TRUE(sense::clock_needs_set(false, at(2026, 0, 24, 0, 0, 0), build));    // junk
  TEST_ASSERT_TRUE(sense::clock_needs_set(false, at(2026, 10, 23, 10, 4, 0), build));  // behind
  TEST_ASSERT_FALSE(sense::clock_needs_set(false, build, build));
  TEST_ASSERT_FALSE(sense::clock_needs_set(false, at(2027, 3, 1, 0, 0, 0), build));  // has run
  TEST_ASSERT_TRUE(sense::clock_needs_set(false, at(2027, 12, 1, 0, 0, 0), build));  // 400+ d
}

static void test_sound_energy_moves_the_mood_with_hysteresis() {
  using sense::Fx;
  TEST_ASSERT_TRUE(sense::pick_fx(0, Fx::kAurora) == Fx::kAurora);
  TEST_ASSERT_TRUE(sense::pick_fx(0.3f, Fx::kAurora) == Fx::kPlasma);
  TEST_ASSERT_TRUE(sense::pick_fx(0.2f, Fx::kPlasma) == Fx::kPlasma);  // inside the band
  TEST_ASSERT_TRUE(sense::pick_fx(0.14f, Fx::kPlasma) == Fx::kAurora);
  TEST_ASSERT_TRUE(sense::pick_fx(0.9f, Fx::kAurora) == Fx::kFire);  // a big jump goes all the way
  TEST_ASSERT_TRUE(sense::pick_fx(0.7f, Fx::kFire) == Fx::kFire);
  TEST_ASSERT_TRUE(sense::pick_fx(0, Fx::kFire) == Fx::kAurora);
  TEST_ASSERT_EQUAL_FLOAT(0.6f, sense::fx_speed(-1));
  TEST_ASSERT_EQUAL_FLOAT(2.0f, sense::fx_speed(5));
  const sense::Daylight day;
  TEST_ASSERT_EQUAL_FLOAT(0.55f, sense::fx_bright(day, 0));
  TEST_ASSERT_EQUAL_FLOAT(1, sense::fx_bright(day, 1));
}

static void test_virtual_clock_keeps_fractions() {
  sense::VirtualClock c;
  for (int i = 0; i < 1000; ++i) c.advance(3, 0.5f);
  TEST_ASSERT_EQUAL_UINT32(1500, c.ms);
  for (int i = 0; i < 100; ++i) c.advance(16, 2.0f);
  TEST_ASSERT_EQUAL_UINT32(1500 + 3200, c.ms);
}

static void test_fade_and_sweep() {
  TEST_ASSERT_EQUAL_FLOAT(0, sense::fade(1000, 1000, 500));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, sense::fade(1250, 1000, 500));
  TEST_ASSERT_EQUAL_FLOAT(1, sense::fade(9000, 1000, 500));
  TEST_ASSERT_EQUAL_FLOAT(1, sense::fade(1000, 1000, 0));
  TEST_ASSERT_EQUAL_UINT8(8, sense::sweep(0, 20000, 8, 80));
  TEST_ASSERT_EQUAL_UINT8(80, sense::sweep(10000, 20000, 8, 80));
  TEST_ASSERT_EQUAL_UINT8(44, sense::sweep(5000, 20000, 8, 80));
  TEST_ASSERT_EQUAL_UINT8(8, sense::sweep(20000, 20000, 8, 80));
}

static void test_buttons_step_scenes_and_levels() {
  sense::Ui ui;
  TEST_ASSERT_FALSE(sense::ui_step(ui, false, false, 4, 100));
  for (int i = 1; i <= 6; ++i) {
    TEST_ASSERT_TRUE(sense::ui_step(ui, true, false, 4, 100 * i));
    TEST_ASSERT_EQUAL_INT(i % 6, static_cast<int>(ui.scene));
    TEST_ASSERT_EQUAL_UINT32(100 * i, ui.started_ms);
  }
  TEST_ASSERT_EQUAL_STRING("ambient", sense::scene_name(ui.scene));
  TEST_ASSERT_EQUAL_STRING("clock", sense::scene_name(sense::Scene::kClock));
  for (int i = 1; i <= 4; ++i) sense::ui_step(ui, false, true, 4, 0);
  TEST_ASSERT_EQUAL_UINT8(0, ui.level);
  TEST_ASSERT_EQUAL_UINT32(600, ui.started_ms);  // B does not restart the scene
}

static void test_longest_sense_line_fits_the_contract() {
  sense::Reading r;
  r.usb = false;
  r.usb_v = r.bat_v = r.imu_c = r.warm_c = r.level = -9999.99f;
  r.usb_ma = r.bat_ma = r.led_ma = -99999;
  r.pmic_c = r.db = r.floor_db = -999.9f;
  r.beats = r.poll_us = 4294967295u;
  char buf[256];
  lab::JsonLine line(buf, sizeof buf, 4294967295u, "sense");
  sense::sense_fields(line, r).finish();
  TEST_ASSERT_TRUE(line.ok());
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"poll_us\":4294967295}"));
}

static void test_column_fills_from_the_bottom() {
  gauge::clear(px, kMatrix);
  gauge::column(px, kMatrix, 2, 0.5f, Pixel(0, 0, 255), Pixel(255, 0, 0));
  for (int y = 0; y < 8; ++y) TEST_ASSERT_EQUAL(y >= 4, lit(2, y));
  TEST_ASSERT_EQUAL_INT(4, lit_count());
  TEST_ASSERT_TRUE(px[kMatrix.index(2, 7)] == Pixel(0, 0, 255));  // bottom color at the bottom
  gauge::column(px, kMatrix, 2, 1, Pixel(0, 0, 255), Pixel(255, 0, 0));
  TEST_ASSERT_TRUE(px[kMatrix.index(2, 0)] == Pixel(255, 0, 0));
  gauge::column(px, kMatrix, 9, 1, Pixel(1, 1, 1), Pixel(1, 1, 1));  // off the matrix: ignored
  gauge::column(px, kMatrix, 3, 0.0625f, Pixel(200, 200, 200), Pixel(200, 200, 200));
  TEST_ASSERT_EQUAL_UINT8(100, px[kMatrix.index(3, 7)].r);  // half a cell: half bright
}

static void test_bar_fills_from_the_left() {
  gauge::clear(px, kMatrix);
  gauge::bar(px, kMatrix, 7, 0.5f, Pixel(255, 0, 0));
  for (int x = 0; x < 8; ++x) TEST_ASSERT_EQUAL(x < 4, lit(x, 7));
  TEST_ASSERT_EQUAL_INT(4, lit_count());
}

static void test_thermo_shows_warmth_and_chip_heat() {
  gauge::thermo(px, kMatrix, 0, 0, 1234);
  TEST_ASSERT_EQUAL_INT(0, lit_count());
  gauge::thermo(px, kMatrix, 1, 0, 1234);
  for (int x = 0; x < 7; ++x) TEST_ASSERT_TRUE(lit(x, 0) && lit(x, 7));
  TEST_ASSERT_FALSE(lit(7, 7));
  gauge::thermo(px, kMatrix, 0, 0.5f, 0);
  TEST_ASSERT_EQUAL_INT(4, lit_count());
  TEST_ASSERT_TRUE(lit(7, 7) && !lit(7, 3));
}

static void test_vu_puts_the_newest_on_the_right() {
  float levels[8] = {};
  bool claps[8] = {};
  levels[5] = 1;  // newest
  claps[4] = true;
  gauge::vu(px, kMatrix, levels, claps, 8, 5);
  TEST_ASSERT_TRUE(lit(7, 0) && lit(7, 7));
  TEST_ASSERT_TRUE(px[kMatrix.index(6, 0)] == Pixel(255, 255, 255));
  TEST_ASSERT_EQUAL_INT(16, lit_count());
}

static void test_meter_draws_the_bottom_two_rows() {
  gauge::clear(px, kMatrix);
  gauge::meter(px, kMatrix, 1, 0.25f);
  for (int x = 0; x < 8; ++x) TEST_ASSERT_TRUE(lit(x, 7));
  TEST_ASSERT_TRUE(lit(1, 6) && !lit(2, 6));
  TEST_ASSERT_EQUAL_INT(10, lit_count());
}

static void test_battery_fill_casing_and_bubbles() {
  gauge::battery(px, kMatrix, 0.5f, false, 0, false, 0);
  TEST_ASSERT_TRUE(lit(0, 0) && lit(7, 0));  // casing
  for (int x = 1; x < 7; ++x) TEST_ASSERT_TRUE(lit(x, 7) && !lit(x, 3));
  TEST_ASSERT_TRUE(px[kMatrix.index(3, 4)].g > px[kMatrix.index(3, 4)].r);
  gauge::battery(px, kMatrix, 0.5f, false, 0, true, 0);
  TEST_ASSERT_TRUE(px[kMatrix.index(3, 4)].r > px[kMatrix.index(3, 4)].g);  // low: red
  Pixel still[64];
  gauge::battery(still, kMatrix, 1, false, 0, false, 500);
  gauge::battery(px, kMatrix, 1, true, 1, false, 500);
  int brighter = 0;
  for (int i = 0; i < 64; ++i) brighter += px[i].r > still[i].r;
  TEST_ASSERT_EQUAL_INT(6, brighter);  // one bubble per body column
}

static void test_ripple_expands_and_ends() {
  gauge::clear(px, kMatrix);
  gauge::ripple(px, kMatrix, 0, 700);
  TEST_ASSERT_TRUE(lit(3, 3) && lit(4, 4));
  TEST_ASSERT_FALSE(lit(0, 0));
  gauge::clear(px, kMatrix);
  gauge::ripple(px, kMatrix, 450, 700);
  TEST_ASSERT_FALSE(lit(3, 3));
  TEST_ASSERT_TRUE(lit(0, 3) || lit(7, 4));
  gauge::clear(px, kMatrix);
  gauge::ripple(px, kMatrix, 700, 700);
  TEST_ASSERT_EQUAL_INT(0, lit_count());
}

static void test_recolor_keeps_shape() {
  gauge::clear(px, kMatrix);
  px[5] = Pixel(0, 255, 0);
  px[6] = Pixel(0, 0, 51);
  gauge::recolor(px, kMatrix, Pixel(100, 200, 250));
  TEST_ASSERT_TRUE(px[5] == Pixel(100, 200, 250));
  TEST_ASSERT_TRUE(px[6] == Pixel(20, 40, 50));
  TEST_ASSERT_EQUAL_INT(2, lit_count());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ema_seeds_then_converges);
  RUN_TEST(test_warmth_settles_then_tracks_changes);
  RUN_TEST(test_led_current_from_the_usb_input);
  RUN_TEST(test_battery_level_is_monotonic_and_clamped);
  RUN_TEST(test_daylight_dims_and_warms_at_night);
  RUN_TEST(test_warm_filter_keeps_dark_dark);
  RUN_TEST(test_calendar_round_trips);
  RUN_TEST(test_build_time_parses_and_moves_to_local);
  RUN_TEST(test_clock_is_set_only_when_wrong);
  RUN_TEST(test_sound_energy_moves_the_mood_with_hysteresis);
  RUN_TEST(test_virtual_clock_keeps_fractions);
  RUN_TEST(test_fade_and_sweep);
  RUN_TEST(test_buttons_step_scenes_and_levels);
  RUN_TEST(test_longest_sense_line_fits_the_contract);
  RUN_TEST(test_column_fills_from_the_bottom);
  RUN_TEST(test_bar_fills_from_the_left);
  RUN_TEST(test_thermo_shows_warmth_and_chip_heat);
  RUN_TEST(test_vu_puts_the_newest_on_the_right);
  RUN_TEST(test_meter_draws_the_bottom_two_rows);
  RUN_TEST(test_battery_fill_casing_and_bubbles);
  RUN_TEST(test_ripple_expands_and_ends);
  RUN_TEST(test_recolor_keeps_shape);
  return UNITY_END();
}
