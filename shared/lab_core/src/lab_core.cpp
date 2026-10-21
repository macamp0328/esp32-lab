#ifdef ARDUINO

#include "lab_core.h"

#include <Arduino.h>
#include <FastLED.h>
#include <esp_arduino_version.h>

#include "lab_board.h"

namespace lab_core {

namespace {
void write_line(const char* line) {
  Serial.write(line);
  Serial.write('\n');
}
}  // namespace

Event::Event(const char* ev) : line_(buf_, sizeof buf_, millis(), ev) {}

void Event::emit() {
  const char* line = line_.finish();
  if (line_.ok()) {
    write_line(line);
  } else {
    error("log_overflow");
  }
}

void error(const char* code) {
  char buf[96];
  lab::JsonLine line(buf, sizeof buf, millis(), "err");
  line.str("code", code);
  write_line(line.finish());
}

void begin(const char* project, const char* fw) {
  Serial.begin(115200);
  const uint32_t start = millis();
  while (!Serial && millis() - start < 2000) delay(10);

  char fastled[16];
  snprintf(fastled, sizeof fastled, "%d.%d.%d", FASTLED_VERSION / 1000000,
           FASTLED_VERSION / 1000 % 1000, FASTLED_VERSION % 1000);
  char core[16];
  snprintf(core, sizeof core, "%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR,
           ESP_ARDUINO_VERSION_PATCH);

  Event("boot")
      .str("proj", project)
      .str("fw", fw)
      .str("board", BOARD_NAME)
      .str("fastled", fastled)
      .str("core", core)
      .emit();
}

}  // namespace lab_core

#endif  // ARDUINO
