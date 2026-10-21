import datetime as dt

import inventory


def item(**over):
    base = {"id": "s3mini-01..03", "model": "Waveshare ESP32-S3 Mini", "qty": 3, "source": "x"}
    return {**base, **over}


def test_committed_stock_is_valid():
    assert inventory.validate(inventory.load()) == []


def test_valid_entry_with_optional_keys():
    data = {"boards": [item(date=dt.date(2026, 10, 20), projects=["p000-blink"], notes="n")]}
    assert inventory.validate(data) == []


def test_reports_missing_unknown_and_bad_values():
    data = {
        "boards": [{"id": "Bad Id", "qty": -1, "colour": "red"}],
        "snacks": [],
    }
    errors = "\n".join(inventory.validate(data))
    assert "unknown category 'snacks'" in errors
    assert "missing 'model'" in errors and "missing 'source'" in errors
    assert "unknown key 'colour'" in errors
    assert "kebab-case" in errors
    assert "qty must be an integer" in errors


def test_duplicate_ids_and_bad_project_refs():
    data = {"boards": [item(), item(projects=["blink"])]}
    errors = "\n".join(inventory.validate(data))
    assert "duplicate id" in errors
    assert "projects must be a list of pNNN-name" in errors


def test_render_lists_every_item():
    out = inventory.render({"leds": [item(id="matrix-8x8-01", model="BTF 8x8", qty=1)]})
    assert "matrix-8x8-01" in out and "BTF 8x8" in out
