# p004-sense

**Status:** bench-verified  ·  **Last HIL check:** 2026-10-23 m5stickc-plus11 ok (all six steps)

The M5StickC Plus's sensors that need no movement (mic, power chip, RTC, IMU temperature) steer
six scenes on the BTF 8x8 matrix (matrix-8x8-01), on the same Grove wiring as p002.

## Wiring
Same as p002: hold the matrix with its cable at the top.

1. Grove cable into the M5's Grove port (bottom end, next to USB-C).
2. Grove **white** (G33, `BOARD_LED_PIN_ALT`) → matrix DIN (BTF **green**).
3. Grove **red** (5 V from the AXP192) → matrix 5 V (BTF **red**).
4. Grove **black** (GND) → matrix GND (BTF **white**).
5. Grove **yellow** (G32) is unused. No level shifter (measured in p002).

| Signal | GPIO | Goes to | Notes |
| ------ | ---- | ------- | ----- |
| LED data | `BOARD_LED_PIN_ALT` (G33) | matrix DIN | wired by columns (`MATRIX_COLUMNS 1`) |
| Mic | onboard SPM1423 PDM (`AUDIO_MODE 2`) | — | on the PCB; `lab_board::mic_read` |

## Power
LEDs: 64 × WS2812B ECO · cap 400 mA (`POWER_MAX_MA`, estimated) · supply: Mac mini USB through
the M5's Grove 5 V. Measured on the USB input (AXP192): the M5 alone ~145 mA, the matrix
~0.74 × estimate + 24 mA, so ~400 mA in all at the cap. Off USB the matrix blanks and its 5 V is
switched off (Miles' call: no LEDs on the 120 mAh battery).

## Demo
`just build p004 && just flash p004 && just watch p004`; expect `boot`, `leds`, `sensors`
(pmic, vbus, imu, mic all true), `clock`, `scene`, a `sense` line every second, `mood` when
ambient changes effect, `fps` ~60 every 5 s. The LEDs stay dark for the first 3 s while the
board's own current is measured.

| Scene (A cycles) | Sensor | What it shows |
| ---------------- | ------ | ------------- |
| ambient | mic, RTC | sound energy (loudness over ~6 s) picks aurora → plasma → lava → fire and speeds it up; loudness swells brightness; a clap throws a ripple; 22:00–06:00 dims and turns amber |
| sound | mic | VU history, newest column on the right, green → red; a clap is a white column |
| warmth | IMU, power chip | IMU die temperature 50–60 °C fills the columns blue → red; the right column is the power chip 30–50 °C |
| power | power chip | plasma sweeping brightness 8 → 80 → 8 over 20 s; bottom row = measured LED mA, row above = `lab::estimate_ma`, full scale 300 mA |
| battery | power chip | charge level, green (red below 3.5 V); bubbles rise while charging |
| clock | RTC | `HH:MM` scrolling, cool white by day, amber at night |

| Control | Does |
| ------- | ---- |
| A click (big M5 button) | next scene |
| B click (side button) | brightness 8 → 20 → 40 → 80 (power sets its own) |

The screen shows the scene, USB (or battery) volts and mA, battery, IMU temperature and its
change, sound dB, level and energy, and the time.

What the sensors turned out to measure (2026-10-23, see docs/lessons.md):
- The IMU reads the board's own heat (~55 °C), which follows the LED current. A hand cannot be
  told from that, so warmth is shown, not used to steer.
- The RTC had never been set; p004 sets it from the build time (`CLOCK_UTC_OFFSET_MIN`) when it
  holds no valid date, runs behind the build, or is far ahead.
- USB current is sampled instantly by the AXP192 and the load is pulsed, so p004 reads it every
  frame and averages over 2 s.

## Open issues
- [ ] `CLOCK_UTC_OFFSET_MIN` is EDT (−240); make it −300 after 2026-11-01. The RTC keeps its
  time, so only a lost or wrong clock picks up a new offset.
- [ ] A hand on the M5 is still unmeasured: HIL step 3 passed (warmth stayed lit) but no `imu`
  readings came back, so whether the IMU sees a hand, and which way, is open.
