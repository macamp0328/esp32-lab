# Decisions

Newest first. Five lines each: date and title, what, instead of, why, revisit when.
Read before changing anything an entry covers. A version bump is a new entry plus a green CI.

## 2026-10-21 · Simulations live in `shared/lab_sim` with caller-owned state; `lab_fx` stays time-only
- What: Life, Ant, Snake, Tetris, Sand, Marble as structs with `reset(seed, layout)`, `step(Input)`, `draw(px, layout)`; fixed 16x16 grids, seeded `lab_sim::Rng`, no heap, no globals
- Instead of: stateful effects inside `lab_fx`, or each project keeping its own copies
- Why: the time-only entry below said to revisit when an effect needs memory; explicit state keeps sims host-testable with fixed seeds and keeps `lab_fx` crossfade-safe
- Revisit when: a sim needs more than 16x16 cells or heap-sized state

## 2026-10-21 · p002 is the matrix demo on the M5; effects are functions of time only
- What: p002-matrix-demo (M5StickC Plus + matrix-8x8-01 on Grove G33, 400 mA cap); 12 new `lab_fx` effects, each `f(t)` with no state
- Instead of: renumbering it p001 under the creation-order rule (below), which landed on main mid-build; stateful effects (life, snake)
- Why: numbered while SPEC still reserved p001, and renumbering a HIL-verified project churns docs; p001 stays unused. Pure `f(t)` effects host-test deterministically, crossfade freely and restart from frame 0
- Revisit when: an effect needs memory (then keep its state in the project, not in `lab_fx`)

## 2026-10-21 · `Layout::matrix` takes `by_columns`, not rotate/flip options
- What: one flag for matrices wired down columns; the origin stays top-left (LED 0)
- Instead of: a general rotation/mirror transform
- Why: matrix-8x8-01 measured as column-wired from the top-left; YAGNI for the other seven orientations
- Revisit when: a panel's LED 0 is not in a corner we can hold at top-left

## 2026-10-21 · Buttons go through `lab_board::buttons()` (M5Unified's debounce)
- What: `{a_click, a_hold, b_click}` per loop, from `M5.BtnA`/`BtnB`; all false on boards without buttons
- Instead of: `digitalRead(BOARD_BUTTON_A)` and our own debounce in each project
- Why: M5Unified is already linked and knows each M5 model's buttons (polarity, hold timing); projects stay board-agnostic
- Revisit when: a non-M5 board with buttons joins (add its branch in lab_board.cpp)

## 2026-10-21 · 400 mA LED cap for the M5 Grove port on laptop USB
- What: `POWER_MAX_MA 400` in p002; measured `ma_max` about 300 mA at brightness 40
- Instead of: the template's 500 mA
- Why: the M5 itself takes ~100 mA; 400 + 100 stays inside a 500 mA USB 2.0 port, well under the 800 mA rule
- Revisit when: the matrix gets its own 5 V supply

## 2026-10-21 · CI: every main push runs; least-privilege tokens; caches keyed on the platform pin
- What: cancel only PR runs; `packages: write` only for the image job; `persist-credentials: false`; `/pio` cache keyed on platformio.common.ini with restore-keys; 30 min timeouts; a `template` job runs `just template-check`
- Instead of: cancel-in-progress on main, write tokens everywhere, keys on every board.ini, templates only linted
- Why: review found a main run cancelled (no result for that commit), 12.8 GB of caches against a 10 GB quota, and a template that never compiled in CI
- Revisit when: PRs from forks are used (the image push needs a guard)

## 2026-10-21 · The template ignores `lab_net` until WiFi is turned on
- What: templates/project/platformio.ini has `lib_ignore = lab_net`; `lab_checks.h` stops a `WIFI_ENABLED 1` build until that line is deleted
- Instead of: `lib_ldf_mode = chain+` (evaluates `#if`), or compiling lab_net into every project
- Why: measured on a fresh template: chain links ~150 KB of dead WiFi code (508 vs 446 KB); chain+ took 244 s instead of 37 s per build
- Revisit when: PlatformIO's chain+ gets fast, or most projects use WiFi

