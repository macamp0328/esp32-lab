"""Scaffold a board: `just add-board <name> --chip esp32s3 --pio-board esp32-s3-devkitc-1`.

Copies boards/_template to boards/<name>, adds its branch to lab_board.h and its env to
platformio.common.ini. The new board.h is UNVERIFIED: fill it from the datasheet, then bring it
up with p000 (add <name> to projects/p000-blink default_envs).
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

from labrepo import BOARD_RE, ROOT, board_flag, board_section

CHIPS = ("esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c5", "esp32c6", "esp32h2", "esp32p4")
DISPATCH_END = '#else\n#error "No LAB_BOARD_* flag'
NATIVE_ENV = "[env:native]"


def env_block(name: str) -> str:
    sec = board_section(name)
    return (
        f"[env:{name}]\n"
        f"extends = common, {sec}\n"
        f"build_flags = ${{common.build_flags}} ${{{sec}.build_flags}}\n"
        f"lib_deps =\n  ${{common.lib_deps}}\n  ${{{sec}.lib_deps}}\n\n"
    )


def add_board(root: Path, name: str, chip: str, pio_board: str) -> list[str]:
    """Creates the board; returns the files touched. Raises ValueError on bad input."""
    boards = root / "boards"
    if not BOARD_RE.match(name):
        raise ValueError(f"'{name}' must be lowercase kebab-case, e.g. xiao-s3")
    if (boards / name).exists():
        raise ValueError(f"boards/{name} already exists")
    if name == "native" or f"[env:{name}]" in (root / "platformio.common.ini").read_text():
        raise ValueError(f"'{name}' clashes with an existing [env:{name}]")
    if chip not in CHIPS:
        raise ValueError(f"chip '{chip}' not one of {', '.join(CHIPS)}")

    dispatcher = root / "shared" / "lab_board" / "src" / "lab_board.h"
    common = root / "platformio.common.ini"
    d_text, c_text = dispatcher.read_text(), common.read_text()
    if DISPATCH_END not in d_text or NATIVE_ENV not in c_text:
        raise ValueError("lab_board.h or platformio.common.ini no longer has the expected anchors")

    shutil.copytree(boards / "_template", boards / name, ignore=shutil.ignore_patterns(".DS_Store"))
    subs = {
        "{{BOARD}}": name,
        "{{SECTION}}": board_section(name).removeprefix("boards_"),
        "{{FLAG}}": board_flag(name),
        "{{PIO_BOARD}}": pio_board,
        "{{CHIP}}": chip,
    }
    for path in (boards / name).iterdir():
        text = path.read_text()
        for k, v in subs.items():
            text = text.replace(k, v)
        path.write_text(text)

    branch = f'#elif defined({board_flag(name)})\n#include "{name}/board.h"\n'
    dispatcher.write_text(d_text.replace(DISPATCH_END, branch + DISPATCH_END, 1))
    common.write_text(c_text.replace(NATIVE_ENV, env_block(name) + NATIVE_ENV, 1))
    return [f"boards/{name}/", str(dispatcher.relative_to(root)), str(common.relative_to(root))]


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just add-board")
    ap.add_argument("name", help="kebab-case, e.g. xiao-s3")
    ap.add_argument("--chip", required=True, choices=CHIPS, help="esptool --chip value")
    ap.add_argument("--pio-board", required=True, help="PlatformIO board id, e.g. esp32dev")
    args = ap.parse_args(argv)
    try:
        touched = add_board(ROOT, args.name, args.chip, args.pio_board)
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    print(f"added board {args.name}: {', '.join(touched)}")
    print(
        f"next: fill boards/{args.name}/board.h + pinout.md from the datasheet, then bring it up:"
    )
    print(f"  add {args.name} to projects/p000-blink default_envs; just build p000 {args.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
