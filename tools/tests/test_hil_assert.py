import hil_assert


def run(lines, raw="", project="p000-blink"):
    events, malformed = hil_assert.parse(lines)
    fails, summary = hil_assert.check(events, raw, project)
    return fails + ([f"{malformed} malformed"] if malformed else []), summary


BOOT = '{"t":1250,"ev":"boot","proj":"p000-blink","fw":"a1b2c3d","board":"s3mini"}'
LEDS = '{"t":1300,"ev":"leds","n":1,"driver":"RMT","cap_ma":500}'
FPS = '{"t":6300,"ev":"fps","fps":98,"heap":214000}'


def test_good_capture_passes():
    fails, summary = run(["ESP-ROM:esp32s3", BOOT, LEDS, FPS, FPS])
    assert fails == []
    assert "fps min 98 (2 samples)" in summary and "+50ms" in summary


def test_missing_boot():
    assert run([LEDS, FPS])[0] == ["no boot event (did the board reset and print to USB CDC?)"]


def test_late_or_missing_leds():
    late = '{"t":3300,"ev":"leds","n":1}'
    assert "2050 ms after boot" in run([BOOT, late, FPS])[0][0]
    assert run([BOOT, FPS])[0] == ["no leds event after boot"]


def test_low_fps_err_and_reboot_fail():
    slow = '{"t":6300,"ev":"fps","fps":12}'
    err = '{"t":7000,"ev":"err","code":"wifi_timeout"}'
    fails = "\n".join(run([BOOT, LEDS, slow, err, BOOT])[0])
    assert "fps dropped to 12" in fails
    assert "wifi_timeout" in fails
    assert "2 boot events" in fails


def test_wrong_project_panic_and_malformed():
    other = BOOT.replace("p000-blink", "p001-matrix")
    fails = "\n".join(run([other, LEDS, FPS, '{"t":1,"ev":'], raw="Guru Meditation Error")[0])
    assert "expected 'p000-blink'" in fails
    assert "panic" in fails
    assert "1 malformed" in fails


def test_idf_error_lines_are_noted_not_failed(tmp_path, capsys):
    cap = tmp_path / "last-run.jsonl"
    cap.write_text("\n".join([BOOT, LEDS, FPS]) + "\n")
    cap.with_suffix(".log").write_text("   0.296 E (8) cache: esp_cache_msync(113): invalid addr\n")
    assert hil_assert.main([str(cap), "--project", "p000-blink"]) == 0
    assert "1 ESP-IDF error log line" in capsys.readouterr().out
