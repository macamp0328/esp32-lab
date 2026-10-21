---
name: hil-check
description: Ask Miles one cheap hardware-in-the-loop question (plug in, look at LEDs, read a screen) after the machine-checkable rungs pass. Use only once `just watch` passes and a visual or physical check remains.
---

# HIL check (docs/hil-protocol.md)

Only after rung 4 (`just watch` PASS). Take the question from `projects/<p>/hil/checklist.md`.

One message, at most four lines, batched (one connect, several checks, one disconnect):
- **Do:** what to do ("plug the M5 into USB-C").
- **Look for:** success, with timing ("red → green → blue, about 1 s each").
- **Fail looks like:** ("dark, stuck on one color, or another order").
- **Reply:** `ok`, `fail: <what you saw>`, or a photo.

After the answer: `ok` → update the project README's **Last HIL check** and run `just status`.
`fail` → fix, then add a `docs/lessons.md` line (`date p/board: symptom → cause → fix`).
