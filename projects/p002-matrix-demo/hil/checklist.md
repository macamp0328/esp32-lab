# p002-matrix-demo HIL checklist

Asked in this order, batched in one connect (see docs/hil-protocol.md). Hold the matrix with its
cable at the top; that edge is "up".

1. **Wiring probe** (only with `DEMO_PROBE 1`). **Do:** nothing. **Look for:** 4 dots: red top-left, green bottom-left, blue just right of green, white top-right. **Fail looks like:** dark, other colors, dots elsewhere. **Reply:** `ok` or a photo.
2. **Demo.** **Do:** press A (the big M5 button) to step through all 13 effects. **Look for:** the screen names each one with `set 1  i/13` and `HOLD`; fire rises toward the cable; heart and invader are upright; `ESP32 LAB` reads normally, moving right to left. **Fail looks like:** a mirrored or sideways picture, flicker, a dark or frozen effect. **Reply:** `ok` or `fail: <effect>: <what you saw>`.
3. **Buttons and brightness.** **Do:** press B (side button) 4 times, then hold A for 1 s. **Look for:** brightness steps up to 80, then wraps to the dimmest (8); after the hold the screen says `AUTO` and effects change every ~15 s. Nothing is uncomfortably bright. **Fail looks like:** no change, too bright at 80. **Reply:** `ok` or `fail: <what you saw>`.
4. **Set 2.** **Do:** hold B (side button) for 1 s, then press A to step through set 2. **Look for:** the screen says `set 2`; aurora curtains, Pac-Man and ghost, Game of Life, starfield, self-playing Tetris, pouring sand, fireworks, pong, two painting ants, tunnel, self-playing snake, a rolling marble, kaleidoscope, snow, DNA helix, lightning. **Fail looks like:** a dark, frozen or garbled effect. **Reply:** `ok` or `fail: <effect>: <what you saw>`.
5. **Tilt.** **Do:** on sand or marble, tilt the M5. **Look for:** the grains or the ball move with the tilt. **Fail looks like:** no response. **Reply:** `ok`, or the direction each tilt moved them.
