#pragma once
// p004-sense: the M5StickC Plus's sensors that need no movement (mic, IMU temperature, power chip,
// RTC) steer animations on the BTF 8x8 WS2812B matrix on the Grove port. Pins resolve through
// boards/<name>/board.h.
#include "lab_board.h"

#define LAB_PROJECT "p004-sense"

// LEDs: the same matrix and wiring as p002 (Grove white wire, G33; serpentine down columns).
#define LED_TYPE          WS2812B
#define LED_ORDER         GRB
#define LED_RGBW          0
#define NUM_LEDS          64
#define LED_DATA_PIN      BOARD_LED_PIN_ALT
#define MATRIX_W          8  // 0 when not a matrix
#define MATRIX_H          8
#define MATRIX_SERPENTINE 1
#define MATRIX_COLUMNS    1  // 1: the wire runs down columns, not along rows

// Power (mandatory): Grove 5 V from the AXP192 boost on laptop USB; the M5 itself takes ~100 mA.
// Off USB the matrix blanks and its 5 V is switched off (Miles' call: no LEDs on the battery).
#define POWER_VOLTS  5
#define POWER_MAX_MA 400

// Audio: 0 off, 1 external I2S (INMP441, from M3), 2 board PDM mic (M5 and friends)
#define AUDIO_MODE  2
#define MIC_RATE_HZ 16000
#define MIC_BLOCK   256  // samples per block: 16 ms, about one frame

// Connectivity
#define WIFI_ENABLED 0  // credentials in secrets.h, never here
#define OTA_ENABLED  0
#define MDNS_NAME    "p004-sense"

// Sensors
#define SENSE_POLL_MS 250    // power chip and IMU temperature (I2C, ~1 ms per poll)
#define SENSE_LOG_MS  1000   // `sense` events this often; 0 = off
#define THERMO_MIN_C  50.0f  // warmth scene: IMU die temperature scale (it idles near 55 C)
#define THERMO_MAX_C  60.0f
#define CHIP_MIN_C    30.0f  // and the power chip's (it idles near 39 C)
#define CHIP_MAX_C    50.0f
#define BATTERY_LOW_V 3.5f  // battery scene turns red below this

// Clock: the RTC is set from the build time (UTC inside the build image) when it lost power or is
// clearly wrong. Offset to local time: -240 is EDT; make it -300 after 2026-11-01 (EST).
#define CLOCK_UTC_OFFSET_MIN -240

// Scenes
#define SCENE_BOOT        0      // 0 ambient, 1 sound, 2 warmth, 3 power, 4 battery, 5 clock
#define AMBIENT_FADE_MS   1500   // crossfade when sound energy moves ambient to another effect
#define AMBIENT_ENERGY_MS 6000   // sound energy: loudness averaged over about this long
#define POWER_SWEEP_MS    20000  // power scene: brightness 8 -> 80 -> 8 over this long
