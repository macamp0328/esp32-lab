"""Flash a built project (host side): `just flash <p> [board] [--port P] [--ota [HOST]]`.

USB: writes .pio/build/<board>/firmware.factory.bin at 0x0 with esptool; `--chip` comes from
the board's BOARD_CHIP, so flashing the wrong board fails instead of bricking the run.
OTA: espota to <MDNS_NAME>.local with OTA_PASSWORD from include/secrets.h; falls back to USB.
Builds nothing: run `just build <p> <board>` first.
"""

from __future__ import annotations

import argparse
import re
import socket
import subprocess
import sys
import time
from pathlib import Path

from labrepo import BOARDS, PROJECTS, ROOT, project_boards
from ports import candidates, pick_port

ESPOTA = ROOT / ".cache" / "espota.py"
ESPOTA_IN_IMAGE = "/pio/packages/framework-arduinoespressif32/tools/espota.py"


def header_define(path: Path, name: str) -> str | None:
    """Value of `#define NAME value` in a header, quotes stripped; None if absent."""
    if not path.is_file():
        return None
    m = re.search(rf"^\s*#\s*define\s+{name}\s+(.+?)\s*(//.*)?$", path.read_text(), re.M)
    return m.group(1).strip('"') if m else None


def board_chip(board: str) -> str:
    chip = header_define(BOARDS / board / "board.h", "BOARD_CHIP")
    if not chip:
        sys.exit(f"boards/{board}/board.h has no BOARD_CHIP")
    return chip


def choose_board(supported: list[str], given: str | None, detected_chip: str | None) -> str:
    if given:
        if given not in supported:
            sys.exit(f"board '{given}' not in this project's default_envs {supported}")
        return given
    if len(supported) == 1:
        return supported[0]
    matches = [b for b in supported if detected_chip and board_chip(b) == detected_chip]
    if len(matches) == 1:
        return matches[0]
    sys.exit(f"cannot tell which board is attached ({detected_chip}); pass one of {supported}")


def detect_chip(port: str) -> str:
    from esptool.cmds import detect_chip as esptool_detect

    with esptool_detect(port) as esp:
        return esp.CHIP_NAME.lower().replace("-", "")


def usb_command(chip: str, port: str, image: Path) -> list[str]:
    return [sys.executable, "-m", "esptool", "--chip", chip, "--port", port, "--baud", "921600",
            "write-flash", "0x0", str(image)]  # fmt: skip


def ota_command(host: str, password: str | None, app: Path) -> list[str]:
    cmd = [sys.executable, str(ESPOTA), "-i", host, "-p", "3232", "-f", str(app), "-r"]
    return cmd + (["-a", password] if password else [])


def flash_usb(project: str, board: str, port: str) -> int:
    image = PROJECTS / project / ".pio" / "build" / board / "firmware.factory.bin"
    if not image.is_file():
        sys.exit(f"{image.relative_to(ROOT)} missing: run `just build {project} {board}`")
    print(
        f"flash: {project} → {board} on {port} via USB ({image.stat().st_size} bytes)", flush=True
    )
    return subprocess.call(usb_command(board_chip(board), port, image))


def flash_ota(project: str, board: str, host: str | None) -> int:
    app = PROJECTS / project / ".pio" / "build" / board / "firmware.bin"
    if not app.is_file():
        sys.exit(f"{app.relative_to(ROOT)} missing: run `just build {project} {board}`")
    if not ESPOTA.is_file():
        ESPOTA.parent.mkdir(exist_ok=True)
        subprocess.check_call(["just", "_c", "cp", ESPOTA_IN_IMAGE, ".cache/espota.py"], cwd=ROOT)
    include = PROJECTS / project / "include"
    host = host or f"{header_define(include / 'config.h', 'MDNS_NAME') or project}.local"
    ip = resolve(host)
    if ip is None:
        print(f"flash: cannot resolve {host} (board off WiFi, or not on this network?)")
        return 1
    password = header_define(include / "secrets.h", "OTA_PASSWORD")
    for attempt in (1, 2):
        print(f"flash: {project} → {board} at {host} ({ip}) via OTA, try {attempt}", flush=True)
        if subprocess.call(ota_command(ip, password, app)) == 0:
            return 0
        time.sleep(2)
    return 1


def resolve(host: str, timeout_s: float = 10) -> str | None:
    """IPv4 for host; mDNS (.local) lookups can miss on a cold cache, so retry for a while."""
    deadline = time.monotonic() + timeout_s
    while True:
        try:
            return socket.getaddrinfo(host, 3232, socket.AF_INET)[0][4][0]
        except OSError:
            if time.monotonic() > deadline:
                return None
            time.sleep(0.5)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="just flash")
    ap.add_argument("project")
    ap.add_argument("board", nargs="?")
    ap.add_argument("--port")
    ap.add_argument("--ota", nargs="?", const="", metavar="HOST", help="default <MDNS_NAME>.local")
    args = ap.parse_args(argv)
    supported = project_boards(args.project)

    if args.ota is not None:
        board = choose_board(supported, args.board, None)
        if flash_ota(args.project, board, args.ota or None) == 0:
            return 0
        if not (args.port or candidates()):
            return 1
        print("flash: OTA failed, falling back to USB", flush=True)
        port = args.port or pick_port()
    else:
        port = args.port or pick_port()
        need_detect = not args.board and len(supported) > 1
        board = choose_board(supported, args.board, detect_chip(port) if need_detect else None)
    return flash_usb(args.project, board, port)


if __name__ == "__main__":
    sys.exit(main())
