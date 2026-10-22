"""Run a project's on-demand benchmark suite and compare boards (host side): `bench.py <project>`.

Resets the board, waits for `leds`, writes `bench`, and records lines until `bench_done`. Fails
on a second `boot`, an `err`, a panic, a timeout or no `bench` results. Writes the capture to
projects/<p>/.hil/bench-<board>.{log,jsonl}; on a pass also results/<board>.json, then rebuilds
RESULTS.md from every results/*.json (one table per group, a column per board). Exit 0 = pass.
"""

from __future__ import annotations

import argparse
import datetime
import json
import math
import re
import sys
import time
from pathlib import Path

import hil_assert
import serial
import serial_watch
from hil_assert import PANIC_MARKERS
from labrepo import PROJECTS
from ports import pick_port_for

GROUPS = ("compute", "led", "dualcore", "memory", "screen")
N_HEADER = {"led": "LEDs", "memory": "bytes"}
BENCH_FIELDS = ("group", "test", "n", "us", "max_us", "per_s")
LEDS_WAIT_S = 15  # no `leds` by then: the firmware is not running
RESEND_S = 3  # `bench` again if no `bench_start` came back (the line got lost on USB re-attach)
MAX_SENDS = 3
GRACE_S = 1.5  # keep reading after a crash so the backtrace lands in the log
RATE_PREFIXES = ((1e9, "G"), (1e6, "M"), (1e3, "k"))
TIME_UNITS = ((1e6, "s"), (1e3, "ms"), (1, "µs"))


class Tracker:
    """What a capture has shown so far, fed one line at a time by collect() and main()."""

    def __init__(self) -> None:
        self.events: list[dict] = []
        self.malformed = 0
        self.panics: list[str] = []

    def feed(self, text: str) -> None:
        events, bad = hil_assert.parse([text])
        self.events += events
        self.malformed += bad
        self.panics += [m for m in PANIC_MARKERS if m in text]

    def of(self, ev: str) -> list[dict]:
        return [e for e in self.events if e["ev"] == ev]

    @property
    def ready(self) -> bool:
        return bool(self.of("leds"))

    @property
    def done(self) -> bool:
        return bool(self.of("bench_done"))

    @property
    def fatal(self) -> bool:
        return len(self.of("boot")) > 1 or bool(self.of("err")) or bool(self.panics)


def collect(port: str, timeout_s: float, baud: int = 115200) -> list[str]:
    """Host-stamped lines from reset until `bench_done`, a crash (plus GRACE_S), or the timeout."""
    lines: list[str] = []
    tracker = Tracker()
    buf = b""
    start = time.monotonic()
    deadline = start + timeout_s
    sends: list[float] = []
    ser = serial_watch.open_port(port, baud)
    serial_watch.reset(ser)
    while time.monotonic() < deadline and not tracker.done:
        now = time.monotonic()
        if not tracker.ready and now - start > LEDS_WAIT_S:
            break
        due = not sends or (now - sends[-1] > RESEND_S and len(sends) < MAX_SENDS)
        try:
            if tracker.ready and not tracker.of("bench_start") and due:
                ser.write(b"bench\n")
                sends.append(now)
            chunk = ser.read(512)
        except (serial.SerialException, OSError):
            ser.close()
            ser = serial_watch.reopen(port, baud, deadline)
            if ser is None:
                break
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            text = raw.decode(errors="replace").rstrip()
            lines.append(f"{time.monotonic() - start:8.3f} {text}")
            was_fatal = tracker.fatal
            tracker.feed(text)
            if tracker.fatal and not was_fatal:
                deadline = min(deadline, time.monotonic() + GRACE_S)
    if ser is not None:
        ser.close()
    return lines


def _is_number(v: object) -> bool:
    return isinstance(v, int | float) and not isinstance(v, bool)


def _bad_fields(e: dict) -> list[str]:
    missing = [k for k in BENCH_FIELDS if e.get(k) is None]
    return missing + [k for k in BENCH_FIELDS[2:] if k not in missing and not _is_number(e[k])]


