# Waveshare ESP32-S3 Mini (`s3mini`)

**Verified:** partly, 2026-10-21 (unit s3mini-01, p000 + esptool + one HIL check). Rows marked
"datasheet" are still unmeasured; safe pins need a wired strip to confirm.

| Fact | Value | Source |
| --- | --- | --- |
| Module | ESP32-S3 (QFN56) rev v0.2, embedded 4 MB XMC quad flash (3.3 V), embedded 2 MB AP PSRAM; 40 MHz crystal | measured |
| USB | native USB-Serial/JTAG (303a:1001), shows as `/dev/cu.usbmodem*`; esptool auto-reset works, no BOOT button needed | measured |
| Onboard RGB | WS2812 on GPIO21, color order RGB | measured 2026-10-27 (HIL ok, both dock units) |
| Default LED data | GPIO4 (`BOARD_LED_PIN`) | lab choice |
| I2S (INMP441) | WS 7, SD 8, CLK 6 | lab choice |
| Strapping pins | 0 (BOOT button), 3, 45, 46: never for LED data or I2S | datasheet |
| Flash/PSRAM pins | 26–32 internal: never usable | datasheet |
| Safe pins | 1, 2, 4–13 (`BOARD_SAFE_PINS`) | datasheet |
| Logic level | 3.3 V; 5 V WS2812 data needs a level shifter (74AHCT125) | datasheet |
| Download mode | hold BOOT, tap RESET (or plug in with BOOT held) | datasheet |

## Measured at bring-up
- 2026-10-21 esptool `flash-id`: ESP32-S3 (QFN56) rev v0.2; Wi-Fi, BT 5 LE, dual core + LP core, 240 MHz; embedded flash 4 MB (XMC, mfr 0x46 dev 0x4016, quad, 3.3 V eFuse); embedded PSRAM 2 MB (AP_3v3).
- 2026-10-21 p000 `hw` event: `flash_kb` 4096, `psram_kb` 2048, `rev` 2, 2 cores, 240 MHz, ~333 KB free heap at boot.
- 2026-10-21 p000: onboard WS2812 on GPIO21 driven by FastLED RMT with order GRB shows red → green → blue (HIL ok).
- 2026-10-27 p000: corrected: with order GRB the a574 and 720c pixels swept green → red → blue (red and green swapped); with RGB both show green right after red (HIL ok). The 2026-10-21 ok was a looping sweep read from the wrong start.
- 2026-10-21 p000: ROM boot banner and app log share the USB CDC port; ESP-IDF logs one benign `esp_cache_msync` error at LED init (docs/lessons.md).
- 2026-10-21 p000: WiFi STA joined in ~5.2 s at RSSI −41; ArduinoOTA on port 3232 as `p000-blink.local`.
