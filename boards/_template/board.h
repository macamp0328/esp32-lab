#pragma once
// {{BOARD}}: <chip, flash, PSRAM, USB>. Source: <datasheet/wiki URL>. UNVERIFIED until p000
// bring-up. Every value here is read by projects; changing one is a stop-and-ask (AGENTS.md).

#define BOARD_NAME     "{{BOARD}}"
#define BOARD_CHIP     "{{CHIP}}"  // esptool --chip
#define BOARD_FLASH_MB 4
#define BOARD_PSRAM_MB 0

#define BOARD_LED_PIN 4  // default data pin for external strips: pick a safe pin from pinout.md

#define BOARD_HAS_PDM_MIC 0
#define BOARD_HAS_TFT     0

#define BOARD_SAFE_PINS {4}  // datasheet; confirm at bring-up
#define BOARD_VERIFIED  0    // 1 once p000 has confirmed this file on real hardware
