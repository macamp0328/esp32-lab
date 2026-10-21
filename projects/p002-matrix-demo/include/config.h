#pragma once
// p002-matrix-demo: animations on the BTF 8x8 WS2812B matrix, on the M5StickC Plus Grove port,
// powered from laptop USB. Pins resolve through boards/<name>/board.h.
#include "lab_board.h"

#define LAB_PROJECT "p002-matrix-demo"

// LEDs: data on the Grove white wire (G33). Wiring probe 2026-10-21, cable at the top: LED 0
// top-left, the wire runs down column 0, then up column 1 (serpentine by columns); GRB confirmed.
#define LED_TYPE          WS2812B
#define LED_ORDER         GRB
#define LED_RGBW          0
#define NUM_LEDS          64
#define LED_DATA_PIN      BOARD_LED_PIN_ALT
#define MATRIX_W          8  // 0 when not a matrix
#define MATRIX_H          8
#define MATRIX_SERPENTINE 1
#define MATRIX_COLUMNS    1  // 1: the wire runs down columns, not along rows

// Power (mandatory): Grove 5 V from the AXP192 boost on laptop USB; the M5 itself takes ~100 mA
#define POWER_VOLTS  5
#define POWER_MAX_MA 400

// Audio: 0 off, 1 external I2S (INMP441), 2 board PDM mic (M5 and friends)
#define AUDIO_MODE 0

// Connectivity
#define WIFI_ENABLED 0  // credentials in secrets.h, never here
#define OTA_ENABLED  0
#define MDNS_NAME    "p002-matrix-demo"

// Demo
#define DEMO_PROBE    0      // 1: show only the wiring probe (lab_fx::wiring) for bring-up
#define DEMO_AUTO_MS  15000  // auto-advance period
#define DEMO_BOOT_SET 1      // set at power-on: 1, 2, or 0 = all
#define DEMO_FADE_MS  600    // crossfade between effects
#define MARQUEE_TEXT  "ESP32 LAB"

// Simulation tick rates (set 2), in ms of each effect's own clock
#define DEMO_TICK_LIFE_MS   160
#define DEMO_TICK_ANT_MS    50
#define DEMO_TICK_SNAKE_MS  125
#define DEMO_TICK_TETRIS_MS 125
#define DEMO_TICK_SAND_MS   33
#define DEMO_TICK_MARBLE_MS 17

// Tilt: maps the IMU's raw axes to matrix gravity (+x = higher columns, +y = toward row 7, the edge
// nearest you with the cable away). Set from the calibration capture in the README.
#define TILT_SWAP_XY 0
#define TILT_X_SIGN  1
#define TILT_Y_SIGN  1
#define TILT_LOG_MS  1000  // `tilt` events this often; 0 = off
