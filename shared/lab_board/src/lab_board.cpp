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
  M5.begin(cfg);
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

}  // namespace lab_board

#endif  // ARDUINO
