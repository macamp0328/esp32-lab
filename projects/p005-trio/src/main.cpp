#include <Arduino.h>
#include <FastLED.h>
#include <Preferences.h>

#include "config.h"
#include "draw.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "lab_link.h"
#include "law.h"
#include "loom.h"
#include "mood.h"
#include "trio.h"
#if AUDIO_MODE == 2
#include "lab_audio.h"
#endif

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
constexpr uint32_t kButtonsMs = 20;      // M5.update reads the power chip over I2C (lessons)
constexpr uint32_t kMicSettleMs = 1000;  // the mic's first blocks are its start-up transient
constexpr float kFlashS = FLASH_MS * 1e-3f;

trio::Trio group;
trio::Osc osc;
trio::Mode mode = static_cast<trio::Mode>(BEAT_BOOT_MODE);
uint16_t cmd = 0;   // the M5's command count (mode flips, scatters)
uint8_t claps = 0;  // the M5's clap count
bool scattered = false;
uint32_t flashes = 0;  // since the last `sync` event
float stir[3] = {};    // each edge's disturbance as shown: rises fast, falls slowly
float stir_raw[3] = {};

trio::MoodConfig mood_cfg() {
  trio::MoodConfig c;
  c.lap_s = MOOD_LAP_MIN * 60.0f;
  c.detune = MOOD_DETUNE;
  c.k_calm = MOOD_K_CALM;
  c.k_storm = MOOD_K_STORM;
  c.calm_below = MOOD_CALM_BELOW;
  c.stir_above = MOOD_STIR_ABOVE;
  c.room_s = MOOD_ROOM_S;
  c.clap_kick = MOOD_CLAP_KICK;
  return c;
}
const trio::MoodConfig kMood = mood_cfg();
trio::Hue hue;
trio::Room room;
Preferences prefs;
float hues[trio::kSlots] = {};  // each corner's hue this frame (absent corners keep their last)
float r_hue = 1;                // how far the three hues agree

#if BOARD_HAS_TFT
constexpr int kScreenW = 240, kScreenH = 135;
trio::Loom loom;
trio::Weft weft;
bool show_loom = true;  // B click: the loom, or the diagram with the numbers
#endif

#if AUDIO_MODE == 2
bool mic_ok = false;
int16_t mic_block[MIC_BLOCK];
lab_audio::Level audio;
uint32_t last_clap_ms = 0;
#endif

lab::Pixel slot_color(int slot) {
  if (slot < 0) slot = trio::kA;
  const float h = trio::shown_hue(hues[slot], slot, r_hue, MOOD_FAN_DEG / 360.0f);
  return lab::hsv(static_cast<uint8_t>(h * 256), 230, 255);
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
  b.hue = trio::to_u16(hue.h);
  trio::fill_heard(group, &b, FIELD_FLOOR_DB, FIELD_SPAN_DB);
  lab_link::send(trio::kBeatKind, &b, sizeof b);
}

void scatter() { osc.phase = (esp_random() >> 8) * (1.0f / 16777216.0f); }

void announce(const char* what, const char* from) {
  lab_core::Event(what)
      .str("mode", mode_name())
      .num("cmd", cmd)
      .num("claps", claps)
      .str("from", from)
      .emit();
}

#if BOARD_HAS_TFT
void draw_loom_column(int col, uint32_t index) {
  lab::Pixel px[kScreenH];
  trio::thread_column(loom.at(col), index, px, kScreenH);
  lab_board::screen_column(col, 0, px, kScreenH);
}

void draw_cursor() {
  const int col = static_cast<int>(loom.woven() % trio::Loom::kMax);
  lab::Pixel px[kScreenH];
  for (int y = 0; y < kScreenH; ++y)
    px[y] = (y & 3) == 0 ? lab::Pixel(90, 90, 90) : lab::Pixel(0, 0, 0);
  lab_board::screen_column(col, 0, px, kScreenH);
}

void draw_loom() {
  lab_board::screen_fill(0, 0, 0);
  const uint32_t first = loom.woven() - loom.columns();
  for (uint32_t i = first; i < loom.woven(); ++i) {
    draw_loom_column(static_cast<int>(i % trio::Loom::kMax), i);
  }
  draw_cursor();
}

void screen();

// The M5 carries Miles' presses to the others: A click scatters, A hold flips the beat's law.
// B click is the M5's own: the TFT shows the loom or the diagram.
void press() {
  const lab_board::Buttons b = lab_board::buttons();
  if (b.b_click) {
    show_loom = !show_loom;
    if (show_loom) {
      draw_loom();
    } else {
      screen();
    }
  }
  if (!b.a_click && !b.a_hold) return;
  ++cmd;
  scattered = b.a_click;
  if (b.a_hold) mode = mode == trio::Mode::kWaltz ? trio::Mode::kUnison : trio::Mode::kWaltz;
  if (scattered) scatter();
  send_beat();
  announce(scattered ? "scatter" : "mode", "self");
}
#endif

