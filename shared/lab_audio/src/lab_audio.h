#pragma once
// Turns blocks of 16-bit mic samples into a lab::AudioFrame: level is loudness above an adaptive
// noise floor, beat marks onsets such as claps. bass/mid/treble stay 0 until M3 adds bands.
// Pure and single-precision; host-tested in shared/tests/test/test_audio.

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "lab_audio_frame.h"

namespace lab_audio {

struct LevelConfig {
  float range_db = 40;           // loudness this far above the gate reads as level 1
  float gate_db = 3;             // jitter just above the floor reads as 0
  float floor_rise_db_s = 0.5f;  // a steady sound fades into the floor this fast
  float floor_fall_db_s = 20;    // the floor follows a quieter room this fast
  float release_ms = 150;        // level falls back with this time constant (it rises at once)
  float onset_db = 9;            // an onset is this much louder than the recent average
  float onset_rise_db = 6;       // and this much louder than the block before (a held note is not)
  float onset_avg_ms = 300;      // time constant of that recent average
  float onset_min_level = 0.15f;
  uint32_t onset_gap_ms = 150;  // at most one onset per gap
};

class Level {
 public:
  explicit Level(const LevelConfig& c = {}) : c_(c) {}

  // One block of samples captured at now_ms. An all-zero block (a mic still starting) is skipped.
  lab::AudioFrame push(const int16_t* s, size_t n, uint32_t now_ms) {
    if (n == 0) return frame_;
    float mean = 0;
    for (size_t i = 0; i < n; ++i) mean += s[i];
    mean /= n;
    float sq = 0;
    for (size_t i = 0; i < n; ++i) {
      const float v = s[i] - mean;
      sq += v * v;
    }
    const float rms = sqrtf(sq / n);
    if (rms <= 0) return frame_;
    db_ = 20 * log10f(rms / 32768.0f);
    if (db_ < kMinDb) db_ = kMinDb;

    const float dt = started_ ? static_cast<float>(now_ms - last_ms_) : 0;
    last_ms_ = now_ms;
    if (!started_) {
      started_ = true;
      floor_db_ = avg_db_ = prev_db_ = db_;
    } else if (db_ < floor_db_) {
      floor_db_ = fmaxf(db_, floor_db_ - c_.floor_fall_db_s * dt / 1000);
    } else {
      floor_db_ = fminf(db_, floor_db_ + c_.floor_rise_db_s * dt / 1000);
    }

    const float raw = clamp01((db_ - floor_db_ - c_.gate_db) / c_.range_db);
    if (raw >= frame_.level) {
      frame_.level = raw;
    } else {
      frame_.level += (raw - frame_.level) * (1 - expf(-dt / c_.release_ms));
    }

    frame_.beat = db_ - avg_db_ > c_.onset_db && db_ - prev_db_ > c_.onset_rise_db &&
                  raw > c_.onset_min_level &&
                  (!beat_seen_ || now_ms - last_beat_ms_ >= c_.onset_gap_ms);
    if (frame_.beat) {
      beat_seen_ = true;
      last_beat_ms_ = now_ms;
    }
    avg_db_ += (db_ - avg_db_) * (1 - expf(-dt / c_.onset_avg_ms));
    prev_db_ = db_;
    return frame_;
  }

  float db() const { return db_; }  // last block's loudness, dBFS
  float floor_db() const { return floor_db_; }

 private:
  static constexpr float kMinDb = -100;
  static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

  LevelConfig c_;
  lab::AudioFrame frame_;
  bool started_ = false;
  bool beat_seen_ = false;
  uint32_t last_ms_ = 0;
  uint32_t last_beat_ms_ = 0;
  float db_ = kMinDb;
  float floor_db_ = kMinDb;
  float avg_db_ = kMinDb;
  float prev_db_ = kMinDb;
};

}  // namespace lab_audio