## 2026-10-21 · Image pins: base and uv by digest, binaries by sha256; apt and Unity float
- What: Debian packages float within trixie point releases; Unity resolves PlatformIO's `^2.6.1`; wokwi-cli/actionlint checked by sha256
- Instead of: snapshot.debian.org sources and a pinned Unity
- Why: point releases only carry fixes and two arches already differ in bytes; a cppcheck or gcc change shows up as a `just check` diff, not silently
- Revisit when: a toolchain float breaks a build or makes local and CI checks disagree

## 2026-10-21 · Project numbers come in creation order; none are reserved
- What: next project = one above the highest pNNN ever committed (`just new` refuses numbers in git history)
- Instead of: reserving SPEC's p001-matrix-audio / p002 numbers
- Why: SPEC's numbers were examples; M3's project takes whatever number is next when it starts
- Revisit when: never

## 2026-10-21 · SPEC items not built yet
- What: no `release.yml` (pNNN/vX.Y tags) and no `wokwi.yml`; inventory `type` is the category key (a `type:` field is also accepted); SPEC's `lab_core::log` is `lab_core::Event`
- Instead of: building them before anything needs them
- Why: no firmware is installed on a real piece yet (release.yml); Wokwi needs a token from Miles
- Revisit when: the first piece is installed (release.yml), a Wokwi token exists (wokwi.yml, `just sim`)

## 2026-10-21 · hil_assert also checks fields and the fps cadence
- What: every contract event must carry its fields; consecutive `fps` events at most 7 s apart; enough `fps` for the capture length
- Instead of: presence checks only
- Why: review found a stalled loop and a boot missing fields both passed
- Revisit when: a project legitimately pauses its loop (then it must still emit `fps`)

## 2026-10-21 · M4 (p002-wheel-chandelier) is shelved; p002 was an example idea
- What: no p002 project or scaffold; SPEC's M4 is not on the roadmap. Next milestone is M3 once its parts arrive
- Instead of: SPEC's M4 "p002 scaffold" milestone
- Why: Miles' call: the wheel chandelier is not planned and may never be built
- Revisit when: Miles decides to build it (then `just new` it like any other idea)

## 2026-10-21 · Effects take `lab::Pixel` (CRGB on the board, plain RGB on the host)
- What: `lab_pixel.h` aliases CRGB under Arduino and defines a 3-byte `{r,g,b}` natively; color math (`lab::hsv`) is ours
- Instead of: SPEC's "CRGB and CRGBPalette16 are available in FastLED's stub build" for host tests
- Why: spec bug: FastLED 3.10.7's stub platform does not compile under PlatformIO native (gcc 14 `std::chrono`, `sleep_for`)
- Revisit when: FastLED's host build works under PlatformIO, or an effect needs FastLED palettes

## 2026-10-21 · `Layout` does its own index math instead of wrapping FastLED's XYMap
- What: strip, serpentine/progressive matrix and rings with index/column/row/ring/angle/position, all host-tested
- Instead of: SPEC's "Layout wraps XYMap and a ring map"
- Why: XYMap lives in FastLED, which cannot be built on the host (entry above); the math is a dozen lines
- Revisit when: an effect needs FastLED's 2D helpers (add `to_xymap()` on the board only)

## 2026-10-21 · Shared libraries are tested in `shared/tests`, a native-only PlatformIO project
- What: `shared/tests/test/test_{layout,power,fx}`, run by `just test-all`; `lib_ignore = tests` stops LDF treating it as a lib
- Instead of: testing shared code only through each project's tests
- Why: one place for library tests, two folders deep so the `../../` paths match projects
- Revisit when: never

## 2026-10-21 · `lab_core::begin(Setup)` applies the power cap; `lab_checks.h` holds compile-time rules
- What: `begin({LAB_PROJECT, LAB_FW, POWER_VOLTS, POWER_MAX_MA})`; `lab_checks.h` has the power, matrix and pin asserts
- Instead of: each main.cpp calling `setMaxPowerInVoltsAndMilliamps` and repeating the `#error` lines
- Why: SPEC: "project code cannot forget it"; begin() will not compile without the cap
- Revisit when: a project needs per-strip caps

