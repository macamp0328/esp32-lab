import ports
import pytest

M5 = ("/dev/cu.usbserial-914AF70348", 0x0403)  # FTDI bridge
S3 = ("/dev/cu.usbmodem312201", 0x303A)  # native USB
S3_TOO = ("/dev/cu.usbmodem312202", 0x303A)
S3_ON_UART = ("/dev/cu.usbserial-0001", 0x10C4)  # an S3 devkit plugged into its UART port


def plug(monkeypatch, *found):
    monkeypatch.setattr(ports, "ports", lambda: sorted(found))
    monkeypatch.delenv("LAB_PORT", raising=False)


def test_one_board_needs_no_hint(monkeypatch):
    plug(monkeypatch, M5)
    assert ports.pick_port() == M5[0]
    assert ports.pick_port(["esp32s3"]) == M5[0]  # one port: esptool's --chip catches a mismatch


def test_chip_tells_the_m5_from_the_s3(monkeypatch):
    plug(monkeypatch, M5, S3)
    assert ports.pick_port(["esp32"]) == M5[0]
    assert ports.pick_port(["esp32s3"]) == S3[0]


def test_still_ambiguous_without_a_board(monkeypatch):
    plug(monkeypatch, M5, S3)
    with pytest.raises(SystemExit, match="name the board"):
        ports.pick_port()
    with pytest.raises(SystemExit, match="name the board"):
        ports.pick_port(["esp32", "esp32s3"])  # a project that runs on both


def test_two_of_the_same_board_need_a_port(monkeypatch):
    plug(monkeypatch, S3, S3_TOO)
    with pytest.raises(SystemExit, match="--port"):
        ports.pick_port(["esp32s3"])


def test_native_chips_fall_back_to_a_bridge():
    assert ports.fitting("esp32s3", [M5]) == [M5[0]]
    assert ports.fitting("esp32s3", [M5, S3]) == [S3[0]]
    assert ports.fitting("esp32", [S3]) == []
    assert ports.fitting("esp32", [M5, S3_ON_UART]) == [M5[0], S3_ON_UART[0]]


def test_no_fit_and_no_ports(monkeypatch):
    plug(monkeypatch, S3, S3_TOO)
    with pytest.raises(SystemExit, match="looks like a esp32 board"):
        ports.pick_port(["esp32"])
    plug(monkeypatch)
    with pytest.raises(SystemExit, match="no ESP32 USB serial port"):
        ports.pick_port(["esp32"])


def test_lab_port_wins(monkeypatch):
    plug(monkeypatch, M5, S3)
    monkeypatch.setenv("LAB_PORT", "/dev/cu.chosen")
    assert ports.pick_port() == "/dev/cu.chosen"


def test_pick_port_for_uses_the_projects_boards(monkeypatch):
    plug(monkeypatch, M5, S3)
    assert ports.pick_port_for("p002-matrix-demo") == M5[0]  # runs on the M5 only
    assert ports.pick_port_for("p003-bench", "s3mini") == S3[0]
    assert ports.pick_port_for("p003-bench", "m5stickc-plus11") == M5[0]
    with pytest.raises(SystemExit, match="name the board"):
        ports.pick_port_for("p003-bench")
    with pytest.raises(SystemExit, match="not in p002-matrix-demo"):
        ports.pick_port_for("p002-matrix-demo", "s3mini")
