"""Capture serial from a board (host side): `serial_watch.py <project> [--seconds 30]`.

Resets the board, records every line for N seconds (reopening the port if USB re-enumerates),
writes projects/<p>/.hil/last-run.log (raw, host-timestamped) and last-run.jsonl (JSON lines),
then runs hil_assert on the capture. Exit code is hil_assert's.
"""

from __future__ import annotations

import argparse
import sys
import time

import hil_assert
import serial
from labrepo import PROJECTS
from ports import pick_port


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
    return lines


def reopen(port: str, baud: int, deadline: float) -> serial.Serial | None:
    while time.monotonic() < deadline:
        try:
            return open_port(port, baud)
        except (serial.SerialException, OSError):
            time.sleep(0.1)
    return None


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just watch")
    ap.add_argument("project")
    ap.add_argument("--seconds", type=float, default=30)
    ap.add_argument("--port")
    ap.add_argument("--no-reset", action="store_true")
    args = ap.parse_args(argv)

    port = args.port or pick_port()
    out = PROJECTS / args.project / ".hil"
    out.mkdir(exist_ok=True)
    print(f"watch: {port} for {args.seconds:.0f} s → {out}/last-run.jsonl", flush=True)
    lines = capture(port, args.seconds, do_reset=not args.no_reset)
    (out / "last-run.log").write_text("".join(f"{line}\n" for line in lines))
    payloads = [line[9:] for line in lines]
    (out / "last-run.jsonl").write_text(
        "".join(f"{p}\n" for p in payloads if p.lstrip().startswith("{"))
    )
    return hil_assert.main([str(out / "last-run.jsonl"), "--project", args.project])


if __name__ == "__main__":
    sys.exit(main())
