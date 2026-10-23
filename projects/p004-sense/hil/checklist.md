# p004-sense HIL checklist

Asked in this order, batched in one connect (see docs/hil-protocol.md). Hold the matrix with its
cable at the top.

1. **Scenes.** **Do:** press A (the big M5 button) six times, about 3 s apart. **Look for:** the screen names ambient, sound, warmth, power, battery, clock, then ambient again; each shows a steady picture, no flicker. **Fail looks like:** flicker or sparkles (the mic and the LEDs fight), a dark or frozen scene. **Reply:** `ok` or `fail: <scene>: <what you saw>`.
2. **Sound.** **Do:** on sound, clap twice, then talk for 10 s; then on ambient, clap once and play music or talk for 20 s. **Look for:** white columns for the claps and green bars while talking; in ambient a ring from the centre on the clap, and the aurora turning to plasma or lava while it stays loud. **Fail looks like:** no reaction, or reacting with the room silent. **Reply:** `ok` or what happened.
3. **Hand.** **Do:** on warmth, read the screen's `imu` line, hold the M5 in your fist (not the matrix) for 60 s, read it again. **Look for:** anything: the temperature may rise, fall or stay put. **Fail looks like:** the scene goes dark. **Reply:** both `imu` readings, e.g. `55.8 +0.1 → 54.9 -0.8`.
4. **Power.** **Do:** go to power and watch for 20 s. **Look for:** the plasma brightens and dims; the orange bottom row and the blue row above grow and shrink with it, the orange a little shorter. **Fail looks like:** both rows stuck. **Reply:** `ok` or what you saw.
5. **USB.** **Do:** unplug the M5's USB cable for 10 s, then plug it back in. **Look for:** the matrix goes dark and the screen says `BAT`; back on USB the scene returns. **Fail looks like:** the matrix stays lit, the M5 resets, or the scene does not return. **Reply:** `ok` or what happened.
6. **Clock.** **Do:** go to clock. **Look for:** the current local time scrolls by. **Fail looks like:** `--:--` or a wrong time. **Reply:** `ok` or the time it showed.
