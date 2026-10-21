# Hardware-in-the-loop protocol

The human is a slow, expensive sensor. Each request must be cheap to answer. Exposed as the
`hil-check` skill (`.claude/skills/hil-check`).

1. **Exhaust the machine first.** Compile output, native tests, Wokwi if present, then
   `just flash` + `just watch` and `tools/hil_assert.py` on `.hil/last-run.jsonl`
   (`boot`, `leds`, `fps` >= 30, no `err`). Only then ask a human.
2. **One message, four parts, at most four lines:**
   - **Do:** what to do ("plug p001 into the left USB-C port").
   - **Look for:** what success looks like ("all 64 pixels sweep red → green → blue, ~1 s each").
   - **Fail looks like:** ("first pixel only, or flicker").
   - **Reply:** `ok`, `fail: <what you saw>`, or a photo.
3. **Batch.** One connect, several checks, one disconnect. Checks come from the project's
   `hil/checklist.md` so repeated runs ask the same questions.
4. **Record.** Every `fail` becomes a line in `docs/lessons.md` once the fix is found, and the
   project README's **Last HIL check** is updated on every answer.
