#include "suite.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "bench.h"
#include "lab_core.h"
#include "lab_fx.h"
#include "lab_sim.h"

#if BENCH_OUTPUTS > 1
static_assert(lab_board::led_pin_ok(BENCH_PIN_2), "BENCH_PIN_2 is not in BOARD_SAFE_PINS");
#endif
#if BENCH_OUTPUTS > 3
static_assert(lab_board::led_pin_ok(BENCH_PIN_3), "BENCH_PIN_3 is not in BOARD_SAFE_PINS");
static_assert(lab_board::led_pin_ok(BENCH_PIN_4), "BENCH_PIN_4 is not in BOARD_SAFE_PINS");
#endif

void add_outputs(Outputs& o) {
  constexpr uint16_t kPer = BENCH_MAX_LEDS / BENCH_OUTPUTS;
#if BENCH_OUTPUTS > 1
  o.out[1] = &FastLED.addLeds<LED_TYPE, BENCH_PIN_2, LED_ORDER>(o.buf + kPer, kPer);
#endif
#if BENCH_OUTPUTS > 3
  o.out[2] = &FastLED.addLeds<LED_TYPE, BENCH_PIN_3, LED_ORDER>(o.buf + 2 * kPer, kPer);
  o.out[3] = &FastLED.addLeds<LED_TYPE, BENCH_PIN_4, LED_ORDER>(o.buf + 3 * kPer, kPer);
#endif
  for (uint8_t j = 1; j < BENCH_OUTPUTS; ++j) o.out[j]->setEnabled(false);
}

const lab::Layout& bench_layout(uint16_t n) {
  static const lab::Layout kMatrix =
      lab::Layout::matrix(MATRIX_W, MATRIX_H, MATRIX_SERPENTINE, MATRIX_COLUMNS);
  static const lab::Layout kGrid16 = lab::Layout::matrix(16, 16, true);
  static const lab::Layout kGrid32 = lab::Layout::matrix(32, 32, true);
  return n <= 64 ? kMatrix : (n <= 256 ? kGrid16 : kGrid32);
}

namespace {

volatile float g_sink;  // keeps the math loops from being optimized away
volatile uint32_t g_sink_u;

// Runs every test twice: a counting pass (so bench_start can say how many), then the real one.
class Runner {
 public:
  bool counting = true;
  uint32_t planned = 0;
  uint32_t done = 0;

  template <typename Body>
  void measure(const char* group, const char* test, uint32_t n, Body body) {
    if (counting) {
      ++planned;
      return;
    }
    report(group, test, n,
           bench::run(esp_timer_get_time, body, BENCH_MIN_US, BENCH_MIN_ITERATIONS));
  }

  void report(const char* group, const char* test, uint32_t n, const bench::Stats& s) {
    if (counting) {
      ++planned;
      return;
    }
    ++done;
    lab_core::Event("bench")
        .str("group", group)
        .str("test", test)
        .num("n", n)
        .real("us", s.mean_us, 2)
        .real("max_us", s.max_us, 1)
        .real("per_s", s.per_s, 1)
        .emit();
  }

