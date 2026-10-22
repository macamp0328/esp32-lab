---
name: flash-and-watch
description: Build, flash and verify esp32-lab firmware on a board over USB or OTA, then check the serial contract with hil_assert. Use when asked to flash, run on hardware, check a board boots, or climb the verification ladder to rung 4.
---

# Flash and watch

1. `just build <p> <board>`: never flash a stale build.
2. `just flash <p> [board]`: USB; board is auto-detected from the chip when the project supports
   several. Over WiFi: `just flash <p> <board> --ota` (falls back to USB if a port exists).
   Several boards plugged in: the port is picked from the board's chip; name the board when the
   project supports several (`just watch p003 s3mini` too); two of the same board need `--port`.
3. `just watch <p>`: resets the board, captures 30 s to `projects/<p>/.hil/last-run.{log,jsonl}`,
   then runs `tools/hil_assert.py`. PASS = one `boot`, `leds` within 2 s, every `fps` >= 30, no `err`.
4. Report the rung reached in one line, e.g. `rung 4: hil_assert PASS fw=<sha> fps min 90`.

On failure: grep `docs/lessons.md` first, read `.hil/last-run.log` (raw, includes ROM and
ESP-IDF lines), fix, rebuild. After OTA, the `hw` event's `app` should flip `app0` ↔ `app1`.
No port at all usually means a charge-only cable: check `ioreg -p IOUSB` and ask Miles to swap it.
