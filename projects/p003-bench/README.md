# p003-bench

**Status:** bench-verified  ·  **Last HIL check:** 2026-10-22 m5stickc-plus11 ok (showcase fps per LED count; A-hold starts the suite)

How fast can each board go? A benchmark suite that runs unchanged on every board (compute,
LED output, dual-core, memory, screen), plus an uncapped showcase. Results for all boards are in
[RESULTS.md](RESULTS.md); the findings are below.

## Wiring
Nothing new.
- **m5stickc-plus11:** the 8x8 matrix as wired for p002 (Grove white, G33). The parallel test
  also drives Grove yellow (G32).
- **s3mini:** no LEDs needed. Frames go to the onboard RGB LED (GPIO21, only pixel 0 shows). The
  parallel test also drives GPIO4, 6 and 7, so leave them unconnected while benchmarking.

A pin with nothing on it gives the same `show()` timing: the RMT peripheral sends the data
either way.

## Power
LEDs: up to 64 real (matrix) · cap 400 mA · the suite runs at brightness 8 · supply: USB.

## Demo
- `just build p003 && just flash p003 && just watch p003`: the showcase (plasma, fire and Life,
  10 s each) with no frame cap. Expect `fps` about 214 on the M5 and 250 on the S3 Mini at
  64 LEDs. On the M5, A cycles the LED count 64 → 256 → 1024, and the screen shows the live
  fps.
- `just bench p003`: resets the board, runs the suite (about 120 tests, 50 s), writes
  `results/<board>.json` and rebuilds RESULTS.md. Holding A on the M5 also runs it.

## Findings (2026-10-22, FastLED 3.10.7, both boards at 240 MHz)
1. **The LED wire sets the ceiling, not the CPU.** A WS2812 needs 30 µs per LED, and `show()`
   blocks until the frame is out: 3.2 ms for 64 LEDs (about 310 fps), 10 ms for 256 (100 fps)
   and 35 ms for 1024 (28 fps). That's the same on both boards, about 16% over the bare wire
   time (FastLED encoding plus the power limiter, 0.56 ms at 1024 on the M5).
2. **More pins, more fps.** The RMT channels transmit at the same time: 1024 LEDs take 35 ms
   on one pin, 19–20 ms on two and 11.5 ms on four (S3 Mini). For 60 fps, keep each pin to
   about 450 LEDs and split anything bigger across pins. The ESP32 has 8 RMT TX channels; the
   S3 has 4.
3. **Rendering scales with LEDs times work per LED.** At the matrix's 64 LEDs, every effect
   draws in under 1.5 ms except starfield (3.5–3.9 ms). At 1024 LEDs the particle effects take
   over: starfield 60–66 ms, fireworks 22 ms, snow 19 ms, helix 18 ms, because each particle
   touches every LED. Pac-Man, pong and the marquee stay under 1 ms. `lab::noise` (3.5–4.2 µs
   per call) is the expensive primitive behind fire, aurora and kaleidoscope.
4. **The S3 Mini computes 15–25% faster at the same clock** (newer core), copies memory twice
   as fast (378 vs 190 MB/s), and sends LEDs at exactly the same speed.
5. **Dual-core pays off when rendering is about as slow as sending.** Rendering the next frame
   on core 0 while core 1 sends the current one:
   - fire at 1024 LEDs goes from 21.5 to 27.6 fps (M5), back at the wire limit;
   - starfield goes from 9.8 to 14.8 fps;
   - light effects gain nothing.
   With WiFi off, core 0 is otherwise idle.
6. **PSRAM (S3 Mini) is fine for frame buffers, slow for bulk copies.** Rendering 1024 LEDs into
   PSRAM costs the same as into internal RAM (5.24 vs 5.31 ms; a frame fits in the cache), but
   copying PSRAM to PSRAM crawls at 9.5 MB/s.
7. **The M5 screen is slow next to LEDs.** A full fill takes 13 ms, the 8x8 preview
   (`lab_board::status_pixels`) 6 ms, and four status lines 20 ms. Update it a few times a
   second, never every frame.
8. **Board calls cost frame time on the M5.** Polling the buttons once per frame costs about
   0.2 ms (an I2C read of the power chip), so the showcase polls at 50 Hz; that took it from 191
   to 214 fps. An IMU read costs 0.23 ms.

## Open issues
- [ ] FastLED 3.10.7 times out every `show()` (4 s each) once an output is added after the first
  frame, or set to 0 LEDs; outputs are created at boot and toggled with `setEnabled()`
  (docs/lessons.md).
- [ ] No board with LEDs on more than one pin yet, so parallel output is timed, not seen.
