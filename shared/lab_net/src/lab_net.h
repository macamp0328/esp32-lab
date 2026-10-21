#pragma once
// Non-blocking WiFi + ArduinoOTA. Credentials come from the project's include/secrets.h.
// Usage: setup(): lab_net::begin(MDNS_NAME, WIFI_SSID, WIFI_PASSWORD, OTA_PASSWORD);
//        loop():  lab_net::handle();
// Events: `wifi` (ip, rssi, ms) once connected, `ota` (state ready/start/end), `err` on failure.

namespace lab_net {

void begin(const char* hostname, const char* ssid, const char* password, const char* ota_password);
void handle();
bool connected();

}  // namespace lab_net
