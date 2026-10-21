---
name: new-project
description: Start a new esp32-lab project from an idea in minutes with `just new`. Use when Miles says he has a new idea, wants a new project, or names a pNNN project that does not exist yet.
---

# New project

1. Pick the next free number: `ls projects` (numbers are never reused). Name: `pNNN-short-name`.
2. Board: if Miles named one, use it. Otherwise run `just new <name>` with no board: it lists the
   boards on hand from `inventory/stock.yaml`. One type on hand → use it; several → ask one
   yes/no ("Build p003 on the s3mini?").
3. `just new <name> --board <b>[,<b>]`, then `just build <name> <b> && just flash <name> <b> &&
   just watch <name>`. Expect hil_assert PASS (boot, leds, fps). A failure here is a template
   bug: fix it in `templates/project/`, not in the project.
4. Put the idea's needs in `include/config.h` (LEDs, `POWER_MAX_MA`, `MATRIX_W/H`, audio,
   WiFi/OTA), behavior in `src/`, pure logic in `lib/` with a test in `test/`. Effects go in
   `shared/lab_fx` when reusable.
5. Read `inventory/stock.yaml` before proposing hardware; wiring is a numbered checklist with pins
   and wire colors. Climb the ladder, then one HIL question (hil-check skill). Update the project
   README and run `just status`.
