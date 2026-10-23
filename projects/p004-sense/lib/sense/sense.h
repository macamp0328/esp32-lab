#pragma once
// p004's sensor logic: smoothing, warmth from the IMU temperature, time of day, when to set the
// RTC, the ambient mood and the scene picker. Pure, so it is host-tested (test/test_sense).

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "lab_fx_math.h"

namespace sense {

// Exponential moving average with time constant tau_ms; the first sample seeds it.
struct Ema {
  float tau_ms;
  float value = 0;
  bool seeded = false;

  float push(float x, float dt_ms) {
    if (!seeded) {
      value = x;
      seeded = true;
    } else {
      value += (x - value) * (1 - expf(-dt_ms / tau_ms));
    }
    return value;
  }
};

// The IMU die temperature, smoothed, and how far it is from its average over the last minute. On
// the M5 it mostly reads the board's own heat, which follows the LED current (docs/lessons.md),
// so p004 shows and logs it rather than steering by it. The average tracks the smoothed value for
// settle_ms first, so one noisy first sample cannot bias it for minutes.
struct Warmth {
  Ema fast{3000};
  Ema slow{60000};
  float settle_ms = 15000;  // 5 time constants of fast
  float age_ms = 0;

  void push(float c, float dt_ms) {
    fast.push(c, dt_ms);
    age_ms += dt_ms;
    if (age_ms < settle_ms) {
      slow.value = fast.value;
      slow.seeded = true;
    } else {
      slow.push(fast.value, dt_ms);
    }
  }
  float now_c() const { return fast.value; }
  float delta_c() const { return fast.value - slow.value; }
};

// LED current seen on the USB input: what the board draws beyond its LEDs-dark idle, less what
// goes into the battery. Comparable with lab::estimate_ma minus its 1 mA per LED idle.
inline float led_ma(float usb_ma, float bat_ma, float idle_ma) {
  const float v = usb_ma - (bat_ma > 0 ? bat_ma : 0) - idle_ma;
  return v > 0 ? v : 0;
}

// Charge left, 0..1, from a resting single-cell LiPo voltage (rough; under load it reads low).
inline float battery_level(float v) {
  constexpr float kV[] = {3.30f, 3.60f, 3.70f, 3.80f, 3.95f, 4.10f};
  constexpr float kL[] = {0.00f, 0.10f, 0.35f, 0.60f, 0.80f, 1.00f};
  constexpr int kN = sizeof kV / sizeof kV[0];
  if (v <= kV[0]) return 0;
  for (int i = 1; i < kN; ++i) {
    if (v < kV[i]) return kL[i - 1] + (kL[i] - kL[i - 1]) * (v - kV[i - 1]) / (kV[i] - kV[i - 1]);
  }
  return 1;
}

// Time of day as a brightness scale and an amount of warm tint: full and neutral 08:00-18:00,
// dim and amber 22:00-06:00, ramping in between.
struct Daylight {
  float scale = 1;
  float warm = 0;
};

inline Daylight daylight(int minute_of_day) {
  const int m = ((minute_of_day % 1440) + 1440) % 1440;
  float night;
  if (m < 6 * 60) {
    night = 1;
  } else if (m < 8 * 60) {
    night = 1 - (m - 6 * 60) / 120.0f;
  } else if (m < 18 * 60) {
    night = 0;
  } else if (m < 22 * 60) {
    night = (m - 18 * 60) / 240.0f;
  } else {
    night = 1;
  }
  return {1 - 0.65f * night, 0.7f * night};
}

// Candle-like filter: keeps red, takes green and blue down by `warm` (0..1). Dark stays dark.
inline lab::Pixel warm_filter(lab::Pixel p, float warm) {
  warm = lab::clamp01(warm);
  return lab::Pixel(p.r, static_cast<uint8_t>(p.g * (1 - 0.45f * warm) + 0.5f),
                    static_cast<uint8_t>(p.b * (1 - 0.85f * warm) + 0.5f));
}

// Wall-clock time, local.
struct Time {
  uint16_t year = 2000;
  uint8_t month = 1, day = 1, hour = 0, minute = 0, second = 0;
};

inline bool valid(const Time& t) {
  return t.year >= 2000 && t.year <= 2099 && t.month >= 1 && t.month <= 12 && t.day >= 1 &&
         t.day <= 31 && t.hour < 24 && t.minute < 60 && t.second < 60;
}

// Days since 1970-01-01 (proleptic Gregorian; H. Hinnant's days_from_civil).
inline int32_t days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

inline int64_t seconds(const Time& t) {
  return static_cast<int64_t>(days_from_civil(t.year, t.month, t.day)) * 86400 + t.hour * 3600 +
         t.minute * 60 + t.second;
}

inline Time from_seconds(int64_t s) {
  int64_t days = s >= 0 ? s / 86400 : (s - 86399) / 86400;
  const int64_t rem = s - days * 86400;
  Time t;
  t.hour = static_cast<uint8_t>(rem / 3600);
  t.minute = static_cast<uint8_t>(rem % 3600 / 60);
  t.second = static_cast<uint8_t>(rem % 60);
  days += 719468;  // civil_from_days
  const int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  const int64_t doe = days - era * 146097;
  const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int64_t mp = (5 * doy + 2) / 153;
  const int64_t m = mp < 10 ? mp + 3 : mp - 9;
  t.year = static_cast<uint16_t>(yoe + era * 400 + (m <= 2));
  t.month = static_cast<uint8_t>(m);
  t.day = static_cast<uint8_t>(doy - (153 * mp + 2) / 5 + 1);
  return t;
}

inline int minute_of_day(const Time& t) { return t.hour * 60 + t.minute; }

// The compiler's __DATE__ ("Oct 23 2026") and __TIME__ ("14:05:09"), moved by offset_min.
inline bool build_time(const char* date, const char* time, int offset_min, Time* out) {
  static const char kMonths[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  if (strlen(date) != 11 || strlen(time) != 8) return false;
  const char mon[4] = {date[0], date[1], date[2], 0};
  const char* m = strstr(kMonths, mon);
  if (!m || (m - kMonths) % 3) return false;
  auto num = [](const char* s, int n) {
    int v = 0;
    for (int i = 0; i < n; ++i) v = v * 10 + (s[i] == ' ' ? 0 : s[i] - '0');
    return v;
  };
  Time t;
  t.month = static_cast<uint8_t>((m - kMonths) / 3 + 1);
  t.day = static_cast<uint8_t>(num(date + 4, 2));
  t.year = static_cast<uint16_t>(num(date + 7, 4));
  t.hour = static_cast<uint8_t>(num(time, 2));
  t.minute = static_cast<uint8_t>(num(time + 3, 2));
  t.second = static_cast<uint8_t>(num(time + 6, 2));
  if (!valid(t)) return false;
  *out = from_seconds(seconds(t) + offset_min * 60);
  return true;
}

// Set the RTC from the build when it lost power, holds junk, runs behind the build (so it is
// wrong), or more than 400 days ahead of it. Otherwise keep it: it has been running since.
inline bool clock_needs_set(bool lost, const Time& rtc, const Time& build) {
  if (lost || !valid(rtc)) return true;
  const int64_t r = seconds(rtc), b = seconds(build);
  return r < b || r > b + 400LL * 86400;
}

// Ambient: the room's sound energy (0..1, loudness averaged over seconds) picks the effect, calmest
// first. Thresholds have hysteresis so a reading near one does not flip the effect back and forth.
enum class Fx : uint8_t { kAurora, kPlasma, kLava, kFire, kCount };

inline Fx pick_fx(float energy, Fx current) {
  constexpr float kUp[] = {0.25f, 0.5f, 0.75f};  // energy to move up past fx 0, 1, 2
  constexpr float kHysteresis = 0.1f;
  int i = static_cast<int>(current);
  while (i < 3 && energy >= kUp[i]) ++i;
  while (i > 0 && energy < kUp[i - 1] - kHysteresis) --i;
  return static_cast<Fx>(i);
}

// Animation speed for an energy: calm in a quiet room, about three times as fast when loud.
inline float fx_speed(float energy) { return 0.6f + 1.4f * lab::clamp01(energy); }

// Brightness factor: the time of day, swelling with sound.
inline float fx_bright(const Daylight& d, float level) {
  return d.scale * (0.55f + 0.45f * lab::clamp01(level));
}

// Time that advances at a variable rate, so changing the speed never jumps the animation.
struct VirtualClock {
  uint32_t ms = 0;
  float frac = 0;

  uint32_t advance(uint32_t dt_ms, float speed) {
    const float d = dt_ms * speed + frac;
    const uint32_t whole = static_cast<uint32_t>(d);
    frac = d - whole;
    ms += whole;
    return ms;
  }
};

// Crossfade progress 0..1 from start_ms over fade_ms.
inline float fade(uint32_t now, uint32_t start_ms, uint32_t fade_ms) {
  if (fade_ms == 0) return 1;
  return lab::clamp01(static_cast<float>(now - start_ms) / fade_ms);
}

// Scene picker: A steps to the next scene, B to the next brightness level; both wrap.
enum class Scene : uint8_t { kAmbient, kSound, kWarmth, kPower, kBattery, kClock, kCount };

inline const char* scene_name(Scene s) {
  static const char* const kNames[] = {"ambient", "sound", "warmth", "power", "battery", "clock"};
  return s < Scene::kCount ? kNames[static_cast<int>(s)] : "?";
}

struct Ui {
  Scene scene = Scene::kAmbient;
  uint8_t level = 0;
  uint32_t started_ms = 0;  // when the scene came on
};

// True when the scene or level changed.
inline bool ui_step(Ui& ui, bool a_click, bool b_click, uint8_t level_count, uint32_t now) {
  if (a_click) {
    ui.scene =
        static_cast<Scene>((static_cast<int>(ui.scene) + 1) % static_cast<int>(Scene::kCount));
    ui.started_ms = now;
  }
  if (b_click && level_count > 0) ui.level = (ui.level + 1) % level_count;
  return a_click || (b_click && level_count > 0);
}

// One `sense` event. Line is lab_core::Event on the board and lab::JsonLine in the host tests, so
// a test checks the longest line still fits the serial contract.
struct Reading {
  bool usb = false;
  float usb_v = 0, usb_ma = 0, bat_v = 0, bat_ma = 0, pmic_c = 0;
  float imu_c = 0, warm_c = 0;            // IMU temperature and Warmth::delta_c()
  float level = 0, db = 0, floor_db = 0;  // lab_audio::Level
  uint32_t beats = 0;                     // onsets since the previous event
  float led_ma = 0;
  uint32_t poll_us = 0;  // time the last power-chip + IMU poll took
};

template <class Line>
Line& sense_fields(Line& l, const Reading& r) {
  return l.flag("usb", r.usb)
      .real("usb_v", r.usb_v)
      .real("usb_ma", r.usb_ma, 0)
      .real("bat_v", r.bat_v)
      .real("bat_ma", roundf(r.bat_ma) + 0.0f, 0)  // + 0: no "-0" when idle
      .real("pmic_c", r.pmic_c, 1)
      .real("imu_c", r.imu_c)
      .real("warm", r.warm_c)
      .real("lvl", r.level)
      .real("db", r.db, 1)
      .real("floor", r.floor_db, 1)
      .num("beats", r.beats)
      .real("led_ma", r.led_ma, 0)
      .num("poll_us", r.poll_us);
}

// Power scene: a triangle wave between lo and hi over period_ms.
inline uint8_t sweep(uint32_t t_ms, uint32_t period_ms, uint8_t lo, uint8_t hi) {
  if (period_ms == 0) return hi;
  const float p = static_cast<float>(t_ms % period_ms) / period_ms;
  const float tri = p < 0.5f ? p * 2 : 2 - p * 2;
  return static_cast<uint8_t>(lo + (hi - lo) * tri + 0.5f);
}

}  // namespace sense
