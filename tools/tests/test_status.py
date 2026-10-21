import status


def test_render_replaces_only_between_markers():
    readme = f"# x\n\nintro\n{status.BEGIN}\nold table\n{status.END}\n\nfooter\n"
    out = status.render(readme)
    assert out.startswith("# x\n\nintro\n" + status.BEGIN + "\n| Project |")
    assert out.endswith(status.END + "\n\nfooter\n")
    assert "old table" not in out
    assert status.render(out) == out  # idempotent


def test_status_line_regex():
    m = status.STATUS_RE.search("**Status:** prototype  ·  **Last HIL check:** never\n")
    assert m and m.group(1) == "prototype" and m.group(2) == "never"
