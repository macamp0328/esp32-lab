#pragma once
// M5StickC Plus 1.1: ESP32-PICO-D4 (4 MB flash, no PSRAM), SPM1423 PDM mic, 1.14" ST7789v2
// 135x240 TFT, AXP192 PMIC. Chip, memory, USB, TFT and red LED measured 2026-10-21 (pinout.md).
// The AXP192 must enable its rails (lab_board::begin()) before the TFT or Grove 5 V is used.

#define BOARD_NAME     "m5stickc-plus11"
#define BOARD_CHIP     "esp32"  // esptool --chip
#define BOARD_FLASH_MB 4
#define BOARD_PSRAM_MB 0

#define BOARD_LED_PIN               32  // Grove port, yellow wire; Grove red is 5 V from the AXP192
#define BOARD_LED_PIN_ALT           33  // Grove port, white wire (p002's matrix data)
#define BOARD_STATUS_LED            10  // red LED
#define BOARD_STATUS_LED_ACTIVE_LOW 1
#define BOARD_IR_LED                9
#define BOARD_BUTTON_A              37
#define BOARD_BUTTON_B              39
#define BOARD_BUZZER                2
#define BOARD_PDM_CLK               0  // SPM1423 mic (G0 is also a strapping pin)
#define BOARD_PDM_DATA              34
#define BOARD_I2C_SDA               21  // internal bus: AXP192, MPU6886, BM8563
#define BOARD_I2C_SCL               22

#define BOARD_HAS_PDM_MIC 1
#define BOARD_HAS_TFT     1
#define BOARD_HAS_IMU     1  // MPU6886 on the internal I2C bus, read through M5Unified
#define BOARD_HAS_PMIC    1  // AXP192: rails, USB and battery voltage/current, die temperature
#define BOARD_HAS_RTC     1  // BM8563 on the internal I2C bus, kept running by the battery

#define BOARD_SAFE_PINS {26, 32, 33}  // datasheet; confirm with a wired strip
#define BOARD_VERIFIED  1             // p000 confirmed chip, memory, USB, TFT and status LED
