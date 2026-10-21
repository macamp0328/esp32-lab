# Lessons

Append-only, one dated line each: `- YYYY-MM-DD <project>/<board>: <symptom> → <cause> → <fix>`.
Grep here before debugging.

- 2026-10-21 repo: `extends = common, boards_x` dropped FastLED and the common build_flags → PlatformIO `extends` does not merge options, the last section wins → compose `build_flags`/`lib_deps` explicitly in each env (platformio.common.ini).
- 2026-10-21 repo: `just new` crashed decoding the template → Finder drops `.DS_Store` into open folders → the copier ignores `.DS_Store` and `.pio`.
- 2026-10-21 repo/m5stickc-plus11: `lib_deps` lost M5GFX, then the build died on `pins_arduino.h` → a comma list does not survive `${...}` interpolation, and the stock `m5stick-c` JSON names a variant core 3.x renamed → one dep per line; `board_build.variant = m5stack_stickc_plus`.
- 2026-10-21 repo: cppcheck read `.h` files as C and hit `#error` without a project's `config.h` → run it with `--language=c++` per project with `-I<project>/include`.
- 2026-10-21 p000/s3mini: `E (8) cache: esp_cache_msync(113): invalid addr or null pointer` at LED init → FastLED 3.10.7 RMT5 cache sync on ESP32-S3 (FastLED issue #2156; it disables the sync after the first failure) → benign; hil_assert reports ESP-IDF `E (` lines as notes, not failures.
- 2026-10-21 p000/m5stickc-plus11: board plugged in, screen on, but no USB device at all → charge-only USB-C cable → use a known data cable (the S3's); check `ioreg -p IOUSB` before blaming drivers.
- 2026-10-21 p000/m5stickc-plus11: esptool `Unable to verify flash chip connection` right after `Changing baud rate to 921600` → the M5's FTDI link does not hold 921600 → flash at 1500000 (stock board JSON's speed), read from `upload_speed` in board.ini.
- 2026-10-21 p000/s3mini: first `just flash --ota` failed, the same espota call seconds later worked → cold mDNS lookup of `<name>.local` → `just flash --ota` resolves the host first (10 s retry) and retries the upload once.
