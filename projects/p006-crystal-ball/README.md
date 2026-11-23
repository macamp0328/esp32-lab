# p006-crystal-ball

**Status:** prototype, demo rotation on (rung 4 on s3mini 720c with the ring wired: WiFi, clock, fps 76; 2 h soak 2026-11-23: fps 76–77, heap flat, WiFi never dropped; OTA update ok, app0 → app1)  ·  **Last HIL check:** 2026-11-22 s3mini 720c: amber and even on all 20 (ok); too dim under the orb at brightness 48, now 72

A hand-blown glass orb on a planter pot, lit from below by a small LED ring. It glows warm from
18:00 to 22:00, every day. Phase 1 runs on a breadboard from laptop USB; the battery build is
[phase 2](../../docs/ideas/p006-crystal-ball.md).

## Wiring
The board is s3mini 720c (USB serial D4:05:92:48:72:0C, off the p005 dock 2026-11-22), headers on.
Solder the four ring leads first: steps and checks in the
[idea note](../../docs/ideas/p006-crystal-ball.md) ("Day one"), drawing in
[p006-phase1-wiring.svg](../../docs/ideas/p006-phase1-wiring.svg), each ring pad marked on a photo in
[p006-ring-pads.jpg](../../docs/ideas/p006-ring-pads.jpg).

| Signal | GPIO | Ring pad | Wire | Notes |
| --- | --- | --- | --- | --- |
| Power | 3V3 pin | 5V | red | never the 5V pin: at 5 V the ring wants a level shifter |
| Ground | GND pin | G | black | |
| Data | `BOARD_LED_PIN` (4) | DI | green | |
| Clock | `BOARD_LED_CLOCK_PIN` (5) | CI | yellow | ~30 cm leads; SPI clock 1 MHz |

On the ring the pads run G, 5V, DI, CI, CO, DO. Leave CO and DO bare. The two square pads by "17"
stay empty (C1, 5V to ground), and the bare mounting rings are ground.

## Power
Ring: 20 × APA102-2020 · cap 150 mA (`POWER_MAX_MA`) · supply: the s3mini's 3V3 pin (USB through
its regulator). The ring is rated at 5 V, so 3.3 V may be dim; the prototype finds out.

## Run
1. Copy `include/secrets.example.h` to `include/secrets.h` and type in the WiFi name and password
   (git ignores the file). Without it the orb still glows for a minute after power-up, then stays
   dark: it has no clock.
2. `just build p006 s3mini && just flash p006 s3mini && just watch p006 s3mini`
   (another s3mini is on the p005 dock: unplug it, or pass `--port`).
3. Expect `boot`, `leds` (n 20, driver SPI), a minute of glow, then `wifi` up, `clock` synced with
   the local time, and `fps` every 5 s with `level` (0 outside 18:00–22:00).

After the first USB flash it updates over WiFi: `just flash p006 --ota`.

## Demo rotation (`DEMO_MODE 1`, on for now)
Nine patterns take 30 s turns all day, crossfading over 3 s, ignoring the schedule; each turn logs
`pattern` (i, name, gain). `DEMO_MODE 0` goes back to the evening glow. Measured 2026-11-22, one loop
(FastLED's estimate at the 150 mA cap):

| # | Pattern | Looks like | Gain | mA est |
| --- | --- | --- | --- | --- |
| 1 | glow | amber, rose, honey crossfading over 6 min | 72 | 150 (cap trims) |
| 2 | breathe | whole ring in and out every 8 s | 72 | 67-145 |
| 3 | ember | coals, deep red to orange | 110 | 139-150 |
| 4 | orbit | one warm light circling every 15 s, long tail | 255 | ~95 |
| 5 | tide | amber and rose lights circling opposite ways, meeting | 200 | 142-150 |
| 6 | lava | orange blobs on deep purple | 110 | 70-99 |
| 7 | aurora | green to teal and violet | 120 | 57-71 |
| 8 | twinkle | stars fading in and out | 200 | 114-150 |
| 9 | drift | soft rainbow turning once a minute | 60 | 140 |

## Open issues
- [x] First light 2026-11-22: BGR is right (amber, all 20 even).
- [ ] Too dim under the orb at `GLOW_BRIGHTNESS` 48. At 72 the 150 mA cap already trims it (`bright` ~70,
      `ma_est` ~148), so more light needs another ring supply (an 18650 as in phase 1b, or 5 V with a level
      shifter), not a bigger number: the 3V3 regulator also feeds the S3 and its WiFi.
- [ ] The ring stays powered when dark (~14 mA idle); phase 2 decides about a switch.
- [ ] 720c once went silent on the radio after ~19 h in p005, cause not found (docs/lessons.md,
      2026-10-27). Watch for WiFi dropping over days.
- [ ] p005's flash image from 720c is saved at `.hil/s3mini-720c-p005-backup.bin` (git ignores it);
      `just flash p005 s3mini` also puts p005 back.
- [ ] NTP needs WiFi at power-up; if the network is down, the orb waits dark and logs `err` `ntp`.
