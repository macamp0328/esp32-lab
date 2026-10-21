# p002-matrix-demo

**Status:** bench-verified  ·  **Last HIL check:** 2026-10-21 m5stickc-plus11 ok (set 1; set 2 tilt responds)

29 animations in two sets on the BTF 8x8 WS2812B matrix (matrix-8x8-01), driven from the
M5StickC Plus Grove port and powered from laptop USB: the lab's first real demo.

## Wiring
Hold the matrix with its cable at the top; that edge is "up" for every effect.

1. Plug a Grove cable into the M5's Grove port (bottom end, next to USB-C).
2. Grove **white** (G33, `BOARD_LED_PIN_ALT`) → matrix DIN (BTF **green**).
3. Grove **red** (5 V from the AXP192) → matrix 5 V (BTF **red**).
4. Grove **black** (GND) → matrix GND (BTF **white**).
5. Grove **yellow** (G32) is unused.
6. No level shifter: the matrix takes the M5's 3.3 V data directly over the short Grove cable
   (measured 2026-10-21).

| Signal | GPIO | Goes to | Notes |
| ------ | ---- | ------- | ----- |
| LED data | `BOARD_LED_PIN_ALT` (G33) | matrix DIN | wired by columns: LED 0 top-left, down column 0, up column 1 (`MATRIX_COLUMNS 1`) |

## Power
LEDs: 64 × WS2812B ECO · cap 400 mA (`POWER_MAX_MA`) · supply: Mac mini USB through the M5's
Grove 5 V. Measured `ma_max` about 300 mA at brightness 40; the cap holds brightness 80 down.

## Demo
`just build p002 && just flash p002 && just watch p002`; expect `boot`, `leds` (pin 33,
columns), `imu` (`ok` true), an `fx` event per change (name, set) and `fps` about 62 every 5 s.
Every set-2 effect, simulations included, measured 60–62 fps.

| Control | Does |
| ------- | ---- |
| (none) | next effect every 15 s, 600 ms crossfade (`DEMO_AUTO_MS`, `DEMO_FADE_MS`) |
| A click (big M5 button) | next effect, then stay on it (`HOLD`) |
| A hold, 0.5 s | next effect, back to `AUTO` |
| B click (side button) | brightness 8 → 20 → 40 → 80 |
| B hold, 0.5 s | next set: 1 → 2 → all, from the set's first effect |

The screen shows the effect, `set N  i/n`, brightness and `AUTO`/`HOLD`.

| Set | Effects |
| --- | ------- |
| 1 (13) | rainbow, plasma, fire, rain, ripple, heart, spiral, twinkle, invader, lava, radar, bounce, marquee (`MARQUEE_TEXT`) |
| 2 (16) | aurora, pacman, life, starfield, tetris, sand, fireworks, pong, ant, tunnel, snake, marble, kaleidoscope, snow, helix, lightning |

Pure effects live in `shared/lab_fx`. The simulations (life, ant, snake, tetris, sand, marble)
live in `shared/lab_sim`; each restarts with a fresh seed when it comes on and steps at its
`DEMO_TICK_*` rate. Sand and marble follow the M5's tilt, measured from the pose it was in when
the effect started; while the M5 sits still, sand falls down and the marble circles.

Boot options in `include/config.h`: `DEMO_BOOT_SET` (1, 2, 0 = all), `DEMO_BOOT_FX` (start on
one effect, held), `DEMO_PROBE 1` (the wiring probe, `lab_fx::wiring`), `TILT_LOG_MS` (1 Hz
`tilt` events for calibration).

## Open issues
- [ ] Tilt directions are not calibrated (`TILT_SWAP_XY`, `TILT_*_SIGN` are defaults). Sand and
  marble respond, but tilting a separate stick feels arbitrary. The better fix is mounting the M5
  on the matrix frame and calibrating against real gravity (Miles, 2026-10-21: fine for now).
- [ ] Set 2 has not had a full HIL pass, effect by effect.
