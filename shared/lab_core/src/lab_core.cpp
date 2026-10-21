#ifdef ARDUINO

#include "lab_core.h"

#include <Arduino.h>
#include <FastLED.h>
#include <esp_arduino_version.h>
#include <esp_ota_ops.h>

#include "lab_board.h"

#if !(defined(FASTLED_RMT5) && FASTLED_RMT5) || defined(FASTLED_ESP32_USE_CLOCKLESS_SPI) || \
    defined(FASTLED_ESP32_I2S)
#error "esp32-lab drives clockless LEDs with FastLED's RMT driver; check FastLED flags"
#endif

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
  write_line(lab::error_line(buf, sizeof buf, millis(), code));
}

void begin(const Setup& setup) {
  Serial.begin(115200);
  const uint32_t start = millis();
  while (!Serial && millis() - start < 2000) delay(10);

  char fastled[16];
  snprintf(fastled, sizeof fastled, "%d.%d.%d", FASTLED_VERSION / 1000000,
           FASTLED_VERSION / 1000 % 1000, FASTLED_VERSION % 1000);
  char core[16];
  snprintf(core, sizeof core, "%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR,
           ESP_ARDUINO_VERSION_PATCH);

  char line[kLineMax];
  write_line(lab::boot_line(line, sizeof line, millis(),
                            {setup.project, setup.fw, BOARD_NAME, fastled, core}));

  const uint64_t mac = ESP.getEfuseMac();
  char mac_str[18];
  snprintf(mac_str, sizeof mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned)(mac & 0xff),
           (unsigned)(mac >> 8 & 0xff), (unsigned)(mac >> 16 & 0xff), (unsigned)(mac >> 24 & 0xff),
           (unsigned)(mac >> 32 & 0xff), (unsigned)(mac >> 40 & 0xff));
  Event("hw")
      .str("chip", ESP.getChipModel())
      .num("rev", ESP.getChipRevision())
      .num("cores", ESP.getChipCores())
      .num("mhz", ESP.getCpuFreqMHz())
      .num("flash_kb", ESP.getFlashChipSize() / 1024)
      .num("psram_kb", ESP.getPsramSize() / 1024)
      .num("heap", ESP.getFreeHeap())
      .str("mac", mac_str)
      .str("app", esp_ota_get_running_partition()->label)  // app0 after USB, app1 after OTA
      .emit();

  FastLED.setMaxPowerInVoltsAndMilliamps(setup.volts, setup.max_ma);
}

const char* led_driver() { return "RMT"; }

}  // namespace lab_core

#endif  // ARDUINO
