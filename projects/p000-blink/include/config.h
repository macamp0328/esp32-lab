#pragma once
// p000-blink: board bring-up. Drives the onboard RGB LED where the board has one (s3mini),
// otherwise a strip on the default data pin (m5stickc-plus11 Grove) plus the status LED.
#include "lab_board.h"

#define LAB_PROJECT "p000-blink"

// LEDs
#define LED_TYPE WS2812B
#define LED_RGBW 0
#ifdef BOARD_ONBOARD_RGB
#define NUM_LEDS     1
#define LED_DATA_PIN BOARD_ONBOARD_RGB
#define LED_ORDER    BOARD_ONBOARD_RGB_ORDER
#else
#define NUM_LEDS     8
#define LED_DATA_PIN BOARD_LED_PIN
#define LED_ORDER    GRB
#endif
#define MATRIX_W          0
#define MATRIX_H          0
#define MATRIX_SERPENTINE 0

// Power (mandatory)
#define POWER_VOLTS  5
#define POWER_MAX_MA 500

// Audio: off
#define AUDIO_MODE 0

// Connectivity: on when include/secrets.h exists (OTA bring-up), otherwise reported as off
#define WIFI_ENABLED 1
#define OTA_ENABLED  1
#define MDNS_NAME    "p000-blink"
