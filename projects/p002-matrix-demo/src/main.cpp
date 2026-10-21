#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "demo.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"

namespace {
CRGB leds[NUM_LEDS];
CRGB fading_out[NUM_LEDS];
const lab::Layout layout =
    lab::Layout::matrix(MATRIX_W, MATRIX_H, MATRIX_SERPENTINE, MATRIX_COLUMNS);
constexpr uint32_t kFpsPeriodMs = 5000;
constexpr uint32_t kFrameMs = 16;  // ~60 fps
constexpr uint8_t kLevels[] = {8, 20, 40, 80};
constexpr uint8_t kBootLevel = 2;

void marquee(lab::Pixel* px, const lab::Layout& l, const lab::AudioFrame&, uint32_t t_ms) {
  lab_fx::text(px, l, MARQUEE_TEXT, t_ms);
}

struct Entry {
  const char* name;
  lab_fx::Fn fn;
};
const Entry kPlaylist[] = {
    {"rainbow", lab_fx::rainbow}, {"plasma", lab_fx::plasma},   {"fire", lab_fx::fire},
    {"rain", lab_fx::rain},       {"ripple", lab_fx::ripple},   {"heart", lab_fx::heart},
    {"spiral", lab_fx::spiral},   {"twinkle", lab_fx::twinkle}, {"invader", lab_fx::invader},
    {"lava", lab_fx::lava},       {"radar", lab_fx::radar},     {"bounce", lab_fx::bounce},
    {"marquee", marquee},
};
constexpr uint8_t kCount = sizeof kPlaylist / sizeof kPlaylist[0];
const demo::Config kDemo{kCount, DEMO_AUTO_MS, DEMO_FADE_MS, kLevels, sizeof kLevels};
demo::State state;

void announce() {
  const uint8_t bri = demo::brightness(state, kDemo);
  FastLED.setBrightness(bri);
  char line[32];
  lab_board::status_clear();
  lab_board::status_text(LAB_PROJECT);
  snprintf(line, sizeof line, "%u/%u %s", state.fx + 1, kCount, kPlaylist[state.fx].name);
  lab_board::status_text(line);
  snprintf(line, sizeof line, "bri %u %s", bri, state.autoplay ? "AUTO" : "HOLD");
  lab_board::status_text(line);
  lab_core::Event("fx")
      .str("name", kPlaylist[state.fx].name)
      .num("i", state.fx)
      .num("bri", bri)
      .flag("auto", state.autoplay)
      .emit();
}

// The playing effect, crossfaded from the previous one for DEMO_FADE_MS after a change. Each
// effect runs on its own clock, so it always starts from its first frame.
void render(uint32_t now) {
  kPlaylist[state.fx].fn(leds, layout, lab::AudioFrame{}, now - state.started_ms);
  const float k = demo::fade(state, kDemo, now);
  if (k >= 1) return;
  kPlaylist[state.prev].fn(fading_out, layout, lab::AudioFrame{}, now - state.prev_started_ms);
  for (uint16_t i = 0; i < NUM_LEDS; ++i) leds[i] = lab::blend(fading_out[i], leds[i], k);
}
}  // namespace

void setup() {
  lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});  // applies the power cap
  lab_board::begin();
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(leds, NUM_LEDS);
  lab_core::Event("leds")
      .num("n", NUM_LEDS)
      .num("pin", LED_DATA_PIN)
      .str("driver", lab_core::led_driver())
      .num("cap_ma", POWER_MAX_MA)
      .num("w", MATRIX_W)
      .num("h", MATRIX_H)
      .flag("serpentine", MATRIX_SERPENTINE)
      .flag("columns", MATRIX_COLUMNS)
      .emit();
#if DEMO_PROBE
  FastLED.setBrightness(kLevels[kBootLevel]);
  lab_board::status_text(LAB_PROJECT);
  lab_board::status_text("probe:");
  lab_board::status_text("0 red  7 green");
  lab_board::status_text("8 blue 63 white");
#else
  state.level = kBootLevel;
  state.started_ms = state.prev_started_ms = millis();
  announce();
#endif
}

void loop() {
  static uint32_t frames = 0;
  static uint32_t ma_max = 0;
  static uint32_t window_start = millis();
  const uint32_t now = millis();

#if DEMO_PROBE
  lab_fx::wiring(leds, layout, lab::AudioFrame{}, now);
#else
  const lab_board::Buttons b = lab_board::buttons();
  if (demo::step(state, kDemo, {b.a_click, b.a_hold, b.b_click}, now)) announce();
  render(now);
#endif
  // the same model FastLED's power cap applies inside show()
  const uint8_t capped =
      lab::brightness_for_cap(leds, NUM_LEDS, FastLED.getBrightness(), POWER_MAX_MA);
  const uint32_t ma = lab::estimate_ma(leds, NUM_LEDS, capped);
  if (ma > ma_max) ma_max = ma;
  FastLED.show();
  lab_board::status_led((now / 500) % 2);
  ++frames;

  if (now - window_start >= kFpsPeriodMs) {
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("ma_est", ma)
        .num("ma_max", ma_max)
        .emit();
    frames = 0;
    ma_max = 0;
    window_start = now;
  }
  const uint32_t spent = millis() - now;
  if (spent < kFrameMs) delay(kFrameMs - spent);
}
