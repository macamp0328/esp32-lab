# esp32-lab verbs. Agent, human and CI run exactly these; nothing else builds, flashes or tests.
# Toolchain verbs run inside the esp32-lab-dev container (started for you on the host);
# flash/watch run on the host because USB does not cross Docker on macOS.

set shell := ["bash", "-euo", "pipefail", "-c"]

# Image tag is a hash of everything that goes into it, so a stale image cannot be used.
toolchain_hash := `cat Dockerfile pyproject.toml uv.lock .python-version | { sha256sum 2>/dev/null || shasum -a 256; } | cut -c1-12`
image := env("LAB_IMAGE", "esp32-lab-dev:tc-" + toolchain_hash)
cpp_dirs := "boards shared templates projects"
# flash/watch talk to USB, so they run on the host with the .venv from `just bootstrap`
host_python := if env("LAB_IN_CONTAINER", "0") == "1" { "python" } else { ".venv/bin/python" }

# list the verbs
default:
    @just --list --unsorted

# one-time host setup: toolchain image + host tools (esptool, pio monitor) in .venv
bootstrap:
    @command -v docker >/dev/null || { echo "need Docker: https://docs.docker.com/get-docker/" >&2; exit 1; }
    @docker info >/dev/null 2>&1 || { echo "Docker is installed but not running; start it" >&2; exit 1; }
    @command -v uv >/dev/null || { echo "need uv: brew install uv" >&2; exit 1; }
    just image
    uv sync --frozen --no-dev
    @echo "bootstrap ok: image {{ image }}; host tools in .venv"

# build the toolchain image if this Dockerfile + lockfile has not been built yet
image:
    @docker image inspect {{ image }} >/dev/null 2>&1 \
      || docker build -t {{ image }} -t esp32-lab-dev:latest .

# interactive shell in the toolchain container
shell:
    just _c bash

# scaffold a project: just new p003-desk-lamp --board s3mini[,m5stickc-plus11]
new name *args:
    just _c python tools/new_project.py {{ name }} {{ args }}

# compile a project for one board, or for every board it supports
build project board="":
    just _c just _build {{ project }} {{ quote(board) }}

# run a project's native (host) unit tests
test project:
    just _c just _test {{ project }}

# every test in the repo: tools, shared libs, then each project's native tests
test-all:
    just _c just _test-all

# lint and repo rules: clang-format, cppcheck, ruff, actionlint, boards/projects/pins/secrets/inventory
check:
    just _c just _check

# same as check (the spec uses both names)
lint: check

# apply clang-format and ruff fixes
fmt:
    just _c just _fmt

# print and validate inventory/stock.yaml
inventory:
    just _c python tools/inventory.py

# regenerate the project status table in README.md
status:
    just _c python tools/status.py --write

# CI build matrix as JSON: every project x supported board
ci-matrix:
    @just _c python tools/ci_matrix.py

# flash a built project over USB, or `--ota [host]` with USB fallback (host side)
flash project *args: _host_tools
    #!/usr/bin/env bash
    set -euo pipefail
    p=$(just _project {{ project }})
    exec {{ host_python }} tools/flash.py "$p" {{ args }}

# reset the board, capture 30 s of serial to projects/<p>/.hil/, then run hil_assert (host side)
watch project *args: _host_tools
    #!/usr/bin/env bash
    set -euo pipefail
    p=$(just _project {{ project }})
    exec {{ host_python }} tools/serial_watch.py "$p" {{ args }}

# run a project's Wokwi scenario (not set up: needs a WOKWI_CLI_TOKEN, see docs/decisions.md)
sim project:
    @echo "just sim: not set up yet; needs a Wokwi token (docs/decisions.md)" >&2; exit 2

# scaffold a board: just add-board xiao-s3 --chip esp32s3 --pio-board seeed_xiao_esp32s3
add-board name *args:
    just _c python tools/add_board.py {{ name }} {{ args }}

# ---- internals ---------------------------------------------------------------------------

# run a command in the toolchain container, or directly when already inside it (CI, devcontainer)
[positional-arguments]
_c +cmd:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ "${LAB_IN_CONTAINER:-0}" = 1 ]; then exec "$@"; fi
    just image
    tty=""; if [ -t 0 ] && [ -t 1 ]; then tty="-it"; fi
    exec docker run --rm $tty -v "{{ justfile_directory() }}:/work" -v esp32-lab-pio:/pio \
      -w /work "{{ image }}" "$@"

[private]
_host_tools:
    @[ "{{ host_python }}" = python ] || [ -x {{ host_python }} ] || { echo "run just bootstrap first" >&2; exit 1; }

# resolve a project name or its pNNN prefix to the folder under projects/
_project name:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ -f "projects/{{ name }}/platformio.ini" ]; then echo "{{ name }}"; exit 0; fi
    shopt -s nullglob; m=(projects/{{ name }}-*/platformio.ini)
    if [ ${#m[@]} -ne 1 ]; then echo "no unique project matches '{{ name }}'; see projects/" >&2; exit 1; fi
    basename "$(dirname "${m[0]}")"

_build project board="":
    #!/usr/bin/env bash
    set -euo pipefail
    p=$(just _project {{ project }})
    cd "projects/$p"
    if [ -n "{{ board }}" ]; then pio run -e "{{ board }}"; else pio run; fi

_test project:
    #!/usr/bin/env bash
    set -euo pipefail
    p=$(just _project {{ project }})
    cd "projects/$p" && pio test -e native

_test-all:
    #!/usr/bin/env bash
    set -euo pipefail
    pytest -q
    echo "== native tests: shared/tests"
    (cd shared/tests && pio test -e native)
    shopt -s nullglob
    for d in projects/*/; do
      [ -f "$d/platformio.ini" ] || continue  # leftovers like a lone .pio/ are not projects
      echo "== native tests: $d"
      (cd "$d" && pio test -e native)
    done

_check:
    #!/usr/bin/env bash
    set -euo pipefail
    python tools/lab_check.py
    ruff check tools
    ruff format --check tools
    actionlint
    mapfile -t files < <(find {{ cpp_dirs }} -path '*/.pio' -prune -o \
      \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' \) -print | sort)
    clang-format --dry-run --Werror "${files[@]}"
    cpp() {
      cppcheck --language=c++ --std=c++17 --enable=warning,performance --inline-suppr --quiet \
        --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unknownMacro \
        -DLAB_BOARD_S3MINI -DARDUINO=10812 -DFASTLED_RMT5=1 -Iboards -Ishared/lab_board/src \
        -Ishared/lab_core/src -Ishared/lab_net/src -Ishared/lab_fx/src \
        --template='{severity}: {file}:{line}: {message} [{id}]' "$@" 2>&1 || true
    }
    log=$(mktemp)
    {
      cpp -ishared/tests/.pio boards shared
      for d in templates/project projects/*; do
        src=(); for s in src include lib test; do [ -d "$d/$s" ] && src+=("$d/$s"); done
        cpp -I"$d/include" "${src[@]}"
      done
    } | tee "$log"
    if grep -q '^error:' "$log"; then echo "cppcheck: errors above" >&2; exit 1; fi
    echo "check: ok"

_fmt:
    #!/usr/bin/env bash
    set -euo pipefail
    find {{ cpp_dirs }} -path '*/.pio' -prune -o \
      \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' \) -print0 \
      | xargs -0 clang-format -i
    ruff format tools
    ruff check --fix --exit-zero tools
