import ci_matrix
from labrepo import boards, project_boards, projects


def test_matrix_covers_every_project_and_supported_board():
    m = ci_matrix.matrix()
    assert {(t["project"], t["board"]) for t in m} == {
        (p, b) for p in projects() for b in project_boards(p)
    }
    assert {"project": "p000-blink", "board": "s3mini"} in m


def test_every_listed_board_exists():
    for p in projects():
        assert set(project_boards(p)) <= set(boards()), p
