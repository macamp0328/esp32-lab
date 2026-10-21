"""PlatformIO post-script: LAB_FW = short git sha (+ when dirty), for project sources only."""

import subprocess

Import("env", "projenv")


def _git(*args: str) -> str:
    return subprocess.check_output(
        ["git", *args],
        cwd=env["PROJECT_DIR"],
        text=True,
        stderr=subprocess.DEVNULL,
    ).strip()


try:
    fw = _git("rev-parse", "--short=7", "HEAD") + ("+" if _git("status", "--porcelain") else "")
except (OSError, subprocess.CalledProcessError):
    fw = "nogit"

projenv.Append(CPPDEFINES=[("LAB_FW", projenv.StringifyMacro(fw))])
