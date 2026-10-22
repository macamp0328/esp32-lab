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
- 2026-10-21 shared/tests: native build of FastLED 3.10.7 fails (`sleep_for`, `std::chrono` in platforms/stub) → its host build targets FastLED's own tooling, not PlatformIO native → host tests use `lab::Pixel`, never FastLED.
- 2026-10-21 shared/tests: `unity.h: No such file` → `lib_extra_dirs = ../../shared` made LDF register shared/tests itself as a library → `lib_ignore = tests`.
- 2026-10-21 repo: a project's `.pio/build/<other board>` vanished → PlatformIO wipes `.pio/build` when any included ini (here boards/_template/board.ini) changes → rebuild before flashing; `just flash` says so.
- 2026-10-21 repo: `rm -rf .pio` failed with "Directory not empty" → Finder rewrites `.DS_Store` while it deletes → retry the rm; never chain a write after it with `&&`.
- 2026-10-21 repo: `just check` died in `git ls-files` (exit 128) from a `.claude/worktrees/*` checkout → a worktree's `.git` file points at the main repo's host path, which the container does not mount → run it from the main checkout, or `docker run` with `-v <git-common-dir>:<same path>` and `safe.directory '*'`.
- 2026-10-21 p002/m5stickc-plus11: matrix on the Grove port stayed dark under p000 → its data wire is on Grove white (G33), p000 drives G32 (yellow) → `BOARD_LED_PIN_ALT` (G33) in board.h; ask which Grove wire before assuming `BOARD_LED_PIN`.
- 2026-10-21 p002/m5stickc-plus11: matrix-8x8-01 is wired down columns (LED 7 bottom-left, 8 beside it), a transpose no rotation fixes → found with `lab_fx::wiring` (one photo) → `Layout::matrix(w, h, serpentine, by_columns)`, `MATRIX_COLUMNS 1`.
- 2026-10-21 repo: `boot` shows `fw=nogit` for builds from a `.claude/worktrees/*` checkout → tools/pio_fw.py runs git in the container, which cannot see the main repo's `.git` (same cause as the `just check` lesson) → fixed by `just _c` mounting the git common dir (4ab4580); p002 then booted with `fw=dc192cd`.
- 2026-10-21 p002/m5stickc-plus11: `macro "F" passed 4 arguments` on a `template <lab_fx::Fn F>` → Arduino defines `F()` (flash strings) as a macro → never name anything `F` in board code (`kFx`).
- 2026-10-21 shared/tests: `deducing from brace-enclosed initializer list requires <initializer_list>` (hit twice) → range-for over `{a, b, c}` needs that header under PlatformIO native → loop over a named array instead.
- 2026-10-21 p002/m5stickc-plus11: sand would pour sideways with the M5 sitting still → it rests about 40 degrees off flat (cable plugs lift one end) → measure tilt from the pose at effect start, not from absolute flat.
- 2026-10-21 lab_sim: autoplay snake circled its own tail forever at length ~22 → chasing the tail by the shortest path makes a stable loop → take the move with the longest way back to the tail, random ties, plus a hunger restart; it now fills the 8x8 board.
- 2026-10-22 p003/m5stickc-plus11: after a second LED output was added mid-run, every `FastLED.show()` took 4 s and logged `ChannelManager: Timeout` → FastLED 3.10.7's channel manager cannot take an output added after the first frame (or one set to 0 LEDs) → add every output in setup() and switch them with `setEnabled()`.
- 2026-10-22 p003/m5stickc-plus11: uncapped showcase stuck at 191 fps while render + show alone did 250 → `M5.update()` (lab_board::buttons) reads the power chip over I2C, about 0.2 ms when called once per frame (back-to-back calls look free) → poll buttons at 50 Hz in fast loops.
- 2026-10-22 p003: `Unity Double Precision Disabled` on TEST_ASSERT_EQUAL_DOUBLE → the native Unity build has no double support → cast to float and use TEST_ASSERT_FLOAT_WITHIN (platformio.common.ini is pinned).
