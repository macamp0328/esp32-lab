#pragma once
// Lab-wide runtime: serial contract and the power cap. On-target parts need Arduino; the pure
// parts (lab_json.h, lab_power.h, lab_audio_frame.h) are host-tested.
// Usage: lab_core::begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA});
//        lab_core::Event("leds").num("n", NUM_LEDS).emit();

#include <stdint.h>

#include "lab_audio_frame.h"
#include "lab_json.h"
#include "lab_power.h"

#ifdef ARDUINO

namespace lab_core {

constexpr size_t kLineMax = 256;

struct Setup {
  const char* project;  // LAB_PROJECT
  const char* fw;       // LAB_FW (git sha, set by tools/pio_fw.py)
  uint8_t volts;        // POWER_VOLTS
  uint32_t max_ma;      // POWER_MAX_MA
};

// Starts Serial (waits up to 2 s for USB CDC), emits `boot` and `hw` (measured chip facts), and
// applies the FastLED power cap, so no project can forget it.
void begin(const Setup& setup);

// FastLED's driver for clockless LEDs; the build fails unless it is RMT (AGENTS.md hardware rules).
const char* led_driver();

// One serial-contract line. Emits on emit(); an overflowing line becomes an `err` event.
class Event {
 public:
  explicit Event(const char* ev);
  Event& str(const char* key, const char* value) {
    line_.str(key, value);
    return *this;
  }
  Event& num(const char* key, long long value) {
    line_.num(key, value);
    return *this;
  }
  Event& real(const char* key, double value, int decimals = 2) {
    line_.real(key, value, decimals);
    return *this;
  }
  Event& flag(const char* key, bool value) {
    line_.flag(key, value);
    return *this;
  }
  void emit();

 private:
  char buf_[kLineMax] = {};
  lab::JsonLine line_;
};

// Emits {"ev":"err","code":<code>}; codes are short snake_case strings.
void error(const char* code);

}  // namespace lab_core

#endif  // ARDUINO
