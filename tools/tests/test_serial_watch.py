import serial_watch

BOOT = (
    '{{"t":1250,"ev":"boot","proj":"p000-blink","fw":"a1b2c3d","board":"{b}",'
    '"fastled":"3.10.7","core":"3.3.12"}}'
)
LEDS = '{"t":1300,"ev":"leds","n":1,"driver":"RMT","cap_ma":500}'
FPS = '{"t":6300,"ev":"fps","fps":98,"heap":214000}'


def link(peer):
    return f'{{"t":6400,"ev":"link","peer":"{peer}","rssi":-41,"rx":40,"loss":0.0}}'


def fake_capture(by_port):
    def capture(port, seconds, do_reset=True):
        return [f"{i:8.3f} {line}" for i, line in enumerate(by_port[port])]

    return capture


def test_watch_all_captures_each_board_and_checks_the_mesh(monkeypatch, tmp_path, capsys):
    nodes = [("s3mini", "/dev/a"), ("s3mini", "/dev/b"), ("m5stickc-plus11", "/dev/m")]
    by_port = {
        "/dev/a": [BOOT.format(b="s3mini"), LEDS, FPS, link("b"), link("m")],
        "/dev/b": [BOOT.format(b="s3mini"), LEDS, FPS, link("a"), link("m")],
        "/dev/m": [BOOT.format(b="m5stickc-plus11"), LEDS, FPS, link("a")],  # never heard b
    }
    monkeypatch.setattr(serial_watch, "all_ports_for", lambda p: nodes)
    monkeypatch.setattr(serial_watch, "node_tag", lambda port: port[-1])
    monkeypatch.setattr(serial_watch, "capture", fake_capture(by_port))
    assert serial_watch.watch_all("p000-blink", 5, True, tmp_path) == 1
    out = capsys.readouterr().out
    assert "last-run-s3mini-a: hil_assert: PASS" in out
    assert "last-run-m5stickc-plus11-m: hil_assert: FAIL" in out and "heard 1 peer" in out
    assert "2 of 3 boards pass" in out
    assert (tmp_path / "last-run-s3mini-b.jsonl").read_text().count("\n") == 5
    assert (tmp_path / "last-run-s3mini-b.log").is_file()


def test_watch_all_without_a_link_skips_the_mesh_check(monkeypatch, tmp_path):
    nodes = [("s3mini", "/dev/a"), ("m5stickc-plus11", "/dev/m")]
    by_port = {
        "/dev/a": [BOOT.format(b="s3mini"), LEDS, FPS],
        "/dev/m": ["rst:0x1 (POWERON_RESET)", BOOT.format(b="m5stickc-plus11"), LEDS, FPS],
    }
    monkeypatch.setattr(serial_watch, "all_ports_for", lambda p: nodes)
    monkeypatch.setattr(serial_watch, "node_tag", lambda port: port[-1])
    monkeypatch.setattr(serial_watch, "capture", fake_capture(by_port))
    assert serial_watch.watch_all("p000-blink", 5, True, tmp_path) == 0
