"""Scaffold projects/<name> from templates/project: `just new p003-lamp --board s3mini`."""

from __future__ import annotations

import argparse
import shutil
import sys

import status
from labrepo import PROJECT_RE, PROJECTS, TEMPLATE, boards, projects, split_list

import inventory


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just new")
    ap.add_argument("name", help="pNNN-short-name, e.g. p003-desk-lamp")
    ap.add_argument("--board", help=f"comma-separated, from: {', '.join(boards())}")
    args = ap.parse_args(argv)

    if not PROJECT_RE.match(args.name):
        return fail(f"'{args.name}' is not pNNN-short-name (lowercase, digits, dashes)")
    number = args.name[:4]
    taken = [p for p in projects() if p[:4] == number]
    if taken:
        return fail(f"{number} is already used by {taken[0]}; numbers are never reused")
    if not args.board:
        on_hand = inventory.boards_on_hand(inventory.load())
        listing = ", ".join(f"{b} ({n} on hand)" for b, n in on_hand.items()) or "none in stock"
        return fail(f"pass --board <name>; inventory/stock.yaml has: {listing}")
    chosen = split_list(args.board)
    unknown = [b for b in chosen if b not in boards()]
    if unknown or not chosen:
        return fail(f"unknown board(s) {unknown}; have: {', '.join(boards())}")

    dest = PROJECTS / args.name
    shutil.copytree(TEMPLATE, dest, ignore=shutil.ignore_patterns(".DS_Store", ".pio"))
    for path in dest.rglob("*"):
        if path.is_file():
            text = path.read_text()
            text = text.replace("{{PROJECT}}", args.name).replace("{{BOARDS}}", ", ".join(chosen))
            path.write_text(text)
    if status.README.is_file():
        status.README.write_text(status.render(status.README.read_text()))
    print(f"created projects/{args.name} for {', '.join(chosen)}; README status table updated")
    print(
        f"next: just build {args.name} {chosen[0]} && just flash {args.name} {chosen[0]} "
        f"&& just watch {args.name}"
    )
    return 0


def fail(msg: str) -> int:
    print(f"error: {msg}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