#if AUDIO_MODE == 2
// A clap is a flash everyone sees: the M5 fires now and tells the others to.
void listen(uint32_t now_ms) {
  if (!mic_ok || !lab_board::mic_read(mic_block, MIC_BLOCK)) return;
  const lab::AudioFrame f = audio.push(mic_block, MIC_BLOCK, now_ms);
  if (!f.beat || now_ms < kMicSettleMs || now_ms - last_clap_ms < CLAP_GAP_MS) return;
  last_clap_ms = now_ms;
  ++claps;
  osc.fire();
  room.clap(kMood);
  weft.clap();
  send_beat();
  announce("clap", "self");
}
#endif

void hear(const lab_link::Rx& rx) {
  if (rx.head.kind != trio::kBeatKind || rx.len != sizeof(trio::Beat)) return;
  trio::Beat b;
  memcpy(&b, rx.body, sizeof b);
  group.heard(rx.mac, b, rx.at_us, rx.rssi);
  if (kRole != trio::kRoleS || b.role != trio::kRoleM) return;
  static trio::Follow follow;
  const bool flipped = follow.seen && static_cast<trio::Mode>(b.mode) != mode;
  const trio::Follow::Act act = follow.update(b);
  mode = static_cast<trio::Mode>(b.mode);
  cmd = b.cmd;
  claps = b.claps;
  if (act.scatter) {
    scatter();
    announce("scatter", "m");
  } else if (flipped) {
    announce("mode", "m");
  }
  if (act.fire) {
    osc.fire();
    room.clap(kMood);
    announce("clap", "m");
  }
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
  for (int e = 0; e < 3; ++e) v.stir[e] = stir[e];
  trio::draw_space(leds, layout, v);
#else
  leds[0] = trio::draw_body(slot_color(group.slot_self()), osc.since_s(), kFlashS, MOOD_GLOW);
#endif
}

float beat_order(uint32_t now_us) {
  float x[trio::kSlots];
  x[0] = osc.phase;
  return trio::order(x, 1 + others_now(now_us, x + 1));
}

#if BOARD_HAS_TFT
// Once a second: what this span of the piece is; every LOOM_THREAD_S: a thread of it, woven.
void gather(uint32_t now_us) {
  bool present[trio::kSlots] = {};
  const int self = group.slot_self();
  if (self >= 0) present[self] = true;
  for (int i = 0; i < trio::Trio::kOthers; ++i) {
    if (group.slot_other(i) >= 0) present[group.slot_other(i)] = true;
  }
  float shown[trio::kSlots];  // as the matrix shows them: agreement fanned out
  for (int s = 0; s < trio::kSlots; ++s) {
    shown[s] = trio::shown_hue(hues[s], s, r_hue, MOOD_FAN_DEG / 360.0f);
  }
  weft.hues(shown, present);
  weft.order(beat_order(now_us));
}

void weave() {
  const trio::Thread t = weft.finish();
  const uint32_t index = loom.woven();
  const int col = loom.add(t);
  lab::Pixel px[kScreenH];
  trio::thread_column(t, index, px, kScreenH);
  if (show_loom) {
    lab_board::screen_column(col, 0, px, kScreenH);
    draw_cursor();
  }
  char state[17], column[4 * LOOM_SAMPLES + 1];
  const uint8_t* raw = reinterpret_cast<const uint8_t*>(&t);
  for (int i = 0; i < 8; ++i) snprintf(state + 2 * i, 3, "%02x", raw[i]);
  trio::column_hex(px, kScreenH, LOOM_SAMPLES, column);
  lab_core::Event("weave").num("i", index).str("s", state).str("c", column).emit();
}
#endif

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

void sense_space(float dt_s) {
  trio::edges(group, FIELD_FLOOR_DB, FIELD_SPAN_DB, stir_raw);
  for (int e = 0; e < 3; ++e) stir[e] = trio::follow(stir[e], stir_raw[e], dt_s, 0.1f, 1.0f);
}

// The mood tempo: the room's restlessness sets the sign of the law on hue.
void feel(float dt_s) {
  room.push((stir_raw[0] + stir_raw[1] + stir_raw[2]) / 3, dt_s, kMood);
  float others[trio::Trio::kOthers];
  int n = 0;
  const int self = group.slot_self();
  if (self >= 0) hues[self] = hue.h;
  for (int i = 0; i < trio::Trio::kOthers; ++i) {
    const trio::Member& m = group.other(i);
    if (!m.present) continue;
    others[n++] = trio::from_u16(m.beat.hue);
    const int s = group.slot_other(i);
    if (s >= 0) hues[s] = trio::from_u16(m.beat.hue);
  }
  hue.step(dt_s, trio::mood_k(room.value, kMood), others, n);
  float all[trio::kSlots] = {hue.h};
  for (int i = 0; i < n; ++i) all[i + 1] = others[i];
  r_hue = trio::order(all, n + 1);
}

