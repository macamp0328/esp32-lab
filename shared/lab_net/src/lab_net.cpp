#ifdef ARDUINO

#include "lab_net.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

#include "lab_core.h"

namespace lab_net {

namespace {
enum class State { kOff, kConnecting, kUp, kFailed };

constexpr uint32_t kConnectTimeoutMs = 20000;
constexpr uint16_t kOtaPort = 3232;

State state = State::kOff;
uint32_t started_ms = 0;
const char* host = "";
const char* ota_password = nullptr;

void start_ota() {
  ArduinoOTA.setHostname(host);
  ArduinoOTA.setPort(kOtaPort);
  if (ota_password && *ota_password) ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.onStart([] { lab_core::Event("ota").str("state", "start").emit(); });
  ArduinoOTA.onEnd([] { lab_core::Event("ota").str("state", "end").emit(); });
  ArduinoOTA.onError([](ota_error_t e) {
    char code[16];
    snprintf(code, sizeof code, "ota_%u", static_cast<unsigned>(e));
    lab_core::error(code);
  });
  ArduinoOTA.begin();
  lab_core::Event("ota").str("state", "ready").str("host", host).num("port", kOtaPort).emit();
}
}  // namespace

void begin(const char* hostname, const char* ssid, const char* password, const char* ota_pw) {
  host = hostname;
  ota_password = ota_pw;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  state = State::kConnecting;
  started_ms = millis();
}

void handle() {
  switch (state) {
    case State::kConnecting:
      if (WiFi.status() == WL_CONNECTED) {
        state = State::kUp;
        lab_core::Event("wifi")
            .str("ip", WiFi.localIP().toString().c_str())
            .num("rssi", WiFi.RSSI())
            .num("ms", millis() - started_ms)
            .emit();
        start_ota();
      } else if (millis() - started_ms > kConnectTimeoutMs) {
        state = State::kFailed;
        lab_core::error("wifi_timeout");
      }
      break;
    case State::kUp:
      ArduinoOTA.handle();
      break;
    default:
      break;
  }
}

bool connected() { return state == State::kUp; }

}  // namespace lab_net

#endif  // ARDUINO
