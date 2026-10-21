# ESP32 Agent Lab: Repo Spec v0.1

Oct 21, 2026 · @Miles Camp

**At a glance (read this; the rest is for Claude Code).** Public, MIT-licensed monorepo `esp32-lab`. New idea to first flash in under five minutes: `just new p00N-name --board s3mini`, then tell the agent what to build. Boards are pluggable (`boards/`), stock is tracked in `inventory/`, the whole toolchain lives in one Docker image, and the agent is held to short replies: files carry the detail, chat carries the decision.

## Purpose and operating principles

One monorepo, `esp32-lab`, holds every ESP32 project, all tooling, and everything learned, and Claude Code writes 100% of the code. Miles supplies hardware, eyes, and decisions. Scope is personal: nothing ships, nothing is sold, so the bias is toward fast iteration with enough rigor that the agent can verify its own work.

Principles the repo is built around:

1. **Agent-first, human-in-the-loop.** Every build, flash, test, and debug step is a shell command the agent runs. Miles is asked only for what a shell cannot do: plug in a board, look at LEDs, read a meter, decide.
2. **A new idea costs minutes, not an evening.** `just new` produces a project that already builds, logs, tests, and knows its board. The agent starts on the idea, never on infrastructure.
3. **Brevity is a feature.** Replies to Miles are a few lines; detail goes to files and logs he can open if he wants. A wall of text is a failed turn.
4. **No legacy ceremony.** Trunk-based commits, CI on push, no pull requests, no review theater, no ADR for every choice. Process exists only where it catches real mistakes (power, pins, versions).
5. **Verification before claims.** Nothing is "working" without a compile, a test, or a serial log. Visual behavior gets one concrete yes/no question.
6. **Boards are interchangeable.** A project declares what it needs (LED pin, mic, display); a board file says what it has. Today: Waveshare ESP32-S3 Mini and M5StickC Plus 1.1.
7. **Pinned and containerized.** Versions live in files; the toolchain lives in a Docker image used by the agent, the devcontainer, and CI alike.
8. **Hobbyist-safe.** Miles solders well but is not an EE. The agent explains wiring as a short checklist, defaults to the safe option, and never assumes a pin is free without reading the board file.
9. **Stock is truth.** `inventory/stock.yaml` is the record of what exists and which revision; Miles updates it on purchase, the agent reads it before proposing hardware.
10. **Borrow structure.** PlatformIO's layout, the AGENTS.md convention, Wokwi's CI pattern, and devcontainers are used as-is.

## Repository layout

Projects are siblings under `projects/`, each a complete PlatformIO project; everything shared sits beside them at the root. A project can be built in isolation (`cd projects/p001 && pio run`) or by the root task runner.

```
esp32-lab/
  AGENTS.md                  # agent contract, under 150 lines; CLAUDE.md = "@AGENTS.md"
  README.md                  # human entry point: 1 screen, project status table
  LICENSE                    # MIT
  justfile                   # every verb: new, build, flash, watch, test, sim, check, inventory
  Dockerfile                 # the toolchain: PlatformIO + pioarduino, uv, just, clang tools, wokwi-cli
  .devcontainer/devcontainer.json
  .github/workflows/         # ci.yml (uses the same image), wokwi.yml, release.yml
  .clang-format  .editorconfig  .gitignore
  .claude/skills/            # new-project, flash-and-watch, hil-check, add-board
  boards/                    # one folder per physical board type, selected by name
    s3mini/                  # Waveshare ESP32-S3 Mini: board.ini, board.h, pinout.md
    m5stickc-plus11/         # M5StickC Plus 1.1: board.ini, board.h, pinout.md
    _template/
  inventory/
    stock.yaml               # what Miles owns: boards, strips, mics, PSUs, with revisions
    README.md                # how to add an entry; `just inventory` prints and validates
  shared/                    # lab_core, lab_audio, lab_fx, lab_board (PlatformIO libs)
  tools/                     # uv-managed Python: serial watcher, hil_assert, inventory, status
  templates/project/         # copied by `just new`
  docs/
    lessons.md               # append-only, dated one-liners
    decisions.md             # one file, newest first, 5 lines per decision
    hil-protocol.md
  projects/
    p000-blink/
    p001-matrix-audio/
    p002-wheel-chandelier/
```

