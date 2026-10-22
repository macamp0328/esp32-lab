#include <Arduino.h>
#include <FastLED.h>
#include <esp_random.h>

#include "config.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "lab_sim.h"
#include "suite.h"

// WiFi (and OTA when OTA_ENABLED) come up only with include/secrets.h present.
#if WIFI_ENABLED && __has_include("secrets.h")
#include "lab_net.h"
#include "secrets.h"
#define LAB_NET_ON 1
#else
#define LAB_NET_ON 0
#endif

namespace {
CRGB buf[BENCH_MAX_LEDS];
Outputs outputs{buf, {}};
constexpr uint32_t kFpsPeriodMs = 5000;
constexpr uint32_t kScreenPeriodMs = 500;
constexpr uint32_t kButtonPeriodMs = 20;  // each poll costs ~1 ms of I2C on the M5 (bench)
constexpr uint16_t kCounts[] = {NUM_LEDS, 256, BENCH_MAX_LEDS};
const char* const kShowcase[] = {"plasma", "fire", "life"};

// Showcase: one effect at a time, no frame cap; A cycles the LED count.
uint8_t fx = 0;
uint8_t count = 0;  // index into kCounts
uint32_t fx_started = 0;
lab_sim::Life life;
uint32_t life_ticks = 0;

void restart_showcase(uint32_t now) {
  fx_started = now;
  life.reset(esp_random(), bench_layout(kCounts[count]));
  life_ticks = 0;
  outputs.out[0]->setLeds(buf, kCounts[count]);
}

void render(uint32_t now) {
  const lab::Layout& l = bench_layout(kCounts[count]);
  const uint32_t t = now - fx_started;
  if (fx == 0) {
    lab_fx::plasma(buf, l, lab::AudioFrame{}, t);
  } else if (fx == 1) {
    lab_fx::fire(buf, l, lab::AudioFrame{}, t);
  } else {
    for (; life_ticks < t / 160; ++life_ticks) life.step(lab_sim::Input{});
    life.draw(buf, l);
  }
}

// True once a full `bench` line has arrived on the serial port.
bool bench_requested() {
  static char line[16];
  static uint8_t len = 0;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      line[len] = 0;
      len = 0;
      if (strcmp(line, "bench") == 0) return true;
    } else if (len < sizeof line - 1) {
      line[len++] = c;
    }
  }
  return false;
}

void show_status(uint32_t fps) {
  char line[24];
  lab_board::status_clear();
  lab_board::status_text(LAB_PROJECT);
  snprintf(line, sizeof line, "fps %u", static_cast<unsigned>(fps));
  lab_board::status_text(line);
  snprintf(line, sizeof line, "n %u %s", kCounts[count], kShowcase[fx]);
  lab_board::status_text(line);
  lab_board::status_text("A: n  hold A: bench");
}
}  // namespace

void setup() {
  lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});  // applies the power cap
  lab_board::begin();
  outputs.out[0] = &FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(buf, NUM_LEDS);
  add_outputs(outputs);
  FastLED.setMaxRefreshRate(0);  // uncapped: the point of this project
  FastLED.setBrightness(SHOWCASE_BRIGHTNESS);
  lab_core::Event("leds")
      .num("n", NUM_LEDS)
      .num("pin", LED_DATA_PIN)
      .str("driver", lab_core::led_driver())
      .num("cap_ma", POWER_MAX_MA)
      .num("max_n", BENCH_MAX_LEDS)
      .num("outputs", BENCH_OUTPUTS)
      .emit();
  restart_showcase(millis());
#if LAB_NET_ON
  lab_net::begin({MDNS_NAME, WIFI_SSID, WIFI_PASSWORD, OTA_ENABLED, OTA_PASSWORD});
#elif WIFI_ENABLED
  lab_core::Event("wifi").str("state", "off").str("why", "no include/secrets.h").emit();
#endif
}

void loop() {
  static uint32_t frames = 0, screen_frames = 0;
  static uint32_t window_start = millis(), screen_start = millis(), polled = 0;
  uint32_t now = millis();

  lab_board::Buttons b;
  if (now - polled >= kButtonPeriodMs) {
    b = lab_board::buttons();
    polled = now;
  }
  if (b.a_hold || bench_requested()) {
    run_suite(outputs);
    now = millis();
    restart_showcase(now);
    frames = screen_frames = 0;
    window_start = screen_start = now;
    return;
  }
  if (b.a_click) {
    count = (count + 1) % (sizeof kCounts / sizeof kCounts[0]);
    restart_showcase(now);
  }
  if (now - fx_started >= SHOWCASE_FX_MS) {
    fx = (fx + 1) % (sizeof kShowcase / sizeof kShowcase[0]);
    restart_showcase(now);
  }

  render(now);
  FastLED.show();
  lab_board::status_led((now / 500) % 2);
  ++frames;
  ++screen_frames;

  if (now - screen_start >= kScreenPeriodMs) {
    show_status(screen_frames * 1000 / (now - screen_start));
    screen_frames = 0;
    screen_start = now;
  }
  if (now - window_start >= kFpsPeriodMs) {
    // Brightness after FastLED's power limiter, and the current that implies.
    const uint8_t shown = calculate_max_brightness_for_power_vmA(
        buf, kCounts[count], FastLED.getBrightness(), POWER_VOLTS, POWER_MAX_MA);
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("n", kCounts[count])
        .str("fx", kShowcase[fx])
        .num("bright", shown)
        .num("ma_est", lab::estimate_ma(buf, kCounts[count], shown))
        .emit();
    frames = 0;
    window_start = now;
  }
#if LAB_NET_ON
  lab_net::handle();
#endif
}
