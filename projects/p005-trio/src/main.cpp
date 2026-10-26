#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "draw.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "lab_link.h"
#include "law.h"
#include "trio.h"

namespace {
CRGB leds[NUM_LEDS];
#if MATRIX_W > 0
const lab::Layout layout =
    lab::Layout::matrix(MATRIX_W, MATRIX_H, MATRIX_SERPENTINE, MATRIX_COLUMNS);
constexpr uint8_t kRole = trio::kRoleM;
constexpr uint8_t kBrightness = 48;
#else
const lab::Layout layout = lab::Layout::strip(NUM_LEDS);
constexpr uint8_t kRole = trio::kRoleS;
constexpr uint8_t kBrightness = 64;
#endif
constexpr uint32_t kFpsPeriodMs = 5000;
constexpr uint32_t kFrameMs = 10;
constexpr uint32_t kBeatUs = 1000000 / LINK_BEAT_HZ;
constexpr uint32_t kMemberTimeoutUs = 2000000;
constexpr uint32_t kScreenMs = 1000;
constexpr float kFlashS = FLASH_MS * 1e-3f;

trio::Trio group;
trio::Osc osc;
trio::Mode mode = trio::Mode::kUnison;
uint16_t cmd = 0;   // the M5's command count (mode flips, scatters)
uint8_t claps = 0;  // the M5's clap count
bool scattered = false;
uint32_t flashes = 0;  // since the last `sync` event

lab::Pixel slot_color(int slot) {
  static const uint8_t kHue[trio::kSlots] = {24, 128, 200};  // m amber, a teal, b violet
  return lab::hsv(kHue[slot < 0 ? trio::kA : slot], 230, 255);
}

const char* mode_name() { return mode == trio::Mode::kWaltz ? "waltz" : "unison"; }

float coupling() { return mode == trio::Mode::kWaltz ? -BEAT_K : BEAT_K; }

void send_beat() {
  trio::Beat b;
  b.role = kRole;
  b.mode = static_cast<uint8_t>(mode);
  b.cmd = cmd;
  b.scatter = scattered;
  b.claps = claps;
  b.phase = trio::to_u16(osc.phase);
  b.rate_mhz = static_cast<uint16_t>(osc.rate * 1000 + 0.5f);
  lab_link::send(trio::kBeatKind, &b, sizeof b);
}

void scatter() { osc.phase = (esp_random() >> 8) * (1.0f / 16777216.0f); }

void hear(const lab_link::Rx& rx) {
  if (rx.head.kind != trio::kBeatKind || rx.len != sizeof(trio::Beat)) return;
  trio::Beat b;
  memcpy(&b, rx.body, sizeof b);
  group.heard(rx.mac, b, rx.at_us);
  if (kRole != trio::kRoleS || b.role != trio::kRoleM) return;
  static trio::Follow follow;
  const trio::Follow::Act act = follow.update(b);
  mode = static_cast<trio::Mode>(b.mode);
  if (act.scatter) scatter();
  if (act.fire) osc.fire();
}

// The others' phases now, carried forward from their last beats.
int others_now(uint32_t now_us, float* out, int* slots = nullptr) {
  int n = 0;
  for (int i = 0; i < trio::Trio::kOthers; ++i) {
    if (!group.other(i).present) continue;
    if (slots) slots[n] = group.slot_other(i);
    out[n++] = group.other(i).phase_at(now_us);
  }
  return n;
}

void render(uint32_t now_us) {
#if MATRIX_W > 0
  trio::View v;
  v.flash_s = kFlashS;
  v.travel_s = osc.period_s / 3;
  const int self = group.slot_self();
  if (self >= 0) {
    v.present[self] = true;
    v.since_s[self] = osc.since_s();
  }
  for (int i = 0; i < trio::Trio::kOthers; ++i) {
    const trio::Member& m = group.other(i);
    const int s = group.slot_other(i);
    if (s < 0) continue;
    v.present[s] = true;
    const float rate = m.beat.rate_mhz > 0 ? m.beat.rate_mhz * 1e-3f : 1;
    v.since_s[s] = m.phase_at(now_us) / rate;
  }
  for (int s = 0; s < trio::kSlots; ++s) v.color[s] = slot_color(s);
  trio::draw_space(leds, layout, v);
#else
  leds[0] = trio::draw_body(slot_color(group.slot_self()), osc.since_s(), kFlashS, 0);
#endif
}

void report(uint32_t now_us) {
  float x[trio::kSlots];
  x[0] = osc.phase;
  const int n = 1 + others_now(now_us, x + 1);
  lab_core::Event("sync")
      .str("mode", mode_name())
      .real("k", coupling())
      .num("n", n)
      .real("r", trio::order(x, n), 3)
      .num("spread_ms", lroundf(trio::spread(x, n) * osc.period_s * 1000))
      .real("min_gap", trio::min_gap(x, n), 3)
      .num("flashes", flashes)
      .emit();
  flashes = 0;
}

#if BOARD_HAS_TFT
void screen() {
  char line[32];
  lab_board::status_clear();
  const int self = group.slot_self();
  snprintf(line, sizeof line, "trio %c %s", trio::kSlotName[self < 0 ? 0 : self], mode_name());
  lab_board::status_text(line);
  for (const lab_link::Peer& p : lab_link::peers()) {
    if (!p.used) continue;
    char tag[5];
    lab_link::mac_tag(p.mac, tag);
    snprintf(line, sizeof line, "%s %4d %3.0f%%", tag, static_cast<int>(lroundf(p.rssi)),
             100 * p.loss());
    lab_board::status_text(line);
    snprintf(line, sizeof line, "  rtt %.1f ms", p.rtt_us * 1e-3f);
    lab_board::status_text(line);
  }
}
#endif
}  // namespace

