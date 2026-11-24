# p006 crystal ball (idea, not scaffolded)

Glass orb on a planter pot; LED ring hidden in the pot under the orb; runs on a battery, no cord.
This becomes `projects/p006-crystal-ball/README.md` once `just new` runs.
Collecting facts for now; the goal comes later.

## Variables
| Variable | Value | Source |
| --- | --- | --- |
| Place | living room, on a shelf | Miles 2026-11-06 |
| Outlet nearby | none, so battery only | Miles 2026-11-06 |
| Mood | soothing background light, not a light show | Miles 2026-11-06 |
| When it glows | when people are probably around | Miles 2026-11-06 |
| Charging | **cell out, into Miles's 18650 charger**; no charging in place, no TP4056 | Miles 2026-11-23 (was USB-C, 2026-11-06) |
| Charge interval wanted | ? (a week was assumed) | Miles |
| Hours a day people are in the living room | ? (sets battery size) | Miles |
| Living room lit in the evening? | ? (a light sensor could gate the glow) | Miles |
| Orb | hollow blown glass, clear with colored confetti chips | photo 2026-11-06 |
| Orb diameter | 7–8 in (18–20 cm) by tape measure; the photos' proportions suggest nearer 6 in straight across, and circumference ÷ 3.14 would settle it | Miles 2026-11-09 |
| Orb weight | ? (rests on the pot rim, never on the insert) | scale |
| Orb hanger | a hanging ornament: clear glass neck and curled loop at the top, now pointing down into the pot | photo 2026-11-09 |
| Glass hook size | ≈ 35–45 mm from the orb's surface to the tip, loop ≈ 30 mm wide (second photo, held beside the pot; the orb was nearer the camera, so ±) | photos 2026-11-09 |
| Copper wire hanger | twisted copper loop through the glass hook, ~60–80 mm long; **comes off** | Miles 2026-11-09 |
| Clear space under the orb's hook | ≈ 1.5 in (38 mm), hook tip to pot floor; the tip sits ≈ 45 mm below the rim | Miles 2026-11-09 |
| Phase 1 schedule | on 18:00, off 22:00, every day; time over WiFi (NTP) | Miles 2026-11-09 |
| Phase 1 board | s3mini 720c (headers soldered 2026-11-22), taken from the p005 dock | its USB serial; Miles ok 2026-11-22 |
| Time zone | US Eastern, `EST5EDT,M3.2.0,M11.1.0` (taken from the Mac's own setting) | Mac 2026-11-22 |
| Pot material | concrete or cement look, opaque | photo |
| Pot height | 3.75 in (95 mm) outside | Miles 2026-11-09 |
| Pot outer diameter | 4 in (102 mm) at the top | Miles 2026-11-09 |
| Pot wall | 1 cm at the top (thicker lower down?) | Miles 2026-11-09 |
| Pot inner diameter | ≈ 3.2 in (82 mm) at the top (4 in − 2 × 1 cm); at the floor ? | derived |
| Pot inner depth | 3.25 in (83 mm), rim to floor; floor ≈ 0.5 in thick | Miles 2026-11-09 |
| Pot drain hole | 1.5 in (38 mm), in the bottom | Miles 2026-11-09 |
| Orb sinks into the mouth | ≈ 8–12 mm (sphere of 6–8 in resting on an 82 mm opening) | derived |
| Usable space | full width ≈ 82 mm × ≈ 26–40 mm tall below the glass hook's tip, plus a band ≈ 25 mm wide around the hook, up to near the rim (wire off) | derived 2026-11-09 |
| Which way the shelf faces the room | ? (where a PIR hole would point) | Miles |
| LEDs | LuMini 1 in ring v10, 20 x APA102-2020; bare, no wires yet | stock.yaml (main), photo 2026-11-22 |
| Ring pads | on the back (no LEDs), in order G, 5V, DI, CI, CO, DO; flat, 1.0 × 2.5 mm, 2.0 mm apart; printed labels agree with the copper | SparkFun board file + photo 2026-11-22 |
| Ring size | 28.7 mm outside, 19.3 mm hole; 43.2 mm across the two outer mounting tabs; the two inner tabs leave a ~5 mm gap. Tabs have a row of 0.5 mm holes, so they look made to snap off (inferred) | SparkFun board file |
| Ring bare metal | all four mounting rings are **ground**; the two square pads by "17" are C1 (5V to ground), marked do-not-populate | SparkFun board file |
| Ring lead wire | `wire-silicone-kit-01` (Trobing, FNSKU X0024CUW83; UL 3132, tinned strands, silicone); gauge ?, printed on the wire (22–26 AWG suits the pads; 22 is ~1.6 mm over the insulation) | photo 2026-11-22 |
| Prototype board | s3mini on laptop USB | decided 2026-11-06 |
| Battery | **18650 Li-ion, Miles's spare cells** (1, or 2 in parallel), one-cell supply | Miles 2026-11-09 |
| Battery location | outside the pot on the shelf, cable under the pot and up through the drain hole (one 18650 would also fit inside) | leaning outside |
| 18650s on hand | **protected, ~70 mm**; count, brand/mAh on the wrap, age/source ?; not in stock.yaml yet | Miles 2026-11-23 |
| 18650 charger on hand? | yes, more than one | Miles 2026-11-23 |
| 18650 holder | buy: 1 cell, open (no switch: cell out = off), fits 70 mm protected cells, in a printed box behind the pot | 2026-11-24 |
| Battery, inside | 2000 mAh pouch or one 18650 fits; the 4400 mAh pack leaves no room under the hook | derived 2026-11-09 |
| Final board | s3mini (on hand), in the printed box, cell on its 5V pin through Q3, a reverse guard (no TP4056 since 2026-11-23); Feather ESP32-S3 is the alternative | Miles 2026-11-09 |
| Ring supply | straight off the 18650 (3.0–4.2 V); no level shifter, no boost. **Below the datasheet's 5.0 V typical / 5.5 V max** (it prints no minimum), so dim or off-color is possible: the prototype decides | follows 2026-11-09; datasheet read 2026-11-22 |
| Presence sensor | PIR, or ecobee occupancy via Home Assistant (candidates) | not decided |
| ecobee sensor in the living room? | ? (a SmartSensor, or the thermostat itself) | Miles |
| Home Assistant running? | ? (needed for the ecobee route) | Miles |
| ecobee paired to Apple Home now? | almost certainly not: no Apple phones in the house, so HA can pair with it directly | Miles 2026-11-06 |
| Brightness wanted | ? (low: background, not a lamp) | prototype |
| Ring height under the orb | ? | prototype |

## What the plan still needs (2026-11-09)
Current list, with the buys: the project README, "Still needed for phase 2" (layout B, 2026-11-24).

**Decisions (Miles):**
1. The goal, in one line. Draft: "a soft glow in the orb whenever someone is in the living room,
   about a week per charge, with nothing visible but the orb and pot."
2. Presence: PIR (hole in the pot) or ecobee via Home Assistant. **Deferred to phase 2**;
   phase 1 runs on a clock.
3. Charge interval: is a week right? **Deferred to phase 2.**
4. ~~`BOARD_LED_CLOCK_PIN 5` in `boards/s3mini/board.h`~~: yes (Miles, 2026-11-22), added. Later
   also SWITCH, ADC and PIR pins.
5. ~~Which s3mini~~: 720c, off the p005 dock (Miles, 2026-11-22; planned as the spare until then).
6. ~~A yes to WiFi in p006 (for NTP time)~~: yes (Miles, 2026-11-22). Time zone: US Eastern, from
   the Mac.

**Facts (Miles, a look or a photo):**
1. Photo of the s3mini's pin labels (header order).
2. The 18650s: how many, brand/mAh, protected or not, condition. Is there a charger?
3. ecobee sensor in the living room? Home Assistant running?
4. Hours a day people are in the living room; is it lit in the evening?
5. Which way the shelf faces (only matters for a PIR).
6. ~~Hook tip below the rim~~: ≈ 1.5 in clear under it (Miles, 2026-11-09).

**Measured by the prototype (no one has to guess):**
1. How the ring looks in the orb at 3.3 V; ring height; palette; lowest brightness that reads.
2. Ring current lit and dark (decides the ring switch).
3. WiFi signal from inside the pot (decides whether OTA and the ecobee route work).
4. Brightness and current on an 18650 (phase 1b).

**Then the buy list** (TP4056 module, holder, plugs, resistors, maybe the switch FETs and a PIR,
felt feet) and **the firmware** (clocked-LED template support, p006 scaffold, effects,
deep sleep, battery sense, presence, OTA).

## Miles's answers
1. (2026-11-06) Prototype first with what is on hand; after that, buying anything is fine.
2. (2026-11-06) No charging pad: charge over USB-C.
3. (2026-11-06) Glow when people are probably around (presence), not all day.
4. (2026-11-06) The orb is clear glass with colored confetti chips (photo notes below).
5. (2026-11-06) Living room shelf, no outlet nearby.
6. (2026-11-06) Soothing background light, not a disco ball.
7. (2026-11-09) Measured: orb 7–8 in; pot 3.75 in tall, 4 in across at the top, 1 cm walls
   at the top, 3.25 in deep inside, 1.5 in drain hole in the bottom.

## Plan
**Phase 1: prototype, on hand, on USB (scope cut 2026-11-09).**
- s3mini 720c + LuMini ring on a breadboard, laptop USB, ring on the board's 3V3.
- **On a clock:** fade up at 18:00, fade down at 22:00, every day. No people sensing, no
  battery, battery life not a concern.
- The s3mini has no RTC (`BOARD_HAS_RTC 0`), so the time comes from WiFi (NTP) at boot and
  every few hours after. WiFi on means OTA works too, and the log can report WiFi signal
  strength (useful later inside the pot).
- Proposed: glow for the first minute after power-up whatever the time, so a reset shows the
  ring works; then follow the clock.
- If NTP fails, stay dark, log an `err` (`code: "ntp"`), retry every minute.
- Pure logic with a native test: `level = schedule_level(minute_of_day, on, off, fade)` → 0–1,
  including windows that cross midnight.
- Still the goal of phase 1: how 20 LEDs look in the orb, the ring height, the lowest
  brightness that reads, the palette.

### Day one: breadboard, laptop USB, no battery (planned 2026-11-09)
Phase 1a from [p006-phase1-wiring.svg](p006-phase1-wiring.svg), on a breadboard, no touch wire.
Goal: the ring glows softly under the orb from 18:00 to 22:00, and `just watch` shows `boot`,
`leds`, `fps`, `wifi`.

Parts (on hand except where noted):
- s3mini 720c, headers soldered; a USB-C cable that carries data (not the M5's
  charge-only one);
- the LuMini ring; a breadboard (not in stock.yaml);
- red, black, green, yellow silicone wire (`wire-silicone-kit-01`), **~30 cm each** (corrected
  2026-11-22 from 15 cm: the wire runs from under the hook, down the drain hole, under the pot and
  out to the breadboard);
- Dupont male crimp pins + crimper; heat-shrink.

**The breadboard stays outside the pot.** A half-size one (82 × 55 mm) does not fit the 82 mm
mouth. Stand the pot on feet or two books so the wires leave by the drain hole; never pinch them
between the glass and the rim.

Solder 1, s3mini headers:
1. Break the header strip to the length of each row. Push the long ends into the breadboard,
   lay the board on the short ends, component side up: that holds the pins square.
2. Solder one corner pin, check the board sits flat, then the rest.
3. Multimeter: no short between neighboring pins; **3V3 to GND** and **5V to GND** not shorted.

Solder 2, ring leads (4 wires; leave CO and DO bare). Photo with each pad marked:
[p006-ring-pads.jpg](p006-ring-pads.jpg). Phone checklist (private to Miles):
https://claude.ai/artifact/B6HV8YAzazwZfveoDzVzbm
0. Wipe the pads with isopropyl alcohol if you have it: they look dull in the photo. Rosin-core
   solder only, never plumbing (acid) flux. Strip ~2 mm, twist, tin.
1. **black → G**, **red → 5V**, **green → DI**, **yellow → CI** (the order they sit in).
2. Tin the pad, tin the wire, then join, a second or two per joint. The datasheet allows the LEDs
   260 °C for 5 s, so keep the iron on the pad and off the LEDs. G sits in a ground pour (2 oz
   copper): it may want a second longer. No solder on the two square C1 pads; keep red clear of
   the bare mounting ring beside it (ground).
3. Wires leave from the side without LEDs, so the ring can face up under the orb.
4. Crimp a male Dupont pin on each free end. The ring pads are flat, so a dab of hot glue over
   the joints is the strain relief (heat-shrink only fits the wire ends).
5. Multimeter: each wire reads through to its pad; **red to black not shorted**.

Breadboard:
1. s3mini straddles the center channel, USB-C end hanging off the edge.
2. Red into the row of the **3V3** pin. Not 5V: the ring's inputs need a level shifter at 5 V.
3. Black into a **GND** row.
4. Green into the **GPIO 4** row (data).
5. Yellow into the **GPIO 5** row (clock).
6. Before plugging in: multimeter, red row to black row, not shorted.
7. USB-C to the laptop. `just flash p006 s3mini && just watch p006 s3mini`.

Unplug the p005 dock first (it holds another s3mini), or name the port with `--port`.

Firmware for day one (`projects/p006-crystal-ball`, flashed to 720c 2026-11-22, rung 4 with nothing
wired: WiFi up in 2 s, clock synced in 5 s, fps 76, level 1 at 20:38):
- Template support for clocked LEDs (`LED_CLOCK_PIN`), scaffold p006 with `just new`.
- `POWER_VOLTS 5`, `POWER_MAX_MA 150`: FastLED's limiter counts milliwatts at 5 V per channel, so
  5 makes 150 mA the real current cap (3 would cap it near 65 mA). Well inside the regulator's
  800 mA (ME6217, if the s3mini is the S3-Zero), with room for WiFi.
- Ring clock ~1 MHz (the LEDs take more; slow is kinder to 30 cm of loose wire).
- SparkFun's guide warns that full white at full brightness damages the ring. Its ring test
  (read on GitHub 2026-11-22) uses `APA102`, `BGR` and brightness 32 of 255: start there, under
  the 150 mA cap. Any two pins work, the guide says.
- `WIFI_ENABLED 1` and `OTA_ENABLED 1`, `lab_net` back in; Miles copies `secrets.example.h` to
  `secrets.h` and types in the network name and password himself.
- NTP time in Miles's time zone (a POSIX TZ string, so daylight saving is handled).
- Soft warm glow with a slow drift between 2–3 hues (minutes per change), 18:00–22:00, with
  fades a few minutes long at each end; 1 minute of glow after power-up.
- If the ring is dim or flickers at 3.3 V, that is the cue for 1b (cell + holder).

Decided 2026-11-22: WiFi on, `BOARD_LED_CLOCK_PIN` 5, time zone US Eastern. `secrets.h` is a copy
of p000's (Miles ok, 2026-11-22). A photo of the s3mini's pin labels lets the breadboard drawing match the real
header order (not recorded in pinout.md, and the web sources disagree), but the code does not
need it. The board is s3mini 720c, headers soldered.

### Phase 1 on the s3mini (asked 2026-11-09: no Feather needed)
The Feather only matters once a battery powers the board. Phase 1 runs the s3mini from laptop
USB; everything is on hand.

Ring power, in order:
1. **From the s3mini's 3V3 pin** (first look, no extra parts). 3.3 V data at a 3.3 V supply is
   fine. Blue and green may come out dimmer than they will on the cell, so this run picks
   placement and palette, not final brightness. Cap `POWER_MAX_MA` low (~150 mA): the board's
   regulator (ME6217, 800 mA if this is the S3-Zero) also feeds the S3 and its WiFi bursts.
2. **From one 18650 in a holder** (once a holder is here), the cell's ground tied to the
   s3mini's GND: the final supply, so the true brightness, color and current.
- Not from USB 5 V: the ring would need a level shifter. The only one on hand is the 74HCT245 on
  `octows2811-01`, with a Teensy soldered to it; a loose 74AHCT125 is a ~$2 buy.
- 3.3 V is outside the datasheet (5.0 V typical, 5.5 V max, no minimum printed; everything tested
  at 5 V). It should not harm the ring; it may be dim. That is what 1a finds out.
- The datasheet caps the data and clock inputs at supply + 0.4 V. So the firmware holds DI and CI
  low whenever the ring has no power (1b with the cell out, phase 2 with the ring switch off).

Diagram: [p006-phase1-wiring.svg](p006-phase1-wiring.svg) (1a and 1b).

Wiring draft, 1a (confirm at build time; the ring is bare, so 4 wires to solder):
1. Ring **5V** pad → s3mini **3V3**, red wire.
2. Ring **G** pad → s3mini **GND**, black wire.
3. Ring **DI** pad → GPIO **4** (`BOARD_LED_PIN`), green wire.
4. Ring **CI** pad → GPIO **5** (`BOARD_LED_CLOCK_PIN`), yellow wire.

Wiring draft, 1b (ring on one 18650; board still on laptop USB):
1. Unsolder or unplug the red wire from the s3mini's **3V3**: one ring supply at a time.
2. Cell holder **+** → ring **5V** pad, red wire.
3. Cell holder **−** → ring **G** pad *and* s3mini **GND** (grounds joined), black wire.
4. Green (DI → GPIO 4) and yellow (CI → GPIO 5) stay as in 1a.
5. Multimeter-01 in series on the red wire: read the ring's current lit, and dark (that
   decides whether phase 2 needs the ring switch).

