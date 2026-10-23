// lab_audio::Level on synthetic 16 kHz blocks: a quiet room, tones, clicks and edge cases.
#include <math.h>
#include <unity.h>

#include "lab_audio.h"
#include "lab_fx_math.h"

namespace {
constexpr size_t kN = 256;  // 16 ms at 16 kHz
constexpr uint32_t kBlockMs = 16;
int16_t buf[kN];
uint32_t now = 0;
uint32_t seed = 1;

// Room noise about -60 dBFS, plus an optional tone and DC offset.
void fill(float tone_amp, float dc = 0) {
  for (size_t i = 0; i < kN; ++i) {
    const float noise = (lab::hash01(seed++) - 0.5f) * 64;
    const float tone = tone_amp * sinf(lab::kTau * 440.0f * i / 16000.0f);
    float v = dc + noise + tone;
    v = v > 32767 ? 32767 : (v < -32768 ? -32768 : v);
    buf[i] = static_cast<int16_t>(v);
  }
}

lab::AudioFrame step(lab_audio::Level& l) {
  now += kBlockMs;
  return l.push(buf, kN, now);
}

// Feeds `ms` of one kind of block; returns the number of beats seen.
int run(lab_audio::Level& l, uint32_t ms, float tone_amp, float dc = 0) {
  int beats = 0;
  for (uint32_t t = 0; t < ms; t += kBlockMs) {
    fill(tone_amp, dc);
    if (step(l).beat) ++beats;
  }
  return beats;
}
}  // namespace

void setUp() { now = 0; }
void tearDown() {}

static void test_quiet_room_reads_zero() {
  lab_audio::Level l;
  TEST_ASSERT_EQUAL(0, run(l, 5000, 0));
  fill(0);
  const lab::AudioFrame f = step(l);
  TEST_ASSERT_TRUE(f.level < 0.05f);
  TEST_ASSERT_FALSE(f.beat);
  TEST_ASSERT_TRUE(l.floor_db() < -50 && l.floor_db() > -75);
}

static void test_dc_offset_is_ignored() {
  lab_audio::Level l;
  TEST_ASSERT_EQUAL(0, run(l, 3000, 0, 5000));
  fill(0, 5000);
  TEST_ASSERT_TRUE(step(l).level < 0.05f);
  TEST_ASSERT_TRUE(l.db() < -50);
}

static void test_tone_rises_fast_then_fades_into_floor() {
  lab_audio::Level l;
  run(l, 2000, 0);
  int beats = 0;
  float level = 0;
  for (int i = 0; i < 3; ++i) {  // 48 ms
    fill(8000);
    const lab::AudioFrame f = step(l);
    beats += f.beat;
    level = f.level;
  }
  TEST_ASSERT_TRUE(level > 0.5f);
  TEST_ASSERT_EQUAL(1, beats);  // the tone's start is one onset, not one per block
  TEST_ASSERT_EQUAL(0, run(l, 120000, 8000));
  fill(8000);
  TEST_ASSERT_TRUE(step(l).level < 0.1f);
}

static void test_level_falls_back_after_a_sound() {
  lab_audio::Level l;
  run(l, 2000, 0);
  run(l, 160, 8000);
  run(l, 1000, 0);
  fill(0);
  TEST_ASSERT_TRUE(step(l).level < 0.05f);
}

// A click: one loud block between quiet ones.
static int clicks(lab_audio::Level& l, int count, uint32_t every_ms) {
  int beats = 0;
  for (int c = 0; c < count; ++c) {
    fill(12000);
    beats += step(l).beat;
    beats += run(l, every_ms - kBlockMs, 0);
  }
  return beats;
}

static void test_each_click_is_one_beat() {
  lab_audio::Level l;
  run(l, 2000, 0);
  TEST_ASSERT_EQUAL(10, clicks(l, 10, 400));
}

static void test_clicks_closer_than_the_gap_count_once() {
  lab_audio::Level l;
  run(l, 2000, 0);
  TEST_ASSERT_EQUAL(5, clicks(l, 10, 96));  // gap 150 ms: every other click
}

static void test_full_scale_stays_in_range() {
  lab_audio::Level l;
  run(l, 2000, 0);
  for (int i = 0; i < 50; ++i) {
    for (size_t k = 0; k < kN; ++k) buf[k] = (k / 8) % 2 ? 32767 : -32768;
    const lab::AudioFrame f = step(l);
    TEST_ASSERT_FALSE(isnan(f.level));
    TEST_ASSERT_TRUE(f.level >= 0 && f.level <= 1);
  }
  TEST_ASSERT_TRUE(l.db() <= 0.01f);
}

static void test_zero_blocks_are_skipped() {
  lab_audio::Level l;
  for (size_t k = 0; k < kN; ++k) buf[k] = 0;
  for (int i = 0; i < 100; ++i) {
    const lab::AudioFrame f = step(l);
    TEST_ASSERT_FALSE(isnan(f.level));
    TEST_ASSERT_EQUAL_FLOAT(0, f.level);
    TEST_ASSERT_FALSE(f.beat);
  }
  // The first real block seeds the floor; zeros did not drag it to -100 dB.
  TEST_ASSERT_EQUAL(0, run(l, 1000, 0));
  fill(0);
  TEST_ASSERT_TRUE(step(l).level < 0.05f);
  TEST_ASSERT_EQUAL(0, l.push(buf, 0, now).beat);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_quiet_room_reads_zero);
  RUN_TEST(test_dc_offset_is_ignored);
  RUN_TEST(test_tone_rises_fast_then_fades_into_floor);
  RUN_TEST(test_level_falls_back_after_a_sound);
  RUN_TEST(test_each_click_is_one_beat);
  RUN_TEST(test_clicks_closer_than_the_gap_count_once);
  RUN_TEST(test_full_scale_stays_in_range);
  RUN_TEST(test_zero_blocks_are_skipped);
  return UNITY_END();
}
