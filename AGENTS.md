# AGENTS.md: contract for every agent session in esp32-lab

You write 100% of the code. Miles supplies hardware, eyes and decisions. He is a software
engineer and a hobbyist at hardware: he solders well, he is not an EE. Full design: docs/SPEC.md.

## 1. Reply budget
- Status to Miles: at most 5 lines. A HIL request: at most 4 lines. End of task: one paragraph
  plus the path to the log or file with the detail.
- Anything longer goes in a file; the reply links to it. A wall of text is a failed turn.
- Questions to Miles are yes/no, batched, and only for what a shell cannot answer.

## 2. Pinned facts (do not upgrade without a line in docs/decisions.md and a green CI)
| What | Pin | Where |
| --- | --- | --- |
| Platform | pioarduino `55.03.312-1` (Arduino-ESP32 core 3.3.12, ESP-IDF 5.5.5) | platformio.common.ini |
| FastLED | `fastled/FastLED @ 3.10.7` | platformio.common.ini |
| M5 libs | `M5Unified @ 0.2.27`, `M5GFX @ 0.2.34` | boards/m5stickc-plus11/board.ini |
| PlatformIO Core | 6.2.0 | pyproject.toml + uv.lock |
| Image | `esp32-lab-dev:tc-<hash of Dockerfile+pyproject+uv.lock>`; CI: `ghcr.io/<owner>/esp32-lab-dev` | Dockerfile |
| Native tests | `platformio/native @ 1.2.1`, Unity | platformio.common.ini |

## 3. The verbs (the only way to build, flash and test; CI runs the same ones)
| Verb | Does | Status |
| --- | --- | --- |
| `just bootstrap` | build image (if missing) + host tools in `.venv` | works |
| `just new <pNNN-name> --board <b>[,<b>]` | scaffold from `templates/project/` | works (hardened in M2) |
| `just build <p> [board]` | compile; no board = every board in `default_envs` | works |
| `just test <p>` / `just test-all` | native Unity tests (+ tools pytest for test-all) | works |
| `just check` (= `just lint`) / `just fmt` | clang-format, cppcheck, ruff, repo rules | works |
| `just inventory` / `just status` | print+validate stock / refresh README table | works |
| `just flash <p> [b]`, `just watch <p>` | USB flash, 30 s serial capture | M1 (exits 2) |
| `just sim <p>` / `just add-board <name>` | Wokwi scenario / scaffold a board | M1 / M2 (exit 2) |

`<p>` may be the full name or its `pNNN` prefix. Toolchain verbs run in the Docker image
automatically; inside the container (CI, devcontainer, `just shell`) they run directly.
Flash and serial run on the host: USB does not cross Docker on macOS.

## 4. Board rule
- Read `boards/<b>/board.h` and `boards/<b>/pinout.md` before touching any pin.
- Project code uses `BOARD_*` names only; a literal GPIO number is a `just check` failure.
- Only `BOARD_SAFE_PINS` may carry LED data or I2S (`lab_board::pin_is_safe`, static_assert).
- Capability flags gate features: no `BOARD_HAS_PDM_MIC`, no PDM code; no TFT, no display code.
- `BOARD_VERIFIED 0` means the file is from datasheets; p000 bring-up confirms it (M1).

## 5. Hardware rules
- `FastLED.setMaxPowerInVoltsAndMilliamps(POWER_VOLTS, POWER_MAX_MA)` is mandatory; both are
  set in `include/config.h` and the build fails without them.
- Never above 800 mA from a laptop USB port. Above 2000 mA needs a README power table naming
  supply and fuse (and `POWER_README_TABLE` defined).
- Assume a level shifter on every 5 V data line. RMT for clockless LEDs. Audio on its own I2S
  peripheral. M5StickC: AXP192 rails on (`lab_board::begin()`) before TFT or Grove 5 V.

## 6. Verification ladder (a claim of "working" names the rung reached)
1. Compiles for the target env (`just build`). A change that only compiles is "compiles, untested".
2. Native tests pass (`just test`); new logic gets a test.
3. Wokwi scenario passes, if the project has `wokwi.toml`.
4. Flashed, and 30 s of serial (`.hil/last-run.jsonl`) shows `boot`, `leds`, `fps` >= 30, no `err`.
5. One human check (docs/hil-protocol.md), only after rung 4.

A failure at any rung goes back to compile. Never edit without compiling afterwards.

## 7. Stop conditions: ask Miles (yes/no) before
- changing a pin map, or anything in `boards/`;
- raising a power cap above 2000 mA;
- editing `platformio.common.ini` or bumping any pinned version;
- deleting a project;
- anything that needs hands on hardware (plugging, wiring, looking, measuring).

## 8. Where to write things down
| What | Where | Shape |
| --- | --- | --- |
| Gotcha, HIL `fail`, anything that took > 20 min | `docs/lessons.md` | `- 2026-10-22 p001/s3mini: symptom → cause → fix` |
| Choice: version, pin, library, architecture | `docs/decisions.md` | newest first, 5 lines |
| Hardware bought/broken/assigned | `inventory/stock.yaml` | Miles edits; you propose the diff |
| Board facts measured at bring-up | `boards/<b>/pinout.md` | table rows |
| Project state | `projects/<p>/README.md` | status line, wiring, power, demo |

Grep `docs/lessons.md` before debugging. Read `docs/decisions.md` before changing anything it covers.

## 9. Hobbyist mode
- Wiring instructions are a numbered checklist with pin numbers and wire colors, never prose.
- When two wirings both work, pick the one with fewer parts.
- Read `inventory/stock.yaml` before proposing hardware; propose only what exists or say "buy".

## 10. Style and layout
- clang-format decides (Google, 100 columns); ruff for `tools/`. Run `just fmt`, do not argue.
- Config in `include/config.h`, behavior in `src/`, reusable code in `shared/lab_*`.
- Pure logic is a pure function with a native test (in `shared/lab_*` or the project's `lib/`).
- Log only through `lab_core::Event` (one JSON object per line); no stray `Serial.print`.
- No comments restating FastLED docs. Secrets in `include/secrets.h` (gitignored) only.

## Serial contract
One JSON object per line on USB CDC at 115200. Required: `boot` (proj, fw, board, fastled,
core), `leds` (n, driver, cap_ma) within 2 s of boot, `fps` every 5 s (fps, heap), `err` with a
`code`. Example: `{"t":1250,"ev":"boot","proj":"p000-blink","fw":"a1b2c3d","fastled":"3.10.7"}`.

## Layout
`boards/<b>/` board.ini + board.h + pinout.md · `shared/lab_*` libs · `projects/pNNN-name/`
(never renumbered) · `templates/project/` (copied by `just new`) · `tools/` Python (uv) ·
`inventory/` · `docs/`. Projects build alone too: `cd projects/p000-blink && pio run`.

## Git and CI
- Trunk only: commit straight to `main`, small Conventional Commits, project as scope
  (`feat(p001): beat comet`, `chore(ci): …`). No PRs, no review theater.
- Run `just check && just test-all` before every commit. CI (`.github/workflows/ci.yml`) runs
  check, test-all, then builds every project × board in the same image.
- Red `main` gets a revert commit, not a debate. Tags `pNNN/vX.Y` mark installed firmware.
- Template bugs are fixed in `templates/project/`, never only in the project that found them.

## Definition of done for a change
Compiles for target and native · native tests pass, new logic tested · `just check` clean ·
flashed with `boot`, `leds`, `fps`, no `err` in `last-run.jsonl` · one HIL `ok` if visuals
changed · README status line and `docs/lessons.md` updated if anything surprised you.
