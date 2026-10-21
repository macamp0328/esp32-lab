# M5StickC Plus 1.1 (`m5stickc-plus11`)

**Verified:** no. From M5Stack's M5StickC PLUS docs; p000 bring-up (M1) confirms each row.

| Fact | Value | Source |
| --- | --- | --- |
| SoC | ESP32-PICO-D4: 4 MB flash, no PSRAM | datasheet |
| USB | USB-C via onboard USB-UART bridge | datasheet |
| PMIC | AXP192 on internal I2C (SDA 21, SCL 22); rails off until `lab_board::begin()` | datasheet |
| Display | 1.14" ST7789v2 135×240: MOSI 15, CLK 13, DC 23, RST 18, CS 5 (via M5Unified) | datasheet |
| Mic | SPM1423 PDM: CLK 0, DATA 34 | datasheet |
| Red LED | GPIO10, active low | datasheet |
| IR LED | GPIO9 | datasheet |
| Buttons | A = GPIO37, B = GPIO39, power button on AXP192 | datasheet |
| Buzzer | GPIO2 | datasheet |
| IMU / RTC | MPU6886, BM8563 on internal I2C | datasheet |
| Grove port | G32 (yellow), G33 (white), 5 V (red, from AXP192), GND (black) | datasheet |
| Header | G0, G26, G25/G36 (shared pin), 3V3, 5V, BAT, GND | datasheet |
| Default LED data | G32 on Grove (`BOARD_LED_PIN`) | lab choice |
| Safe pins | 26, 32, 33 (`BOARD_SAFE_PINS`); G0 is strapping + mic, G36 input-only, G25 shares G36 | datasheet |
| Logic level | 3.3 V; 5 V WS2812 data needs a level shifter | datasheet |

## Measured at bring-up
_(empty until M1)_
