#pragma once
// Everything this project needs; pins resolve through the selected boards/<name>/board.h.
#include "lab_board.h"

#define LAB_PROJECT "{{PROJECT}}"

// LEDs (pins default from the board; override only when wired differently)
#define LED_TYPE          WS2812B
#define LED_ORDER         GRB
#define LED_RGBW          0
#define NUM_LEDS          8
#define LED_DATA_PIN      BOARD_LED_PIN
#define MATRIX_W          0  // 0 when not a matrix
#define MATRIX_H          0
#define MATRIX_SERPENTINE 0

// Power (mandatory; the build fails without both)
#define POWER_VOLTS  5
#define POWER_MAX_MA 500

// Audio: 0 off, 1 external I2S (INMP441), 2 board PDM mic (M5 and friends)
#define AUDIO_MODE 0

// Connectivity
#define WIFI_ENABLED 0  // credentials in secrets.h, never here
#define OTA_ENABLED  0
#define MDNS_NAME    "{{PROJECT}}"
