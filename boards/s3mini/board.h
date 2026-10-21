#pragma once
// Waveshare ESP32-S3 Mini: ESP32-S3FH4R2 (4 MB quad flash, 2 MB quad PSRAM), native USB-C.
// From the Waveshare wiki and the ESP32-S3 datasheet. UNVERIFIED until p000 bring-up (M1).
// Every value here is read by projects; changing one is a stop-and-ask (AGENTS.md).

#define BOARD_NAME     "s3mini"
#define BOARD_CHIP     "esp32s3"  // esptool --chip
#define BOARD_FLASH_MB 4
#define BOARD_PSRAM_MB 2  // not BOARD_HAS_PSRAM: Arduino-ESP32 reserves that macro

#define BOARD_LED_PIN           4    // default data pin for external strips
#define BOARD_ONBOARD_RGB       21   // onboard WS2812, if present
#define BOARD_ONBOARD_RGB_ORDER GRB  // color order of that LED
#define BOARD_I2S_WS            7
#define BOARD_I2S_SD            8
#define BOARD_I2S_CLK           6

#define BOARD_HAS_PDM_MIC 0
#define BOARD_HAS_TFT     0

#define BOARD_SAFE_PINS {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}  // verified at bring-up
#define BOARD_VERIFIED  0  // 1 once p000 has confirmed this file on real hardware
