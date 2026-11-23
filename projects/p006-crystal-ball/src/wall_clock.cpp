#include "wall_clock.h"

#include <Arduino.h>
#include <time.h>

#include "lab_core.h"

namespace wall_clock {

namespace {
constexpr time_t kPlausible = 1700000000;  // 2023-11: an earlier time means the clock is unset
constexpr uint32_t kRetryMs = 60000;

const char* tz_ = "";
const char* server_ = "";
uint32_t asked_ms = 0;
bool started = false;
bool synced = false;

void ask() {
  configTzTime(tz_, server_);
  asked_ms = millis();
}
}  // namespace

void begin(const char* tz, const char* server) {
  tz_ = tz;
  server_ = server;
  started = true;
  ask();
}

void poll() {
  if (!started || synced) return;
  const time_t now = time(nullptr);
  if (now >= kPlausible) {
    synced = true;
    struct tm local;
    localtime_r(&now, &local);
    char text[20];
    strftime(text, sizeof text, "%Y-%m-%d %H:%M:%S", &local);
    lab_core::Event("clock").str("state", "synced").str("local", text).emit();
  } else if (millis() - asked_ms >= kRetryMs) {
    lab_core::error("ntp");
    ask();
  }
}

bool now_s(uint32_t* seconds) {
  const time_t now = time(nullptr);
  if (now < kPlausible) return false;
  struct tm local;
  localtime_r(&now, &local);
  *seconds = local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
  return true;
}

}  // namespace wall_clock
