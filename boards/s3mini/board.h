#pragma once
// Waveshare ESP32-S3 Mini: ESP32-S3FH4R2 (4 MB quad flash, 2 MB quad PSRAM), native USB-C.
// Chip, memory, USB and onboard LED measured 2026-10-21 (pinout.md); safe pins from datasheet.
// Every value here is read by projects; changing one is a stop-and-ask (AGENTS.md).

#define BOARD_NAME     "s3mini"
#define BOARD_CHIP     "esp32s3"  // esptool --chip
#define BOARD_FLASH_MB 4
#define BOARD_PSRAM_MB 2  // not BOARD_HAS_PSRAM: Arduino-ESP32 reserves that macro

#define BOARD_LED_PIN           4    // default data pin for external strips
#define BOARD_ONBOARD_RGB       21   // onboard WS2812, if present
#define BOARD_ONBOARD_RGB_ORDER GRB  // color order of that LED (HIL ok)
#define BOARD_I2S_WS            7
#define BOARD_I2S_SD            8
#define BOARD_I2S_CLK           6

#define BOARD_HAS_PDM_MIC 0
#define BOARD_HAS_TFT     0
#define BOARD_HAS_IMU     0
#define BOARD_HAS_PMIC    0
#define BOARD_HAS_RTC     0

#define BOARD_SAFE_PINS \
  {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}  // datasheet; confirm with a wired strip
#define BOARD_VERIFIED 1  // p000 confirmed chip, memory, USB and onboard LED (pinout.md)
