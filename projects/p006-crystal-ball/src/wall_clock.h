#pragma once
// Local time from NTP. The s3mini has no RTC, so time is unknown until the first answer, then the
// chip's own clock keeps it between NTP's hourly refreshes.

#include <stdint.h>

namespace wall_clock {

// Starts asking `server`; call once WiFi is up. `tz` is a POSIX TZ string (daylight saving too).
void begin(const char* tz, const char* server);

// Call every loop. Emits `clock` on the first answer; no answer for a minute is `err` ntp, then it
// asks again every minute.
void poll();

// Seconds since local midnight; false until the time is known.
bool now_s(uint32_t* seconds);

}  // namespace wall_clock
