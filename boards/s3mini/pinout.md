# Waveshare ESP32-S3 Mini (`s3mini`)

**Verified:** no. From the Waveshare ESP32-S3-Zero/Mini wiki and the ESP32-S3 datasheet;
p000 bring-up (M1) replaces "datasheet" with "measured" row by row.

| Fact | Value | Source |
| --- | --- | --- |
| Module | ESP32-S3FH4R2: 4 MB quad flash, 2 MB quad PSRAM (in-package) | datasheet |
| USB | native USB-C (GPIO19 D-, GPIO20 D+), USB CDC on boot | datasheet |
| Onboard RGB | WS2812 on GPIO21 | datasheet |
| Default LED data | GPIO4 (`BOARD_LED_PIN`) | lab choice |
| I2S (INMP441) | WS 7, SD 8, CLK 6 | lab choice |
| Strapping pins | 0 (BOOT button), 3, 45, 46: never for LED data or I2S | datasheet |
| Flash/PSRAM pins | 26–32 internal: never usable | datasheet |
| Safe pins | 1, 2, 4–13 (`BOARD_SAFE_PINS`) | datasheet |
| Logic level | 3.3 V; 5 V WS2812 data needs a level shifter (74AHCT125) | datasheet |
| Download mode | hold BOOT, tap RESET (or plug in with BOOT held) | datasheet |

## Measured at bring-up
_(empty until M1)_
