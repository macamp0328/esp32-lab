import sys
from pathlib import Path

import flash
import pytest


def test_board_chip_comes_from_board_h():
    assert flash.board_chip("s3mini") == "esp32s3"
    assert flash.board_chip("m5stickc-plus11") == "esp32"


def test_choose_board():
    both = ["s3mini", "m5stickc-plus11"]
    assert flash.choose_board(["s3mini"], None, None) == "s3mini"
    assert flash.choose_board(both, "m5stickc-plus11", None) == "m5stickc-plus11"
    assert flash.choose_board(both, None, "esp32s3") == "s3mini"
    assert flash.choose_board(both, None, "esp32") == "m5stickc-plus11"
    with pytest.raises(SystemExit):
        flash.choose_board(both, None, "esp32c6")
    with pytest.raises(SystemExit):
        flash.choose_board(["s3mini"], "uno", None)


def test_header_define(tmp_path):
    h = tmp_path / "config.h"
    h.write_text('#pragma once\n#define MDNS_NAME         "p000-blink"  // host\n#define N 8\n')
    assert flash.header_define(h, "MDNS_NAME") == "p000-blink"
    assert flash.header_define(h, "N") == "8"
    assert flash.header_define(h, "MISSING") is None
    assert flash.header_define(tmp_path / "none.h", "N") is None


def test_commands():
    usb = flash.usb_command("esp32s3", "/dev/cu.x", Path("f.bin"))
    assert usb[:3] == [sys.executable, "-m", "esptool"]
    assert usb[3:] == ["--chip", "esp32s3", "--port", "/dev/cu.x", "--baud", "921600",
                       "write-flash", "0x0", "f.bin"]  # fmt: skip
    ota = flash.ota_command("p000-blink.local", "pw", Path("a.bin"))
    assert ota[2:] == ["-i", "p000-blink.local", "-p", "3232", "-f", "a.bin", "-r", "-a", "pw"]
    assert "-a" not in flash.ota_command("h", None, Path("a.bin"))
