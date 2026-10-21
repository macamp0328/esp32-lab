"""Validate and print inventory/stock.yaml: `just inventory`; `--check` validates only."""

from __future__ import annotations

import argparse
import datetime as dt
import re
import sys
from pathlib import Path

import yaml
from labrepo import PROJECT_RE, ROOT, boards

STOCK = ROOT / "inventory" / "stock.yaml"
CATEGORIES = {"boards", "leds", "mics", "power", "parts", "tools"}
REQUIRED = {"id", "model", "qty", "source"}
OPTIONAL = {"revision", "specs", "date", "projects", "notes", "board", "type"}
TEXT_KEYS = ("model", "source", "revision", "specs", "notes", "type")
ID_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*(?:\.\.\d+)?$")


def validate(data: object) -> list[str]:
    """Returns human-readable problems; empty means valid."""
    if not isinstance(data, dict):
        return ["top level must be a mapping of category -> list of items"]
    errors: list[str] = []
    seen: set[str] = set()
    for category, items in data.items():
        if category not in CATEGORIES:
            errors.append(
                f"unknown category '{category}' (allowed: {', '.join(sorted(CATEGORIES))})"
            )
            continue
        if not isinstance(items, list):
            errors.append(f"{category}: must be a list")
            continue
        for i, item in enumerate(items):
            where = f"{category}[{i}]"
            if not isinstance(item, dict):
                errors.append(f"{where}: must be a mapping")
                continue
            where = f"{category}/{item.get('id', i)}"
            errors += [f"{where}: missing '{k}'" for k in sorted(REQUIRED - item.keys())]
            errors += [
                f"{where}: unknown key '{k}'" for k in sorted(item.keys() - REQUIRED - OPTIONAL)
            ]
            if "id" in item:
                if not isinstance(item["id"], str) or not ID_RE.match(item["id"]):
                    errors.append(f"{where}: id must be kebab-case, optional '..N' range suffix")
                elif item["id"] in seen:
                    errors.append(f"{where}: duplicate id")
                else:
                    seen.add(item["id"])
            errors += [
                f"{where}: {k} must be text (quote it)"
                for k in TEXT_KEYS
                if k in item and not isinstance(item[k], str)
            ]
            qty = item.get("qty")
            if "qty" in item and (not isinstance(qty, int) or isinstance(qty, bool) or qty < 0):
                errors.append(f"{where}: qty must be an integer >= 0")
            if "date" in item and not isinstance(item["date"], dt.date):
                errors.append(f"{where}: date must be YYYY-MM-DD")
            if "board" in item and item["board"] not in boards():
                errors.append(f"{where}: board '{item['board']}' has no boards/ folder")
            projects = item.get("projects", [])
            if not isinstance(projects, list) or not all(
                isinstance(p, str) and PROJECT_RE.match(p) for p in projects
            ):
                errors.append(f"{where}: projects must be a list of pNNN-name")
    return errors


def boards_on_hand(data: dict) -> dict[str, int]:
    """Board name -> quantity on hand, for entries that name a `board:`."""
    found: dict[str, int] = {}
    for item in data.get("boards", []):
        if item.get("board") and item.get("qty", 0) > 0:
            found[item["board"]] = found.get(item["board"], 0) + item["qty"]
    return found


def load(path: Path = STOCK) -> object:
    return yaml.safe_load(path.read_text())


def render(data: dict) -> str:
    rows = [("category", "id", "qty", "model", "projects")]
    for category, items in data.items():
        for item in items:
            projects = ", ".join(item.get("projects", [])) or "-"
            rows.append((category, item["id"], str(item["qty"]), item["model"], projects))
    widths = [max(len(r[c]) for r in rows) for c in range(len(rows[0]))]
    return "\n".join("  ".join(v.ljust(w) for v, w in zip(r, widths, strict=True)) for r in rows)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just inventory")
    ap.add_argument("--check", action="store_true", help="validate only")
    args = ap.parse_args(argv)
    data = load()
    errors = validate(data)
    for e in errors:
        print(f"inventory/stock.yaml: {e}", file=sys.stderr)
    if errors:
        return 1
    if not args.check:
        print(render(data))
    return 0


if __name__ == "__main__":
    sys.exit(main())
