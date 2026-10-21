#pragma once
// Lab-wide runtime: serial contract output. On-target only; the encoder is lab_json.h.
// Usage: lab_core::begin(LAB_PROJECT, LAB_FW);
//        lab_core::Event("leds").num("n", NUM_LEDS).emit();

#include "lab_json.h"

#ifdef ARDUINO

namespace lab_core {

constexpr size_t kLineMax = 256;

// Starts Serial (waits up to 2 s for USB CDC) and emits the `boot` event.
void begin(const char* project, const char* fw);

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
