#ifdef ARDUINO

#include "lab_board.h"

#include <Arduino.h>

#if defined(LAB_BOARD_M5STICKC_PLUS11)
#include <M5Unified.h>
#endif

namespace lab_board {

void begin() {
#if defined(LAB_BOARD_M5STICKC_PLUS11)
  auto cfg = M5.config();
  cfg.serial_baudrate = 0;  // lab_core already started Serial
  cfg.output_power = true;  // Grove 5 V
  cfg.internal_imu = true;
  M5.begin(cfg);
#if BOARD_HAS_PMIC
  // The AXP192 powers up with its USB voltage and current ADCs off.
  if (M5.Power.getType() == m5::Power_Class::pmic_axp192) M5.Power.Axp192.setAdcState(true);
#endif
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextSize(2);
#endif
#ifdef BOARD_STATUS_LED
  pinMode(BOARD_STATUS_LED, OUTPUT);
  status_led(false);
#endif
}

void status_led(bool on) {
#ifdef BOARD_STATUS_LED
  digitalWrite(BOARD_STATUS_LED, (on != BOARD_STATUS_LED_ACTIVE_LOW) ? HIGH : LOW);
#else
  (void)on;
#endif
}

void status_text(const char* line) {
#if BOARD_HAS_TFT && defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.Display.println(line);
#else
  (void)line;
#endif
}

void status_clear() {
#if BOARD_HAS_TFT && defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
#endif
}

void screen_fill(uint8_t r, uint8_t g, uint8_t b) {
#if BOARD_HAS_TFT && defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.Display.fillScreen(M5.Display.color565(r, g, b));
#else
  (void)r, (void)g, (void)b;
#endif
}

void status_pixels(const lab::Pixel* px, const lab::Layout& layout) {
#if BOARD_HAS_TFT && defined(LAB_BOARD_M5STICKC_PLUS11)
  const int w = layout.width(), h = layout.height();
  if (w == 0 || h == 0) return;
  const int cw = M5.Display.width() / w, ch = M5.Display.height() / h;
  const int cell = cw < ch ? cw : ch;
  if (cell < 1) return;
  const int x0 = M5.Display.width() - cell * w;
  const int gap = cell > 3 ? 1 : 0;
  M5.Display.startWrite();
  for (uint16_t i = 0; i < layout.size(); ++i) {
    M5.Display.fillRect(x0 + layout.column(i) * cell, layout.row(i) * cell, cell - gap, cell - gap,
                        M5.Display.color565(px[i].r, px[i].g, px[i].b));
  }
  M5.Display.endWrite();
#else
  (void)px, (void)layout;
#endif
}

Buttons buttons() {
  Buttons b;
#if defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.update();
  b.a_click = M5.BtnA.wasClicked();
  b.a_hold = M5.BtnA.wasHold();
  b.b_click = M5.BtnB.wasClicked();
  b.b_hold = M5.BtnB.wasHold();
#endif
  return b;
}

bool accel(float* x, float* y, float* z) {
#if BOARD_HAS_IMU && defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.Imu.update();
  if (M5.Imu.getAccel(x, y, z)) return true;
#endif
  *x = *y = *z = 0;
  return false;
}

bool imu_temp(float* c) {
#if BOARD_HAS_IMU && defined(LAB_BOARD_M5STICKC_PLUS11)
  if (M5.Imu.getTemp(c)) return true;
#endif
  *c = 0;
  return false;
}

Power power() {
  Power p;
#if BOARD_HAS_PMIC && defined(LAB_BOARD_M5STICKC_PLUS11)
  if (M5.Power.getType() != m5::Power_Class::pmic_axp192) return p;
  auto& axp = M5.Power.Axp192;
  const uint8_t status = axp.readRegister8(0x00);  // input power status
  p.acin = status & 0x80;
  p.vbus = status & 0x20;
  p.charging = status & 0x04;
  p.usb = p.vbus || p.acin;
  p.usb_v = p.vbus || !p.acin ? axp.getVBUSVoltage() : axp.getACINVoltage();
  p.usb_ma = p.vbus || !p.acin ? axp.getVBUSCurrent() : axp.getACINCurrent();
  p.bat_v = axp.getBatteryVoltage();
  p.bat_ma = axp.getBatteryChargeCurrent() - axp.getBatteryDischargeCurrent();
  p.pmic_c = axp.getInternalTemperature();
  p.ok = true;
#endif
  return p;
}

float vbus_ma() {
#if BOARD_HAS_PMIC && defined(LAB_BOARD_M5STICKC_PLUS11)
  if (M5.Power.getType() == m5::Power_Class::pmic_axp192) return M5.Power.Axp192.getVBUSCurrent();
#endif
  return 0;
}

void ext_power(bool on) {
#if BOARD_HAS_PMIC && defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.Power.setExtOutput(on);
#else
  (void)on;
#endif
}

DateTime rtc_now() {
  DateTime t;
#if BOARD_HAS_RTC && defined(LAB_BOARD_M5STICKC_PLUS11)
  if (!M5.Rtc.isEnabled()) return t;
  t.ok = true;
  m5::rtc_datetime_t dt;
  if (!M5.Rtc.getDateTime(&dt)) {  // registers hold no valid date: never set since power-up
    t.lost = true;
    return t;
  }
  t.lost = M5.Rtc.getVoltLow();
  t.year = dt.date.year;
  t.month = dt.date.month;
  t.day = dt.date.date;
  t.hour = dt.time.hours;
  t.minute = dt.time.minutes;
  t.second = dt.time.seconds;
#endif
  return t;
}

void rtc_set(const DateTime& t) {
#if BOARD_HAS_RTC && defined(LAB_BOARD_M5STICKC_PLUS11)
  // Setting the time also clears the RTC's lost-power flag.
  M5.Rtc.setDateTime(
      m5::rtc_datetime_t{m5::rtc_date_t{static_cast<int16_t>(t.year), static_cast<int8_t>(t.month),
                                        static_cast<int8_t>(t.day)},
                         m5::rtc_time_t{static_cast<int8_t>(t.hour), static_cast<int8_t>(t.minute),
                                        static_cast<int8_t>(t.second)}});
#else
  (void)t;
#endif
}

#if BOARD_HAS_PDM_MIC && defined(LAB_BOARD_M5STICKC_PLUS11)
namespace {
uint32_t mic_rate = 0;
int16_t mic_buf[2][kMicBlockMax];
uint8_t mic_oldest = 0;  // of the blocks queued with M5.Mic.record(), the one that fills first
uint8_t mic_queued = 0;
}  // namespace
#endif

bool mic_begin(uint32_t rate_hz) {
#if BOARD_HAS_PDM_MIC && defined(LAB_BOARD_M5STICKC_PLUS11)
  M5.Speaker.end();  // the speaker and the mic share I2S0
  auto cfg = M5.Mic.config();
  cfg.sample_rate = rate_hz;
  M5.Mic.config(cfg);
  mic_rate = M5.Mic.begin() ? rate_hz : 0;
  return mic_rate != 0;
#else
  (void)rate_hz;
  return false;
#endif
}

bool mic_read(int16_t* out, size_t n) {
#if BOARD_HAS_PDM_MIC && defined(LAB_BOARD_M5STICKC_PLUS11)
  if (mic_rate == 0 || n == 0 || n > kMicBlockMax) return false;
  // Two blocks stay queued, so capture runs on while the caller works on the one before.
  while (mic_queued < 2) {
    if (!M5.Mic.record(mic_buf[(mic_oldest + mic_queued) % 2], n, mic_rate)) return false;
    ++mic_queued;
  }
  if (M5.Mic.isRecording() >= 2) return false;  // the oldest block is still filling
  memcpy(out, mic_buf[mic_oldest], n * sizeof(int16_t));
  --mic_queued;
  mic_oldest ^= 1;
  if (M5.Mic.record(mic_buf[mic_oldest ^ 1], n, mic_rate)) ++mic_queued;
  return true;
#else
  (void)out, (void)n;
  return false;
#endif
}

}  // namespace lab_board

#endif  // ARDUINO
