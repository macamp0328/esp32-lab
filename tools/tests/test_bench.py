import datetime
import json
import time
from pathlib import Path

import bench
import pytest

CAPTURE = (Path(__file__).parent / "fixtures" / "bench-s3mini.txt").read_text().splitlines()
SPLIT = next(i for i, line in enumerate(CAPTURE) if '"bench_start"' in line)
BOOT, LEDS = CAPTURE[3], CAPTURE[5]
DAY = datetime.date(2026, 10, 21)


def track(lines):
    t = bench.Tracker()
    for line in lines:
        t.feed(line)
    return t


def fails_of(lines, project="p003-bench"):
    return "\n".join(bench.check(track(lines), project)[0])


def test_fixture_passes_and_keeps_only_bench_fields():
    t = track(CAPTURE)
    assert bench.check(t, "p003-bench") == ([], [])
    doc = bench.result_doc(t, "p003-bench", DAY)
    assert (doc["board"], doc["fw"], doc["core"], doc["date"], doc["ms"]) == (
        "s3mini", "a1b2c3d", "3.3.12", "2026-10-21", 9088)  # fmt: skip
    assert len(doc["results"]) == 10
    assert doc["results"][0] == {"group": "compute", "test": "int_add", "n": 1, "us": 0.0125,
                                 "max_us": 0.05, "per_s": 80000000}  # fmt: skip
    assert doc["results"][6]["note"] == "2 cores"


@pytest.mark.parametrize(
    ("lines", "expected"),
    [
        (CAPTURE[:12] + [BOOT] + CAPTURE[12:], "2 boot events"),
        (CAPTURE + ['{"t":17000,"ev":"err","code":"rmt_timeout"}'], "rmt_timeout"),
        (CAPTURE[:12] + ["Guru Meditation Error: Core  1 panic'ed"], "panic in raw log"),
        (CAPTURE[:-2], "no bench_done: timed out"),
        (CAPTURE[:SPLIT], "no bench_start"),
        (CAPTURE[: SPLIT + 1] + CAPTURE[-2:], "no bench results"),
        ([BOOT], "no leds event"),
        (CAPTURE[6:], "no boot event"),
        ([BOOT.replace("p003-bench", "p002-matrix-demo"), *CAPTURE[4:]], "expected 'p003-bench'"),
        (
            CAPTURE + ['{"t":1,"ev":"bench","group":"led","test":"x","n":1,"us":"fast"}'],
            "non-numeric",
        ),
    ],
    ids="reboot err panic timeout no-start no-results no-leds no-boot project bad-field".split(),
)
def test_failures(lines, expected):
    assert expected in fails_of(lines)


def test_a_crash_is_not_also_reported_as_a_timeout():
    fails = fails_of(CAPTURE[:12] + [BOOT])
    assert "2 boot events" in fails and "timed out" not in fails


def test_count_mismatch_and_malformed_lines_only_warn():
    lines = [line for line in CAPTURE if "sha256" not in line] + ['{"t":1,"ev":']
    fails, warns = bench.check(track(lines), "p003-bench")
    assert fails == []
    assert "1 malformed JSON line(s)" in warns
    assert "bench_done says 10 tests, got 9 bench events" in warns


def test_board_name_is_file_safe():
    assert bench.board_name(track([BOOT.replace('"s3mini"', '"M5/Stick C"')])) == "m5_stick_c"
    assert bench.board_name(track([])) == "unknown"


def test_formatters():
    assert [bench._sig3(v) for v in (0, 1.234, 12.34, 123.4, 999.6)] == ["0", "1.23", "12.3",
                                                                         "123", "1000"]  # fmt: skip
    assert bench.fmt_time(0.0125) == "12.5 ns"
    assert bench.fmt_time(95.2) == "95.2 µs"
    assert bench.fmt_time(2110) == "2.11 ms"
    assert bench.fmt_time(1.5e6) == "1.50 s"
    assert bench.fmt_rate(473.9) == "474/s"
    assert bench.fmt_rate(10504) == "10.5 k/s"
    assert bench.fmt_rate(80e6) == "80.0 M/s"
    assert bench.fmt_rate(409.6e6, "B") == "410 MB/s"
    assert bench.fmt_rate(512, "B") == "512 B/s"


