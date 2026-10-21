# Decisions

Newest first. Five lines each: date and title, what, instead of, why, revisit when.
Read before changing anything an entry covers. A version bump is a new entry plus a green CI.

## 2026-10-21 · flash, watch, sim, add-board exit 2 until their milestone
- What: the verbs exist (AGENTS.md lists them) but print "not implemented until M1/M2" and exit 2
- Instead of: best-effort implementations in M0
- Why: none can be verified without a board (M1) or M2's scaffolding; an unverified verb breaks principle 5
- Revisit when: M1 (flash, watch, sim) and M2 (add-board)

## 2026-10-21 · M5StickC Plus 1.1 uses variant `m5stack_stickc_plus`
- What: `board = m5stick-c` plus `board_build.variant = m5stack_stickc_plus` and `-DARDUINO_M5STACK_STICKC_PLUS`
- Instead of: SPEC's bare `board = m5stick-c`
- Why: spec bug: that JSON names variant `m5stick_c`, which core 3.x renamed, so the build failed on `pins_arduino.h`
- Revisit when: pioarduino ships an M5StickC Plus board JSON

## 2026-10-21 · Minimal `lab_core` (serial encoder) and `lab_board` (board dispatcher) in M0
- What: `lab::JsonLine` (pure, host-tested) + `lab_core::Event`; `lab_board.h` maps `LAB_BOARD_*` to `boards/<b>/board.h`
- Instead of: creating every shared library in M2
- Why: the M0 template must print the serial contract and pass a native test; the encoder is that test
- Revisit when: M2 (adds `lab_board::begin()`, power cap inside `lab_core::begin()`, `lab_fx`)

## 2026-10-21 · `BOARD_PSRAM_MB` and `BOARD_FLASH_MB` replace `BOARD_HAS_PSRAM`
- What: board.h states sizes (`BOARD_PSRAM_MB 2` / `0`) instead of a has-PSRAM flag
- Instead of: SPEC's `#define BOARD_HAS_PSRAM 1|0`
- Why: spec bug: Arduino-ESP32 reserves `BOARD_HAS_PSRAM` and tests it with `#ifdef`, so `0` would read as "has PSRAM"
- Revisit when: never, unless Arduino-ESP32 drops that macro

## 2026-10-21 · Board envs compose `build_flags` and `lib_deps` explicitly
- What: `build_flags = ${common.build_flags} ${boards_<b>.build_flags}`; `lib_deps` likewise, one dep per line
- Instead of: SPEC's bare `extends = common, boards_<b>`
- Why: spec bug, seen in `pio project config`: `extends` does not merge, the last section wins, so FastLED vanished
- Revisit when: PlatformIO adds merging `extends`

## 2026-10-21 · S3 Mini 4 MB via `board_upload.*` on stock `esp32-s3-devkitc-1`
- What: `board_upload.flash_size = 4MB`, `maximum_size = 4194304`, `board_build.partitions = default.csv`
- Instead of: SPEC's `board_build.flash_size` (not a PlatformIO option) or a custom board JSON
- Why: documented keys; built bootloader header reads 4MB and the app partition is 1.25 MB
- Revisit when: M1 measures flash and PSRAM (SPEC open question on a custom board JSON)

## 2026-10-21 · Pin pioarduino `55.03.312-1` (Arduino 3.3.12, IDF 5.5.5) by release URL
- What: `platform = …/releases/download/55.03.312-1/platform-espressif32.zip`
- Instead of: SPEC's `…/releases/download/stable/…` URL
- Why: spec bug: `stable` moves with every pioarduino release, so it pins nothing; 61.04 (Arduino 4.0) is still RC
- Revisit when: Arduino-ESP32 4.0 ships stable, or we need a fix from a newer release

## 2026-10-21 · Supported boards = the project's `default_envs`
- What: `[platformio] default_envs = s3mini, m5stickc-plus11` is the list; `tools/ci_matrix.py` reads it
- Instead of: a custom key or a separate list file
- Why: PlatformIO-native: `cd projects/p && pio run` builds exactly the supported boards
- Revisit when: a project needs a board-specific env override

## 2026-10-21 · Image tagged by content hash; ci.yml calls image.yml first
- What: `esp32-lab-dev:tc-<sha256(Dockerfile, pyproject.toml, uv.lock, .python-version)>`, built only when missing
- Instead of: SPEC's image.yml on Dockerfile change, tags per commit, ci.yml on `:latest`, matrix job on a bare runner
- Why: spec bug for "CI green on first push": ci.yml would race image.yml for an image not yet built (and needs uv)
- Revisit when: image builds get slow enough to want a scheduled rebuild

## 2026-10-21 · Local image is built from the Dockerfile, not pulled from GHCR
- What: `just image` / `just bootstrap` build `esp32-lab-dev:tc-<hash>` for the host's arch
- Instead of: pulling `ghcr.io/<owner>/esp32-lab-dev:latest`
- Why: CI image is amd64 and Miles' Mac is arm64 (emulation is slow); a fresh clone needs no registry login
- Revisit when: image.yml builds multi-arch cheaply

## 2026-10-21 · PlatformIO packages live in a volume/cache, not in the image
- What: `PLATFORMIO_CORE_DIR=/pio`: the `esp32-lab-pio` volume locally, `actions/cache` in CI
- Instead of: installing pioarduino and toolchains during `docker build`
- Why: platform pins stay in one file (platformio.common.ini) and the image stays ~280 MB compressed
- Revisit when: CI cache misses dominate build time

## 2026-10-21 · Python tools pinned in pyproject.toml + uv.lock
- What: PlatformIO, esptool, pyserial, pyyaml; dev group ruff, pytest, clang-format, just (`rust-just`)
- Instead of: separate `uv tool install` pins in the Dockerfile
- Why: one lockfile for container and host; host `uv sync --no-dev` gets esptool and `pio device monitor`
- Revisit when: a tool needs a Python the lockfile cannot satisfy

## 2026-10-21 · `just lint` is an alias of `just check`
- What: one lint verb with two names
- Instead of: two verbs that could drift
- Why: SPEC calls `just check` the single lint verb but its definition of done says `just lint`
- Revisit when: never

## 2026-10-21 · Host prerequisites: Docker, just, uv
- What: README says `brew install just uv`; `just bootstrap` checks Docker and uv and does the rest
- Instead of: a shell script that installs them
- Why: `just bootstrap` needs `just` to exist first; two formulae are the smallest host footprint
- Revisit when: a non-macOS host joins
