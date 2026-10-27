import hil_assert
import pytest


def run(lines, raw="", project="p000-blink"):
    events, malformed = hil_assert.parse(lines)
    fails, summary = hil_assert.check(events, raw, project)
    return fails + ([f"{malformed} malformed"] if malformed else []), summary


BOOT = (
    '{"t":1250,"ev":"boot","proj":"p000-blink","fw":"a1b2c3d","board":"s3mini",'
    '"fastled":"3.10.7","core":"3.3.12"}'
)
LEDS = '{"t":1300,"ev":"leds","n":1,"driver":"RMT","cap_ma":500}'
FPS = '{"t":6300,"ev":"fps","fps":98,"heap":214000}'


def test_good_capture_passes():
    fails, summary = run(["ESP-ROM:esp32s3", BOOT, LEDS, FPS, FPS])
    assert fails == []
    assert "fps min 98 (2 samples)" in summary and "+50ms" in summary


def test_missing_boot():
    assert run([LEDS, FPS])[0] == ["no boot event (did the board reset and print to USB CDC?)"]


def test_late_or_missing_leds():
    late = '{"t":3300,"ev":"leds","n":1,"driver":"RMT","cap_ma":500}'
    assert "2050 ms after boot" in run([BOOT, late, FPS])[0][0]
    assert run([BOOT, FPS])[0] == ["no leds event after boot"]


def test_low_fps_err_and_reboot_fail():
    slow = '{"t":6300,"ev":"fps","fps":12,"heap":1}'
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


def test_fps_must_keep_coming_every_5_s():
    stall = '{"t":20300,"ev":"fps","fps":98,"heap":1}'
    fails = run([BOOT, LEDS, FPS, stall])[0]
    assert any("ms without an fps event" in f for f in fails)
    events, _ = hil_assert.parse([BOOT, LEDS, FPS])
    fails, _ = hil_assert.check(events, project="p000-blink", seconds=30)
    assert any("only 1 fps events in 30 s" in f for f in fails)


def test_a_long_capture_allows_windows_slightly_over_5_s():
    # measured on the M5 over 3 h: 2157 windows of ~5005 ms, one fewer than 5000 ms would give
    lines = [BOOT, LEDS] + [
        f'{{"t":{1250 + 5443 + 5005 * k},"ev":"fps","fps":97,"heap":1}}' for k in range(2157)
    ]
    events, _ = hil_assert.parse(lines)
    fails, _ = hil_assert.check(events, project="p000-blink", seconds=10800)
    assert fails == []


def test_missing_or_bad_fields_fail_instead_of_crashing():
    bare_boot = '{"t":1,"ev":"boot","proj":"p000-blink"}'
    no_t = '{"ev":"leds","n":1,"driver":"RMT","cap_ma":500}'
    null_fps = '{"t":6300,"ev":"fps","fps":null,"heap":1}'
    fails = "\n".join(run([bare_boot, no_t, null_fps])[0])
    assert "boot event missing" in fails and "fw" in fails
    assert "leds event missing" in fails and "fps event missing" in fails


def test_empty_project_is_rejected(tmp_path):
    cap = tmp_path / "last-run.jsonl"
    cap.write_text("\n".join([BOOT, LEDS, FPS]) + "\n")
    with pytest.raises(SystemExit):
        hil_assert.main([str(cap), "--project", ""])


def link(peer, rx=40):
    return f'{{"t":6400,"ev":"link","peer":"{peer}","rssi":-41,"rx":{rx},"loss":0.02}}'


def test_peers_must_be_heard_on_the_link():
    events, _ = hil_assert.parse([BOOT, LEDS, FPS, link("44a574"), link("48720c")])
    fails, summary = hil_assert.check(events, project="p000-blink", peers=2)
    assert fails == [] and "peers 44a574,48720c" in summary
    events, _ = hil_assert.parse([BOOT, LEDS, FPS, link("44a574"), link("48720c", rx=0)])
    fails, _ = hil_assert.check(events, project="p000-blink", peers=2)
    assert fails == ["heard 1 peer(s) ['44a574'] on the link, expected >= 2"]


def test_link_events_need_their_fields():
    bad = '{"t":6400,"ev":"link","peer":"a","rssi":"loud","rx":3}'
    fails = "\n".join(run([BOOT, LEDS, FPS, bad])[0])
    assert "link event missing/non-numeric" in fails and "loss" in fails and "rssi" in fails
