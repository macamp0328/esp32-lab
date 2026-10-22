#pragma once
// p003-bench: how fast each board renders and sends LED frames. Boots into an uncapped showcase
// (live fps); the serial line `bench`, or an A hold on boards with buttons, runs the full suite.
#include "lab_board.h"

#define LAB_PROJECT "p003-bench"

// LEDs: the showcase drives an 8x8 matrix; the suite resizes the output up to BENCH_MAX_LEDS.
#define LED_TYPE       WS2812B
#define LED_ORDER      GRB
#define LED_RGBW       0
#define NUM_LEDS       64
#define BENCH_MAX_LEDS 1024
#ifdef BOARD_ONBOARD_RGB
// S3 Mini: frames go to the onboard LED (pixel 0 shows); the parallel test adds three pins with
// nothing attached (headers unsoldered).
#define LED_DATA_PIN  BOARD_ONBOARD_RGB
#define BENCH_PIN_2   BOARD_LED_PIN
#define BENCH_PIN_3   BOARD_I2S_CLK
#define BENCH_PIN_4   BOARD_I2S_WS
#define BENCH_OUTPUTS 4
#elif defined(BOARD_LED_PIN_ALT)
// M5StickC Plus: the 8x8 matrix on Grove white (p002's wiring); Grove yellow is the second output.
#define LED_DATA_PIN  BOARD_LED_PIN_ALT
#define BENCH_PIN_2   BOARD_LED_PIN
#define BENCH_OUTPUTS 2
#else
#define LED_DATA_PIN  BOARD_LED_PIN
#define BENCH_OUTPUTS 1
#endif
#define MATRIX_W          8
#define MATRIX_H          8
#define MATRIX_SERPENTINE 1
#define MATRIX_COLUMNS    1  // matrix-8x8-01 is wired down columns (p002)

// Power (mandatory)
#define POWER_VOLTS  5
#define POWER_MAX_MA 400

// Audio: 0 off, 1 external I2S (INMP441), 2 board PDM mic (M5 and friends)
#define AUDIO_MODE 0

// Connectivity
#define WIFI_ENABLED 0  // credentials in secrets.h, never here
#define OTA_ENABLED  0
#define MDNS_NAME    "p003-bench"

// Bench
#define SHOWCASE_BRIGHTNESS  40
#define SHOWCASE_FX_MS       10000   // each showcase effect plays this long
#define BENCH_BRIGHTNESS     8       // the suite flashes the LEDs hard; keep it dim
#define BENCH_MIN_US         300000  // every measurement runs at least this long...
#define BENCH_MIN_ITERATIONS 20      // ...and at least this many times
#define BENCH_DUALCORE_MS    2000    // each dual-core run (under the 5 s task watchdog)