def check(tracker: Tracker, project: str | None = None) -> tuple[list[str], list[str]]:
    """Returns (failures, warnings) for a finished capture. No failures means pass."""
    fails: list[str] = []
    warns: list[str] = []
    boots = tracker.of("boot")
    if not boots:
        return ["no boot event (did the board reset and print to USB CDC?)"], warns
    if len(boots) > 1:
        fails.append(f"{len(boots)} boot events: the board rebooted during the run")
    if project is not None and boots[0].get("proj") != project:
        fails.append(f"boot.proj is {boots[0].get('proj')!r}, expected {project!r}")
    if not tracker.ready:
        fails.append(f"no leds event within {LEDS_WAIT_S} s, so `bench` was never sent")
    fails += [f"err event: code={e.get('code')!r} at t={e.get('t')}" for e in tracker.of("err")]
    fails += [f"panic in raw log: {m!r}" for m in dict.fromkeys(tracker.panics)]
    results = tracker.of("bench")
    fails += [
        f"bench event missing/non-numeric {bad}: {e}" for e in results if (bad := _bad_fields(e))
    ]
    if tracker.ready and not tracker.of("bench_start"):
        fails.append("no bench_start: the firmware never answered `bench`")
    elif tracker.ready and not tracker.done and not tracker.fatal:
        fails.append("no bench_done: timed out before the suite finished")
    if not results:
        fails.append("no bench results")
    if tracker.malformed:
        warns.append(f"{tracker.malformed} malformed JSON line(s)")
    for ev in ("bench_start", "bench_done"):
        said = [e.get("tests") for e in tracker.of(ev)]
        if said and said[0] != len(results):
            warns.append(f"{ev} says {said[0]} tests, got {len(results)} bench events")
    return fails, warns


def board_name(tracker: Tracker) -> str:
    """The boot event's board, made safe for a file name."""
    boots = tracker.of("boot")
    board = str(boots[0].get("board") or "unknown") if boots else "unknown"
    return re.sub(r"[^a-z0-9-]", "_", board.lower())


def result_doc(tracker: Tracker, project: str, date: datetime.date) -> dict:
    boot, done = tracker.of("boot")[0], tracker.of("bench_done")[0]
    keep = (*BENCH_FIELDS, "note")
    return {
        "project": project,
        "board": board_name(tracker),
        "fw": boot.get("fw"),
        "core": boot.get("core"),
        "fastled": boot.get("fastled"),
        "date": date.isoformat(),
        "ms": done.get("ms"),
        "results": [{k: e[k] for k in keep if k in e} for e in tracker.of("bench")],
    }


def write_result(root: Path, doc: dict) -> Path:
    path = root / "results" / f"{doc['board']}.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return path


def load_results(root: Path) -> list[dict]:
    files = sorted((root / "results").glob("*.json"))
    return [json.loads(p.read_text(encoding="utf-8")) for p in files]


# ---- RESULTS.md ---------------------------------------------------------------------------


def _sig3(v: float) -> str:
    """Three significant figures, never an exponent: 1.23, 12.3, 123, 1230."""
    if v == 0:
        return "0"
    return f"{v:.{max(0, 2 - math.floor(math.log10(abs(v))))}f}"


def fmt_time(us: float) -> str:
    scale, unit = next(((s, u) for s, u in TIME_UNITS if us >= s), (1e-3, "ns"))
    return f"{_sig3(us / scale)} {unit}"


def fmt_rate(per_s: float, unit: str = "") -> str:
    scale, prefix = next(((s, p) for s, p in RATE_PREFIXES if per_s >= s), (1, ""))
    return f"{_sig3(per_s / scale)}{' ' if prefix or unit else ''}{prefix}{unit}/s"


def _per_s(e: dict) -> float:
    return e.get("per_s") or (1e6 / e["us"] if e["us"] else 0.0)


def row_kind(group: str, n: int, entries: list[dict]) -> str:
    """bytes: memory throughput; per_op: math/ loops of n calls; time: 1 ms or more per iteration;
    rate: anything faster."""
    if group == "memory" and n > 1:
        return "bytes"
    if n > 1 and entries[0]["test"].startswith("math/"):
        return "per_op"
    return "time" if max(e["us"] for e in entries) >= 1000 else "rate"


def fmt_cell(kind: str, e: dict) -> str:
    if kind == "bytes":
        return fmt_rate(_per_s(e) * e["n"], "B")
    if kind == "per_op":
        return f"{fmt_time(e['us'] / e['n'])}/call"
    return fmt_time(e["us"]) if kind == "time" else fmt_rate(_per_s(e))


def fmt_ratio(a: dict, b: dict) -> str:
    """How many times faster a ran than b."""
    return f"{_sig3(b['us'] / a['us'])}×" if a["us"] and b["us"] else "–"


def groups(docs: list[dict]) -> list[str]:
    seen = {r["group"] for d in docs for r in d["results"]}
    return [g for g in GROUPS if g in seen] + sorted(seen - set(GROUPS))


def _row(cells: list[object]) -> str:
    return "| " + " | ".join(str(c).replace("|", "\\|") for c in cells) + " |"


