"""Project status table for the root README, pulled from each project README's status line.

`status.py` prints it, `--write` updates README.md between the markers, `--check` fails if stale.
"""

from __future__ import annotations

import argparse
import re
import sys

from labrepo import PROJECTS, ROOT, project_boards, projects

README = ROOT / "README.md"
BEGIN, END = "<!-- status:begin -->", "<!-- status:end -->"
STATUS_RE = re.compile(r"^\*\*Status:\*\*\s*(.+?)\s*·\s*\*\*Last HIL check:\*\*\s*(.+?)\s*$", re.M)


def status_line(project: str) -> tuple[str, str]:
    text = (PROJECTS / project / "README.md").read_text()
    m = STATUS_RE.search(text)
    return (m.group(1), m.group(2)) if m else ("(no status line)", "-")


def table() -> str:
    rows = ["| Project | Boards | Status | Last HIL check |", "| --- | --- | --- | --- |"]
    for p in projects():
        status, hil = status_line(p)
        boards = ", ".join(project_boards(p))
        rows.append(f"| [{p}](projects/{p}/README.md) | {boards} | {status} | {hil} |")
    return "\n".join(rows)


def render(readme: str) -> str:
    head, rest = readme.split(BEGIN, 1)
    _, tail = rest.split(END, 1)
    return f"{head}{BEGIN}\n{table()}\n{END}{tail}"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just status")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv)
    current = README.read_text()
    wanted = render(current)
    if args.check:
        if current != wanted:
            print("README.md status table is stale: run `just status`", file=sys.stderr)
            return 1
        return 0
    if args.write:
        README.write_text(wanted)
    print(table())
    return 0


if __name__ == "__main__":
    sys.exit(main())
