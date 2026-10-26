#pragma once
// Resolves the selected board's header from the LAB_BOARD_* flag in boards/<name>/board.ini.
// `just add-board` appends a branch here.

#if defined(LAB_BOARD_S3MINI)
#include "s3mini/board.h"
#elif defined(LAB_BOARD_M5STICKC_PLUS11)
#include "m5stickc-plus11/board.h"
#else
#error "No LAB_BOARD_* flag: build a board env, e.g. `just build <project> s3mini`"
#endif

#ifdef ARDUINO
#include <stddef.h>
#include <stdint.h>

#include "lab_layout.h"
#include "lab_pixel.h"
#endif

namespace lab_board {

inline constexpr int kSafePins[] = BOARD_SAFE_PINS;

// True when `pin` may carry LED data or I2S on this board (see boards/<name>/pinout.md).
constexpr bool pin_is_safe(int pin) {
  for (int p : kSafePins) {
    if (p == pin) return true;
  }
  return false;
}

// LED data may also go to the board's own onboard RGB LED, which is wired on the PCB.
constexpr bool led_pin_ok(int pin) {
#ifdef BOARD_ONBOARD_RGB
  if (pin == BOARD_ONBOARD_RGB) return true;
#endif
  return pin_is_safe(pin);
}

#ifdef ARDUINO
// Board power-up: on the M5StickC this enables the AXP192 rails (TFT, Grove 5 V) via M5Unified.
// Call after lab_core::begin() so Serial is already owned by lab_core.
void begin();

// Drives BOARD_STATUS_LED if the board has one (handles active-low); no-op otherwise.
void status_led(bool on);

// Prints a line on the board's display if BOARD_HAS_TFT; no-op otherwise.
void status_text(const char* line);

// Clears the board's display and homes the cursor if BOARD_HAS_TFT; no-op otherwise.
void status_clear();

// Fills the board's display with one color if BOARD_HAS_TFT; no-op otherwise.
void screen_fill(uint8_t r, uint8_t g, uint8_t b);

// Draws n pixels down from (x, y) on the board's display (240x135, landscape) if BOARD_HAS_TFT;
// no-op otherwise. One column a call: fast enough for a few columns per frame.
void screen_column(int x, int y, const lab::Pixel* px, int n);

// Draws the LEDs as a grid of squares (layout columns x rows, full brightness), right-aligned on
// the board's display, if BOARD_HAS_TFT; no-op otherwise.
void status_pixels(const lab::Pixel* px, const lab::Layout& layout);

// Debounced button events since the last call; call once per loop(). All false on boards
// without buttons. Hold fires once, after 500 ms.
struct Buttons {
  bool a_click = false;
  bool a_hold = false;
  bool b_click = false;
  bool b_hold = false;
};
Buttons buttons();

// Acceleration in g on the IMU chip's own axes (projects map them to their frame); false, with
// zeros, on boards without BOARD_HAS_IMU or when the read fails.
bool accel(float* x, float* y, float* z);

// IMU die temperature in deg C; false on boards without BOARD_HAS_IMU or when the read fails.
bool imu_temp(float* c);

// Power chip readings; ok is false on boards without BOARD_HAS_PMIC. About ten I2C reads per
// call, so read a few times a second, not every frame.
struct Power {
  bool ok = false;
  bool usb = false;   // 5 V present on either input below
  bool vbus = false;  // 5 V on the USB input
  bool acin = false;  // 5 V on the AC/header input
  bool charging = false;
  float usb_v = 0;  // voltage and current of the input that is present (VBUS first)
  float usb_ma = 0;
  float bat_v = 0;
  float bat_ma = 0;  // + charging, - discharging
  float pmic_c = 0;  // power chip die temperature
};
Power power();

// Current on the USB (VBUS) input in mA, one register read: cheap enough for every frame. The
// power chip samples it instantly and the load is pulsed (LED PWM), so average many readings.
// 0 on boards without BOARD_HAS_PMIC.
float vbus_ma();

// Switches the board's 5 V output (Grove red, header 5V) if BOARD_HAS_PMIC; no-op otherwise.
void ext_power(bool on);

// Wall-clock time from the board's RTC; ok is false on boards without BOARD_HAS_RTC. lost: the RTC
// lost power or was never set, so the time is junk (or zero) until rtc_set().
struct DateTime {
  bool ok = false;
  bool lost = false;
  uint16_t year = 0;
  uint8_t month = 0, day = 0, hour = 0, minute = 0, second = 0;
};
DateTime rtc_now();
void rtc_set(const DateTime& t);

// The board's PDM mic (BOARD_HAS_PDM_MIC): mic_begin() starts capture at rate_hz; mic_read() copies
// the next n samples (n <= kMicBlockMax, the same n every call) and is false until a new block has
// arrived. Both are false on boards without the mic. The mic is wired on the PCB, so its pins are
// exempt from BOARD_SAFE_PINS, like BOARD_ONBOARD_RGB in led_pin_ok().
constexpr size_t kMicBlockMax = 512;
bool mic_begin(uint32_t rate_hz);
bool mic_read(int16_t* out, size_t n);
#endif

}  // namespace lab_board
