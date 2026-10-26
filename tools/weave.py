"""Print a loom: `weave.py <capture.jsonl> [-o print.png] [--px 4]`.

p005-trio's M5 weaves one thread of the piece per minute and logs each as a `weave` event: its
column sampled down to N points, RGB565 as 4 hex digits each (`c`). This lays the threads side by
side in the order they were woven (`i`) and writes a PNG, each thread --px wide and each sample
--px tall. Standard library only.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zlib
from pathlib import Path


def threads(lines: list[str]) -> list[tuple[int, list[tuple[int, int, int]]]]:
    """(index, column of RGB) for every well-formed `weave` event, ordered by index, one per
    index (a capture of the same loom twice keeps the last)."""
    found: dict[int, list[tuple[int, int, int]]] = {}
    for line in lines:
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            e = json.loads(line)
        except json.JSONDecodeError:
            continue
        hexes = e.get("c") if isinstance(e, dict) and e.get("ev") == "weave" else None
        if not isinstance(hexes, str) or not hexes or len(hexes) % 4 or "i" not in e:
            continue
        try:
            found[int(e["i"])] = [
                rgb565(int(hexes[k : k + 4], 16)) for k in range(0, len(hexes), 4)
            ]
        except ValueError:
            continue
    return sorted(found.items())


def rgb565(v: int) -> tuple[int, int, int]:
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def png(width: int, height: int, rows: list[bytes]) -> bytes:
    def chunk(kind: bytes, data: bytes) -> bytes:
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    raw = b"".join(b"\x00" + row for row in rows)  # filter type 0 on every row
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # 8-bit RGB
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )


def render(woven: list[tuple[int, list[tuple[int, int, int]]]], px: int) -> bytes:
    """The threads side by side; a thread shorter than the tallest is padded with black."""
    tall = max(len(col) for _, col in woven)
    rows = []
    for y in range(tall):
        row = bytearray()
        for _, col in woven:
            r, g, b = col[y] if y < len(col) else (0, 0, 0)
            row += bytes((r, g, b)) * px
        rows += [bytes(row)] * px
    return png(len(woven) * px, tall * px, rows)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="weave.py")
    ap.add_argument("capture", type=Path, help="a .hil/*.jsonl with `weave` events")
    ap.add_argument("-o", "--out", type=Path, help="default: weave-<capture>.png beside it")
    ap.add_argument("--px", type=int, default=4, help="pixels per thread and per sample")
    args = ap.parse_args(argv)
    woven = threads(args.capture.read_text(errors="replace").splitlines())
    if not woven:
        print(f"weave: no weave events in {args.capture}", file=sys.stderr)
        return 1
    out = args.out or args.capture.with_name(f"weave-{args.capture.stem}.png")
    out.write_bytes(render(woven, max(1, args.px)))
    first, last = woven[0][0], woven[-1][0]
    print(f"weave: {len(woven)} threads ({first}..{last}) → {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
