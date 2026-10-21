"""Assert a serial capture meets the contract: `hil_assert.py projects/<p>/.hil/last-run.jsonl`.

Pass = exactly one `boot` (a second one means a crash or brown-out), `leds` within 2 s of it,
at least one `fps` and every `fps` >= --min-fps, no `err`, no panic in the raw log, and `boot`
naming the expected project. Prints one summary line; exit 0 on pass, 1 on fail.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

PANIC_MARKERS = ("Guru Meditation", "Backtrace:", "abort() was called", "Brownout detector")
IDF_ERROR = re.compile(r"(^|\s)E \(\d+\) [\w-]+:")


def parse(lines: list[str]) -> tuple[list[dict], int]:
    """JSON objects from capture lines, plus a count of lines that looked like JSON but weren't."""
    events, malformed = [], 0
    for line in lines:
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            malformed += 1
            continue
        if isinstance(obj, dict) and "ev" in obj:
            events.append(obj)
        else:
            malformed += 1
    return events, malformed


def check(
    events: list[dict],
    raw_log: str = "",
    project: str | None = None,
    min_fps: float = 30,
    leds_within_ms: int = 2000,
) -> tuple[list[str], str]:
    """Returns (failures, summary). No failures means pass."""
    fails: list[str] = []
    boots = [e for e in events if e["ev"] == "boot"]
    if not boots:
        return ["no boot event (did the board reset and print to USB CDC?)"], "no boot"
    if len(boots) > 1:
        fails.append(f"{len(boots)} boot events: the board rebooted during the capture")
    boot = boots[0]
    if project and boot.get("proj") != project:
        fails.append(f"boot.proj is {boot.get('proj')!r}, expected {project!r} (wrong firmware?)")

    after = events[events.index(boot) + 1 :]
    leds = next((e for e in after if e["ev"] == "leds"), None)
    if leds is None:
        fails.append("no leds event after boot")
    elif leds.get("t", 0) - boot.get("t", 0) > leds_within_ms:
        fails.append(f"leds came {leds['t'] - boot['t']} ms after boot (limit {leds_within_ms})")

    fps = [e.get("fps", 0) for e in after if e["ev"] == "fps"]
    if not fps:
        fails.append("no fps event (capture shorter than 5 s, or loop() stalled)")
    elif min(fps) < min_fps:
        fails.append(f"fps dropped to {min(fps)} (minimum {min_fps})")

    errs = [e for e in events if e["ev"] == "err"]
    fails += [f"err event: code={e.get('code')!r} at t={e.get('t')}" for e in errs]
    fails += [f"panic in raw log: {m!r}" for m in PANIC_MARKERS if m in raw_log]

    parts = [f"boot proj={boot.get('proj')} fw={boot.get('fw')} board={boot.get('board')}"]
    if leds:
        parts.append(f"leds n={leds.get('n')} {leds.get('driver')} +{leds['t'] - boot['t']}ms")
    if fps:
        parts.append(f"fps min {min(fps)} ({len(fps)} samples)")
    parts.append(f"{len(errs)} err")
    return fails, " · ".join(parts)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="hil_assert")
    ap.add_argument("capture", type=Path, help=".hil/last-run.jsonl")
    ap.add_argument("--project")
    ap.add_argument("--min-fps", type=float, default=30)
    args = ap.parse_args(argv)

    events, malformed = parse(args.capture.read_text(errors="replace").splitlines())
    raw = args.capture.with_suffix(".log")
    raw_text = raw.read_text(errors="replace") if raw.is_file() else ""
    fails, summary = check(events, raw_text, args.project, args.min_fps)
    if malformed:
        fails.append(f"{malformed} malformed JSON line(s)")
    idf_errors = [ln for ln in raw_text.splitlines() if IDF_ERROR.search(ln)]
    print(f"hil_assert: {'FAIL' if fails else 'PASS'} {summary}")
    for f in fails:
        print(f"  - {f}")
    if idf_errors:
        print(
            f"  note: {len(idf_errors)} ESP-IDF error log line(s), not fatal; grep docs/lessons.md"
        )
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
