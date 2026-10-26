import struct
import zlib

import weave

RED, BLUE = "f800", "001f"


def event(i, col):
    return f'{{"t":{i},"ev":"weave","i":{i},"s":"0055aaff00000000","c":"{col}"}}'


def decode(data):
    """(width, height, rows of RGB bytes) from a PNG weave wrote: one IDAT, filter 0 rows."""
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    width, height = struct.unpack(">II", data[16:24])
    idat = data.index(b"IDAT")
    size = struct.unpack(">I", data[idat - 4 : idat])[0]
    raw = zlib.decompress(data[idat + 4 : idat + 4 + size])
    stride = 1 + 3 * width
    return width, height, [raw[y * stride + 1 : (y + 1) * stride] for y in range(height)]


def test_rgb565_expands_to_full_range():
    assert weave.rgb565(0xF800) == (255, 0, 0)
    assert weave.rgb565(0x07E0) == (0, 255, 0)
    assert weave.rgb565(0x0000) == (0, 0, 0)


def test_threads_keep_order_and_skip_junk():
    lines = [
        event(1, BLUE + BLUE),
        '{"t":5,"ev":"fps","fps":95,"heap":1}',
        event(0, RED + BLUE),
        '{"t":6,"ev":"weave","i":2,"c":"12"}',  # not whole samples
        '{"t":7,"ev":"weave","i":3,"c":"zzzz"}',
        "not json",
    ]
    woven = weave.threads(lines)
    assert [i for i, _ in woven] == [0, 1]
    assert woven[0][1] == [(255, 0, 0), (0, 0, 255)]


def test_render_lays_threads_side_by_side(tmp_path):
    cap = tmp_path / "last-run-m5.jsonl"
    cap.write_text("\n".join([event(0, RED + BLUE), event(1, BLUE)]) + "\n")
    assert weave.main([str(cap), "--px", "2"]) == 0
    width, height, rows = decode((tmp_path / "weave-last-run-m5.png").read_bytes())
    assert (width, height) == (4, 4)
    assert rows[0][:3] == bytes((255, 0, 0)) and rows[0][6:9] == bytes((0, 0, 255))
    assert rows[3][:3] == bytes((0, 0, 255)) and rows[3][6:9] == bytes((0, 0, 0))  # padded


def test_no_weave_events_fails(tmp_path, capsys):
    cap = tmp_path / "empty.jsonl"
    cap.write_text('{"t":1,"ev":"boot"}\n')
    assert weave.main([str(cap)]) == 1
    assert "no weave events" in capsys.readouterr().err
