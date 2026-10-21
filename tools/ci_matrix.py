"""Print the CI build matrix as JSON: [{"project": ..., "board": ...}] for every project x board."""

from __future__ import annotations

import json

from labrepo import project_boards, projects


def matrix() -> list[dict[str, str]]:
    return [{"project": p, "board": b} for p in projects() for b in project_boards(p)]


if __name__ == "__main__":
    print(json.dumps(matrix(), separators=(",", ":")))
