#pragma once
// p006-crystal-ball: a LuMini ring under a glass orb, glowing warm from 18:00 to 22:00 by NTP time.
#include "lab_board.h"

#define LAB_PROJECT "p006-crystal-ball"

// LEDs: SparkFun LuMini 1 in ring, 20 x APA102-2020, clocked (two signal wires).
#define LED_TYPE          APA102
#define LED_ORDER         BGR  // SparkFun's LuMini ring test; confirm on the first light
#define LED_RGBW          0
#define NUM_LEDS          20
#define LED_DATA_PIN      BOARD_LED_PIN        // ring DI, green
#define LED_CLOCK_PIN     BOARD_LED_CLOCK_PIN  // ring CI, yellow
#define LED_RATE_MHZ      1                    // ~30 cm of loose wire: slow is safe
#define MATRIX_W          0
#define MATRIX_H          0
#define MATRIX_SERPENTINE 0

// Power: the ring runs off the s3mini's 3V3 pin, so the regulator carries it plus the chip and
// WiFi. POWER_VOLTS stays 5: FastLED's milliwatts per channel assume 5 V, so 5 makes
// POWER_MAX_MA the real current cap. Full white at full brightness damages the ring (SparkFun).
#define POWER_VOLTS     5
#define POWER_MAX_MA    150
#define GLOW_BRIGHTNESS 72  // 0..255; 48 was too dim under the orb (2026-11-22); ~72 meets the cap

#define AUDIO_MODE 0

// Connectivity
#define WIFI_ENABLED 1  // credentials in secrets.h, never here
#define OTA_ENABLED  1
#define MDNS_NAME    "p006-crystal-ball"

// Clock and schedule: local time, from NTP once WiFi is up (the board has no RTC).
#define CLOCK_TZ     "EST5EDT,M3.2.0,M11.1.0"
#define CLOCK_SERVER "pool.ntp.org"
#define GLOW_ON_S    (18 * 3600)
#define GLOW_OFF_S   (22 * 3600)
#define GLOW_FADE_S  300
#define BOOT_GLOW_S  60  // glows this long after power-up whatever the time; 0 turns it off

// Demo: 1 rotates every pattern below all day (the schedule and boot glow are ignored), to pick
// one on the real ring; 0 is the evening glow by the clock.
#define DEMO_MODE      1
#define DEMO_PATTERN_S 30  // each pattern's turn
#define DEMO_FADE_S    3   // crossfade into the next, at the end of a turn
