#pragma once
// <Board name>: <chip, flash, PSRAM, USB>. Source: <datasheet/wiki URL>. UNVERIFIED until bring-up.

#define BOARD_NAME     "_template"
#define BOARD_CHIP     "esp32"
#define BOARD_FLASH_MB 4
#define BOARD_PSRAM_MB 0

#define BOARD_LED_PIN 4

#define BOARD_HAS_PDM_MIC 0
#define BOARD_HAS_TFT     0

#define BOARD_SAFE_PINS {4}  // verified at bring-up
#define BOARD_VERIFIED  0
