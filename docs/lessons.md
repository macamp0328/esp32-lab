# Lessons

Append-only, one dated line each: `- YYYY-MM-DD <project>/<board>: <symptom> → <cause> → <fix>`.
Grep here before debugging.

- 2026-10-21 repo: `extends = common, boards_x` dropped FastLED and the common build_flags → PlatformIO `extends` does not merge options, the last section wins → compose `build_flags`/`lib_deps` explicitly in each env (platformio.common.ini).
- 2026-10-21 repo: `just new` crashed decoding the template → Finder drops `.DS_Store` into open folders → the copier ignores `.DS_Store` and `.pio`.
- 2026-10-21 repo/m5stickc-plus11: `lib_deps` lost M5GFX, then the build died on `pins_arduino.h` → a comma list does not survive `${...}` interpolation, and the stock `m5stick-c` JSON names a variant core 3.x renamed → one dep per line; `board_build.variant = m5stack_stickc_plus`.
- 2026-10-21 repo: cppcheck read `.h` files as C and hit `#error` without a project's `config.h` → run it with `--language=c++` per project with `-I<project>/include`.