### Phase 2 wiring, Feather alternative (sketch; nothing bought yet)
Diagram: [p006-phase2-feather-wiring.svg](p006-phase2-feather-wiring.svg). The s3mini plan
below is the one to build; this stays as the fallback. Feather pin numbers come from its
`board.h` at `just add-board`; DATA, CLOCK, SWITCH, PIR below are roles.
1. Cells in the holder → (1S protection board, only if unprotected cells) → 2-wire lead, red +
   and black −, under the pot and up through the drain hole.
2. Lead → JST-PH 2-pin plug → Feather **BAT** jack. Check red lands on + before plugging in:
   JST-PH polarity differs between vendors.
3. Feather **BAT** pin → ring switch IN → ring **5V** pad, red. No switch: BAT straight to 5V.
4. Feather **GND** → ring **G** pad and switch GND, black.
5. Feather **DATA pin** → ring **DI**, green. Feather **CLOCK pin** → ring **CI**, yellow.
6. Feather **SWITCH pin** → switch EN, blue.
7. Option A, PIR (AM312): **VCC** → Feather 3V3 (red), **OUT** → PIR pin (white, an RTC GPIO so
   it can wake the S3), **GND** → GND (black). Option B, ecobee via HA: no wires.

### Phase 2 on the s3mini instead of a Feather (asked 2026-11-09)
The s3mini matches Waveshare's **ESP32-S3-Zero** (same ESP32-S3FH4R2, RGB LED on GPIO 21):
- Its regulator is an ME6217C33 LDO (800 mA). Waveshare and CNX Software list **3.7–6 V on the
  5V pin**, so one Li-ion cell can feed it directly. No charger on the board.
