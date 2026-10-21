"""Repo rules that clang-format, cppcheck and ruff cannot see. Run by `just check`.

Boards are complete and wired in, projects are well-formed, project code takes pins from
board.h and logs through lab_core, nothing secret is committed, inventory and README are valid.
A line containing `lab:allow` is exempt from the pin, serial and secret rules.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

import status
from labrepo import (
    BOARDS,
    PROJECT_RE,
    PROJECTS,
    ROOT,
    board_flag,
    board_section,
    boards,
    project_boards,
    projects,
    read_ini,
)

import inventory

BOARD_MACROS = [
    "BOARD_NAME",
    "BOARD_CHIP",
    "BOARD_FLASH_MB",
    "BOARD_PSRAM_MB",
    "BOARD_LED_PIN",
    "BOARD_HAS_PDM_MIC",
    "BOARD_HAS_TFT",
    "BOARD_SAFE_PINS",
    "BOARD_VERIFIED",
]
CONFIG_MACROS = ["LAB_PROJECT", "NUM_LEDS", "LED_DATA_PIN", "POWER_VOLTS", "POWER_MAX_MA"]
PIN_LITERAL = [
    re.compile(
        r"\b(pinMode|digitalWrite|digitalRead|analogRead|analogWrite|ledcAttach|touchRead)"
        r"\s*\(\s*\d+"
    ),
    re.compile(r"\bGPIO_NUM_\d+"),
    re.compile(r"\baddLeds\s*<[^>]*,\s*\d+\s*[,>]"),
    re.compile(r"#\s*define\s+\w*(PIN|GPIO)\w*\s+\(?\d+\)?(\s|$)"),
    re.compile(r"\b(Wire\d?|SPI)\s*\.\s*begin\s*\(\s*\d"),  # Wire.begin(21, 22), SPI.begin(18, ...)
    re.compile(
        r"\bSerial\d\s*\.\s*begin\s*\([^,)]*,[^,)]*,\s*\d"
    ),  # Serial1.begin(baud, cfg, 16, 17)
    re.compile(r"_io_num\s*=\s*\d"),  # i2s/rmt config structs: .bck_io_num = 26
    re.compile(r"\b(const|constexpr)\b[^=;]*\b\w*(Pin|PIN|_pin)\s*=\s*\d"),  # kLedPin = 5
]
STRAY_SERIAL = re.compile(
    r"\b(\w*Serial\d?\s*\.\s*(print|println|printf|write)|printf|ets_printf|puts)\s*\("
)
# A credential-looking identifier (name ends in the key word) assigned a literal.
SECRET_KEY = r"\w*(ssid|pass|passwd|password|pwd|psk|api[_-]?key|token|secret)"
SECRET = re.compile(
    rf"(?i)\b{SECRET_KEY}\b[\"']?\s*(?:=|:|\s)\s*(?:\"(?P<dq>[^\"]*)\"|'(?P<sq>[^']*)')"
)
SECRET_YAML = re.compile(rf"(?i)^\s*{SECRET_KEY}\s*:\s*(?P<val>[^\s#'\"][^#]*?)\s*$")
# Not credentials: empty, placeholders, and CI expressions like ${{ secrets.GITHUB_TOKEN }}.
PLACEHOLDER = re.compile(r"(?i)^(|changeme|your[-_ ].*|<.*>|x+|\*+|\$\{\{.*\}\})$")
CODE_SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".ino"}
TEXT_SUFFIXES = CODE_SUFFIXES | {".ini", ".py", ".yaml", ".yml", ".json", ".toml", ".sh", ""}


def repo_files() -> list[Path]:
    """Tracked plus untracked-but-not-ignored files (what the next commit could contain)."""
    out = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=ROOT
    )
    return [ROOT / f for f in out.decode().split("\0") if f and (ROOT / f).is_file()]


def strip_comment(line: str) -> str:
    return line.split("//", 1)[0]


def check_boards() -> list[str]:
    errors = []
    dispatcher = (ROOT / "shared/lab_board/src/lab_board.h").read_text()
    common = read_ini(ROOT / "platformio.common.ini")
    for b in boards():
        d = BOARDS / b
        for f in ("board.ini", "board.h", "pinout.md"):
            if not (d / f).is_file():
                errors.append(f"boards/{b}: missing {f}")
        if (d / "board.ini").is_file():
            ini = read_ini(d / "board.ini")
            sec = board_section(b)
            if not ini.has_section(sec):
                errors.append(f"boards/{b}/board.ini: needs section [{sec}]")
            else:
                for key in ("board", "build_flags", "lib_deps"):
                    if not ini.has_option(sec, key):
                        errors.append(f"boards/{b}/board.ini: [{sec}] missing '{key}'")
                if f"-D{board_flag(b)}" not in ini.get(sec, "build_flags", fallback="").split():
                    errors.append(f"boards/{b}/board.ini: build_flags must set -D{board_flag(b)}")
        if (d / "board.h").is_file():
            header = (d / "board.h").read_text()
            for macro in BOARD_MACROS:
                if not re.search(rf"#\s*define\s+{macro}\b", header):
                    errors.append(f"boards/{b}/board.h: missing #define {macro}")
        if f"defined({board_flag(b)})" not in dispatcher:
            errors.append(f"shared/lab_board/src/lab_board.h: no branch for {board_flag(b)}")
        if not common.has_section(f"env:{b}"):
            errors.append(f"platformio.common.ini: missing [env:{b}]")
    return errors


def check_projects() -> list[str]:
    errors = []
    for p in projects():
        where = f"projects/{p}"
        if not PROJECT_RE.match(p):
            errors.append(f"{where}: name must be pNNN-short-name")
        supported = project_boards(p)
        if not supported:
            errors.append(f"{where}/platformio.ini: default_envs must list supported boards")
        errors += [
            f"{where}/platformio.ini: unknown board '{b}' in default_envs"
            for b in supported
            if b not in boards()
        ]
        config = PROJECTS / p / "include" / "config.h"
        text = config.read_text() if config.is_file() else ""
        errors += [
            f"{where}/include/config.h: missing #define {m}"
            for m in CONFIG_MACROS
            if not re.search(rf"#\s*define\s+{m}\b", text)
        ]
        if text and not re.search(rf'#\s*define\s+LAB_PROJECT\s+"{re.escape(p)}"', text):
            errors.append(f'{where}/include/config.h: LAB_PROJECT must be "{p}"')
        if not list((PROJECTS / p / "test").glob("test_*/*.c*")):
            errors.append(f"{where}: needs at least one native test in test/test_*/")
        readme = PROJECTS / p / "README.md"
        if not readme.is_file() or not status.STATUS_RE.search(readme.read_text()):
            errors.append(f"{where}/README.md: needs the **Status:** · **Last HIL check:** line")
    numbers = [p[:4] for p in projects()]
    errors += [
        f"projects: number {n} used twice"
        for n in sorted({n for n in numbers if numbers.count(n) > 1})
    ]
    return errors


def check_code(files: list[Path]) -> list[str]:
    errors = []
    for f in files:
        rel = f.relative_to(ROOT)
        parts = rel.parts
        if parts[0] != "projects" or f.suffix not in CODE_SUFFIXES or len(parts) < 4:
            continue
        area = parts[2]
        if area not in ("src", "include", "lib"):
            continue
        for n, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            if "lab:allow" in line:
                continue
            code = strip_comment(line)
            if any(r.search(code) for r in PIN_LITERAL):
                errors.append(f"{rel}:{n}: literal GPIO number; use a BOARD_* name from board.h")
            if STRAY_SERIAL.search(code):
                errors.append(f"{rel}:{n}: stray Serial/printf; log through lab_core::Event")
    return errors


def check_secrets(files: list[Path]) -> list[str]:
    errors = []
    for f in files:
        rel = f.relative_to(ROOT)
        if f.name == "secrets.h":
            errors.append(f"{rel}: secrets.h must stay untracked (it is gitignored)")
            continue
        if f.suffix not in TEXT_SUFFIXES or f.name == "uv.lock":
            continue
        for n, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            if "lab:allow" in line:
                continue
            values = [m.group("dq") if m.group("dq") is not None else m.group("sq")
                      for m in SECRET.finditer(line)]  # fmt: skip
            if f.suffix in (".yaml", ".yml") and (y := SECRET_YAML.match(line)):
                values.append(y.group("val"))
            for value in values:
                if not PLACEHOLDER.match(value):
                    errors.append(f"{rel}:{n}: looks like a credential; move it to secrets.h")
    return errors


def main() -> int:
    files = repo_files()
    errors = check_boards() + check_projects() + check_code(files) + check_secrets(files)
    errors += [f"inventory/stock.yaml: {e}" for e in inventory.validate(inventory.load())]
    if (
        status.README.is_file()
        and status.render(status.README.read_text()) != status.README.read_text()
    ):
        errors.append("README.md: status table is stale; run `just status`")
    for e in errors:
        print(e, file=sys.stderr)
    print(f"lab_check: {len(errors)} problem(s)" if errors else "lab_check: ok")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
