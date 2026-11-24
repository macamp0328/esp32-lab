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

## Still needed for phase 2 (layout B, 2026-11-24)
Target: one protected 18650, the s3mini and a small perfboard in **one printed box on the shelf
behind the pot** (Miles, 2026-11-24). A 4-wire cable runs under the pot on felt feet and up the
drain hole to the ring, the only part in the pot. No charging in place: the cell comes out and
goes in Miles's 18650 charger. Why this shape, and the corners it fixes: the idea note's
"Design review". Wiring checklist: the [idea note](../../docs/ideas/p006-crystal-ball.md)
("Recommended wiring, layout B"); drawing with the buy list:
[p006-phase2-wiring.svg](../../docs/ideas/p006-phase2-wiring.svg). Inventory has no holder,
resistors, capacitors or small FETs, so those are buys.

**Buy now**
| # | Part | Why | Check on the listing |
| --- | --- | --- | --- |
| 1 | 18650 holder, **1 cell, open**, wire leads or PCB pins | holds the cell in the box; no switch: taking the cell out is the off switch, and putting it back opens the OTA window | 1 cell only: 2-slot holders are often wired in **series** (7.4 V), which would kill the ring (5.5 V max) and the s3mini's regulator (6 V max). Must fit **protected** cells, ~70 mm: the listing says "protected" or "button top", or gives 70 mm+ inside |
| 2 | AO3401A P-FET (SOT-23) ×5, SOT-23 to DIP adapter boards | Q3: a cell put in backwards stays harmless. Q1: the ring switch | packs are cheap; spares cover a slip of the iron |
| 3 | 2N7000 N-FET (TO-92) ×5 | Q2, drives Q1 from GPIO 2 | |
| 4 | 1/4 W resistor assortment with 100k | R4, R5: the divider the firmware reads the cell through; R1, R3 for the ring switch | any metal-film kit |
| 5 | Ceramic capacitor assortment with 100 nF | across R5: a steady cell reading | |
| 6 | Small perfboard (~2 × 8 cm) | carries Q3, the divider, the ring switch and the cable's plug, in the box | any single-sided pack |
| 7 | Adhesive felt, pads and a sheet | ~3 mm feet lift the pot over the cable; a thin ring on the rim keeps the glass off the concrete | or print the feet |
| 8 | ~~Filament, PLA or PETG~~ | the box and the drain-hole plug | on hand: two new PLAs (2026-11-24) |

**Maybe:** 74AHCT125 (if the ring's data flickers on a full cell, phase 1b); PIR, AM312 (if
presence goes the PIR way: it can look out of the box); a capacitor ≤220 µF (if WiFi resets on a
low cell).

**On hand, no buy:** the cells and 18650 chargers; red, black, green and yellow from
`wire-silicone-kit-01` for the cable, white and blue inside the box; a 4-pin housing from
`dupont-kit-01` as the cable's plug on the perfboard; heat-shrink, multimeter, crimper; the A1 mini
and PLA for it.

### Not running the cell too low (asked 2026-11-23)
Three layers, the first doing the work:
| Layer | Trips at | What it does | Left drawing |
| --- | --- | --- | --- |
| 1. Firmware, reading the divider | ~3.65 V warn, ~3.5 V cut | warn: in the evening window, a short dim red pulse (~2 s, every ~30 s) instead of the glow, so you swap the cell (Miles ok, 2026-11-23). Cut: ring switched off, S3 in deep sleep until the cell comes out | ~1 mA (the s3mini's own RGB LED and regulator idle; the divider ~20 µA) |
| 2. Ring switch | with layer 1 | without it the dark ring keeps drawing (~14 mA by the datasheet at 5 V), and layer 1 only saves the S3 | — |
| 3. The cells' own protection | ~2.5 V (typical; the wrap or datasheet gives it) | cuts the cell off entirely; a backstop, not the plan | µA |

3.5 V is also where the s3mini's regulator starts to sag in WiFi bursts, so the cut costs little
charge. Below 3.5 V a cell has ~5% left: at ~1 mA after the cut, roughly a week before layer 3
trips (estimate). A cell left in after that is still recoverable; most chargers wake a tripped
protected cell. Take the cell out when the orb is put away.

**Build** (printed on the A1 mini in PLA, no supports; OpenSCAD source, STLs, previews, the
measure list and print settings in [cad/](cad/README.md))
0. Fit test first: [fit_box](cad/stl/p006-fit_box.stl) (s3mini cradle, USB hole, cell slot) and
   [fit_plug](cad/stl/p006-fit_plug.stl) (drain-hole fit, post socket, ring seat), ~22 g.
1. The box, 88 × 53 × 30 mm with its lid: [box](cad/stl/p006-box.stl) + [lid](cad/stl/p006-lid.stl),
   eSUN PLA+ (its snaps flex at every cell swap). The holder against the back wall under the lid,
   the s3mini and the perfboard in front, a notch for the cable. The s3mini's USB-C port faces
   the cell across its slot, in line with a hole in the back wall: with the cell in, a plug stops
   against the cell ([section](cad/png/box-section.png)). + and − inside the back wall. Re-export
   with the holder's real size (`holder_*` in the `.scad`) before printing.
2. Drain-hole plug with a post: [plug](cad/stl/p006-plug_base.stl) + [post](cad/stl/p006-plug_post.stl),
   Panchroma Matte. The ring sits in the post's seat with its tabs on, pointing up, just under
   the hook's tip; grooves carry the cable up the drain hole and the post, and the plug is solid
   under its socket, so no light reaches the shelf. Posts for ring faces 26, 30 and 34 mm above
   the floor; `ring_z` sets any other.
3. Feet, optional: [feet](cad/stl/p006-feet.stl), 3 mm, or the felt pads.
4. Off the breadboard: solder the six wires straight onto the s3mini's header pins (the box has
   no room for Dupont sockets under it); heat-shrink on every joint.

