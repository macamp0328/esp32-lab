#ifdef ARDUINO

#include "lab_net.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

#include "lab_core.h"

namespace lab_net {

namespace {
enum class State { kOff, kConnecting, kUp, kWaiting };

constexpr uint32_t kConnectTimeoutMs = 20000;
constexpr uint16_t kOtaPort = 3232;

State state = State::kOff;
uint32_t started_ms = 0;
bool ota_started = false;
Config cfg = {"", "", "", false, nullptr};

void start_ota() {
  ArduinoOTA.setHostname(cfg.hostname);
  ArduinoOTA.setPort(kOtaPort);
  if (cfg.ota_password && *cfg.ota_password) ArduinoOTA.setPassword(cfg.ota_password);
  ArduinoOTA.onStart([] { lab_core::Event("ota").str("state", "start").emit(); });
  ArduinoOTA.onEnd([] { lab_core::Event("ota").str("state", "end").emit(); });
  ArduinoOTA.onError([](ota_error_t e) {
    char code[16];
    snprintf(code, sizeof code, "ota_%u", static_cast<unsigned>(e));
    lab_core::error(code);
  });
  ArduinoOTA.begin();
  ota_started = true;
  lab_core::Event("ota")
      .str("state", "ready")
      .str("host", cfg.hostname)
      .num("port", kOtaPort)
      .emit();
}

void came_up() {
  state = State::kUp;
  lab_core::Event("wifi")
      .str("state", "up")
      .str("ip", WiFi.localIP().toString().c_str())
      .num("rssi", WiFi.RSSI())
      .num("ms", millis() - started_ms)
      .emit();
  if (cfg.ota && !ota_started) start_ota();
}
}  // namespace

void begin(const Config& config) {
  cfg = config;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(cfg.hostname);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.ssid, cfg.password);
  state = State::kConnecting;
  started_ms = millis();
}

void handle() {
  const bool linked = WiFi.status() == WL_CONNECTED;
  switch (state) {
    case State::kConnecting:
      if (linked) {
        came_up();
      } else if (millis() - started_ms > kConnectTimeoutMs) {
        state = State::kWaiting;  // keep trying in the background (auto-reconnect)
        lab_core::error("wifi_timeout");
      }
      break;
    case State::kWaiting:
      if (linked) came_up();
      break;
    case State::kUp:
      if (!linked) {
        state = State::kWaiting;
        lab_core::Event("wifi").str("state", "lost").emit();
      } else if (ota_started) {
        ArduinoOTA.handle();
      }
      break;
    default:
      break;
  }
}

bool connected() { return state == State::kUp; }

}  // namespace lab_net

#endif  // ARDUINO
