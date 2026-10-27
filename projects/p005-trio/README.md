# p005-trio

**Status:** bench-verified  ·  **Last HIL check:** 2026-10-27 S3 pixels match their corners (RGB fix); step 5 to redo

Two S3 Minis and the M5StickC Plus (with matrix-8x8-01) as one piece: **one law, three tempos**.
Same firmware on all three; they talk over ESP-NOW (`shared/lab_link`, no router), and no board
is in charge.

## The piece
**The law.** Each of the three nudges toward or away from the other two:
`dx/dt = ν + K·Σ sin 2π(xⱼ − x)`. K > 0 is agreement; K < 0 is dissent, and for three the
only stable dissent is exactly a third of a turn apart.

| Tempo | What obeys the law | Who sets K | Agree (K > 0) | Dissent (K < 0) | Shown on |
| ----- | ------------------ | ---------- | ------------- | --------------- | -------- |
| Beat, ~1 s | flash phase (1 s ± 1%, by MAC) | Miles: A hold flips it | unison flash | waltz: the flash passes round the triangle | each S3's pixel; the matrix corners |
| Mood, minutes | hue on the color wheel | the room: 2 min of link disturbance + claps | one palette, fanned ±12.5° | a triad | every color |
| Memory, hours | nothing: it records | — | straight, smooth threads | zig-zag, frayed threads, knots | the M5's TFT: one thread a minute, 240 = 4 h |

**The matrix is the space between the bodies**: an equilateral triangle, m (the M5) on top, a and
b (the S3s, by MAC) below. Corners flash in their board's color; every flash sends a pulse down
both of its edges that reaches the neighbor a third of a beat later (a baton pass in waltz);
each edge glows with its radio link's disturbance, so a hand in the gap lights it.

| Gesture | Beat (now) | Mood (minutes) | Memory (hours) |
| ------- | ---------- | -------------- | -------------- |
| Hand between two boards | that edge flares | stirs the room toward a triad | that border frays |
| Clap (M5 mic) | all three fire at once | each clap stirs the room; five make a triad for ~2 min | a white knot |
| A quiet room | unison or waltz holds | the hues come back together, circling the wheel every ~2 h | threads run straight |
| A click / A hold | scatter the phases / flip the beat's K | | |
| B click | TFT: loom ↔ diagram (link numbers) | | |

Hues persist in NVS (every 5 min), so the palette picks up where it left off after a reset or
power cut; a USB flash starts them over (the factory image blanks NVS).

## Wiring
Nothing new: the S3s show their onboard pixel; the M5 has the p002/p004 matrix on Grove (white
wire G33 → DIN, red 5 V, black GND; hold it with the cable at the top).

## Power
| Board | LEDs | Cap | Supply |
| ----- | ---- | --- | ------ |
| m5stickc-plus11 | 64 × WS2812B ECO (matrix-8x8-01) | 400 mA | dock USB via the M5's Grove 5 V |
| s3mini × 2 | 1 onboard WS2812 each | 100 mA | dock USB |

## Demo
```
just build p005 && just flash p005 --all && just watch p005 --all
```
Expect on every board `boot`, `leds`, `radio`, `node`, then `sync` (mode, r, spread_ms), `link`
per peer, `field` (edges) each second, `mood` every 5 s; the M5 adds `weave` per thread and
`clap`/`scatter`/`mode` when they happen. A print of a capture:
`.venv/bin/python tools/weave.py projects/p005-trio/.hil/last-run-m5stickc-plus11-<tag>.jsonl`.

## What the link can do (dock distance, ~10-30 cm)
Measured 2026-10-26 with `just watch p005 --all --seconds 60`.

| Link | RSSI 8 dBm | RSSI 20 dBm | Loss | RTT median (max) |
| ---- | ---------- | ----------- | ---- | ---------------- |
| M5 hears 720c | −35 | −16 | 0.3% | 4.1 ms (6.3) |
| M5 hears a574 | −57 | −45 | 1.3% | 3.9 ms (6.0) |
| 720c hears M5 | −29 | −28 | 0.3% | 4.0 ms (12.7) |
| 720c hears a574 | −39 | −30 | 1.2% | 4.2 ms (6.6) |
| a574 hears M5 | −49 | −47 | 0.5% | 4.0 ms (16.0) |
| a574 hears 720c | −40 | −26 | 0.8% | 3.5 ms (12.0) |

- The M5's own TX power barely changes what it radiates (docs/lessons.md); the S3s follow theirs.
- Beat: unison locks within 5 s of boot, the three flashing within 1-5 ms of each other by each
  board's estimate (R 1.000); waltz settles a third of a turn apart in ~20 s (min gap 0.327-0.329).
- Space: a quiet room reads 0.4-2 dB of RSSI disturbance, no edge lit. With `LINK_CSI 1` the
  broadcasts go out at OFDM 6 Mb/s (no loss, same RTT); the CSI shape changes 0.010-0.019 per
  packet in a quiet room, and RSSI at OFDM barely jitters at all (0-0.7 dB).
- Mood: a quiet room brought three random hues within 24 degrees in a minute; a burst of claps
  (14 heard in 6 min) split them into an exact 120-degree triad within ~90 s.
- The S3s run the same unwatched as watched since `lab_core` drops unread serial lines
  (docs/decisions.md, 2026-10-26).
- Hand in the 3-inch gap between the S3s (HIL step 7): their link's RSSI left its ~2 dB calm
  for 7-20 dB for the whole 16 s wave; CSI shape change only spiked (2-4x) on fast movement.
  RSSI drives the edges.
- Claps vs a mechanical keyboard beside the M5: claps -10 to -21 dBFS, key clicks -26 to -53
  (`CLAP_MIN_DB` -23); before the gate the M5 took 585 "claps" in 20 min of typing.

## Open issues
- [ ] HIL step 5 again with the clap gate: five claps, no typing, the colors split and heal.
  While they are split, ask whether the left S3 matches the bottom-left corner (`TRIO_SWAP_AB`).
- [x] The S3 pixels' colors did not match their matrix corners → the S3's onboard LED is RGB, not
  GRB (boards/s3mini/board.h, 2026-10-27).
- [ ] After ~19 h unwatched, S3 720c kept flashing but the M5 heard nothing from it until it was
  reset (docs/lessons.md): catch it with a long M5-only capture.
- [ ] First print: an evening's capture of the M5 through `tools/weave.py`, into `prints/`.