void log_mood() {
  float all[trio::kSlots] = {hue.h};
  int n = 1;
  for (int i = 0; i < trio::Trio::kOthers; ++i) {
    if (group.other(i).present) all[n++] = trio::from_u16(group.other(i).beat.hue);
  }
  lab_core::Event("mood")
      .num("hm", lroundf(hues[trio::kM] * 360))
      .num("ha", lroundf(hues[trio::kA] * 360))
      .num("hb", lroundf(hues[trio::kB] * 360))
      .real("k_milli", trio::mood_k(room.value, kMood) * 1000, 2)
      .real("room", room.value, 3)
      .real("r", r_hue, 3)
      .num("gap_deg", lroundf(trio::min_gap(all, n) * 360))
      .num("spread_deg", lroundf(trio::spread(all, n) * 360))
      .emit();
}

// The three edges as shown, and the raw numbers of this board's own links (to tune the floor).
void log_space() {
  lab_core::Event ev("field");
  ev.real("ma", stir[0]).real("ab", stir[1]).real("bm", stir[2]);
  static const char* const kKeys[2][4] = {{"p0", "s0", "d0", "c0"}, {"p1", "s1", "d1", "c1"}};
  int n = 0;
  for (int i = 0; i < trio::Trio::kOthers; ++i) {
    const trio::Member& m = group.other(i);
    if (!m.present) continue;
    char tag[5];
    lab_link::mac_tag(m.mac, tag);
    ev.str(kKeys[n][0], tag)
        .real(kKeys[n][1], m.link.slow(), 1)
        .real(kKeys[n][2], m.link.dev())
        .real(kKeys[n][3], m.link.calm());
    ++n;
  }
  ev.emit();
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
  // Mood: this board's own drift, and the hue it left off with (NVS), else one from its MAC.
  hue.set_drift(lab::hash01(id ^ 0x85ebca6bu), kMood);
  prefs.begin("p005", false);
  const float saved = prefs.getFloat("hue", -1);
  hue.h = saved >= 0 && saved < 1 ? saved : lab::hash01(id ^ 0x9e3779b9u);
  lab_core::Event("node")
      .str("role", kRole == trio::kRoleM ? "m" : "s")
      .num("period_ms", lroundf(osc.period_s * 1000))
      .real("k", BEAT_K)
      .num("hue_deg", lroundf(hue.h * 360))
      .flag("hue_saved", saved >= 0)
      .emit();
#if AUDIO_MODE == 2
  mic_ok = lab_board::mic_begin(MIC_RATE_HZ);
  if (!mic_ok) lab_core::error("mic_begin");
#endif
#if BOARD_HAS_TFT
  draw_loom();
#endif
}

void loop() {
  static uint32_t frames = 0;
  static uint32_t window_start = millis();
  static uint32_t last_us = micros();
  static uint32_t beat_us = 0;
  static uint32_t screen_ms = 0;
  static uint32_t buttons_ms = 0;
  static uint32_t field_ms = 0;
  static uint32_t mood_ms = 0;
  static uint32_t saved_ms = 0;
  static uint32_t gather_ms = 0;
  static uint32_t thread_ms = 0;
  const uint32_t frame_start = millis();
  lab_link::handle(frame_start);
  lab_link::Rx rx;
  while (lab_link::poll(&rx)) hear(rx);
  const uint32_t now_us = micros();  // after the drain: no packet is stamped later than now
  const float dt = (now_us - last_us) * 1e-6f;
  last_us = now_us;
  group.age(now_us, kMemberTimeoutUs);
#if BOARD_HAS_TFT
  if (frame_start - buttons_ms >= kButtonsMs) {
    buttons_ms = frame_start;
    press();
  }
#endif
#if AUDIO_MODE == 2
  listen(frame_start);
#endif

  float others[trio::Trio::kOthers];
  const int n = others_now(now_us, others);
  flashes += osc.step(dt, coupling(), others, n);
  sense_space(dt);
  feel(dt);
  if (frame_start - field_ms >= FIELD_LOG_MS) {
    field_ms = frame_start;
    log_space();
  }
  if (frame_start - mood_ms >= MOOD_LOG_MS) {
    mood_ms = frame_start;
    log_mood();
  }
  if (frame_start - saved_ms >= MOOD_SAVE_MS) {
    saved_ms = frame_start;
    prefs.putFloat("hue", hue.h);
  }
#if BOARD_HAS_TFT
  weft.edges(stir);
  if (frame_start - gather_ms >= 1000) {
    gather_ms = frame_start;
    gather(now_us);
  }
  if (frame_start - thread_ms >= LOOM_THREAD_S * 1000u) {
    thread_ms = frame_start;
    weave();
  }
#endif
  if (now_us - beat_us >= kBeatUs) {
    beat_us = now_us;
    send_beat();
  }

  render(now_us);
  FastLED.show();
  ++frames;
#if BOARD_HAS_TFT
  if (!show_loom && frame_start - screen_ms >= kScreenMs) {
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