void setup() {
  lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});  // applies the power cap
  lab_board::begin();
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(kBrightness);
  lab_core::Event("leds")
      .num("n", NUM_LEDS)
      .num("pin", LED_DATA_PIN)
      .str("driver", lab_core::led_driver())
      .num("cap_ma", POWER_MAX_MA)
      .emit();
  lab_link::begin({LAB_PROJECT, LINK_CHANNEL, LINK_TX_DBM});
  const uint8_t* mac = lab_link::self_mac();
  group.set_self(mac, kRole, TRIO_SWAP_AB);
  // Natural period: each board's own, spread by its MAC, so the law has something to pull.
  const uint32_t id = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];
  osc.period_s = BEAT_PERIOD_MS * 1e-3f * (1 + BEAT_SPREAD * (2 * lab::hash01(id) - 1));
  osc.rate = 1 / osc.period_s;
  scatter();
  lab_core::Event("node")
      .str("role", kRole == trio::kRoleM ? "m" : "s")
      .num("period_ms", lroundf(osc.period_s * 1000))
      .real("k", BEAT_K)
      .emit();
}

void loop() {
  static uint32_t frames = 0;
  static uint32_t window_start = millis();
  static uint32_t last_us = micros();
  static uint32_t beat_us = 0;
  static uint32_t screen_ms = 0;
  const uint32_t frame_start = millis();
  const uint32_t now_us = micros();
  const float dt = (now_us - last_us) * 1e-6f;
  last_us = now_us;

  lab_link::handle(frame_start);
  lab_link::Rx rx;
  while (lab_link::poll(&rx)) hear(rx);
  group.age(now_us, kMemberTimeoutUs);

  float others[trio::Trio::kOthers];
  const int n = others_now(now_us, others);
  flashes += osc.step(dt, coupling(), others, n);
  if (now_us - beat_us >= kBeatUs) {
    beat_us = now_us;
    send_beat();
  }

  render(now_us);
  FastLED.show();
  ++frames;
#if BOARD_HAS_TFT
  if (frame_start - screen_ms >= kScreenMs) {
    screen_ms = frame_start;
    screen();
  }
#endif

  if (frame_start - window_start >= kFpsPeriodMs) {
    const uint8_t shown = calculate_max_brightness_for_power_vmA(
        leds, NUM_LEDS, FastLED.getBrightness(), POWER_VOLTS, POWER_MAX_MA);
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (frame_start - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("bright", shown)
        .num("ma_est", lab::estimate_ma(leds, NUM_LEDS, shown))
        .emit();
    report(now_us);
    lab_link::emit();
    frames = 0;
    window_start = frame_start;
  }
  const uint32_t spent = millis() - frame_start;
  if (spent < kFrameMs) delay(kFrameMs - spent);
}