- **Treat the 5V pin as USB VBUS with no diode.** Inferred, not seen drawn: the schematic PDF's
  text has no diode labels, and a netlist mirror puts the 5V header pin, the USB-C VBUS pins and
  the LDO input on one net. Multimeter diode test, VBUS to 5V pin, would confirm it.
  So **never plug USB into the s3mini while the cell is connected**: USB's 5 V would go
  straight into the cell. Flash it over WiFi (OTA) once it is in the pot, or unplug the battery
  connector first.
- Below ~3.6 V on the cell, the 3.3 V rail starts to sag (LDO dropout), and WiFi bursts
  (~300 mA) make it worse. The firmware should sleep for good at ~3.5 V. A Li-ion cell holds
  most of its charge above that, so the loss is small.
- No fuel gauge: a 2-resistor divider (2 × 100k, ~20 µA) from cell + to an ADC pin
  (GPIO 1–10 are ADC1) reads the voltage. No resistors on hand: buy an assortment.

Diagram: [p006-phase2-wiring.svg](p006-phase2-wiring.svg) (redrawn 2026-11-24 for layout B: the
cell, s3mini and perfboard in one printed box behind the pot, Q3 reverse guard, a 4-wire cable to
the ring; cell sense and ring switch on proposed GPIO 1 and 2 *).