def test_write_result_round_trips(tmp_path):
    doc = bench.result_doc(track(CAPTURE), "p003-bench", DAY)
    path = bench.write_result(tmp_path, doc)
    assert path == tmp_path / "results" / "s3mini.json"
    assert json.loads(path.read_text()) == doc
    assert bench.load_results(tmp_path) == [doc]


def _m5(doc):
    """The same suite on a board half as fast, with no PSRAM and a 25-LED strip on show."""
    out = dict(doc, board="m5stickc-plus11", fw="e4f5a6b", ms=18000)
    out["results"] = [dict(r, us=r["us"] * 2, per_s=r["per_s"] / 2) for r in doc["results"]
                      if r["test"] != "memcpy_psram"]  # fmt: skip
    out["results"][4] = dict(out["results"][4], n=25)
    return out


def _table(text, group):
    block = text.split(f"## {group}\n\n", 1)[1].split("\n\n", 1)[0]
    return [[c.strip() for c in row.strip("|").split("|")] for row in block.splitlines()]


def test_results_md_one_board():
    doc = bench.result_doc(track(CAPTURE), "p003-bench", DAY)
    text = bench.render("p003-bench", [doc])
    assert text.startswith("# p003-bench results\n")
    assert "| s3mini | a1b2c3d | 3.3.12 | 2026-10-21 | 10 | 9.1 s |" in text
    assert [line[3:] for line in text.splitlines() if line.startswith("## ")] == [
        "compute", "led", "dualcore", "memory"]  # fmt: skip
    compute = _table(text, "compute")
    assert compute[0] == ["test", "n", "s3mini"]
    assert compute[2:] == [["int_add", "1", "80.0 M/s"], ["float_mul", "1", "46.9 M/s"],
                           ["sha256_1k", "1", "10.5 k/s"]]  # fmt: skip
    led = _table(text, "led")
    assert led[0] == ["test", "LEDs", "s3mini"]
    assert led[3] == ["show", "64", "2.11 ms"]
    memory = _table(text, "memory")
    assert memory[0] == ["test", "bytes", "s3mini", "note"]
    assert memory[2:] == [["memcpy_internal", "4096", "410 MB/s", ""],
                          ["memcpy_psram", "4096", "66.7 MB/s", "PSRAM"],
                          ["malloc_free", "1", "323 k/s", ""]]  # fmt: skip
    assert " vs " not in text


def test_results_md_two_boards_has_a_ratio_column():
    s3 = bench.result_doc(track(CAPTURE), "p003-bench", DAY)
    text = bench.render("p003-bench", [s3, _m5(s3)])
    compute = _table(text, "compute")
    assert compute[0] == ["test", "n", "m5stickc-plus11", "s3mini", "m5stickc-plus11 vs s3mini"]
    assert compute[2] == ["int_add", "1", "40.0 M/s", "80.0 M/s", "0.500×"]
    led = _table(text, "led")
    assert led[2:] == [["render_rainbow", "64", "13.0 k/s", "26.0 k/s", "0.500×"],
                       ["show", "25", "4.22 ms", "–", "–"],
                       ["show", "64", "–", "2.11 ms", "–"]]  # fmt: skip
    memory = _table(text, "memory")
    assert memory[3] == ["memcpy_psram", "4096", "–", "66.7 MB/s", "–", "PSRAM"]
    assert "`m5stickc-plus11 vs s3mini` = s3mini's time ÷ m5stickc-plus11's" in text


class FakeSerial:
    """Plays a capture: lines before the split come after reset, the rest answer `bench`."""

    def __init__(self, before, after):
        self.pending = [line.encode() + b"\n" for line in before]
        self.answer = [line.encode() + b"\n" for line in after]
        self.writes = []

    def write(self, data):
        self.writes.append(data)
        self.pending += self.answer
        self.answer = []

    def read(self, _n):
        if self.pending:
            return self.pending.pop(0)
        time.sleep(0.005)
        return b""

    def close(self):
        pass