def group_table(group: str, docs: list[dict]) -> list[str]:
    boards = [d["board"] for d in docs]
    pair = len(docs) == 2
    by_board = [{(r["test"], r["n"]): r for r in d["results"] if r["group"] == group} for d in docs]
    # Firmware order from the board that ran the most of this group, then the others' extras.
    keys = list(dict.fromkeys(k for b in sorted(by_board, key=len, reverse=True) for k in b))
    notes = any("note" in r for b in by_board for r in b.values())
    head = ["test", N_HEADER.get(group, "n"), *boards]
    head += [f"{boards[0]} vs {boards[1]}"] if pair else []
    out = [_row(head + (["note"] if notes else []))]
    out.append(_row(["---"] + ["---:"] * (len(head) - 1) + (["---"] if notes else [])))
    for test, n in keys:
        entries = [b.get((test, n)) for b in by_board]
        kind = row_kind(group, n, [e for e in entries if e])
        cells = [test, str(n), *(fmt_cell(kind, e) if e else "–" for e in entries)]
        if pair:
            cells.append(fmt_ratio(*entries) if all(entries) else "–")
        if notes:
            cells.append(" / ".join(dict.fromkeys(e["note"] for e in entries if e and "note" in e)))
        out.append(_row(cells))
    return out


def render(project: str, docs: list[dict]) -> str:
    docs = sorted(docs, key=lambda d: d["board"])
    out = [
        f"# {project} results",
        "",
        "Generated by `just bench` from `results/*.json`; do not edit.",
        "",
        _row(["board", "fw", "core", "date", "tests", "suite"]),
        _row(["---", "---", "---", "---", "---:", "---:"]),
    ]
    for d in docs:
        suite = f"{d['ms'] / 1000:.1f} s" if d.get("ms") is not None else "–"
        info = [d.get("fw") or "–", d.get("core") or "–", d["date"], len(d["results"]), suite]
        out.append(_row([d["board"], *info]))
    for group in groups(docs):
        out += ["", f"## {group}", "", *group_table(group, docs)]
    out += ["", "Each row shows its most readable figure: iterations per second when one takes"]
    out.append("under 1 ms, the mean time per iteration when longer, time per call for `math/`")
    out.append("rows, and throughput for memory.")
    if len(docs) == 2:
        a, b = docs[0]["board"], docs[1]["board"]
        out.append(f"`{a} vs {b}` = {b}'s time ÷ {a}'s: how many times faster {a} ran.")
    return "\n".join(out) + "\n"


def summary(doc: dict) -> list[str]:
    lines = []
    for group in groups([doc]):
        rs = sorted((r for r in doc["results"] if r["group"] == group), key=lambda r: r["us"])
        lines.append(
            f"  {group}: {len(rs)} tests · fastest {rs[0]['test']} ({fmt_time(rs[0]['us'])})"
            f" · slowest {rs[-1]['test']} ({fmt_time(rs[-1]['us'])})"
        )
    return lines


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just bench")
    ap.add_argument("project")
    ap.add_argument("board", nargs="?", help="which board, when several are plugged in")
    ap.add_argument("--port")
    ap.add_argument("--timeout", type=float, default=180, help="seconds from reset to bench_done")
    args = ap.parse_args(argv)

    root = PROJECTS / args.project
    port = args.port or pick_port_for(args.project, args.board)
    print(f"bench: {port}: reset, send `bench` after leds, up to {args.timeout:.0f} s", flush=True)
    lines = collect(port, args.timeout)
    tracker = Tracker()
    payloads = [line[9:] for line in lines]
    for p in payloads:
        tracker.feed(p)

    board = board_name(tracker)
    out = root / ".hil"
    out.mkdir(parents=True, exist_ok=True)
    (out / f"bench-{board}.log").write_text("".join(f"{x}\n" for x in lines), encoding="utf-8")
    jsonl = "".join(f"{p}\n" for p in payloads if p.lstrip().startswith("{"))
    (out / f"bench-{board}.jsonl").write_text(jsonl, encoding="utf-8")
    fails, warns = check(tracker, args.project)
    count = len(tracker.of("bench"))
    print(
        f"bench: {'FAIL' if fails else 'PASS'} {board}: {count} results, log .hil/bench-{board}.log"
    )
    for f in fails + [f"note: {w}" for w in warns]:
        print(f"  - {f}")
    if fails:
        return 1

    doc = result_doc(tracker, args.project, datetime.date.today())
    write_result(root, doc)
    docs = load_results(root)
    (root / "RESULTS.md").write_text(render(args.project, docs), encoding="utf-8")
    print("\n".join(summary(doc)))
    print(f"bench: wrote results/{board}.json and RESULTS.md ({len(docs)} board(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