Recommended wiring, layout B (2026-11-24; the TP4056 went 2026-11-23, cells charge out of the
orb). Everything but the ring sits in the printed box. Low-cell protection: the project README,
"Not running the cell too low". * = proposed pin, not in board.h yet.
1. Holder **−** (black) → perfboard ground.
2. Holder **+** (red) → **Q3** drain. Q3 gate → ground. Q3 source is the **+ bus**.
3. + bus → s3mini **5V** pin (red). s3mini **GND** → ground (black).
4. Cell sense: + bus → R4 100k → **GPIO 1** * (white) → R5 100k → ground; 100 nF across R5.
5. Ring switch: + bus → Q1 source; R1 100k from + bus to Q1 gate; Q2 drain → Q1 gate, Q2 source
   → ground, Q2 gate → **GPIO 2** * (blue); R3 100k from Q2 gate to ground.
6. 4-pin Dupont plug on the perfboard (kit on hand): Q1 drain (red, ring +), ground (black),
   **GPIO 4** (green, data), **GPIO 5** (yellow, clock).
7. Cable, ~30–40 cm, silicone wire: from the plug out of the box, under the pot on felt feet, up
   the drain hole. Red → ring **5V**, black → **G**, green → **DI**, yellow → **CI**. CO, DO bare.
8. No ring switch (1b reads the dark ring under ~2 mA): + bus straight to the plug's red pin.
9. Only if WiFi resets show up on a low cell: ≤220 µF across the s3mini's 5V and GND (the 1000 µF
   on hand can trip the cell's protection when the cell goes in).

Energy: about the same as the Feather (s3mini asleep ~1 mA, mostly its RGB LED idling):
~8 days on one ~3,500 mAh cell at 5 h/day, ~2 weeks on two.

| | Feather ESP32-S3 | s3mini (on hand) |
| --- | --- | --- |
| Buy | Feather, holder, JST lead | holder, TP4056 module, resistors |
| Battery % | MAX17048 gauge | voltage via divider (rougher) |
| Charge in place | USB-C on the Feather | USB-C on the TP4056 module, behind the pot |
| USB while battery in | safe | **never**: flash over WiFi |
| Usable cell range | down to ~3.3 V | down to ~3.5 V |

Ring switch (fit only if 1b shows a dark draw above ~2 mA):
| Part | What | Connections |
| --- | --- | --- |
| Q1 | P-FET, logic level (AO3401A or similar, SOT-23) | S → BAT, D → ring 5V, G → node X |
| R1 | 100k | BAT → node X (holds Q1 off) |
| Q2 | small N-FET (BSS138, or 2N7000 in TO-92 for easy soldering) | D → node X, S → GND, G → SWITCH pin |
| R3 | 100k | Q2 gate → GND (holds Q2 off while the S3 sleeps or boots) |
SWITCH high → Q2 pulls node X to ground → Q1 on → ring powered. SWITCH low or floating → ring
off, ~0 mA. None of these parts are on hand: buy.

**Phase 2: battery build, bought parts.** s3mini (on hand) + one of Miles's 18650s in a printed box
behind the pot (layout B, 2026-11-24) + Q3 reverse guard + ring power switch + presence sensor,
sized from what phase 1 measures.

### 18650 plan (decided 2026-11-09)
1. **Check the spare cells first** (multimeter-01, no load): resting voltage 3.0–4.2 V is fine.
   Under ~2.5 V, a dented or torn wrap, swelling, or heat while charging means recycle it.
   Old laptop-pack cells are usually flat-top and unprotected.
2. **Protection:** protected cells have a small circuit at the + end and run ~69–70 mm long.
   Unprotected cells need a 1S protection board (~$1, DW01-type) in the holder's lead. The
   firmware also sleeps for good at ~3.3 V (MAX17048 reading), but that alone is not enough.