## 2026-10-21 · Inventory board entries name their `board:`; `just new` without `--board` lists them
- What: `board: s3mini` on inventory boards; `just new p` with no board prints what is on hand and exits 1
- Instead of: an interactive prompt in the CLI
- Why: the agent asks Miles (new-project skill) and the CLI stays scriptable; the list comes from stock truth
- Revisit when: two board types share a boards/ folder

## 2026-10-21 · The agent maintains inventory/stock.yaml
- What: the agent edits stock.yaml when Miles asks (from an order, breakage or assignment), then `just inventory`
- Instead of: SPEC's "Miles edits it; the agent proposes diffs"
- Why: Miles' call: he will mostly prompt the agent ("look at this order and update stock") rather than edit YAML
- Revisit when: never, unless stock drifts from reality

## 2026-10-21 · M1 open questions closed: stock S3 board JSON, M5Unified on the M5
- What: s3mini stays `esp32-s3-devkitc-1` + overrides; the M5 uses M5Unified 0.2.27 for AXP192 and TFT
- Instead of: a committed custom board JSON; a bare M5 with only LED and mic
- Why: measured 4 MB flash / 2 MB PSRAM match the overrides; M5Unified lit the TFT on the first try
- Revisit when: a board needs flash/PSRAM modes the overrides cannot express, or M5Unified costs too much flash

## 2026-10-21 · On the M5, p000 proves life with the red LED and the TFT
- What: boards without `BOARD_ONBOARD_RGB` drive 8 px on `BOARD_LED_PIN` (Grove) and blink `BOARD_STATUS_LED`
- Instead of: SPEC's "onboard LED via FastLED on RMT" on both boards
- Why: spec bug: the M5StickC Plus has no addressable onboard LED, only a plain red LED on GPIO10
- Revisit when: the 8x8 matrix is wired to the M5 Grove port

## 2026-10-21 · `lab_net`: WiFi + ArduinoOTA as its own shared library
- What: `shared/lab_net` (non-blocking connect, `wifi`/`ota` events); projects opt in via `secrets.h`
- Instead of: folding WiFi into `lab_core` (SPEC lists only core, audio, fx, board)
- Why: PlatformIO's LDF would compile WiFi/OTA into every project that includes `lab_core`
- Revisit when: M3's web UI needs a home (likely `lab_net` too)

## 2026-10-21 · esptool baud is each board's `upload_speed` in board.ini
- What: s3mini 921600, m5stickc-plus11 1500000; `just flash` reads the same key PlatformIO uses
- Instead of: one hard-coded 921600
- Why: measured: the M5's FTDI link fails right after switching to 921600; 1500000 is its stock speed
- Revisit when: a board flashes unreliably at its speed

## 2026-10-21 · hil_assert: exactly one boot; ESP-IDF `E (` lines are notes
- What: a second `boot` fails the run (crash or brown-out); IDF error log lines are printed, not failed
- Instead of: SPEC's boot/leds/fps/err checks alone
- Why: a reboot loop can still produce fps lines; FastLED logs one benign IDF error on the S3 every boot
- Revisit when: an IDF error line hides a real fault (then allowlist known ones instead)

## 2026-10-21 · `hw` event names the running app partition
- What: `"app":"app0"` after a USB flash (factory image), `"app1"` after an OTA update
- Instead of: trusting espota's exit code
- Why: `just flash --ota` falls back to USB, so a fresh fw hash alone cannot prove OTA delivered it
- Revisit when: a partition table without two OTA slots is used

## 2026-10-21 · M1: flash and watch work; sim exits 2 until a Wokwi token exists
- What: `just flash` (USB, `--ota`) and `just watch` (30 s capture + hil_assert) work on both boards; add-board landed in M2
- Instead of: the M0 stubs
- Why: verified on hardware; Wokwi needs a `WOKWI_CLI_TOKEN` from Miles
- Revisit when: a Wokwi token exists (sim)

## 2026-10-21 · flash, watch, sim, add-board exit 2 until their milestone (superseded by M1 entry)
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
- Revisit when: done in M2 (begin(), power cap, lab_fx; see the M2 entries above)

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
- Revisit when: done in M1 (measured 4 MB / 2 MB; see "M1 open questions closed")

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
