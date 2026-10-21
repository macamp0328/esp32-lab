# M5StickC Plus 1.1 (`m5stickc-plus11`)

**Verified:** partly, 2026-10-21 (unit m5stickc-01, p000 + esptool + one HIL check). Rows marked
"datasheet" are still unmeasured; safe pins need a wired strip to confirm.

| Fact | Value | Source |
| --- | --- | --- |
| SoC | ESP32-PICO-D4 rev v1.0, 240 MHz dual core; 4 MB GigaDevice flash (0xc8/0x4016, 3.3 V by strapping), no PSRAM; 40 MHz crystal | measured |
| USB | USB-C via FTDI bridge (0403:6001, "M5stack"), `/dev/cu.usbserial-*`; auto-reset works; flash at 1500000 baud (921600 drops after the baud switch) | measured |
| PMIC | AXP192 on internal I2C (SDA 21, SCL 22); `lab_board::begin()` (M5Unified 0.2.27) brings up the TFT rails | measured (TFT lit) |
| Display | 1.14" ST7789v2 135×240 (MOSI 15, CLK 13, DC 23, RST 18, CS 5) via M5Unified; text readable at rotation 1, size 2 | measured (HIL ok) |
| Mic | SPM1423 PDM: CLK 0, DATA 34 | datasheet |
| Red LED | GPIO10 blinks 1 Hz from `lab_board::status_led`; active-low is from datasheet (a 50% blink cannot show polarity) | measured (HIL ok) |
| IR LED | GPIO9 | datasheet |
| Buttons | A = GPIO37, B = GPIO39, power button on AXP192 | datasheet |
| Buzzer | GPIO2 | datasheet |
| IMU | MPU6886 via M5Unified `M5.Imu` (`BOARD_HAS_IMU`, `lab_board::accel`): answers at boot; +z out of the screen (az ≈ +1.0 lying screen-up); x/y directions not mapped yet | measured (p002) |
| RTC | BM8563 on internal I2C | datasheet |
| Grove port | G32 (yellow), G33 (white), 5 V (red, from AXP192), GND (black) | datasheet |
| Header | G0, G26, G25/G36 (shared pin), 3V3, 5V, BAT, GND | datasheet |
| Default LED data | G32 on Grove (`BOARD_LED_PIN`) | lab choice |
| Second LED data | G33 on Grove white (`BOARD_LED_PIN_ALT`); drives matrix-8x8-01 (64 px, RMT, 62 fps) | measured (p002, HIL ok) |
| Safe pins | 26, 32, 33 (`BOARD_SAFE_PINS`); G0 is strapping + mic, G36 input-only, G25 shares G36. G33 confirmed with a wired matrix | datasheet; G33 measured |
| Logic level | 3.3 V; WS2812B ECO (matrix-8x8-01) took 3.3 V data on G33 with no shifter over a short Grove cable; other strips may still need one | measured (p002) |

## Measured at bring-up
- 2026-10-21 esptool `flash-id`: ESP32-PICO-D4 rev v1.0; Wi-Fi, BT, dual core, 240 MHz; flash mfr 0xc8 dev 0x4016, 4 MB, 3.3 V set by strapping pin.
- 2026-10-21 p000 `hw` event: `flash_kb` 4096, `psram_kb` 0, `rev` 100 (= v1.0), ~263 KB free heap at boot.
- 2026-10-21 p000: `leds` arrives +289 ms after `boot` (M5Unified begin + TFT init), inside the 2 s budget.
- 2026-10-21 p000: WiFi STA joined in ~1.8 s at RSSI −41; ArduinoOTA ready on 3232.
- 2026-10-21 p000 with WiFi + OTA + M5Unified + FastLED uses 94% of the 1.25 MB app slot (default.csv).
- 2026-10-21: the cable shipped with this unit appeared charge-only (no USB device at all); a data cable fixed it.
- 2026-10-21 p002: resting on the bench with cables plugged in, the M5 reads ax -0.62, ay 0.20, az 0.75 (|a| = 1.0 g, about 40 degrees off flat).
- 2026-10-21 p002: 64 WS2812B on G33 with Grove 5 V from USB: 62 fps steady, ~300 mA estimated peak at brightness 40, no brown-out in 40 s; data straight at 3.3 V.
