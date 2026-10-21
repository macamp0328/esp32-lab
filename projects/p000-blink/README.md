# p000-blink

**Status:** bench-verified (s3mini)  ·  **Last HIL check:** 2026-10-21 s3mini ok

Board bring-up firmware: proves a board builds, flashes, logs the serial contract (plus a `hw`
event with measured chip facts), lights its LED, and takes one OTA update.

## Wiring
Nothing to wire.

| Board | LED | GPIO | Notes |
| ----- | --- | ---- | ----- |
| s3mini | onboard WS2812 | `BOARD_ONBOARD_RGB` | 1 pixel, red → green → blue, 1 s each |
| m5stickc-plus11 | red status LED | `BOARD_STATUS_LED` | 1 Hz blink; FastLED drives 8 px on Grove `BOARD_LED_PIN` (nothing attached) |

## Power
LEDs: 1 onboard (s3mini) or none attached (M5) · cap 500 mA · supply: laptop USB

## Demo
`just build p000 s3mini && just flash p000 s3mini && just watch p000`; expect `boot`, `hw`,
`leds`, `fps` every 5 s, and with `include/secrets.h` present `wifi` + `ota` ready.
OTA: `just flash p000 s3mini --ota`.

## Open issues
- [ ] …
