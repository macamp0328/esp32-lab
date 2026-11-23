# esp32-lab

One monorepo for every ESP32 project: tooling, boards, inventory and everything learned.
An agent (Claude Code) writes the code; [AGENTS.md](AGENTS.md) is its contract and
[docs/SPEC.md](docs/SPEC.md) the design.

## Start
Prerequisites on the host: Docker, [just](https://just.systems) and [uv](https://docs.astral.sh/uv/)
(`brew install just uv`). Everything else lives in the toolchain image.

```bash
just bootstrap                        # build the toolchain image, install host flash tools
just test-all                         # every native test
just build p000-blink s3mini          # compile one project for one board
just new p003-desk-lamp --board s3mini
```

`just` with no arguments lists every verb.

## Projects
<!-- status:begin -->
| Project | Boards | Status | Last HIL check |
| --- | --- | --- | --- |
| [p000-blink](projects/p000-blink/README.md) | s3mini, m5stickc-plus11 | bench-verified | 2026-10-21 s3mini ok, m5stickc-plus11 ok |
| [p002-matrix-demo](projects/p002-matrix-demo/README.md) | m5stickc-plus11 | bench-verified | 2026-10-21 m5stickc-plus11 ok (sets 1 and 2) |
| [p003-bench](projects/p003-bench/README.md) | m5stickc-plus11, s3mini | bench-verified | 2026-10-22 m5stickc-plus11 ok (showcase fps per LED count; A-hold starts the suite) |
| [p004-sense](projects/p004-sense/README.md) | m5stickc-plus11 | bench-verified | 2026-10-23 m5stickc-plus11 ok (all six steps) |
| [p005-trio](projects/p005-trio/README.md) | m5stickc-plus11, s3mini | bench-verified | 2026-10-27 S3 pixels match their corners (RGB fix); step 5 to redo |
| [p006-crystal-ball](projects/p006-crystal-ball/README.md) | s3mini | prototype, demo rotation on (rung 4 on s3mini 720c with the ring wired: WiFi, clock, fps 76; 2 h soak 2026-11-23: fps 76–77, heap flat, WiFi never dropped; OTA update ok, app0 → app1) | 2026-11-22 s3mini 720c: amber and even on all 20 (ok); too dim under the orb at brightness 48, now 72 |
<!-- status:end -->

## Layout
`boards/` one folder per physical board · `shared/` `lab_*` libraries · `projects/` one
PlatformIO project each · `templates/project/` what `just new` copies · `inventory/stock.yaml`
what exists · `docs/` decisions, lessons, HIL protocol.

MIT licensed. See [LICENSE](LICENSE).
