#pragma once
// p003's measurement core: time a body repeatedly against a clock and summarize it. Pure (the
// clock is passed in), so it is host-tested (test/test_bench); the board passes
// esp_timer_get_time.

#include <stddef.h>
#include <stdint.h>

namespace bench {

struct Stats {
  uint32_t iterations = 0;
  double mean_us = 0;
  double max_us = 0;
  double per_s = 0;  // iterations per second at the mean
};

class Accumulator {
 public:
  void add(double us) {
    ++n_;
    total_ += us;
    if (us > max_) max_ = us;
  }
  Stats stats() const {
    Stats s;
    s.iterations = n_;
    s.mean_us = n_ ? total_ / n_ : 0;
    s.max_us = max_;
    s.per_s = s.mean_us > 0 ? 1e6 / s.mean_us : 0;
    return s;
  }

 private:
  uint32_t n_ = 0;
  double total_ = 0;
  double max_ = 0;
};

// Runs body() until at least min_us have passed and min_iterations are done, timing each call.
template <typename Clock, typename Body>
Stats run(Clock now_us, Body body, int64_t min_us = 300000, uint32_t min_iterations = 20) {
  Accumulator acc;
  const int64_t start = now_us();
  uint32_t done = 0;
  while (done < min_iterations || now_us() - start < min_us) {
    const int64_t t0 = now_us();
    body();
    acc.add(static_cast<double>(now_us() - t0));
    ++done;
  }
  return acc.stats();
}

// WS2812 on the wire: 24 bits at 1.25 us each per LED, then a 280 us latch.
constexpr double kUsPerLed = 30.0;
constexpr double kLatchUs = 280.0;
inline double wire_us(uint32_t leds) { return leds * kUsPerLed + kLatchUs; }

// Frames per second the wire allows at `leds` LEDs on one pin.
inline double wire_fps(uint32_t leds) { return 1e6 / wire_us(leds); }

// bytes per microsecond is MB/s (10^6 bytes per second).
inline double mb_per_s(size_t bytes, double us) { return us > 0 ? bytes / us : 0; }

}  // namespace bench
