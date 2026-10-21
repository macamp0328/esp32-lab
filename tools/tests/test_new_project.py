import new_project
import pytest


@pytest.fixture
def tmp_projects(tmp_path, monkeypatch):
    monkeypatch.setattr(new_project, "PROJECTS", tmp_path)
    monkeypatch.setattr(new_project, "projects", lambda: sorted(p.name for p in tmp_path.iterdir()))
    monkeypatch.setattr(new_project.status, "README", tmp_path / "no-readme.md")
    monkeypatch.setattr(new_project, "numbers_in_history", lambda: set())
    return tmp_path


def test_creates_project_with_names_and_boards(tmp_projects):
    assert new_project.main(["p042-desk-lamp", "--board", "s3mini,m5stickc-plus11"]) == 0
    root = tmp_projects / "p042-desk-lamp"
    assert "default_envs = s3mini, m5stickc-plus11" in (root / "platformio.ini").read_text()
    assert '"p042-desk-lamp"' in (root / "include" / "config.h").read_text()
    leftovers = [p for p in root.rglob("*") if p.is_file() and "{{" in p.read_text()]
    assert leftovers == []


@pytest.mark.parametrize(
    "args",
    [
        ["desk-lamp", "--board", "s3mini"],
        ["p042-Desk", "--board", "s3mini"],
        ["p042-lamp", "--board", "uno"],
    ],
)
def test_rejects_bad_names_and_boards(tmp_projects, args):
    assert new_project.main(args) == 1
    assert list(tmp_projects.iterdir()) == []


def test_never_reuses_a_number(tmp_projects):
    assert new_project.main(["p042-lamp", "--board", "s3mini"]) == 0
    assert new_project.main(["p042-other", "--board", "s3mini"]) == 1


def test_missing_board_lists_inventory(tmp_projects, monkeypatch, capsys):
    stock = {"boards": [{"id": "a", "board": "s3mini", "qty": 3}, {"id": "b", "qty": 2},
                        {"id": "c", "board": "m5stickc-plus11", "qty": 1}]}  # fmt: skip
    monkeypatch.setattr(new_project.inventory, "load", lambda: stock)
    assert new_project.main(["p042-lamp"]) == 1
    err = capsys.readouterr().err
    assert "s3mini (3 on hand)" in err and "m5stickc-plus11 (1 on hand)" in err
    assert list(tmp_projects.iterdir()) == []


def test_numbers_of_deleted_projects_are_not_reused(tmp_projects, monkeypatch):
    monkeypatch.setattr(new_project, "numbers_in_history", lambda: {"p042"})
    assert new_project.main(["p042-lamp", "--board", "s3mini"]) == 1
    assert list(tmp_projects.iterdir()) == []


def test_history_sees_committed_projects():
    assert "p000" in new_project.numbers_in_history()
