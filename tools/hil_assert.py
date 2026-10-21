"""Assert a serial capture meets the contract: `hil_assert.py projects/<p>/.hil/last-run.jsonl`.

Pass = every contract event carries its fields, exactly one `boot` (a second one means a crash
or brown-out), `leds` within 2 s of it, `fps` at least every 7 s and >= --min-fps, no `err`, no
panic in the raw log, and `boot` naming the expected project. One summary line; exit 0 = pass.
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


# Serial contract (AGENTS.md): fields each required event must carry.
REQUIRED = {
    "boot": ("proj", "fw", "board", "fastled", "core"),
    "leds": ("n", "driver", "cap_ma"),
    "fps": ("fps", "heap"),
    "err": ("code",),
}
NUMERIC = {"t", "fps", "heap", "n", "cap_ma"}
FPS_PERIOD_MS = 5000
FPS_SLACK_MS = 2000


def _is_number(v: object) -> bool:
    return isinstance(v, int | float) and not isinstance(v, bool)


def check(
    events: list[dict],
    raw_log: str = "",
    project: str | None = None,
    min_fps: float = 30,
    leds_within_ms: int = 2000,
    seconds: float | None = None,
) -> tuple[list[str], str]:
    """Returns (failures, summary). No failures means pass. `seconds` = capture length, if known."""
    fails: list[str] = []
    for e in events:
        fields = ("t", *REQUIRED.get(e["ev"], ()))
        missing = [k for k in fields if e.get(k) is None]
        not_numbers = [k for k in fields if k in NUMERIC and not _is_number(e.get(k))]
        if missing or not_numbers:
            fails.append(f"{e['ev']} event missing/non-numeric {missing + not_numbers}: {e}")
    if fails:
        return fails, "malformed events"

    boots = [e for e in events if e["ev"] == "boot"]
    if not boots:
        return ["no boot event (did the board reset and print to USB CDC?)"], "no boot"
    if len(boots) > 1:
        fails.append(f"{len(boots)} boot events: the board rebooted during the capture")
    boot = boots[0]
    if project is not None and boot["proj"] != project:
        fails.append(f"boot.proj is {boot['proj']!r}, expected {project!r} (wrong firmware?)")

    after = events[events.index(boot) + 1 :]
    leds = next((e for e in after if e["ev"] == "leds"), None)
    if leds is None:
        fails.append("no leds event after boot")
    elif leds["t"] - boot["t"] > leds_within_ms:
        fails.append(f"leds came {leds['t'] - boot['t']} ms after boot (limit {leds_within_ms})")

    fps_events = [e for e in after if e["ev"] == "fps"]
    fps = [e["fps"] for e in fps_events]
    limit = FPS_PERIOD_MS + FPS_SLACK_MS
    stamps = [boot["t"]] + [e["t"] for e in fps_events]
    gaps = [b - a for a, b in zip(stamps, stamps[1:], strict=False)]
    if not fps:
        fails.append("no fps event (capture shorter than 5 s, or loop() stalled)")
    else:
        if min(fps) < min_fps:
            fails.append(f"fps dropped to {min(fps)} (minimum {min_fps})")
        if max(gaps) > limit:
            fails.append(f"{max(gaps)} ms without an fps event (contract: every 5 s)")
    if seconds is not None:
        expected = max(1, int((seconds * 1000 - limit) // FPS_PERIOD_MS))
        if len(fps) < expected:
            fails.append(f"only {len(fps)} fps events in {seconds:.0f} s (expected >= {expected})")

    errs = [e for e in events if e["ev"] == "err"]
    fails += [f"err event: code={e['code']!r} at t={e['t']}" for e in errs]
    fails += [f"panic in raw log: {m!r}" for m in PANIC_MARKERS if m in raw_log]

    parts = [f"boot proj={boot['proj']} fw={boot['fw']} board={boot['board']}"]
    if leds:
        parts.append(f"leds n={leds['n']} {leds['driver']} +{leds['t'] - boot['t']}ms")
    if fps:
        parts.append(f"fps min {min(fps)} ({len(fps)} samples)")
    parts.append(f"{len(errs)} err")
    return fails, " · ".join(parts)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="hil_assert")
    ap.add_argument("capture", type=Path, help=".hil/last-run.jsonl")
    ap.add_argument("--project")
    ap.add_argument("--min-fps", type=float, default=30)
    ap.add_argument("--seconds", type=float, help="capture length, to check the fps cadence")
    args = ap.parse_args(argv)
    if args.project == "":
        ap.error("--project is empty (did project resolution fail?)")

    events, malformed = parse(args.capture.read_text(errors="replace").splitlines())
    raw = args.capture.with_suffix(".log")
    raw_text = raw.read_text(errors="replace") if raw.is_file() else ""
    fails, summary = check(events, raw_text, args.project, args.min_fps, seconds=args.seconds)
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