**Measure** (the prototype answers these)
| What | How | Decides |
| --- | --- | --- |
| Ring height under the orb | ring in the pot on laptop USB, orb on; try the 26, 30 and 34 mm posts | the post's length (`ring_z`) |
| Holder, cell, USB-C plug, cut perfboard, drain hole | calipers, once the parts arrive | the box's size: [cad/README.md](cad/README.md) "Measure" |
| Ring dark draw | phase 1b, multimeter in series on red | whether the ring switch is fitted (above ~2 mA) |
| Brightness, color and data on a full cell | phase 1b, a fresh 4.2 V cell | `GLOW_BRIGHTNESS` and `POWER_MAX_MA`; whether the 74AHCT125 is needed |
| s3mini deep-sleep draw | phase 2, multimeter in series | runtime; whether ~1 mA holds |
| Days per cell | first week on the battery | warn and cut voltages, charge interval |

**Facts from Miles**
1. Cells: how many, brand/mAh on the wrap, resting voltage of each (3.0–4.2 V keep; under 2.5 V,
   dented or swollen: recycle). ~~Length~~: ~70 mm, protected (2026-11-23).
2. ~~Filament for the A1 mini~~: two new PLAs on hand, Polymaker Panchroma Matte and eSUN PLA+
   (2026-11-24); the 2023 PETG is probably damp, with no dryer. Not in stock.yaml.
3. For presence: an ecobee sensor in the living room? Home Assistant running? Hours a day people
   are there?
4. Schedule: still 18:00–22:00, every day?

**Decisions**
1. Presence: PIR, ecobee via Home Assistant, or stay on the clock.
2. Charge interval: a week?
3. Which patterns stay from the demo.
4. Brightness on the cell against days per cell (after the measurements).
5. ~~Box behind the pot or inside~~: one printed box behind the pot (Miles, 2026-11-24).

**Firmware, no parts**
1. Cell readout, warn (red pulse) and cut, with a margin so it does not flap at the edge.
2. Ring switch on and off; DI and CI held low while the ring is off.
3. Deep sleep between glows, waking on the RTC timer; NTP once a day on a short wake. Today's
   firmware keeps WiFi on all day (~50–100 mA, estimated): a day or two per cell.
4. An OTA window: WiFi and OTA for ~5 min after power-up, so taking the cell out and putting it
   back opens it for an update. Once the board sleeps, nothing else can reach it.
5. `DEMO_MODE 0` with the chosen pattern(s); `POWER_VOLTS` ~3.7 on the cell.
6. Presence, once decided.

**Admin (Miles's ok)**
1. Add the cells and chargers to `inventory/stock.yaml`, and 720c's move from p005 to p006.
2. A GPIO 5 row in `boards/s3mini/pinout.md` (the clock pin, verified with the ring).
3. GPIO 1 (cell sense) and GPIO 2 (ring on) named in `boards/s3mini/board.h`, before the firmware.

**Order of work:** parts arrive → phase 1b (cell to the ring only, board still on laptop USB):
true brightness, color, data and dark draw → perfboard built and tested on the bench → box and
plug printed → phase 2. With a cell in, never plug USB into the s3mini (its 5V pin is USB's, no
diode); the box makes that hard, and updates go over WiFi.

## Open issues
- [x] First light 2026-11-22: BGR is right (amber, all 20 even).
- [ ] Too dim under the orb at `GLOW_BRIGHTNESS` 48. At 72 the 150 mA cap already trims it (`bright` ~70,
      `ma_est` ~148), so more light needs another ring supply (an 18650 as in phase 1b, or 5 V with a level
      shifter), not a bigger number: the 3V3 regulator also feeds the S3 and its WiFi.
- [ ] The ring stays powered when dark (~14 mA idle by the datasheet): the ring switch is on the phase 2
      buy list; 1b measures it.
- [ ] 720c once went silent on the radio after ~19 h in p005, cause not found (docs/lessons.md,
      2026-10-27). Watch for WiFi dropping over days.
- [ ] p005's flash image from 720c is saved at `.hil/s3mini-720c-p005-backup.bin` (git ignores it);
      `just flash p005 s3mini` also puts p005 back.
- [ ] NTP needs WiFi at power-up; if the network is down, the orb waits dark and logs `err` `ntp`.
