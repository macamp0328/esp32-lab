"""Capture serial from a board (host side): `serial_watch.py <project> [--seconds 30] [--all]`.

Resets the board, records every line for N seconds (reopening the port if USB re-enumerates),
writes projects/<p>/.hil/last-run.log (raw, host-timestamped) and last-run.jsonl (JSON lines),
then runs hil_assert on the capture. Exit code is hil_assert's.

--all captures every plugged-in board of the project at once, one thread per port, into
last-run-<board>-<tag>.{log,jsonl} (tag: end of the USB serial). When any capture has `link`
events, each must name every other board as a peer. Exit code is 1 if any board fails.
"""

from __future__ import annotations

import argparse
import re
import sys
import threading
import time
from pathlib import Path

import hil_assert
import serial
from labrepo import PROJECTS
from ports import all_ports_for, node_tag, pick_port_for

ROM_RESET = re.compile(r"rst:0x[0-9a-f]+ \(")  # the boot ROM's first line after a reset
BANNER_WINDOW_S = 3.0  # it comes within this of our reset


def reset(ser: serial.Serial) -> None:
    """Hard reset via RTS (EN), keeping DTR (IO0) released so the app boots, as esptool does."""
    ser.dtr = False
    ser.rts = True
    time.sleep(0.2)
    ser.rts = False
    time.sleep(0.05)


def open_port(port: str, baud: int) -> serial.Serial:
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, baud, 0.2
    ser.dtr = ser.rts = False  # set before open: opening must not hold the chip in reset
    ser.open()
    return ser


def capture(port: str, seconds: float, baud: int = 115200, do_reset: bool = True) -> list[str]:
    """Lines read for `seconds`, each prefixed with seconds since the capture started.

    With a reset, lines before the boot ROM's reset banner (`rst:0x..`) are dropped: they were
    queued before it. A native-USB board (the S3) nobody reads keeps output for the next host,
    and the reset cuts it mid-line.
    """
    lines: list[str] = []
    buf = b""
    deadline = time.monotonic() + seconds
    ser = open_port(port, baud)
    if do_reset:
        reset(ser)
    start = time.monotonic()
    while time.monotonic() < deadline:
        try:
            chunk = ser.read(512)
        except (serial.SerialException, OSError):
            ser.close()
            ser = reopen(port, baud, deadline)
            if ser is None:
                break
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            stamp = time.monotonic() - start
            lines.append(f"{stamp:8.3f} {raw.decode(errors='replace').rstrip()}")
    if ser is not None:
        ser.close()
    if do_reset:
        banner = next(
            (k for k, line in enumerate(lines) if float(line[:8]) < BANNER_WINDOW_S
             and ROM_RESET.match(line[9:])),
            None,
        )  # fmt: skip
        lines = lines[banner:] if banner is not None else lines
    return lines


def reopen(port: str, baud: int, deadline: float) -> serial.Serial | None:
    while time.monotonic() < deadline:
        try:
            return open_port(port, baud)
        except (serial.SerialException, OSError):
            time.sleep(0.1)
    return None


def save(out: Path, stem: str, lines: list[str]) -> Path:
    """Writes <stem>.log (raw, host-timestamped) and <stem>.jsonl; returns the .jsonl path."""
    (out / f"{stem}.log").write_text("".join(f"{line}\n" for line in lines))
    payloads = [line[9:] for line in lines]
    jsonl = out / f"{stem}.jsonl"
    jsonl.write_text("".join(f"{p}\n" for p in payloads if p.lstrip().startswith("{")))
    return jsonl


def watch_all(project: str, seconds: float, do_reset: bool, out: Path) -> int:
    nodes = all_ports_for(project)
    names = {port: f"last-run-{board}-{node_tag(port)}" for board, port in nodes}
    print(f"watch: {len(nodes)} boards for {seconds:.0f} s → {out}/", flush=True)
    for _, port in nodes:
        print(f"  {names[port]}  {port}", flush=True)
    captured: dict[str, list[str]] = {}

    def run(port: str) -> None:
        captured[port] = capture(port, seconds, do_reset=do_reset)

    threads = [threading.Thread(target=run, args=(port,)) for _, port in nodes]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    files = {port: save(out, names[port], captured.get(port, [])) for _, port in nodes}
    mesh = len(nodes) > 1 and any('"ev":"link"' in f.read_text() for f in files.values())
    failed = 0
    for _, port in nodes:
        print(f"{names[port]}: ", end="", flush=True)
        argv = [str(files[port]), "--project", project, "--seconds", str(seconds)]
        failed += hil_assert.main(argv + (["--peers", str(len(nodes) - 1)] if mesh else [])) != 0
    print(f"watch: {len(nodes) - failed} of {len(nodes)} boards pass", flush=True)
    return 1 if failed else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just watch")
    ap.add_argument("project")
    ap.add_argument("board", nargs="?", help="which board, when several are plugged in")
    ap.add_argument("--seconds", type=float, default=30)
    ap.add_argument("--port")
    ap.add_argument("--no-reset", action="store_true")
    ap.add_argument("--all", action="store_true", help="every plugged-in board, at once")
    args = ap.parse_args(argv)

    out = PROJECTS / args.project / ".hil"
    out.mkdir(exist_ok=True)
    if args.all:
        if args.board or args.port:
            ap.error("--all watches every plugged-in board: drop board and --port")
        return watch_all(args.project, args.seconds, not args.no_reset, out)
    port = args.port or pick_port_for(args.project, args.board)
    print(f"watch: {port} for {args.seconds:.0f} s → {out}/last-run.jsonl", flush=True)
    lines = capture(port, args.seconds, do_reset=not args.no_reset)
    capture_file = str(save(out, "last-run", lines))
    return hil_assert.main(
        [capture_file, "--project", args.project, "--seconds", str(args.seconds)]
    )


if __name__ == "__main__":
    sys.exit(main())
