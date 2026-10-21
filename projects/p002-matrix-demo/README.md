# p002-matrix-demo

**Status:** bench-verified  ·  **Last HIL check:** 2026-10-21 m5stickc-plus11 ok

Thirteen animations on the BTF 8x8 WS2812B matrix (matrix-8x8-01), driven from the M5StickC
Plus Grove port and powered from laptop USB: the lab's first real demo.

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
columns), an `fx` event per change and `fps` about 62 every 5 s.

| Control | Does |
| ------- | ---- |
| (none) | next effect every 15 s, 600 ms crossfade (`DEMO_AUTO_MS`, `DEMO_FADE_MS`) |
| A click (big M5 button) | next effect, then stay on it (`HOLD`) |
| A hold, 0.5 s | next effect, back to `AUTO` |
| B click (side button) | brightness 8 → 20 → 40 → 80 |

Effects: rainbow, plasma, fire, rain, ripple, heart, spiral, twinkle, invader, lava, radar,
bounce, marquee (`MARQUEE_TEXT`). They all live in `shared/lab_fx`.
`DEMO_PROBE 1` shows the wiring probe (`lab_fx::wiring`) instead.

## Open issues
- [ ] Built from a `.claude/worktrees/*` checkout, the screen and `boot` show `fw=nogit`
  (docs/lessons.md); build from the main checkout for a real hash.
