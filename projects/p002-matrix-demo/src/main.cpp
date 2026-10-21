#include <Arduino.h>
#include <FastLED.h>
#include <esp_random.h>

#include "config.h"
#include "demo.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "lab_sim.h"

// WiFi (and OTA when OTA_ENABLED) come up only with include/secrets.h present.
#if WIFI_ENABLED && __has_include("secrets.h")
#include "lab_net.h"
#include "secrets.h"
#define LAB_NET_ON 1
#else
#define LAB_NET_ON 0
#endif

namespace {
CRGB leds[NUM_LEDS];
CRGB fading_out[NUM_LEDS];
const lab::Layout layout =
    lab::Layout::matrix(MATRIX_W, MATRIX_H, MATRIX_SERPENTINE, MATRIX_COLUMNS);
constexpr uint32_t kFpsPeriodMs = 5000;
constexpr uint32_t kFrameMs = 16;  // ~60 fps
constexpr uint8_t kLevels[] = {8, 20, 40, 80};
constexpr uint8_t kBootLevel = 2;
constexpr float kLevelG2 = 0.04f;  // tilt below ~11 degrees counts as lying flat

lab_sim::Input gravity;  // matrix space, from the IMU; straight down when flat or without one
bool lying_flat = true;
float raw[3] = {};  // last IMU reading, chip axes, in g

// Every playlist entry renders into px at its own local time.
using Render = void (*)(lab::Pixel* px, uint32_t t_ms);

template <lab_fx::Fn kFx>
void effect(lab::Pixel* px, uint32_t t_ms) {
  kFx(px, layout, lab::AudioFrame{}, t_ms);
}

void marquee(lab::Pixel* px, uint32_t t_ms) { lab_fx::text(px, layout, MARQUEE_TEXT, t_ms); }

// A simulation stepped every kTickMs of its local clock, restarted with a fresh seed whenever that
// clock goes back to zero (each time the effect comes on). kWander: while the M5 lies flat,
// gravity circles slowly instead of pointing down, so the marble keeps rolling.
template <class Sim, uint32_t kTickMs, bool kWander = false>
void sim(lab::Pixel* px, uint32_t t_ms) {
  static Sim s;
  static uint32_t ticks = 0;
  static uint32_t last_ms = UINT32_MAX;
  if (t_ms < last_ms) {
    s.reset(esp_random(), layout);
    ticks = 0;
  }
  last_ms = t_ms;
  lab_sim::Input in = gravity;
  if (kWander && lying_flat) in = {0.5f * cosf(t_ms * 0.0007f), 0.5f * sinf(t_ms * 0.0007f)};
  const uint32_t due = t_ms / kTickMs;
  if (due - ticks > 4) ticks = due - 4;  // after a stall, skip ahead instead of racing
  for (; ticks < due; ++ticks) s.step(in);
  s.draw(px, layout);
}

struct Entry {
  const char* name;
  uint8_t set;
  Render render;
};
const Entry kPlaylist[] = {
    {"rainbow", 1, effect<lab_fx::rainbow>},
    {"plasma", 1, effect<lab_fx::plasma>},
    {"fire", 1, effect<lab_fx::fire>},
    {"rain", 1, effect<lab_fx::rain>},
    {"ripple", 1, effect<lab_fx::ripple>},
    {"heart", 1, effect<lab_fx::heart>},
    {"spiral", 1, effect<lab_fx::spiral>},
    {"twinkle", 1, effect<lab_fx::twinkle>},
    {"invader", 1, effect<lab_fx::invader>},
    {"lava", 1, effect<lab_fx::lava>},
    {"radar", 1, effect<lab_fx::radar>},
    {"bounce", 1, effect<lab_fx::bounce>},
    {"marquee", 1, marquee},
    {"aurora", 2, effect<lab_fx::aurora>},
    {"pacman", 2, effect<lab_fx::pacman>},
    {"life", 2, sim<lab_sim::Life, DEMO_TICK_LIFE_MS>},
    {"starfield", 2, effect<lab_fx::starfield>},
    {"tetris", 2, sim<lab_sim::Tetris, DEMO_TICK_TETRIS_MS>},
    {"sand", 2, sim<lab_sim::Sand, DEMO_TICK_SAND_MS>},
    {"fireworks", 2, effect<lab_fx::fireworks>},
    {"pong", 2, effect<lab_fx::pong>},
    {"ant", 2, sim<lab_sim::Ant, DEMO_TICK_ANT_MS>},
    {"tunnel", 2, effect<lab_fx::tunnel>},
    {"snake", 2, sim<lab_sim::Snake, DEMO_TICK_SNAKE_MS>},
    {"marble", 2, sim<lab_sim::Marble, DEMO_TICK_MARBLE_MS, true>},
    {"kaleidoscope", 2, effect<lab_fx::kaleidoscope>},
    {"snow", 2, effect<lab_fx::snow>},
    {"helix", 2, effect<lab_fx::helix>},
    {"lightning", 2, effect<lab_fx::lightning>},
};
constexpr uint8_t kCount = sizeof kPlaylist / sizeof kPlaylist[0];
uint8_t set_of[kCount];  // filled from kPlaylist in setup()
const demo::Config kDemo{kCount, DEMO_AUTO_MS, DEMO_FADE_MS, kLevels, sizeof kLevels, set_of, 2};
demo::State state;

// Accelerometer to matrix gravity (TILT_* from the calibration in the README).
void read_tilt() {
  if (!lab_board::accel(&raw[0], &raw[1], &raw[2])) return;
  const float gx = TILT_X_SIGN * raw[TILT_SWAP_XY ? 1 : 0];
  const float gy = TILT_Y_SIGN * raw[TILT_SWAP_XY ? 0 : 1];
  lying_flat = gx * gx + gy * gy < kLevelG2;
  gravity = lying_flat ? lab_sim::Input{} : lab_sim::Input{gx, gy};
}

void announce() {
  const uint8_t bri = demo::brightness(state, kDemo);
  FastLED.setBrightness(bri);
  uint8_t pos = 0, of = 0;
  for (uint8_t i = 0; i < kCount; ++i) {
    if (!demo::in_set(state, kDemo, i)) continue;
    ++of;
    if (i <= state.fx) ++pos;
  }
  char set_name[4];
  if (state.set == demo::kAllSets) {
    snprintf(set_name, sizeof set_name, "all");
  } else {
    snprintf(set_name, sizeof set_name, "%u", state.set);
  }
  char line[32];
  lab_board::status_clear();
  lab_board::status_text(LAB_PROJECT);
  lab_board::status_text(kPlaylist[state.fx].name);
  snprintf(line, sizeof line, "set %s  %u/%u", set_name, pos, of);
  lab_board::status_text(line);
  snprintf(line, sizeof line, "bri %u %s", bri, state.autoplay ? "AUTO" : "HOLD");
  lab_board::status_text(line);
  lab_core::Event("fx")
      .str("name", kPlaylist[state.fx].name)
      .num("i", state.fx)
      .str("set", set_name)
      .num("bri", bri)
      .flag("auto", state.autoplay)
      .emit();
}

// The playing effect, crossfaded from the previous one for DEMO_FADE_MS after a change. Each
// effect runs on its own clock, so it always starts from its first frame.
void render(uint32_t now) {
  kPlaylist[state.fx].render(leds, now - state.started_ms);
  const float k = demo::fade(state, kDemo, now);
  if (k >= 1) return;
  kPlaylist[state.prev].render(fading_out, now - state.prev_started_ms);
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
#if BOARD_HAS_IMU
  const bool imu = lab_board::accel(&raw[0], &raw[1], &raw[2]);
  lab_core::Event("imu")
      .flag("ok", imu)
      .real("ax", raw[0])
      .real("ay", raw[1])
      .real("az", raw[2])
      .emit();
#endif
#if DEMO_PROBE
  FastLED.setBrightness(kLevels[kBootLevel]);
  lab_board::status_text(LAB_PROJECT);
  lab_board::status_text("probe:");
  lab_board::status_text("0 red  7 green");
  lab_board::status_text("8 blue 63 white");
#else
  for (uint8_t i = 0; i < kCount; ++i) set_of[i] = kPlaylist[i].set;
  state.level = kBootLevel;
  state.set = DEMO_BOOT_SET;
  state.fx = state.prev = demo::next_in_set(state, kDemo, kCount - 1);
  state.started_ms = state.prev_started_ms = millis();
  announce();
#endif
#if LAB_NET_ON
  lab_net::begin({MDNS_NAME, WIFI_SSID, WIFI_PASSWORD, OTA_ENABLED, OTA_PASSWORD});
#elif WIFI_ENABLED
  lab_core::Event("wifi").str("state", "off").str("why", "no include/secrets.h").emit();
#endif
}

void loop() {
  static uint32_t frames = 0;
  static uint32_t ma_max = 0;
  static uint32_t window_start = millis();
  static uint32_t tilt_logged = 0;
  const uint32_t now = millis();

#if DEMO_PROBE
  lab_fx::wiring(leds, layout, lab::AudioFrame{}, now);
#else
  const lab_board::Buttons b = lab_board::buttons();
  if (demo::step(state, kDemo, {b.a_click, b.a_hold, b.b_click, b.b_hold}, now)) announce();
  read_tilt();
  render(now);
#endif
  // Brightness after FastLED's power limiter, and the current that implies.
  const uint8_t shown = calculate_max_brightness_for_power_vmA(
      leds, NUM_LEDS, FastLED.getBrightness(), POWER_VOLTS, POWER_MAX_MA);
  const uint32_t ma = lab::estimate_ma(leds, NUM_LEDS, shown);
  if (ma > ma_max) ma_max = ma;
  FastLED.show();
  lab_board::status_led((now / 500) % 2);
  ++frames;

  if (now - window_start >= kFpsPeriodMs) {
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("bright", shown)
        .num("ma_est", ma)
        .num("ma_max", ma_max)
        .emit();
    frames = 0;
    ma_max = 0;
    window_start = now;
  }
  if (BOARD_HAS_IMU && TILT_LOG_MS > 0 && now - tilt_logged >= TILT_LOG_MS) {
    lab_core::Event("tilt")
        .real("ax", raw[0])
        .real("ay", raw[1])
        .real("az", raw[2])
        .real("gx", gravity.gx)
        .real("gy", gravity.gy)
        .emit();
    tilt_logged = now;
  }
#if LAB_NET_ON
  lab_net::handle();
#endif
  const uint32_t spent = millis() - now;
  if (spent < kFrameMs) delay(kFrameMs - spent);
}
