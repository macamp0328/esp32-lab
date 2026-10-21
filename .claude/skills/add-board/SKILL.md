---
name: add-board
description: Add a new physical board type to esp32-lab (boards/<name>) and bring it up with p000. Use when Miles gets a board that has no boards/ folder yet.
---

# Add a board

Editing `boards/` and `platformio.common.ini` is a stop condition: confirm with Miles first
(one yes/no), unless he asked for the board to be added.

1. Find the PlatformIO board id closest to the hardware (`ls /pio/platforms/espressif32/boards`
   inside `just shell`) and the esptool chip.
2. `just add-board <name> --chip <chip> --pio-board <id>`: creates `boards/<name>/` (board.ini,
   board.h, pinout.md), the `lab_board.h` branch and `[env:<name>]`.
3. Fill `board.h` and `pinout.md` from the datasheet: flash/PSRAM, default LED pin, safe pins
   (no strapping, flash, USB or input-only pins), capability flags, `upload_speed`. Override
   flash size/partitions in board.ini like boards/s3mini does if the stock JSON differs.
4. Add the board to `projects/p000-blink` `default_envs`, add an inventory entry with
   `board: <name>`, `just build p000 <name>`, then flash-and-watch, then one HIL check.
5. Record measured facts in `pinout.md`, set `BOARD_VERIFIED 1`, add a decisions.md entry if the
   board needed overrides, and a lessons.md line for anything surprising.
