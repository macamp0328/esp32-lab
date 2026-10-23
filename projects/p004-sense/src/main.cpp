#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "gauges.h"
#include "lab_audio.h"
#include "lab_checks.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "sense.h"

// WiFi (and OTA when OTA_ENABLED) come up only with include/secrets.h present.
#if WIFI_ENABLED && __has_include("secrets.h")
#include "lab_net.h"
#include "secrets.h"
#define LAB_NET_ON 1
#else
#define LAB_NET_ON 0
#endif

static_assert(MIC_BLOCK <= lab_board::kMicBlockMax, "MIC_BLOCK is larger than lab_board's buffer");

namespace {
CRGB leds[NUM_LEDS];
CRGB fading_out[NUM_LEDS];
const lab::Layout layout =
    lab::Layout::matrix(MATRIX_W, MATRIX_H, MATRIX_SERPENTINE, MATRIX_COLUMNS);
constexpr uint32_t kFpsPeriodMs = 5000;
constexpr uint32_t kFrameMs = 16;     // ~60 fps
constexpr uint32_t kScreenMs = 1000;  // TFT refresh (about 20 ms each)
constexpr uint32_t kIdleMs = 3000;    // LEDs dark this long after boot, to measure the board alone
constexpr uint32_t kVuStepMs = 100;
constexpr uint32_t kRippleMs = 700;
// Power scene: the bars' full scale (the LEDs measured ~270 mA at the cap).
constexpr float kMeterFullMa = 300;
constexpr uint8_t kLevels[] = {8, 20, 40, 80};
constexpr uint8_t kBootLevel = 2;

sense::Ui ui;
lab_board::Power power;  // last poll; ok stays false until the first one
bool imu_ok = false;
float imu_c = 0;
uint32_t poll_us = 0;
sense::Warmth warmth;
sense::Ema usb_ma{2000};  // USB input current, read every frame and averaged (it is pulsed)
float idle_ma = 0;        // its mean with the LEDs dark, over the loop's first kIdleMs
uint32_t idle_samples = 0;
uint32_t loop_start = 0;

bool mic_ok = false;
int16_t mic_block[MIC_BLOCK];
lab_audio::Level audio;
lab::AudioFrame sound;
sense::Ema energy{AMBIENT_ENERGY_MS};  // the room's loudness over the last seconds
uint32_t beats = 0;                    // since the last `sense` event
uint32_t last_beat_ms = 0;
bool heard_beat = false;
float vu_level[MATRIX_W] = {};
bool vu_clap[MATRIX_W] = {};
int vu_newest = 0;

sense::Time local;  // last RTC read
bool clock_ok = false;

sense::Fx ambient = sense::Fx::kAurora;
sense::Fx ambient_prev = sense::Fx::kAurora;
uint32_t ambient_since = 0;
sense::VirtualClock ambient_clock;
sense::Ema est_ma{2000};  // lab::estimate_ma less its 1 mA/LED idle, averaged like usb_ma

bool leds_on = true;  // false while off USB: matrix dark and its 5 V switched off
uint32_t usb_lost_ms = 0;

void draw_fx(sense::Fx fx, lab::Pixel* px, uint32_t t_ms) {
  switch (fx) {
    case sense::Fx::kAurora:
      return lab_fx::aurora(px, layout, sound, t_ms);
    case sense::Fx::kPlasma:
      return lab_fx::plasma(px, layout, sound, t_ms);
    case sense::Fx::kLava:
      return lab_fx::lava(px, layout, sound, t_ms);
    default:
      return lab_fx::fire(px, layout, sound, t_ms);
  }
}

const char* fx_name(sense::Fx fx) {
  static const char* const kNames[] = {"aurora", "plasma", "lava", "fire"};
  return kNames[static_cast<int>(fx)];
}

void announce() {
  lab_core::Event("scene")
      .str("name", sense::scene_name(ui.scene))
      .num("i", static_cast<int>(ui.scene))
      .num("bri", kLevels[ui.level])
      .emit();
}

// Power chip and IMU temperature, a few times a second (I2C).
void poll(uint32_t dt_ms) {
  const uint32_t start = micros();
  power = lab_board::power();
  imu_ok = lab_board::imu_temp(&imu_c);
  poll_us = micros() - start;
  if (imu_ok) warmth.push(imu_c, dt_ms);
}

// Every frame: the USB current, averaged; while idle, its LEDs-dark mean less battery charging.
void meter_usb(uint32_t dt_ms, bool idle) {
  const float ma = lab_board::vbus_ma();
  usb_ma.push(ma, dt_ms);
  if (idle) idle_ma += (ma - (power.bat_ma > 0 ? power.bat_ma : 0) - idle_ma) / ++idle_samples;
}

float led_ma() { return sense::led_ma(usb_ma.value, power.bat_ma, idle_ma); }

// The mic's first blocks are its start-up transient, so they are read and dropped while idle.
void listen(uint32_t now, bool idle) {
  if (!mic_ok || !lab_board::mic_read(mic_block, MIC_BLOCK) || idle) return;
  sound = audio.push(mic_block, MIC_BLOCK, now);
  energy.push(sound.level, MIC_BLOCK * 1000.0f / MIC_RATE_HZ);
  if (sound.beat) {
    ++beats;
    last_beat_ms = now;
    heard_beat = true;
    vu_clap[vu_newest] = true;
  }
  if (sound.level > vu_level[vu_newest]) vu_level[vu_newest] = sound.level;
}

void read_clock() {
  const lab_board::DateTime t = lab_board::rtc_now();
  clock_ok = t.ok && !t.lost;
  local = {t.year, t.month, t.day, t.hour, t.minute, t.second};
}

// Sets the RTC from the build time when it lost power or is clearly wrong; reports what it did.
void set_clock_if_needed() {
  const lab_board::DateTime t = lab_board::rtc_now();
  sense::Time build;
  const bool have_build = sense::build_time(__DATE__, __TIME__, CLOCK_UTC_OFFSET_MIN, &build);
  const sense::Time rtc{t.year, t.month, t.day, t.hour, t.minute, t.second};
  const bool set = t.ok && have_build && sense::clock_needs_set(t.lost, rtc, build);
  if (set) {
    lab_board::DateTime b;
    b.year = build.year;
    b.month = build.month;
    b.day = build.day;
    b.hour = build.hour;
    b.minute = build.minute;
    b.second = build.second;
    lab_board::rtc_set(b);
  }
  read_clock();
  char when[20];
  snprintf(when, sizeof when, "%04u-%02u-%02u %02u:%02u:%02u", local.year, local.month, local.day,
           local.hour, local.minute, local.second);
  lab_core::Event("clock")
      .flag("ok", t.ok)
      .flag("lost", t.lost)
      .flag("set", set)
      .str("local", when)
      .emit();
  if (!t.ok) lab_core::error("rtc_read");
}

// Renders the current scene into leds; returns the brightness to show it at.
uint8_t render(uint32_t now, uint32_t dt_ms) {
  const uint8_t level = kLevels[ui.level];
  const sense::Daylight day =
      clock_ok ? sense::daylight(sense::minute_of_day(local)) : sense::Daylight{};
  switch (ui.scene) {
    case sense::Scene::kAmbient: {
      const sense::Fx fx = sense::pick_fx(energy.value, ambient);
      if (fx != ambient) {
        ambient_prev = ambient;
        ambient = fx;
        ambient_since = now;
        lab_core::Event("mood").str("fx", fx_name(fx)).real("energy", energy.value).emit();
      }
      const uint32_t t = ambient_clock.advance(dt_ms, sense::fx_speed(energy.value));
      draw_fx(ambient, leds, t);
      const float k = sense::fade(now, ambient_since, AMBIENT_FADE_MS);
      if (k < 1) {
        draw_fx(ambient_prev, fading_out, t);
        for (uint16_t i = 0; i < NUM_LEDS; ++i) leds[i] = lab::blend(fading_out[i], leds[i], k);
      }
      if (heard_beat) gauge::ripple(leds, layout, now - last_beat_ms, kRippleMs);
      for (uint16_t i = 0; i < NUM_LEDS; ++i) leds[i] = sense::warm_filter(leds[i], day.warm);
      return static_cast<uint8_t>(level * sense::fx_bright(day, sound.level) + 0.5f);
    }
    case sense::Scene::kSound:
      gauge::vu(leds, layout, vu_level, vu_clap, MATRIX_W, vu_newest);
      return level;
    case sense::Scene::kWarmth:
      gauge::thermo(leds, layout,
                    lab::clamp01((warmth.now_c() - THERMO_MIN_C) / (THERMO_MAX_C - THERMO_MIN_C)),
                    lab::clamp01((power.pmic_c - CHIP_MIN_C) / (CHIP_MAX_C - CHIP_MIN_C)), now);
      return level;
    case sense::Scene::kPower: {
      lab_fx::plasma(leds, layout, sound, now);
      gauge::meter(leds, layout, led_ma() / kMeterFullMa, est_ma.value / kMeterFullMa);
      return sense::sweep(now - ui.started_ms, POWER_SWEEP_MS, kLevels[0], kLevels[3]);
    }
    case sense::Scene::kBattery:
      gauge::battery(leds, layout, sense::battery_level(power.bat_v), power.bat_ma > 1,
                     lab::clamp01(power.bat_ma / 100), power.bat_v < BATTERY_LOW_V, now);
      return level;
    default: {
      char hhmm[6];
      snprintf(hhmm, sizeof hhmm, "%02u:%02u", local.hour, local.minute);
      lab_fx::text(leds, layout, clock_ok ? hhmm : "--:--", now - ui.started_ms);
      gauge::recolor(leds, layout, sense::warm_filter(lab::Pixel(170, 210, 255), day.warm));
      return static_cast<uint8_t>(level * day.scale + 0.5f);
    }
  }
}

void screen() {
  char line[32];
  lab_board::status_clear();
  snprintf(line, sizeof line, "%s bri %u", sense::scene_name(ui.scene), kLevels[ui.level]);
  lab_board::status_text(line);
  if (power.usb) {
    snprintf(line, sizeof line, "USB %.2fV %.0fmA", power.usb_v, usb_ma.value);
  } else {
    snprintf(line, sizeof line, "BAT %.2fV %.0fmA", power.bat_v, power.bat_ma);
  }
  lab_board::status_text(line);
  snprintf(line, sizeof line, "bat %.2fV %+.0fmA", power.bat_v, power.bat_ma);
  lab_board::status_text(line);
  snprintf(line, sizeof line, "imu %.1fC %+.2f", warmth.now_c(), warmth.delta_c());
  lab_board::status_text(line);
  snprintf(line, sizeof line, "snd %.0fdB %.2f e%.2f", audio.db(), sound.level, energy.value);
  lab_board::status_text(line);
  if (clock_ok) {
    snprintf(line, sizeof line, "%02u:%02u:%02u", local.hour, local.minute, local.second);
  } else {
    snprintf(line, sizeof line, "clock not set");
  }
  lab_board::status_text(line);
}

// Off USB the matrix goes dark and its 5 V is switched off, so 64 idle LEDs do not drain the
// M5's small battery; FastLED stops sending, so the data line idles low into the unpowered LEDs.
void follow_usb(uint32_t now) {
  static uint8_t differ = 0;  // polls in a row where USB disagrees with leds_on
  if (!power.ok || power.usb == leds_on) {
    differ = 0;
    return;
  }
  if (++differ < 2) return;  // one odd reading is not an unplug
  differ = 0;
  if (!power.usb) {
    FastLED.clear(true);
    lab_board::ext_power(false);
    usb_lost_ms = now;
  } else {
    lab_board::ext_power(true);
  }
  leds_on = power.usb;
  lab_core::Event e("power");
  e.flag("usb", power.usb).real("bat_v", power.bat_v);
  if (power.usb) e.num("off_s", (now - usb_lost_ms) / 1000);
  e.emit();
}
}  // namespace