3. **Parallel pair:** only cells of the same model, charged together, inserted full.
4. **Buy:** Feather ESP32-S3 (#5477); an 18650 holder with a cover (1 or 2 cells, parallel);
   a JST-PH 2-pin lead to the Feather's battery jack (check polarity: red to +); a protection
   board if the cells are unprotected; a standalone 18650 charger if none is on hand.
5. **Runtime at 5 h/day, ring switched off asleep:** ~8 days on one ~3,500 mAh cell, ~2.5 weeks
   on two. Old spares may have lost a good share of their capacity: the MAX17048 log will show
   the real figure after the first week.

## Design review (asked 2026-11-23: simpler? cutting corners?)
### Corners the plan was cutting
| # | Risk | What happens | Fix |
| --- | --- | --- | --- |
| 1 | Cell put in backwards | about −3.7 V into the s3mini and the ring: both likely dead. Holders do not stop it | **Q3**, an AO3401A (same pack as Q1) as a reverse guard in the + line: drain to cell +, source to the load, gate to GND. Correct way round it conducts (~0.05 Ω); backwards it is off. Mark + in the print too |
| 2 | USB into the s3mini with the cell in | USB's 5 V goes straight into the cell (5V pin = USB power, no diode) | by design: the s3mini's USB-C reachable only through the cell's hatch, so the cell is out first. A diode would cost ~0.3 V of the cell's range |
| 3 | Data level on a full cell | APA102 wants **0.7 × VDD** (datasheet): 2.94 V at 4.2 V; the S3 gives ~3.3 V. Thin margin, fine at 3.3 V supply today | phase 1b on a fresh cell watches for flicker. Fallback: a 74AHCT125 (buy) between S3 and ring; the 74HC125 on hand has the same 0.7 × VCC threshold, so it does not help |
| 4 | Color drift as the cell falls 4.2 → 3.5 V | blue and green need headroom; warm colors drift least | 1b shows it; the firmware can scale brightness by the cell reading |
| 5 | Cell reading off | 100k/100k is a weak source for the S3's ADC; ±0.1 V moves the 3.5 V cut | 100 nF across R5, average 16+ reads, calibrate once against the multimeter |
| 6 | Big capacitor across the cell | the 1000 µF on hand can trip the cell's short protection on insertion | only if WiFi resets show up, and ≤220 µF |

Not cut: overcharge (Miles's 18650 chargers), shorts and overcurrent (the cells' own protection),
over-discharge (three layers: firmware cut, ring switch, cell protection).

### What can go
| Part | Verdict |
| --- | --- |
| Switched battery box | **go**: an open 1-cell holder. The cell comes out to charge anyway, so "cell out" is the off switch, and putting it back in opens the OTA window |
| JST plug in the lead | **go**: the cable needs 4 wires, and a 4-pin Dupont plug (kit on hand) where it meets the perfboard lets it be threaded up the drain hole first |
| Felt feet | **stay** with B (the cable runs under the pot); they would go with C |
| Ring switch (Q1, Q2) | keep on the buy list (the AO3401A pack also gives Q3), fit only if 1b reads the dark ring above ~2 mA. The datasheet's 0.7 mA per LED is at 5 V; at 3.7 V it may be less |
| Cell sense divider | **stays**: it is the main protection, two resistors |
| Diode against USB backfeed, boost to 5 V | not worth it: range lost, or more parts and idle draw |

### Everything in one printed box (asked 2026-11-23)
Moving the cell, the s3mini and the perfboard out of the pot into one print takes everything but the
ring out of the pot, gets WiFi out of the concrete (the "WiFi inside the pot" measurement goes away),
and gives a PIR a plastic window instead of a hole drilled in concrete.

| | B: box behind the pot | C: base under the pot |
| --- | --- | --- |
| Into the pot | 4-wire cable under the pot, up the drain hole, to a ring stand inside | a post rises from the base through the drain hole, the ring on top; nothing else in the pot |
| In view | a small box behind; the pot on ~3 mm feet over the cable | a ~30 mm base under the pot; no wires |
| Ring wire length | ~30–40 cm (30 cm works today at 1 MHz) | ~10 cm |
| Cell swap | the box's lid | a hatch in the base's side; the pot stays put |
| Pot | needs feet | sits on the base, lifts off with nothing attached |
| Needs | nothing new | the ring's two outer tabs snapped or filed off (43 → 28.7 mm, to pass the 38 mm drain hole); the pot's bottom diameter and whether it sits flat |

Sizes: 18650 holder ~78 × 21 × 20 mm, s3mini 24 × 18 mm, perfboard ~20 × 40 mm: a box ~90 × 50 × 30 mm
(B) or a base ~110 mm across × ~30 mm (C), both well inside the A1 mini's 180 mm. The ring's height
under the hook becomes the post's length (C), so 1a/1b can try two or three posts.

**Chosen 2026-11-24: B, the box behind the pot (Miles).** The ring keeps its outer tabs (it goes
in from the top), and the pot's bottom needs no measuring. In the box: the open holder under the
lid, the s3mini and the perfboard; the s3mini's USB-C port opens into the cell's slot, so a cable
fits only with the cell out (corner 2). In the pot: a printed drain-hole plug with a post that
holds the ring just under the hook's tip, and a notch for the cable. WiFi now works from the box,
outside the concrete. Wiring: "Recommended wiring, layout B" above.

Buy list for B (replaces the switched box and the JST plug):
1. 18650 holder, 1 cell, open, fits ~70 mm protected cells (wire leads or PCB pins).
2. AO3401A P-FET (SOT-23) ×5 + SOT-23 to DIP adapters: Q1 ring switch, Q3 reverse guard.
3. 2N7000 N-FET (TO-92) ×5: Q2.
4. Resistor kit with 100k.
5. Ceramic capacitor kit with 100 nF (cell sense).
6. Perfboard, ~2 × 8 cm.
7. Adhesive felt, pads and a thin sheet: feet over the cable, a ring on the rim under the glass.
8. Filament, if none is on hand.
Maybe: 74AHCT125 (if the data level flickers on a full cell), PIR.

## On hand that helps (inventory/stock.yaml on main, 2026-11-06)
- `ring-apa102-20-01`: the ring. Bare pads, so 4 wires to solder (G, 5V, DI, CI).
- `printer-a1mini-01` (Bambu A1 mini, 180 mm cube): a ring stand at the right height inside
  the pot, a board and battery cradle, a bezel for a PIR hole, a diffuser if one is needed.
- `multimeter-01` (µA/mA ranges): measures the ring's idle draw and the board's sleep current,
  the two numbers the battery size hangs on.
- `wire-silicone-kit-01`, `heatshrink-kit-01`, `standoffs-m25-kit-01`, `soldering-station-01`.
- `mosfet-dual-01..02` (NOYITO, low-side): a possible ring power switch. Unverified at LiPo
  voltage and 3.3 V gate drive, and cutting the ring's ground while DI/CI stay connected can
  feed the ring through its data pins. Measure it before trusting it.
- Boards: s3mini x3 (no charger), M5StickC Plus x2 (120 mAh internal cell: about an hour),
  ATOM Lite (no board folder, no charger).
- Still none: LiPo cell, charger board, PIR, light sensor.

## Presence on a battery
| Sensor | Idle draw | Sees | Verdict |
| --- | --- | --- | --- |
| PIR (AM312 ~$2, or Panasonic EKMB ~$15) | ~15 µA (AM312), 1–6 µA (EKMB) | motion, line of sight; **not through glass** | pick: its output pin wakes the S3 from deep sleep |
| Room light (photoresistor/ALS on ADC) | ~µA with timed wakes | lights on in the room | cheap second signal: "lights on" ≈ people |
| mmWave radar (LD2410-class) | ~80 mA always on | people sitting still, through plastic/ceramic | battery dead in about a day: no |
| WiFi RF (p005's RSSI/CSI trick) | radio on, ~100 mA | movement near the link | same problem; p005 also needed hands within inches |
| ecobee room occupancy, via Home Assistant | ~1–3 mA average (a short WiFi wake every 1–3 min) | the room's ecobee sensor: occupancy, held a while after motion | strong candidate if HA is (or can be) running; see below |

### ecobee as the presence signal (asked 2026-11-06)
- **Cloud API: closed.** ecobee stopped issuing developer API keys on 2024-03-28 (Home
  Assistant's ecobee docs); only existing keys still work. So the orb cannot ask ecobee's cloud
  directly.
- **Local route: HomeKit → Home Assistant → orb.** The ecobee publishes its sensors' occupancy over
  HomeKit. Home Assistant's HomeKit Device integration reads it locally, and users report it
  updates near-instantly (the cloud path was 1–3 min). The orb wakes every 1–3 min, joins
  WiFi, asks HA's REST API (`/api/states/binary_sensor.<living room occupancy>`, long-lived
  token in `secrets.h`), and goes back to sleep. Plain HTTP on the LAN, no TLS.
- **One HomeKit controller at a time:** no Apple phones in the house (Miles, 2026-11-06), so the
  ecobee is almost certainly unpaired and HA can pair with it using the HomeKit code the
  thermostat shows in its settings. No Apple device is needed for that.
- **Fit:** occupancy is "someone has been here recently", not motion, so it covers someone
  reading still on the couch and changes slowly. That suits a soothing light better than a PIR.
  Users report the thermostat's own occupancy sometimes sticking "occupied"; the remote
  sensors did not.
- **Cost:** ~30–70 mAh/day for the polling vs. under 1 mAh for a PIR. At 5 h/day of glow, a 3500
  mAh 18650 goes from ~10 days to ~9. No hole in the pot, no PIR to buy.
- **Risks:** WiFi through a concrete pot (the prototype logs RSSI); the orb goes dark when HA is down
  (fallback: glow on a schedule, or touch).
- Needed: is there an ecobee sensor in the living room, and is HA running (or would Miles run
  it).

PIR needs to look out of the pot (a small hole in the side, with a printed bezel), not up
through the orb. A PIR sees motion, not stillness: someone reading on the couch stops
triggering it, so the glow needs a long hold (minutes) before it fades.

## Power budget (estimates, nothing measured yet)
| Load | Current |
| --- | --- |
| Ring, full white (SparkFun's ~60 mA/LED rule) | ~1.2 A peak; the FastLED cap stops it |
| Ring, soothing background glow | ~20–50 mA (guess; prototype measures it) |
| Ring, all LEDs dark but powered (datasheet: 0.7 mA per LED at 5 V) | ~14 mA: needs a power switch for sleep |
| ESP32-S3 awake, WiFi off, 80 MHz | ~25–40 mA |
| Feather ESP32-S3 deep sleep + PIR, ring switched off | ~0.05–0.1 mA |

A soothing glow is the cheap case: ~70 mA total while glowing.
| Hours glowing per day | 2000 mAh pouch | 3500 mAh 18650 |
| --- | --- | --- |
| 3 h | ~9 days | ~2 weeks |
| 5 h | ~5.5 days | ~10 days |
| 8 h | ~3.5 days | ~6 days |

The ESP32 itself becomes the biggest draw at this brightness. APA102s hold their color without
being refreshed, so a slow animation could let the S3 light-sleep between updates. Phase 2
can measure whether that helps.

## The orb and pot
- **Orb:** hollow blown glass, confetti of colored chips: translucent teal, green, amber, brown,
  blue, purple, and opaque-looking white, pink, pale blue, black. Light from below makes the
  translucent chips glow like stained glass; opaque ones stay dark or glow faintly. With this
  many chips the 20 LED dots will hardly show, so it probably needs no diffuser.
- **Color picks the chips:** warm white lights every chip; a pure color lights only the chips
  that pass it (red → red/pink/amber; teal → teal/green/blue).
- **Big orb, small ring.** A 6–8 in ball lit by one 1 in ring through an 82 mm opening: the
  bottom will glow most and the top least. At a soothing brightness that gradient may suit it.
  The prototype shows whether the top goes dark.
- **Light enters only where the orb sits in the pot's mouth.** Ring centered under it, pointing
  up; its height sets hot spot vs. spread. Find the height in the prototype.
- **The glass hook is a window.** The neck around the hook is mostly clear glass with few
  chips, and it now faces down into the pot. Light aimed up at it enters the hollow ball through
  clear glass, lights the inside, and comes out through the chips. That should spread more
  evenly than light pushed through chips from outside. The hook itself is a solid clear rod,
  so it may glow as a bright light pipe; it sits below the rim, so nobody sees it.
- **Pot:** concrete or cement look, opaque; the orb rests on its rim, no gap. A PIR needs a
  ~10 mm hole drilled through the 1 cm side wall (masonry bit). WiFi/OTA through it will be
  weaker.

## Space inside the pot (measured 2026-11-09)
- Inside: ≈ 82 mm across at the top, 83 mm from rim to floor, minus ~8–12 mm where the orb
  sinks in: a space ≈ 82 mm across × ≈ 70 mm tall. The floor's width is unknown; the walls
  may thicken lower down.
- **The orb's glass hook hangs down the middle** (photos 2026-11-09): ≈ 35–45 mm of glass
  below the orb, which itself sits ~8–12 mm below the rim. Its tip is then ~43–57 mm below the
  rim, leaving ~26–40 mm of full-width space above the floor. Around the hook there is a
  band ≈ 25 mm wide (82 mm across minus the ~30 mm loop) reaching almost to the rim, since
  the ball meets the pot at the rim's inner edge. The Feather (51 × 23 × ~7 mm) stands on end
  in that band against the wall. The copper wire comes off (Miles, 2026-11-09).
- The 1.5 in (38 mm) drain hole is in the middle of the floor: an insert can be pushed out
  from below, and air gets in.
- Ring (28.7 mm; 43 mm with its outer tabs, wider than the drain hole) and board fit easily:
  Feather ESP32-S3 is 51 × 23 mm, s3mini is smaller.
- **Battery fit:**
  | Cell | Size | Fits? |
  | --- | --- | --- |
  | 2000 mAh flat pouch | ~60 × 36 × 7 mm | yes, flat on the floor |
  | single 18650 with leads, ~2200–3500 mAh | ~69 × 18 mm | yes, lying across the floor |
  | 4400 mAh two-cell pack | ~69 × 37 × 18 mm | flat needs ~79 mm at the floor (probably not); on edge it is 37 mm tall, which fills the space under the hook: no |
- Insert stack (printed): battery flat on the floor (7–18 mm), ring just under the hook's tip
  pointing up, board standing beside the hook. Nothing touches the orb or the hook. At the
  tight end (26 mm under the tip), an 18650 (18 mm) leaves ~8 mm for the ring and its mount:
  the flat pouch is the safer pick.
- The ring cannot go around the hook: its hole (19.3 mm, ~5 mm between the inner tabs) is smaller
  than the ~30 mm loop.
- With the hook, plan on 2000–3500 mAh (pouch or one 18650): at 5 h/day of glow, ~5.5–10 days
  a charge.
- **Charging access:** lift the orb off and plug USB-C in from the top (no parts). A
  port in the drain hole would face the shelf, so the pot would have to be tipped to plug in:
  not worth it.

## Battery outside the pot (Miles's idea, 2026-11-09)
A USB power bank sits behind the pot on the shelf; a USB cable runs under the pot and up through
the drain hole to the board. To charge: unplug the bank, charge it anywhere, plug it back in
(or keep two and swap).

**What changes**
- The pot holds only the board and the ring (and a shifter): no battery under the hook, so the
  ring can sit wherever looks best.
- No charger or fuel gauge on the board, so the s3mini (on hand) can be the final board, not
  just the prototype. The Feather is only needed if the battery goes inside.
- The phase 1 prototype (s3mini on USB) is already this power setup, with a laptop standing in
  for the bank.
- The bank is a finished, protected product. No bare LiPo cell sits on the shelf.

**The catch: most power banks switch themselves off** when the draw is small, so the orb
would go dark and never wake from sleep. Voltaic's V25/V50/V75 have an "Always On" mode
meant for this: they stay on at any draw. Use their USB-A port: Voltaic says the V50's USB-C
PD port should not be used in Always On mode. Voltaic lists ~7 mA (26 mW) at USB-A for the
pack itself. Voltaic updated the line in 2025, so check which revision a listing sells.

**5 V on the ring.** USB gives 5 V. At 5 V the ring's inputs want ~3.5 V for a high, above the
board's 3.3 V, so: a 74AHCT125 level shifter (buy, ~$2; the 74HC125 on hand cannot do it), or
power the ring from the board's 3.3 V and see if it is bright and even enough at a soothing
level. The prototype can try both.

**Cable under the pot.** The drain hole faces the shelf, so the cable needs a way out: three
printed or felt feet (~5 mm) under the pot, or a printed coaster with a cable channel. A
right-angle USB plug keeps the bend short. The bank hides behind the pot and the orb.

**Energy, outside bank (estimates):** glow ~0.35 W at 5 V (ring ~40 mA + S3 ~30 mA); asleep
~5 mW for the s3mini (its onboard WS2812 idles ~1 mA); bank's own ~26 mW; ecobee polling
~10 mW average.
| Bank | Usable at 5 V (≈80%) | Ring switched off asleep | Ring left powered (~75 mW idle) |
| --- | --- | --- | --- |
| V25, 6,400 mAh | ~19 Wh | ~1 week | ~4–5 days |
| V50, 12,800 mAh | ~38 Wh | ~2 weeks | ~9 days |
| V75, 19,200 mAh | ~57 Wh | ~3 weeks | ~2 weeks |
(at 5 h/day of glow)

### Plain cells instead of a bank (asked 2026-11-09)
No, it does not have to be a bank. Plain cells in a holder behind the pot, wire through the drain
hole, charged by pulling the cells and dropping them in a standalone charger.

**Best fit: Li-ion cells, one cell's voltage (3.0–4.2 V).** The ring runs straight off it, and at
that supply 3.3 V data is fine, so no level shifter and no boost converter. Nothing is wasted
converting up to 5 V and back, so the same energy lasts longer than in a bank. Glow ~0.26 W,
about 1.5 Wh/day at 5 h/day with the ring switched off asleep (~2.5 Wh/day if not).
| Cells (protected, in a holder) | Energy | Days at 5 h/day (ring switched / not) |
| --- | --- | --- |
| 1 × 18650, ~3,500 mAh | ~13 Wh | ~8 / ~5 |
| 1 × 21700, ~5,000 mAh | ~18 Wh | ~12 / ~7 |
| 2 × 18650 in parallel, ~7,000 mAh | ~26 Wh | ~17 / ~10 |

- Use **protected** cells (built-in cutoff near 2.5 V), and a holder with a cover. Protected
  cells run ~69–70 mm long, so check the holder takes them.
- Parallel cells: always charge them together and put them in full; a full cell next to an
  empty one pushes a large current between them.
- Board: the Feather ESP32-S3 is built for a one-cell supply (BAT input, regulator, and its
  MAX17048 gauge reads any one-cell Li-ion: battery % comes back). The s3mini fed into its 5 V
  pin starts to sag below ~3.6 V (ME6217 dropout, see the s3mini section), and it would need two
  resistors to read the voltage.
- The Feather can also charge the cells in place over USB-C, but slowly; a standalone charger
  is quicker.

**AA rechargeables (NiMH, like Eneloop) also work, with costs:**
- 4 × AA (4.0–5.6 V, ~9.6 Wh with 2,000 mAh cells): ~3–5 days. 4.8 V on the ring needs the
  level shifter again (0.7 × 4.8 V ≈ 3.4 V wanted for a high).
- 3 × AA (3.0–4.2 V, like Li-ion): no shifter, but only ~7 Wh (~3–5 days), and it sags low
  enough to brown out the board near the end.
- Familiar and safe to handle, but half the energy of Li-ion for the same space on the shelf.

**Ranking for this orb:** one 21700 (or 2 × 18650) + Feather > Voltaic bank + s3mini + shifter >
AA NiMH. The bank still wins on "nothing to think about": no loose cells, charge it like a
phone.

**Lost vs. inside:** no battery % on the board (the bank just stops at empty; its own LEDs show
the charge). The firmware could log hours since the last power-up as a rough fuel gauge.

| | Battery inside (Feather + LiPo) | Battery outside (s3mini + bank) |
| --- | --- | --- |
| Charge | lift the orb, plug USB-C into the pot | unplug the bank, charge it anywhere |
| Runtime at 5 h/day | ~5.5–10 days | ~1–3 weeks, by bank size |
| Visible from the room | nothing | nothing if the bank stays behind the pot; a short cable |
| Parts to buy | Feather, LiPo, switch | always-on bank, 74AHCT125, switch, feet |
| Battery % in the log | yes (MAX17048) | no |
| Space inside the pot | tight under the hook | plenty |

## Soothing, not a show
- Changes slow enough that you only notice them if you look away and back (minutes per
  color shift), low brightness, no flashing, no fast chases.
- Arrive: fade up over a few seconds. Leave: hold for minutes, then fade out over a minute.
- The confetti glass does the visual work: drifting slowly between a few hues makes the lit
  chips change slowly across the ball.
- Ideas to try in the prototype: slow drift between 2–3 palette hues; a very slow "breath"
  (a few percent of brightness, ~10 s period); warm low glow late at night.
- Low battery: dim further and go warm instead of flashing a warning.

## Firmware work this needs
- The template is clockless-only (`LED_DATA_PIN`, `addLeds<LED_TYPE, PIN, ORDER>`). Clocked
  chips need `LED_CLOCK_PIN` in `config.h`, a branch in `main.cpp`, and a safe-pin check in
  `lab_checks.h`. That is a template change; `just template-check` must pass after it.
- `boards/s3mini/board.h` has `BOARD_LED_CLOCK_PIN` (GPIO 5, 2026-11-22). Other `boards/` changes
  stay stop-and-ask.
- Phase 1: the 18:00–22:00 schedule (NTP) and the slow effects. Phase 2: deep sleep, PIR wake
  with a hold timer, a `batt` event (V from the divider; % only with the Feather's MAX17048).

## Sources
- LuMini 1 in ring: https://sparkfun.com/products/14967
- LuMini hookup guide (not read directly, it refuses my reader; search summary only): https://learn.sparkfun.com/tutorials/lumini-ring-hookup-guide/all
- SparkFun's ring board file, schematic and ring test (read 2026-11-22): https://github.com/sparkfun/LuMini_1_Inch
- UL 3132 22 AWG wire, 1.60 mm overall (search excerpt): https://www.e-sonic.com/productfiles/mf-CCT/CCT-313222105000011TS.pdf
- APA102-2020 datasheet (page 3 table read 2026-11-22): https://cdn-shop.adafruit.com/product-files/3341/APA102-2020-8A+LED+Datasheet.pdf
- ~60 mA/LED rule: https://community.sparkfun.com/t/3-lumini-ring-daisy-chaining-and-current-calculations/38491
- Dim on battery: https://community.sparkfun.com/t/lumini-rings-20-led-each-are-extremely-dim-on-battery-power/61164
- Feather ESP32-S3: https://adafruit.com/product/5477 and https://learn.adafruit.com/adafruit-esp32-s3-feather
- ecobee API closed to new keys: https://home-assistant.io/components/ecobee/ and https://www.ecobee.com/en-us/developers/
- HomeKit vs cloud update speed: https://community.home-assistant.io/t/ecobee-via-homekit-controller-refresh-time/564241
- Stuck thermostat occupancy: https://community.home-assistant.io/t/ecobee-3-and-homekit-thermostat-occupancy-stuck-in-occupied/684547
- Voltaic Always On: https://support.voltaicsystems.com/kb/getting-started/always-on-mode and https://voltaicsystems.com/always-on/
- V50 (USB-C PD port caveat): https://voltaicsystems.com/V50
- 2025 revisions: https://seesense.eu/2025/09/10/voltaic-battery-updates/
- ESP32-S3-Zero: https://www.waveshare.com/esp32-s3-zero.htm (ME6217C33, 800 mA),
  https://files.waveshare.com/wiki/ESP32-S3-Zero/ESP32-S3-Zero-Sch.pdf,
  https://www.cnx-software.com/2024/08/07/waveshare-esp32-s3-zero-is-a-tiny-wifi-and-ble-iot-module-with-a-usb-c-port-up-to-32-gpios/
  (3.7–6 V on the 5V pin, no charger), https://www.espboards.dev/esp32/esp32-s3-zero/
