"""Find a board's USB serial port (host side). Set LAB_PORT to override.

With several boards plugged in, a board's chip tells its port apart: chips with native USB
(ESP32-S3 and newer) enumerate with Espressif's own vendor id, while the classic ESP32 always sits
behind a USB-UART bridge (FTDI, WCH, SiLabs). Two boards of the same kind still need --port.
"""

from __future__ import annotations

import os
import sys

from labrepo import board_chip, project_boards
from serial.tools import list_ports

ESPRESSIF_VID = 0x303A  # native USB-Serial/JTAG
BRIDGE_VIDS = {0x1A86, 0x10C4, 0x0403}  # WCH CH9102/CH340, SiLabs CP210x, FTDI
KNOWN_VIDS = {ESPRESSIF_VID} | BRIDGE_VIDS
NATIVE_USB_CHIPS = {"esp32s2", "esp32s3", "esp32c3", "esp32c5", "esp32c6", "esp32h2", "esp32p4"}


def ports() -> list[tuple[str, int]]:
    """(device, USB vendor id) for every port that looks like an ESP32 board."""
    return sorted((p.device, p.vid) for p in list_ports.comports() if p.vid in KNOWN_VIDS)


def candidates() -> list[str]:
    return [device for device, _ in ports()]


def fitting(chip: str, found: list[tuple[str, int]]) -> list[str]:
    """Ports that can be a board with `chip`. Native-USB chips prefer Espressif's own port and
    fall back to a bridge (some devkits wire the UART instead); the classic ESP32 is always
    bridged."""
    native = [d for d, vid in found if vid == ESPRESSIF_VID]
    bridged = [d for d, vid in found if vid in BRIDGE_VIDS]
    if chip in NATIVE_USB_CHIPS and native:
        return native
    return bridged


def pick_port(chips: list[str] | None = None) -> str:
    """LAB_PORT; else the only ESP32 port; else the only one that fits `chips`, the chips of the
    boards the caller could mean."""
    if os.environ.get("LAB_PORT"):
        return os.environ["LAB_PORT"]
    found = ports()
    if len(found) == 1:
        return found[0][0]
    if not found:
        sys.exit("no ESP32 USB serial port found: is the board plugged in (data cable)?")
    devices = [d for d, _ in found]
    if chips:
        fit = sorted({d for chip in chips for d in fitting(chip, found)})
        if len(fit) == 1:
            return fit[0]
        if not fit:
            sys.exit(f"none of {devices} looks like a {'/'.join(chips)} board: pass --port")
    sys.exit(
        f"several ports {devices}: name the board (`just watch p003 s3mini`), "
        "or pass --port / set LAB_PORT"
    )


def pick_port_for(project: str, board: str | None = None) -> str:
    """The port of `board`, or of whichever board `project` supports is plugged in."""
    supported = project_boards(project)
    if board and board not in supported:
        sys.exit(f"board '{board}' not in {project}'s default_envs {supported}")
    return pick_port([board_chip(b) for b in ([board] if board else supported)])
