"""Find the board's USB serial port (host side). Set LAB_PORT to override."""

from __future__ import annotations

import os
import sys

from serial.tools import list_ports

# USB-serial bridges seen on ESP32 boards: Espressif native USB, WCH CH9102/CH340, SiLabs, FTDI.
KNOWN_VIDS = {0x303A, 0x1A86, 0x10C4, 0x0403}


def candidates() -> list[str]:
    return sorted(p.device for p in list_ports.comports() if p.vid in KNOWN_VIDS)


def pick_port() -> str:
    if os.environ.get("LAB_PORT"):
        return os.environ["LAB_PORT"]
    found = candidates()
    if len(found) == 1:
        return found[0]
    if not found:
        sys.exit("no ESP32 USB serial port found: is the board plugged in (data cable)?")
    sys.exit(f"several ports {found}: pass --port or set LAB_PORT")