  void group(const char* name) const {
    if (counting) return;
    char line[24];
    snprintf(line, sizeof line, "%u/%u", static_cast<unsigned>(done),
             static_cast<unsigned>(planned));
    lab_board::status_clear();
    lab_board::status_text("bench running");
    lab_board::status_text(name);
    lab_board::status_text(line);
  }
};

void marquee(lab::Pixel* px, const lab::Layout& l, const lab::AudioFrame&, uint32_t t_ms) {
  lab_fx::text(px, l, "ESP32 LAB", t_ms);
}

struct Fx {
  const char* name;
  lab_fx::Fn fn;
};
const Fx kEffects[] = {
    {"rainbow", lab_fx::rainbow},
    {"plasma", lab_fx::plasma},
    {"fire", lab_fx::fire},
    {"rain", lab_fx::rain},
    {"ripple", lab_fx::ripple},
    {"heart", lab_fx::heart},
    {"spiral", lab_fx::spiral},
    {"twinkle", lab_fx::twinkle},
    {"invader", lab_fx::invader},
    {"lava", lab_fx::lava},
    {"radar", lab_fx::radar},
    {"bounce", lab_fx::bounce},
    {"marquee", marquee},
    {"aurora", lab_fx::aurora},
    {"pacman", lab_fx::pacman},
    {"starfield", lab_fx::starfield},
    {"fireworks", lab_fx::fireworks},
    {"pong", lab_fx::pong},
    {"tunnel", lab_fx::tunnel},
    {"kaleidoscope", lab_fx::kaleidoscope},
    {"snow", lab_fx::snow},
    {"helix", lab_fx::helix},
    {"lightning", lab_fx::lightning},
};

// ---- compute ------------------------------------------------------------------------------

// 1000 calls per iteration, so the reported us per iteration reads as ns per call.
template <typename Op>
void math(Runner& r, const char* test, Op op) {
  r.measure("compute", test, 1000, [&op] {
    float x = 0.1f, acc = 0;
    for (int k = 0; k < 1000; ++k) {
      acc += op(x, k);
      x += 0.001f;
    }
    g_sink = acc;
  });
}

template <class Sim>
void sim(Runner& r, Outputs& o, const char* test) {
  static Sim s;
  static const uint16_t kSizes[] = {64, 256};
  for (uint16_t n : kSizes) {
    const lab::Layout& l = bench_layout(n);
    s.reset(7, l);
    r.measure("compute", test, n, [&] {
      s.step(lab_sim::Input{});
      s.draw(o.buf, l);
    });
  }
}

void compute(Runner& r, Outputs& o) {
  r.group("compute");
  math(r, "math/sinf", [](float x, int) { return sinf(x); });
  math(r, "math/sqrtf", [](float x, int) { return sqrtf(x); });
  math(r, "math/atan2f", [](float x, int) { return atan2f(x, 1.5f - x); });
  math(r, "math/noise", [](float x, int) { return lab::noise(x, 0.5f * x, 0.25f); });
  math(r, "math/hsv", [](float, int k) {
    return static_cast<float>(lab::hsv(static_cast<uint8_t>(k), 255, 255).r);
  });
  // Per-frame board calls. Buttons are timed 4 ms apart, like real frames: back to back, M5Unified
  // skips its I2C read of the power chip and the call looks free.
  {
    bench::Accumulator acc;
    for (uint32_t k = 0; !r.counting && k < 100; ++k) {
      delay(4);
      const int64_t t0 = esp_timer_get_time();
      (void)lab_board::buttons();
      acc.add(static_cast<double>(esp_timer_get_time() - t0));
    }
    r.report("compute", "board/buttons_per_frame", 1, acc.stats());
  }
  r.measure("compute", "board/status_led", 1, [] { lab_board::status_led(true); });
#if BOARD_HAS_IMU
  float ax, ay, az;
  r.measure("compute", "board/accel", 1, [&] { lab_board::accel(&ax, &ay, &az); });
#endif
  static const uint16_t kSizes[] = {64, 256, 1024};
  char test[32];
  for (const Fx& fx : kEffects) {
    snprintf(test, sizeof test, "fx/%s", fx.name);
    for (uint16_t n : kSizes) {
      const lab::Layout& l = bench_layout(n);
      uint32_t t = 0;
      r.measure("compute", test, n, [&] {
        fx.fn(o.buf, l, lab::AudioFrame{}, t);
        t += 16;
      });
    }
  }
  sim<lab_sim::Life>(r, o, "sim/life");
  sim<lab_sim::Ant>(r, o, "sim/ant");
  sim<lab_sim::Snake>(r, o, "sim/snake");
  sim<lab_sim::Tetris>(r, o, "sim/tetris");
  sim<lab_sim::Sand>(r, o, "sim/sand");
  sim<lab_sim::Marble>(r, o, "sim/marble");
}

// ---- led ----------------------------------------------------------------------------------

// The first `active` outputs on, each with `per_pin` LEDs from consecutive slices of buf.
void use_outputs(Outputs& o, uint8_t active, uint16_t per_pin) {
  for (uint8_t j = 0; j < BENCH_OUTPUTS; ++j) {
    if (!o.out[j]) continue;
    if (j < active) o.out[j]->setLeds(o.buf + j * per_pin, per_pin);
    o.out[j]->setEnabled(j < active);
  }
}

// Same total LEDs over 1, 2, 4 pins: equal times mean the pins go out one after another, a
// halving each time means the RMT channels transmit in parallel.
void parallel(Runner& r, Outputs& o) {
  char test[24];
  for (uint8_t pins = 1; pins <= BENCH_OUTPUTS; pins *= 2) {
    use_outputs(o, pins, BENCH_MAX_LEDS / pins);
    snprintf(test, sizeof test, "parallel/%u_pins", pins);
    r.measure("led", test, BENCH_MAX_LEDS, [] { FastLED.show(); });
  }
  use_outputs(o, 1, NUM_LEDS);
}

void led(Runner& r, Outputs& o) {
  r.group("led");
  FastLED.setBrightness(BENCH_BRIGHTNESS);
  for (uint16_t i = 0; i < BENCH_MAX_LEDS; ++i)
    o.buf[i] = lab::hsv(static_cast<uint8_t>(i * 7), 255, 255);
  static const uint16_t kSizes[] = {64, 256, 512, 1024};
  for (uint16_t n : kSizes) {
    use_outputs(o, 1, n);
    r.measure("led", "show", n, [] { FastLED.show(); });  // back to back: the sustained rate
  }
  for (uint16_t n : kSizes) {  // one show() with nothing in flight: how long the call blocks
    use_outputs(o, 1, n);
    bench::Accumulator acc;
    for (uint32_t k = 0; !r.counting && k < BENCH_MIN_ITERATIONS; ++k) {
      delay(40);
      const int64_t t0 = esp_timer_get_time();
      FastLED.show();
      acc.add(static_cast<double>(esp_timer_get_time() - t0));
    }
    r.report("led", "show_call", n, acc.stats());
  }
  for (uint16_t n : kSizes) {
    r.measure("led", "limiter", n, [&o, n] {
      g_sink_u = calculate_max_brightness_for_power_vmA(o.buf, n, 255, POWER_VOLTS, POWER_MAX_MA);
    });
  }
  static const uint16_t kRender[] = {64, 256, 1024};
  for (uint16_t n : kRender) {
    use_outputs(o, 1, n);
    const lab::Layout& l = bench_layout(n);
    uint32_t t = 0;
    r.measure("led", "plasma+show", n, [&] {
      lab_fx::plasma(o.buf, l, lab::AudioFrame{}, t);
      t += 16;
      FastLED.show();
    });
  }
  {  // one showcase frame without its 2 Hz screen update: buttons, serial poll, render, show
    use_outputs(o, 1, NUM_LEDS);
    const lab::Layout& l = bench_layout(NUM_LEDS);
    uint32_t t = 0;
    r.measure("led", "showcase_frame", NUM_LEDS, [&] {
      (void)lab_board::buttons();
      g_sink_u = Serial.available();
      lab_fx::plasma(o.buf, l, lab::AudioFrame{}, t);
      t += 16;
      FastLED.show();
      lab_board::status_led(t & 512);
    });
  }
  parallel(r, o);
}

// ---- dualcore -----------------------------------------------------------------------------

CRGB g_second[BENCH_MAX_LEDS];  // the double buffer's other half

struct Pipe {
  const lab::Layout* layout;
  lab_fx::Fn fx;
  CRGB* back;  // the render task draws here; only touched between `free_` and `ready`
  uint32_t t;
  SemaphoreHandle_t ready;
  SemaphoreHandle_t free_;
  SemaphoreHandle_t done;
  volatile bool stop;
};

void render_task(void* arg) {
  Pipe& p = *static_cast<Pipe*>(arg);
  for (;;) {
    xSemaphoreTake(p.free_, portMAX_DELAY);
    if (p.stop) break;
    p.fx(p.back, *p.layout, lab::AudioFrame{}, p.t);
    p.t += 16;
    xSemaphoreGive(p.ready);
  }
  xSemaphoreGive(p.done);
  vTaskDelete(nullptr);
}

// Core 0 renders frame k+1 into one buffer while this core (1) sends frame k from the other.
bench::Stats dual(Outputs& o, lab_fx::Fn fx, uint16_t n) {
  Pipe p{&bench_layout(n),
         fx,
         g_second,
         0,
         xSemaphoreCreateBinary(),
         xSemaphoreCreateBinary(),
         xSemaphoreCreateBinary(),
         false};
  CRGB* front = o.buf;
  xTaskCreatePinnedToCore(render_task, "render", 4096, &p, 1, nullptr, 0);
  xSemaphoreGive(p.free_);
  bench::Accumulator acc;
  int64_t last = esp_timer_get_time();
  const int64_t end = last + static_cast<int64_t>(BENCH_DUALCORE_MS) * 1000;
  while (last < end) {
    xSemaphoreTake(p.ready, portMAX_DELAY);
    CRGB* const fresh = p.back;
    p.back = front;
    front = fresh;
    o.out[0]->setLeds(front, n);
    xSemaphoreGive(p.free_);
    FastLED.show();
    const int64_t now = esp_timer_get_time();
    acc.add(static_cast<double>(now - last));
    last = now;
  }
  xSemaphoreTake(p.ready, portMAX_DELAY);  // the frame still in flight
  p.stop = true;
  xSemaphoreGive(p.free_);
  xSemaphoreTake(p.done, portMAX_DELAY);
  vSemaphoreDelete(p.ready);
  vSemaphoreDelete(p.free_);
  vSemaphoreDelete(p.done);
  use_outputs(o, 1, n);
  return acc.stats();
}

void dualcore(Runner& r, Outputs& o) {
  r.group("dualcore");
  static const Fx kHeavyToLight[] = {
      {"starfield", lab_fx::starfield}, {"fire", lab_fx::fire}, {"rainbow", lab_fx::rainbow}};
  constexpr uint16_t n = BENCH_MAX_LEDS;
  char test[32];
  for (const Fx& fx : kHeavyToLight) {
    use_outputs(o, 1, n);
    const lab::Layout& l = bench_layout(n);
    uint32_t t = 0;
    snprintf(test, sizeof test, "%s/single", fx.name);
    r.measure("dualcore", test, n, [&] {
      fx.fn(o.buf, l, lab::AudioFrame{}, t);
      t += 16;
      FastLED.show();
    });
    snprintf(test, sizeof test, "%s/dual", fx.name);
    r.report("dualcore", test, n, r.counting ? bench::Stats{} : dual(o, fx.fn, n));
  }
  use_outputs(o, 1, NUM_LEDS);
}

// ---- memory -------------------------------------------------------------------------------

void memory(Runner& r, Outputs& o) {
  r.group("memory");
  constexpr size_t kBytes = 32 * 1024;
  auto* a = static_cast<uint8_t*>(heap_caps_malloc(kBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  auto* b = static_cast<uint8_t*>(heap_caps_malloc(kBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  uint8_t v = 0;
  if (a && b) {
    r.measure("memory", "memcpy/sram", kBytes, [&] { memcpy(b, a, kBytes); });
    r.measure("memory", "memset/sram", kBytes, [&] { memset(a, ++v, kBytes); });
  }
#if BOARD_PSRAM_MB > 0
  auto* pa = static_cast<uint8_t*>(heap_caps_malloc(kBytes, MALLOC_CAP_SPIRAM));
  auto* pb = static_cast<uint8_t*>(heap_caps_malloc(kBytes, MALLOC_CAP_SPIRAM));
  auto* pbuf =
      static_cast<CRGB*>(heap_caps_malloc(BENCH_MAX_LEDS * sizeof(CRGB), MALLOC_CAP_SPIRAM));
  if (a && pa && pb && pbuf) {
    r.measure("memory", "memcpy/psram", kBytes, [&] { memcpy(pb, pa, kBytes); });
    r.measure("memory", "memset/psram", kBytes, [&] { memset(pa, ++v, kBytes); });
    r.measure("memory", "memcpy/sram_to_psram", kBytes, [&] { memcpy(pa, a, kBytes); });
    const lab::Layout& l = bench_layout(BENCH_MAX_LEDS);
    uint32_t t = 0;
    r.measure("compute", "fx/plasma_into_sram", BENCH_MAX_LEDS, [&] {
      lab_fx::plasma(o.buf, l, lab::AudioFrame{}, t);
      t += 16;
    });
    r.measure("compute", "fx/plasma_into_psram", BENCH_MAX_LEDS, [&] {
      lab_fx::plasma(pbuf, l, lab::AudioFrame{}, t);
      t += 16;
    });
  }
  heap_caps_free(pa);
  heap_caps_free(pb);
  heap_caps_free(pbuf);
#endif
  heap_caps_free(a);
  heap_caps_free(b);
}

// ---- screen -------------------------------------------------------------------------------

void screen(Runner& r, Outputs& o) {
#if BOARD_HAS_TFT
  r.group("screen");
  uint8_t k = 0;
  r.measure("screen", "fill", 1, [&k] { lab_board::screen_fill((k++ & 1) ? 60 : 0, 0, 40); });
  r.measure("screen", "status_4_lines", 1, [] {  // the showcase's 2 Hz update
    lab_board::status_clear();
    lab_board::status_text("p003-bench");
    lab_board::status_text("fps 191");
    lab_board::status_text("n 64 plasma");
    lab_board::status_text("A: n  hold A: bench");
  });
  static const uint16_t kSizes[] = {64, 256};
  for (uint16_t n : kSizes) {
    const lab::Layout& l = bench_layout(n);
    lab_fx::plasma(o.buf, l, lab::AudioFrame{}, 1000);
    r.measure("screen", n == 64 ? "mirror/8x8" : "mirror/16x16", n,
              [&o, &l] { lab_board::status_pixels(o.buf, l); });
  }
  if (!r.counting) lab_board::status_clear();
#else
  (void)r, (void)o;
#endif
}

}  // namespace

void run_suite(Outputs& o) {
  Runner r;
  const int64_t start = esp_timer_get_time();
  for (int pass = 0; pass < 2; ++pass) {
    r.counting = pass == 0;
    if (!r.counting) lab_core::Event("bench_start").num("tests", r.planned).emit();
    compute(r, o);
    led(r, o);
    dualcore(r, o);
    memory(r, o);
    screen(r, o);
  }
  lab_core::Event("bench_done")
      .num("ms", (esp_timer_get_time() - start) / 1000)
      .num("tests", r.done)
      .emit();
  use_outputs(o, 1, NUM_LEDS);
  FastLED.setBrightness(SHOWCASE_BRIGHTNESS);
}
