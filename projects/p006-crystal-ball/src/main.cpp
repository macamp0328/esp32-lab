#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "demo.h"
#include "lab_checks.h"
#include "lab_clocked.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "schedule.h"
#include "wall_clock.h"

// WiFi, OTA and the clock come up only with include/secrets.h present.
#if WIFI_ENABLED && __has_include("secrets.h")
#include "lab_net.h"
#include "secrets.h"
#define LAB_NET_ON 1
#else
#define LAB_NET_ON 0
#endif

namespace {
CRGB leds[NUM_LEDS];
constexpr uint16_t kRing[] = {NUM_LEDS};
const lab::Layout layout = lab::Layout::rings(kRing);
constexpr uint32_t kFpsPeriodMs = 5000;
constexpr uint32_t kBootFadeMs = 2000;

#if DEMO_MODE
// gain: the pattern's own brightness (0..255). Patterns that light the whole ring sit near the
// 150 mA cap; sparse ones run brighter. FastLED's limiter still caps the total.
struct Pattern {
  const char* name;
  lab_fx::Fn fn;
  uint8_t gain;
};
const Pattern kPatterns[] = {
    {"glow", lab_fx::glow, GLOW_BRIGHTNESS},
    {"breathe", lab_fx::breathe, 72},
    {"ember", lab_fx::ember, 110},
    {"orbit", lab_fx::orbit, 255},
    {"tide", lab_fx::tide, 200},
    {"lava", lab_fx::lava, 110},
    {"aurora", lab_fx::aurora, 120},
    {"twinkle", lab_fx::twinkle, 200},
    {"drift", lab_fx::drift, 60},
};
constexpr uint8_t kPatternCount = sizeof(kPatterns) / sizeof(kPatterns[0]);
CRGB incoming[NUM_LEDS];

void render_pattern(const Pattern& p, CRGB* out, uint32_t now_ms) {
  p.fn(out, layout, lab::AudioFrame{}, now_ms);
  for (uint16_t i = 0; i < NUM_LEDS; ++i) out[i] = lab::scale(out[i], p.gain / 255.0f);
}

// The demo frame at now_ms, crossfading between turns; logs `pattern` when a turn starts.
void render_demo(uint32_t now_ms) {
  const demo::Slot slot =
      demo::at(now_ms, kPatternCount, DEMO_PATTERN_S * 1000UL, DEMO_FADE_S * 1000UL);
  render_pattern(kPatterns[slot.from], leds, now_ms);
  if (slot.mix > 0) {
    render_pattern(kPatterns[slot.to], incoming, now_ms);
    for (uint16_t i = 0; i < NUM_LEDS; ++i) leds[i] = lab::blend(leds[i], incoming[i], slot.mix);
  }
  static int shown = -1;
  if (slot.from != shown) {
    shown = slot.from;
    lab_core::Event("pattern")
        .num("i", slot.from + 1)
        .num("of", kPatternCount)
        .str("name", kPatterns[slot.from].name)
        .num("gain", kPatterns[slot.from].gain)
        .emit();
  }
}
#else
// 0..1: the schedule once the time is known, or the boot glow if that is brighter.
float glow_level(uint32_t now_ms) {
  const float boot = schedule::boot_level(now_ms, BOOT_GLOW_S * 1000UL, kBootFadeMs);
  uint32_t second = 0;
  if (!wall_clock::now_s(&second)) return boot;
  const float timed = schedule::level(second, GLOW_ON_S, GLOW_OFF_S, GLOW_FADE_S);
  return timed > boot ? timed : boot;
}
#endif
}  // namespace

void setup() {
  lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});  // applies the power cap
  lab_board::begin();
  lab_core::add_clocked_leds<LED_TYPE, LED_DATA_PIN, LED_CLOCK_PIN, LED_ORDER,
                             DATA_RATE_MHZ(LED_RATE_MHZ)>(leds, NUM_LEDS);
  FastLED.setBrightness(DEMO_MODE ? 255 : GLOW_BRIGHTNESS);  // demo: each pattern has a gain
  lab_core::Event("leds")
      .num("n", NUM_LEDS)
      .num("pin", LED_DATA_PIN)
      .num("clk", LED_CLOCK_PIN)
      .str("driver", "SPI")
      .num("cap_ma", POWER_MAX_MA)
      .emit();
#if LAB_NET_ON
  lab_net::begin({MDNS_NAME, WIFI_SSID, WIFI_PASSWORD, OTA_ENABLED, OTA_PASSWORD});
#else
  lab_core::Event("wifi").str("state", "off").str("why", "no include/secrets.h").emit();
#endif
}

void loop() {
  static uint32_t frames = 0;
  static uint32_t window_start = millis();
  const uint32_t now = millis();

#if DEMO_MODE
  const float level = 1;
  render_demo(now);
#else
  const float level = glow_level(now);
  lab_fx::glow(leds, layout, lab::AudioFrame{}, now);
  for (CRGB& px : leds) px = lab::scale(px, level);
#endif
  FastLED.show();
  ++frames;

  if (now - window_start >= kFpsPeriodMs) {
    // Brightness after FastLED's power limiter, and the current that implies.
    const uint8_t shown = calculate_max_brightness_for_power_vmA(
        leds, NUM_LEDS, FastLED.getBrightness(), POWER_VOLTS, POWER_MAX_MA);
    lab_core::Event event("fps");
    event.num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("bright", shown)
        .num("ma_est", lab::estimate_ma(leds, NUM_LEDS, shown))
        .real("level", level);
#if LAB_NET_ON
    event.num("rssi", lab_net::rssi());
#endif
    event.emit();
    frames = 0;
    window_start = now;
  }
#if LAB_NET_ON
  lab_net::handle();
  static bool clock_started = false;
  if (!clock_started && lab_net::connected()) {
    clock_started = true;
    wall_clock::begin(CLOCK_TZ, CLOCK_SERVER);
  }
  wall_clock::poll();
#endif
  delay(10);
}