@pytest.fixture
def board(tmp_path, monkeypatch):
    """A fake s3mini running the fixture; bench writes under tmp_path, never projects/."""
    monkeypatch.setattr(bench, "PROJECTS", tmp_path)
    monkeypatch.setattr(bench, "GRACE_S", 0)
    monkeypatch.setattr(bench.serial_watch, "reset", lambda ser: None)

    def plug(before=CAPTURE[:SPLIT], after=CAPTURE[SPLIT:]):
        fake = FakeSerial(before, after)
        monkeypatch.setattr(bench.serial_watch, "open_port", lambda port, baud: fake)
        return fake

    return plug


def test_main_sends_bench_after_leds_and_writes_everything(tmp_path, board, capsys):
    fake = board()
    m5 = _m5(bench.result_doc(track(CAPTURE), "p003-bench", DAY))
    bench.write_result(tmp_path / "p003-bench", m5)
    assert bench.main(["p003-bench", "--port", "/dev/fake"]) == 0
    assert fake.writes == [b"bench\n"]
    root = tmp_path / "p003-bench"
    log = (root / ".hil" / "bench-s3mini.log").read_text().splitlines()
    assert log[0].endswith("ESP-ROM:esp32s3-20210327") and '"bench_done"' in log[-1]
    jsonl = (root / ".hil" / "bench-s3mini.jsonl").read_text().splitlines()
    assert all(line.startswith("{") for line in jsonl) and len(jsonl) == len(CAPTURE) - 4
    doc = json.loads((root / "results" / "s3mini.json").read_text())
    assert doc["date"] == datetime.date.today().isoformat() and len(doc["results"]) == 10
    assert "m5stickc-plus11 vs s3mini" in (root / "RESULTS.md").read_text(encoding="utf-8")
    out = capsys.readouterr().out
    assert "PASS s3mini: 10 results" in out
    assert "compute: 3 tests · fastest int_add (12.5 ns) · slowest sha256_1k (95.2 µs)" in out


def test_main_timeout_writes_the_capture_but_no_results(tmp_path, board, capsys):
    board(after=CAPTURE[SPLIT:-2])
    assert bench.main(["p003-bench", "--port", "/dev/fake", "--timeout", "0.3"]) == 1
    root = tmp_path / "p003-bench"
    assert (root / ".hil" / "bench-s3mini.log").is_file()
    assert not (root / "results").exists() and not (root / "RESULTS.md").exists()
    assert "no bench_done: timed out" in capsys.readouterr().out


def test_main_stops_at_a_reboot(tmp_path, board, capsys):
    board(after=CAPTURE[SPLIT : SPLIT + 3] + [CAPTURE[0], BOOT] + CAPTURE[SPLIT + 3 :])
    start = time.monotonic()
    assert bench.main(["p003-bench", "--port", "/dev/fake", "--timeout", "30"]) == 1
    assert time.monotonic() - start < 5
    assert "2 boot events" in capsys.readouterr().out
    assert not (tmp_path / "p003-bench" / "results").exists()


def test_main_resends_bench_when_the_board_did_not_answer(board, monkeypatch):
    monkeypatch.setattr(bench, "RESEND_S", 0.05)
    fake = board(after=[])
    assert bench.main(["p003-bench", "--port", "/dev/fake", "--timeout", "0.5"]) == 1
    assert fake.writes == [b"bench\n"] * bench.MAX_SENDS


def test_math_rows_show_time_per_call():
    e = {
        "group": "compute",
        "test": "math/sinf",
        "n": 1000,
        "us": 695.4,
        "max_us": 716,
        "per_s": 1438,
    }
    assert bench.row_kind("compute", 1000, [e]) == "per_op"
    assert bench.fmt_cell("per_op", e) == bench.fmt_time(0.6954) + "/call"
    assert bench.row_kind("compute", 64, [{**e, "test": "fx/plasma", "n": 64}]) == "rate"
