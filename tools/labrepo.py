"""Shared helpers for tools/: repo paths, board and project discovery."""

from __future__ import annotations

import configparser
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BOARDS = ROOT / "boards"
PROJECTS = ROOT / "projects"
TEMPLATE = ROOT / "templates" / "project"
PROJECT_RE = re.compile(r"^p\d{3}-[a-z0-9]+(?:-[a-z0-9]+)*$")
BOARD_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")


def boards() -> list[str]:
    """Physical board names (folders under boards/, excluding _template)."""
    return sorted(p.name for p in BOARDS.iterdir() if p.is_dir() and not p.name.startswith("_"))


def board_section(board: str) -> str:
    return "boards_" + board.replace("-", "_")


def board_flag(board: str) -> str:
    return "LAB_BOARD_" + board.replace("-", "_").upper()


def projects() -> list[str]:
    if not PROJECTS.is_dir():
        return []
    return sorted(p.name for p in PROJECTS.iterdir() if (p / "platformio.ini").is_file())


def read_ini(path: Path) -> configparser.ConfigParser:
    cp = configparser.ConfigParser(interpolation=None, inline_comment_prefixes=(";",))
    cp.read(path)
    return cp


def split_list(value: str) -> list[str]:
    return [v.strip() for v in re.split(r"[,\n]", value) if v.strip()]


def project_boards(project: str) -> list[str]:
    """Boards a project supports: its [platformio] default_envs."""
    cp = read_ini(PROJECTS / project / "platformio.ini")
    return split_list(cp.get("platformio", "default_envs", fallback=""))


def header_define(path: Path, name: str) -> str | None:
    """Value of `#define NAME value` in a header, quotes stripped; None if absent."""
    if not path.is_file():
        return None
    m = re.search(rf"^\s*#\s*define\s+{name}\s+(.+?)\s*(//.*)?$", path.read_text(), re.M)
    return m.group(1).strip('"') if m else None


def board_chip(board: str) -> str:
    """The board's BOARD_CHIP (esptool's --chip name)."""
    chip = header_define(BOARDS / board / "board.h", "BOARD_CHIP")
    if not chip:
        sys.exit(f"boards/{board}/board.h has no BOARD_CHIP")
    return chip