Inside a project, PlatformIO's standard shape is kept so tooling and docs apply unchanged:

```
projects/p001-matrix-audio/
  platformio.ini             # extends ../../platformio.common.ini; lists which boards it supports
  README.md                  # status line, wiring table, power budget, how to demo (short)
  include/config.h           # project needs only; pins resolve through the selected board.h
  src/main.cpp
  test/test_native/          # pure-logic tests, run on the host
  wokwi.toml  diagram.json   # optional
  hil/checklist.md           # human verification steps
```

Naming: `pNNN-short-name`, zero-padded, never renumbered. Shared libraries are prefixed `lab_` so they cannot collide with registry libraries.

## Toolchain decisions

PlatformIO Core with the pioarduino platform is the build system; it is the only way to get Arduino-ESP32 core 3.x (and the S3/C6/P4 chips) under PlatformIO, because the official `espressif32` platform stopped at core 2.x after Espressif and PlatformIO parted ways. Everything below is pinned in `platformio.common.ini`, which each project extends.

| Layer | Choice | Pin | Why |
| --- | --- | --- | --- |
| Toolchain container | Docker image `esp32-lab-dev` built from the root `Dockerfile`, published to GHCR | tag per commit, `latest` for daily use | One image for the agent's shell, the devcontainer, and CI; nothing to install but Docker |
| Build system | PlatformIO Core (CLI) inside the image | 6.x via `uv tool install platformio` | Scriptable, native test runner, headless |
| Platform | [pioarduino platform-espressif32](https://github.com/pioarduino/platform-espressif32) | stable zip URL, bumped by a line in `docs/decisions.md` | Only route to Arduino-ESP32 core 3.x under PlatformIO |
| Framework | Arduino on ESP-IDF 5.x | implied by platform | FastLED, WiFi, OTA, community |
| Boards | Pluggable: `boards/<name>/board.ini` + `board.h`; a project picks one or several | `s3mini` (ESP32-S3FH4R2, 4 MB, 2 MB PSRAM), `m5stickc-plus11` (ESP32-PICO-D4, 4 MB, no PSRAM, PDM mic, 1.14" TFT, AXP192 PMIC) | No single-board assumption; `just add-board` scaffolds the next one |
| LED library | FastLED | `fastled/FastLED @ 3.10.7` | Audio API, RMT5, RGBW, XYMap, WASM compiler |
| Other libs | ArduinoJson, ESPAsyncWebServer, ArduinoOTA, M5Unified (M5 boards only) | exact versions in each board or project `lib_deps` | Web UI, OTA, display and PMIC on the M5 |
| Host tests | PlatformIO `native` env with Unity | pinned | Pure logic in CI, no hardware |
| Simulation | [Wokwi CLI](https://docs.wokwi.com/wokwi-ci/github-actions) | `wokwi-ci-action@v1`, token in secrets | Serial-level behavior in CI, per project |
| Flash and serial | esptool and `pio device monitor` on the host (USB does not cross Docker on macOS/Windows) | installed by `just bootstrap` via uv | Build in the container, flash from the host; on Linux the container can take `--device` |
| Python tooling | uv | `pyproject.toml` at root | Serial watcher, inventory, status |
| Task runner | just | `justfile` at root | Same verbs for agent, human, CI |
| Format and lint | clang-format, cppcheck, ruff | `just check` | Agent output stays uniform without discussion |

Deliberately not chosen for now, with the escape hatch noted: Arduino IDE (no headless story), ESP-IDF native (later, per project, when a piece needs it), FastLED's `fbuild` (promising but young; revisit once p000 to p002 are stable), MicroPython or CircuitPython (FastLED is the point).

The common ini every project extends:

```ini
; platformio.common.ini  (projects extend this; boards are picked per env)
[common]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time
build_flags = -DLAB_SERIAL_JSON=1 -I${PROJECT_DIR}/../../boards
lib_deps = fastled/FastLED @ 3.10.7
lib_extra_dirs = ${PROJECT_DIR}/../../shared

; one env per supported board; each pulls its board.ini fragment
[env:s3mini]
extends = common, boards_s3mini

[env:m5stickc-plus11]
extends = common, boards_m5stickc_plus11

[env:native]
platform = native
test_framework = unity
build_flags = -std=c++17 -DLAB_NATIVE=1 -I${PROJECT_DIR}/../../boards
```

```ini
; boards/s3mini/board.ini  (included via extra_configs)
[boards_s3mini]
board = esp32-s3-devkitc-1
board_build.flash_size = 4MB
board_build.arduino.memory_type = qio_qspi
build_flags = -DLAB_BOARD_S3MINI -DARDUINO_USB_CDC_ON_BOOT=1 -DBOARD_HAS_PSRAM

; boards/m5stickc-plus11/board.ini
[boards_m5stickc_plus11]
board = m5stick-c
build_flags = -DLAB_BOARD_M5STICKC_PLUS11
lib_deps = m5stack/M5Unified
```

```cpp
// boards/s3mini/board.h  (board.h picks itself from the LAB_BOARD_* flag)
#define BOARD_NAME        "s3mini"
#define BOARD_LED_PIN     4        // default data pin for external strips
#define BOARD_ONBOARD_RGB 21       // onboard WS2812, if present
#define BOARD_I2S_WS 7
#define BOARD_I2S_SD 8
#define BOARD_I2S_CLK 6
#define BOARD_HAS_PSRAM   1
#define BOARD_HAS_PDM_MIC 0
#define BOARD_HAS_TFT     0
#define BOARD_SAFE_PINS   {1,2,4,5,6,7,8,9,10,11,12,13}   // verified at bring-up
```

Every `board.h` value marked "verified at bring-up" is confirmed by p000 on that board before any project trusts it. The M5StickC Plus 1.1 needs its AXP192 rails enabled before the display or Grove port is powered; `lab_board` hides that behind `lab_board::begin()`.

## Agent workflow

AGENTS.md is the contract: it tells every Claude Code session what is pinned, which commands are the only way to build, flash, and test, and when it must stop and ask a human. CLAUDE.md contains only `@AGENTS.md` so the same file serves any agent.

### What AGENTS.md must contain

1. **Reply budget.** Status to Miles in at most five lines. A HIL request in at most four. End-of-task report is one paragraph plus the path to the log. Anything longer goes in a file, and the reply links to it.
2. **Pinned facts.** Platform URL, core version, FastLED version, image tag. "Do not upgrade without a line in `docs/decisions.md`."
3. **The verbs.** `just new <p> --board <b>`, `just build <p> [b]`, `just test <p>`, `just flash <p> [b]`, `just watch <p>`, `just sim <p>`, `just check`, `just inventory`, `just add-board <name>`. Nothing else is used to build, flash, or test, so CI and humans run identical commands.
4. **Board rule.** Read `boards/<b>/board.h` and `pinout.md` before touching a pin. Only `BOARD_SAFE_PINS` may carry LED data or I2S. Capability flags gate features: no `BOARD_HAS_PDM_MIC`, no PDM code.
5. **Hardware rules.** `setMaxPowerInVoltsAndMilliamps` is mandatory and set in `config.h`; level shifter assumed on every 5 V data line; never above 800 mA from a laptop port; RMT for clockless LEDs; audio on its own I2S peripheral.
6. **Verification ladder.** Compile, native tests, Wokwi if present, flash and 30 s of serial, then one human check. A claim of "working" names the rung reached.
7. **Stop conditions.** Ask before changing a pin map, raising a power cap above 2000 mA, editing `platformio.common.ini` or anything in `boards/`, deleting a project, or anything that needs hands on hardware. Ask as a yes/no, not a paragraph.
8. **Where to write things down.** Gotcha → `docs/lessons.md` (one dated line). Choice → `docs/decisions.md` (five lines). Hardware bought or broken → Miles edits `inventory/stock.yaml`; the agent may propose the diff.
9. **Hobbyist mode.** Wiring instructions are a numbered checklist with pin numbers and wire colors, never prose. When two wirings would both work, pick the one with fewer parts.
10. **Style.** clang-format decides; config in `config.h`, behavior in `src/`, reusable code in `shared/lab_*`; no comments restating FastLED docs.

### Starting a new idea

From idea to a board doing something new in under five minutes, with Miles typing one line.

1. Miles: `just new p003-desk-lamp --board s3mini` (the skill asks the board if omitted, reading `inventory/stock.yaml` for what is on hand).
2. The template lands with `config.h` defaults, a passing native test, the serial contract, OTA off, WiFi off.
3. The agent runs `just build p003-desk-lamp && just flash p003-desk-lamp && just watch p003-desk-lamp` and confirms `boot` and `leds` in the log. If that fails, it is a template bug, fixed in `templates/`, never in the project.
4. Miles describes the idea in two sentences. The agent works until the ladder is climbed, then asks one HIL question.
5. Nothing about CI, lint, or docs is touched by hand; `just check` and the template already satisfy them.

### Hardware-in-the-loop protocol

The human is a slow, expensive sensor; the protocol keeps each request cheap. Written in `docs/hil-protocol.md` and exposed as the `/hil-check` skill.

- The agent first exhausts everything it can see itself: compile output, native tests, serial JSON.
- A human request is one message with four parts: what to do ("plug in p001 via the left USB-C port"), what to look for ("all 64 pixels should sweep red → green → blue, about 1 s per color"), what a failure would look like ("first pixel only, or flicker"), and the exact reply format ("reply `ok`, `fail: <what you saw>`, or a photo").
- Requests are batched: one connect, several checks, one disconnect.
- Any `fail` reply goes to `docs/lessons.md` with the fix once found.

### Serial contract

Firmware prints one JSON object per line on the USB CDC port so the agent can assert on it instead of reading prose. `shared/lab_core` provides the helper; `just watch` captures it to `projects/<p>/.hil/last-run.jsonl`.

```json
{"t":1250,"ev":"boot","proj":"p001-matrix-audio","fw":"a1b2c3d","fastled":"3.10.7","core":"3.2.1"}
{"t":1300,"ev":"leds","n":64,"driver":"RMT","cap_ma":800}
{"t":1320,"ev":"audio","mic":"INMP441","i2s":0,"ok":true}
{"t":6320,"ev":"fps","fps":118,"heap":214000,"ma_est":410}
{"t":9000,"ev":"beat","bass":0.71}
```

Required events: `boot`, `leds`, `fps` every 5 s, `err` with a `code` string. Everything else is per project. A boot without `leds` within 2 s is a test failure.

### Definition of done for a change

- Compiles for the target env and the native env.
- Native tests pass; new logic has a test.
- `just lint` clean.
- Flashed and `last-run.jsonl` shows `boot`, `leds`, and at least one `fps` with no `err`.
- If visual behavior changed: one HIL check answered `ok`.
- README status line and `lessons.md` updated if anything surprised the agent.

## Testing strategy

Four tiers, cheapest first; each tier catches what the one above cannot, and the agent climbs only as far as the change requires.

| Tier | Runs where | What it proves | Needs a human | Gate |
| --- | --- | --- | --- | --- |
| 1. Native unit tests | Host, `pio test -e native` | Pure logic: XY/ring mappers, palette math, audio band math, power estimator, JSON serial encoder | No | Every commit, CI |
| 2. Compile for target | Host, `pio run` | Pin macros, FastLED templates, library versions all resolve for the S3 | No | Every commit, CI |
| 3. Wokwi scenario | Cloud sim, `wokwi-cli` | Boot sequence, serial contract, WiFi connect, button and timing logic | No | Projects with `wokwi.toml`, CI nightly or on demand |
| 4. Hardware smoke | Bench, `just flash` + `just watch` + HIL check | Real LED output, mic input, power draw, RF behavior | Yes, for visuals | Before marking a change done |

&#91;embedded content: verification ladder · 5 rungs, fail loops back to compile\]

A failure at any rung sends the agent back to fix and recompile; only a change that reaches rung 4 may spend a human question on rung 5.

How each tier is kept honest:

- **Tier 1 design rule.** Anything that can be a pure function is one, in `shared/lab_*` or the project's `lib/`, with FastLED types behind a thin header so it compiles on the host. `CRGB` and `CRGBPalette16` are available in FastLED's stub build; drivers are not. Audio logic is tested with FastLED's fake audio input so beat and band code runs without a mic.
- **Tier 2 is the floor.** The agent never edits without a compile. A change that only compiles is reported as "compiles, untested."
- **Tier 3 is scoped.** Wokwi simulates the S3, serial, WiFi, and a WS2812 strip or matrix well enough to assert on the serial contract and on which pixels light; it does not simulate an I2S microphone or power draw. Scenarios are YAML (`expect-text`, `wait-serial`, `set-control`), stored in `test/wokwi/`. Each project opts in with a `wokwi.toml` and `diagram.json`; CI runs them with the `wokwi-ci-action` and a `WOKWI_CLI_TOKEN` secret.
- **Tier 4 is structured, not vibes.** `just watch` records 30 s of JSON serial to `.hil/last-run.jsonl` and `tools/hil_assert.py` checks for `boot`, `leds`, `fps ≥ 30`, and no `err`. Only then does the agent spend a human question, using the HIL protocol. Each project keeps its checks in `hil/checklist.md` so repeated runs ask the same questions.
- **Golden frames, later.** Once effects stabilize, a tier-1 test renders N frames of an effect into a buffer and compares a hash to a committed golden; a changed hash forces an intentional update. This catches accidental palette or mapping regressions without hardware.

What is explicitly not tested automatically: brightness perception, color accuracy, mic placement, enclosure fit. Those are HIL checks and photographs in the project README.

## CI, linting and formatting

CI runs the same `just` verbs the agent runs locally, across every project, on every push and pull request; there is no CI-only logic to drift. GitHub Actions is free for a public personal repo and cheap enough for a private one.

### Workflows

| Workflow | Trigger | Jobs | Time budget |
| --- | --- | --- | --- |
| `image.yml` | change to `Dockerfile` or `pyproject.toml` | build and push `ghcr.io/<owner>/esp32-lab-dev`, tagged by commit and `latest` | once per toolchain change |
| `ci.yml` | push to `main`, PR if one exists | in the image: `just check`, `just test-all`, then a build matrix generated from `projects/` × its supported boards; `firmware.bin` per project-board as an artifact | under 10 min with the PlatformIO cache |
| `wokwi.yml` | nightly, `workflow_dispatch` | build then `wokwi-ci-action` per project with `wokwi.toml` | minutes, free beta token |
| `release.yml` | tag `pNNN/vX.Y` | attach `firmware.bin` and `littlefs.bin` to a GitHub release | so OTA can pull a known binary |

Red `main` is handled by a revert commit from the agent, not by blocking merges; there is no required review.

The build matrix is generated from the `projects/` directory listing so a new project is in CI the moment it exists. PlatformIO's `~/.platformio` is cached by hash of `platformio.common.ini` plus each `platformio.ini`.

```yaml
# .github/workflows/ci.yml (sketch)
name: ci
on: { push: { branches: [main] }, pull_request: {} }
jobs:
  check:
    runs-on: ubuntu-latest
    container: ghcr.io/${{ github.repository_owner }}/esp32-lab-dev:latest
    steps:
      - uses: actions/checkout@v4
      - run: just check && just test-all
  matrix:
    needs: check
    runs-on: ubuntu-latest
    outputs: { targets: ${{ steps.gen.outputs.targets }} }
    steps:
      - uses: actions/checkout@v4
      - id: gen
        run: echo "targets=$(uv run tools/ci_matrix.py)" >> "$GITHUB_OUTPUT"   # [{project, board}, ...]
  build:
    needs: matrix
    runs-on: ubuntu-latest
    container: ghcr.io/${{ github.repository_owner }}/esp32-lab-dev:latest
    strategy: { matrix: { target: ${{ fromJson(needs.matrix.outputs.targets) }} } }
    steps:
      - uses: actions/checkout@v4
      - uses: actions/cache@v4
        with: { path: ~/.platformio, key: pio-${{ hashFiles('platformio.common.ini', 'boards/**/board.ini') }} }
      - run: just build ${{ matrix.target.project }} ${{ matrix.target.board }}
      - uses: actions/upload-artifact@v4
        with: { name: ${{ matrix.target.project }}-${{ matrix.target.board }}, path: projects/${{ matrix.target.project }}/.pio/build/${{ matrix.target.board }}/firmware.bin }
```

### Lint and format

- **`just check`** is the single lint verb: clang-format (`BasedOnStyle: Google`, 100 columns) in check mode, cppcheck (`--enable=warning,performance`, errors only fail), ruff for `tools/`, and a YAML schema check on `inventory/stock.yaml` and every `board.ini`. The agent runs it before every commit; CI runs the same command. No pre-commit hooks, no git-side nagging.
- **`just fmt`** applies clang-format and ruff; the agent runs it rather than arguing about style.
- **.editorconfig** so a human editor agrees with the formatter on the basics.
- **No Dependabot.** It does not understand `lib_deps`; version bumps are a deliberate line in `docs/decisions.md` plus a green CI.

## Documentation and knowledge capture

Knowledge is written where the next agent session will read it, in four files with fixed shapes, so capturing a lesson costs one line and finding it costs one grep.

| File | Shape | Written when | Read by |
| --- | --- | --- | --- |
| `inventory/stock.yaml` | One entry per item: id, type, model, revision, qty, specs, source, date, `projects:`, notes. Validated by `just inventory` | Miles on every purchase, breakage, or assignment; the agent proposes diffs | `just new` to pick a board; the agent before any hardware proposal; the README status page |
| `docs/lessons.md` | Append-only. `- 2026-10-22 p001/s3mini: <symptom> → <cause> → <fix>` | Any HIL `fail`, any "that took more than 20 minutes" | AGENTS.md tells the agent to grep it before debugging |
| `docs/decisions.md` | One file, newest first, five lines per entry: date, what, instead of, why, revisit when | Any version bump, pin change, library or architecture choice | Agent before proposing a change to the same thing |
| `boards/<name>/pinout.md` | Pinout table, strapping pins, safe pins, USB and power quirks, measured facts | First bring-up, then corrections | Every project that targets the board |
| `projects/pNNN/README.md` | Status line, one photo, wiring table, pin map, power budget, how to demo. One screen | Every change that affects any of those | Miles at a glance; the agent at session start |

### Per-project README template

```markdown
# pNNN short name

**Status:** prototype | bench-verified | installed | retired  ·  **Last HIL check:** 2026-10-22 ok

One sentence on what it is and where it lives.

![photo](docs/photo.jpg)

## Wiring
| Signal | GPIO | Goes to | Notes |
| ------ | ---- | ------- | ----- |
| LED data | 4 | 74AHCT125 → strip DIN | 330 Ω inline |
| I2S WS / SD / CLK | 7 / 8 / 6 | INMP441 | L/R to GND |

## Power
LEDs: 64 × WS2812B · cap 800 mA · supply: USB-C 5 V 3 A brick

## Demo
`just flash p001 && just watch p001`, expect `fps` ≥ 60 and the matrix reacting to a clap.

## Open issues
- [ ] …
```

### Inventory entry

```yaml
# inventory/stock.yaml
boards:
  - id: s3mini-01..03
    model: Waveshare ESP32-S3 Mini
    revision: ESP32-S3FH4R2, 4MB flash, 2MB PSRAM
    qty: 3
    source: Amazon B0CR2RH7PS, 2026-10-20
    projects: [p000-blink]
    notes: headers shipped loose
  - id: m5stickc-01
    model: M5StickC Plus 1.1
    revision: ESP32-PICO-D4, 4MB flash, SPM1423 PDM mic, 1.14in ST7789, AXP192
    qty: 1
    source: on hand
    projects: []
leds:
  - id: matrix-8x8-01
    model: BTF-LIGHTING WS2812B 8x8 flexible
    revision: WS2812B 5050, 5V
    qty: 1
    source: Amazon, 2020-12
    projects: [p001-matrix-audio]
mics:
  - id: inmp441-01..03
    model: INMP441 breakout
    qty: 3
    source: Amazon B0972XP1YS, 2026-10-20
power:
  - id: psu-5v10a-01
    model: BTF 5V 10A 50W UL
    qty: 0
    source: saved for later
```

The `README.md` at the root is for a human arriving cold: what the repo is, `just bootstrap`, and a table of projects with their status lines pulled from each README by `tools/status.py`.

## Project template and conventions

`just new pNNN-name` copies `templates/project/`, and the result builds, passes its one native test, and prints the serial contract before a single line is changed. Everything a piece needs to declare lives in `include/config.h`; `src/main.cpp` stays generic for as long as possible.

### config.h

```cpp
#pragma once
#include "lab_board.h"              // resolves to boards/<selected>/board.h

#define LAB_PROJECT      "p001-matrix-audio"

// LEDs (pins default from the board; override only when wired differently)
#define LED_TYPE         WS2812B
#define LED_ORDER        GRB
#define LED_RGBW         0
#define NUM_LEDS         64
#define LED_DATA_PIN     BOARD_LED_PIN
#define MATRIX_W         8            // 0 when not a matrix
#define MATRIX_H         8
#define MATRIX_SERPENTINE 1

// Power (mandatory; the template refuses to build without both)
#define POWER_VOLTS      5
#define POWER_MAX_MA     800

// Audio: 0 off, 1 external I2S (INMP441), 2 board PDM mic (M5 and friends)
#define AUDIO_MODE       1

// Connectivity
#define WIFI_ENABLED     0            // credentials in secrets.h, never here
#define OTA_ENABLED      0
#define MDNS_NAME        "matrix"
```

### Conventions

- **Pins come from the board.** Projects use `BOARD_*` names from the selected `board.h`; a literal GPIO number in project code is a `just check` failure. Strapping pins are excluded from `BOARD_SAFE_PINS` per board.
- **Power.** `lab_core::begin()` applies the cap; project code cannot forget it. Above 2000 mA requires a README power table naming supply and fuse.
- **Drivers.** RMT for clockless LEDs on every ESP32 family; audio on its own I2S peripheral; parallel LED drivers opt-in per project.
- **Secrets.** `include/secrets.h` gitignored, `secrets.example.h` committed. Public repo, so the agent greps for SSIDs and passwords before every commit (`just check` does it).
- **OTA.** Hostname is the project name; `just flash --ota` first, USB fallback.
- **Serial.** JSON lines through `lab_core::log`; no stray `Serial.println` in committed code.
- **Effects and mapping.** Effects in `shared/lab_fx` as functions of `(CRGB*, const Layout&, const AudioFrame&, uint32_t t)`; `Layout` wraps XYMap and a ring map so effects are written once.
- **Git.** Trunk only. The agent commits to `main` in small Conventional Commits with the project as scope (`feat(p001): beat comet`). CI runs on push; a red build is reverted, not debated. Tags `pNNN/vX.Y` mark firmware installed on a real piece.
- **License.** MIT at the root; third-party code keeps its own notice.

## Bootstrap milestones for Claude Code

Five milestones, each with a check Miles can run or see; Claude Code stops at each gate and reports which rungs of the verification ladder it reached. M0 to M2 need no hardware beyond one S3 Mini on USB.

1. **M0: Skeleton and image.** Tree, `justfile`, `Dockerfile` and `image.yml`, `platformio.common.ini`, `boards/s3mini` and `boards/m5stickc-plus11` from datasheets (marked unverified), `inventory/stock.yaml` with what is on hand, AGENTS.md under 150 lines, MIT license, `ci.yml`. `templates/project` builds for both boards and native with one passing test. Gate: fresh clone, `just bootstrap && just test-all && just build p000-blink s3mini` passes; CI green on first push.
2. **M1: p000-blink on both boards.** Onboard LED via FastLED on RMT, serial contract, `hil_assert` passes, OTA once on the S3. Gate: one HIL `ok` per board; both `pinout.md` files corrected from measured facts; first `lessons.md` lines.
3. **M2: Shared libs and `just new`.** `lab_core`, `lab_board`, `lab_fx` (two effects), `Layout`, native tests for mappers and the power estimator, `just new` and `just add-board` working end to end. Gate: `just new p099-scratch --board m5stickc-plus11` flashes and logs `boot` in under five minutes of wall time, then is deleted.
4. **M3: p001-matrix-audio.** 8x8 matrix plus INMP441 on the S3, `lab_audio` over FastLED's audio API, two audio-driven effects, web UI, OTA. Gate: HIL checklist `ok`; `fps ≥ 60` with audio and WiFi on.
5. **M4: p002-wheel-chandelier scaffold.** Created from the template with its config (235 LEDs, two rings, `POWER_MAX_MA 5000`), ring layout, three modes drafted against the WASM compiler or a strip offcut. Gate: builds and passes native tests; hardware waits for the saved-for-later parts.

Acceptance for the repo as a whole: a brand-new Claude Code session, given only the repo and a board on USB, can read AGENTS.md, run `just flash p001 && just watch p001`, and correctly report whether the firmware booted, without any instruction from Miles.

### Open questions

- [ ] Stock `esp32-s3-devkitc-1` with overrides, or a committed custom board JSON for the Waveshare Mini? Decide at M1 from measured flash and PSRAM.
- [ ] M5StickC Plus 1.1: drive the TFT and PMIC through M5Unified, or keep it bare and only use the LED and mic? M5Unified is the low-friction default.
- [ ] Does Wokwi earn its keep for LED-only pieces? Trial on p000 and p001, then one line in `decisions.md`.
- [ ] Which strips are actually in the box? Inventory before M3 so p001 is built from what exists.

### Sources

- [pioarduino platform-espressif32](https://github.com/pioarduino/platform-espressif32): Arduino core 3.x support under PlatformIO
- [CNX Software on the pioarduino fork](https://www.cnx-software.com/2024/08/27/platform-espressif32-fork-platformio-arduino-esp32-c6-esp32-c5-esp32-h2-esp32-p4): why the official platform stalled at core 2.x
- [Wokwi CI with GitHub Actions](https://docs.wokwi.com/wokwi-ci/github-actions) and [automation scenarios](https://docs.wokwi.com/wokwi-ci/automation-scenarios)
- [FastLED audio API](https://fastled.io/docs/dir_80b55e5ea7143e3a561e896c6c19b58f.html) and [releases](https://github.com/fastled/fastled/releases)
- [Claude Code AGENTS.md support](https://www.eesel.ai/blog/claude-code-agents-md)
