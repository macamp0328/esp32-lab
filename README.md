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
| [p002-matrix-demo](projects/p002-matrix-demo/README.md) | m5stickc-plus11 | prototype | never |
<!-- status:end -->

## Layout
`boards/` one folder per physical board · `shared/` `lab_*` libraries · `projects/` one
PlatformIO project each · `templates/project/` what `just new` copies · `inventory/stock.yaml`
what exists · `docs/` decisions, lessons, HIL protocol.

MIT licensed. See [LICENSE](LICENSE).
