#pragma once
// Non-blocking WiFi + optional ArduinoOTA. Credentials come from the project's include/secrets.h.
// Usage: setup(): lab_net::begin({MDNS_NAME, WIFI_SSID, WIFI_PASSWORD, OTA_ENABLED, OTA_PASSWORD});
//        loop():  lab_net::handle();
// Events: `wifi` (state up with ip/rssi/ms, or lost), `ota` (ready/start/end), `err` on failure.
// A failed or dropped connection keeps retrying; OTA starts the first time WiFi comes up.

namespace lab_net {

struct Config {
  const char* hostname;  // also the mDNS / OTA name (MDNS_NAME)
  const char* ssid;
  const char* password;
  bool ota;                  // OTA_ENABLED
  const char* ota_password;  // may be empty
};

void begin(const Config& config);
void handle();
bool connected();

}  // namespace lab_net