void setup() {
  lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});  // applies the power cap
  lab_board::begin();
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_ORDER>(leds, NUM_LEDS);
  FastLED.clear(true);
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

  mic_ok = lab_board::mic_begin(MIC_RATE_HZ);
  power = lab_board::power();
  imu_ok = lab_board::imu_temp(&imu_c);
  lab_core::Event("sensors")
      .flag("pmic", power.ok)
      .flag("vbus", power.vbus)
      .flag("acin", power.acin)
      .flag("imu", imu_ok)
      .real("imu_c", imu_c)
      .flag("mic", mic_ok)
      .emit();
  if (!power.ok) lab_core::error("pmic_read");
  if (!mic_ok) lab_core::error("mic_begin");
  set_clock_if_needed();

  ui.scene = static_cast<sense::Scene>(SCENE_BOOT);
  ui.level = kBootLevel;
  announce();
  screen();
  loop_start = millis();
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
  static uint32_t last_ms = millis();
  static uint32_t polled = 0, logged = 0, drawn = 0, vu_stepped = 0;
  const uint32_t now = millis();
  const uint32_t dt = now - last_ms;
  const bool idle = now - loop_start < kIdleMs;
  last_ms = now;

  const lab_board::Buttons b = lab_board::buttons();
  if (sense::ui_step(ui, b.a_click, b.b_click, sizeof kLevels, now)) {
    announce();
    drawn = 0;  // redraw the screen now
  }
  listen(now, idle);
  meter_usb(dt, idle);
  if (now - vu_stepped >= kVuStepMs) {
    vu_newest = (vu_newest + 1) % MATRIX_W;
    vu_level[vu_newest] = 0;
    vu_clap[vu_newest] = false;
    vu_stepped = now;
  }
  if (now - polled >= SENSE_POLL_MS) {
    poll(now - polled);
    polled = now;
    follow_usb(now);
  }

  uint8_t shown = 0;
  uint32_t ma = 0;
  if (leds_on) {
    const uint8_t want = idle ? 0 : render(now, dt);
    if (idle) FastLED.clear();
    FastLED.setBrightness(want);
    // Brightness after FastLED's power limiter, and the current that implies.
    shown = calculate_max_brightness_for_power_vmA(leds, NUM_LEDS, want, POWER_VOLTS, POWER_MAX_MA);
    ma = lab::estimate_ma(leds, NUM_LEDS, shown);
    est_ma.push(ma > NUM_LEDS ? ma - NUM_LEDS : 0, dt);
    if (ma > ma_max) ma_max = ma;
    FastLED.show();
  }
  lab_board::status_led((now / 500) % 2);
  ++frames;

  if (now - drawn >= kScreenMs) {
    read_clock();
    screen();
    drawn = now;
  }
  if (SENSE_LOG_MS > 0 && now - logged >= SENSE_LOG_MS) {
    sense::Reading r;
    r.usb = power.usb;
    r.usb_v = power.usb_v;
    r.usb_ma = usb_ma.value;
    r.bat_v = power.bat_v;
    r.bat_ma = power.bat_ma;
    r.pmic_c = power.pmic_c;
    r.imu_c = imu_c;
    r.warm_c = warmth.delta_c();
    r.level = sound.level;
    r.db = audio.db();
    r.floor_db = audio.floor_db();
    r.beats = beats;
    r.led_ma = led_ma();
    r.poll_us = poll_us;
    lab_core::Event e("sense");
    sense::sense_fields(e, r).emit();
    if (ui.scene == sense::Scene::kPower && leds_on) {  // pairs for estimate vs measured
      lab_core::Event("pwr")
          .num("bri", shown)
          .real("est_ma", est_ma.value, 0)
          .real("led_ma", led_ma(), 0)
          .real("usb_v", power.usb_v)
          .emit();
    }
    beats = 0;
    logged = now;
  }
  if (now - window_start >= kFpsPeriodMs) {
    lab_core::Event("fps")
        .num("fps", frames * 1000 / (now - window_start))
        .num("heap", ESP.getFreeHeap())
        .num("bright", shown)
        .num("ma_est", ma)
        .num("ma_max", ma_max)
        .real("led_ma", led_ma(), 0)
        .real("idle_ma", idle_ma, 0)
        .emit();
    frames = 0;
    ma_max = 0;
    window_start = now;
  }
#if LAB_NET_ON
  lab_net::handle();
#endif
  const uint32_t spent = millis() - now;
  if (spent < kFrameMs) delay(kFrameMs - spent);
}
