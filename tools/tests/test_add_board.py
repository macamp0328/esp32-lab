import shutil

import add_board
import pytest
from labrepo import ROOT, read_ini


@pytest.fixture
def repo(tmp_path):
    """Just the files add_board touches, copied from the real repo."""
    shutil.copytree(ROOT / "boards" / "_template", tmp_path / "boards" / "_template")
    (tmp_path / "shared" / "lab_board" / "src").mkdir(parents=True)
    shutil.copy(ROOT / "shared/lab_board/src/lab_board.h", tmp_path / "shared/lab_board/src")
    shutil.copy(ROOT / "platformio.common.ini", tmp_path)
    return tmp_path


def test_adds_board_files_dispatch_branch_and_env(repo):
    add_board.add_board(repo, "xiao-s3", "esp32s3", "seeed_xiao_esp32s3")
    d = repo / "boards" / "xiao-s3"
    ini = read_ini(d / "board.ini")
    assert ini.get("boards_xiao_s3", "board") == "seeed_xiao_esp32s3"
    assert ini.get("boards_xiao_s3", "build_flags") == "-DLAB_BOARD_XIAO_S3"
    header = (d / "board.h").read_text()
    assert '#define BOARD_NAME     "xiao-s3"' in header and '"esp32s3"' in header
    assert "{{" not in header + (d / "pinout.md").read_text() + (d / "board.ini").read_text()

    dispatcher = (repo / "shared/lab_board/src/lab_board.h").read_text()
    assert '#elif defined(LAB_BOARD_XIAO_S3)\n#include "xiao-s3/board.h"\n#else' in dispatcher
    common = read_ini(repo / "platformio.common.ini")
    assert common.get("env:xiao-s3", "extends") == "common, boards_xiao_s3"
    assert "${boards_xiao_s3.lib_deps}" in common.get("env:xiao-s3", "lib_deps")
    assert common.has_section("env:native") and common.has_section("env:s3mini")


@pytest.mark.parametrize(
    ("name", "chip", "msg"),
    [
        ("Xiao", "esp32s3", "kebab-case"),
        ("_template", "esp32s3", "kebab-case"),
        ("xiao", "esp8266", "chip"),
    ],  # fmt: skip
)
def test_rejects_bad_input(repo, name, chip, msg):
    with pytest.raises(ValueError, match=msg):
        add_board.add_board(repo, name, chip, "esp32dev")


def test_rejects_existing_board(repo):
    add_board.add_board(repo, "xiao", "esp32s3", "esp32dev")
    with pytest.raises(ValueError, match="already exists"):
        add_board.add_board(repo, "xiao", "esp32s3", "esp32dev")
