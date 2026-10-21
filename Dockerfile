# esp32-lab-dev: the whole toolchain for the agent's shell, the devcontainer and CI.
# Every version is pinned here, in pyproject.toml/uv.lock, or in platformio.common.ini.
# Bump with a line in docs/decisions.md. PlatformIO packages live in the /pio volume, not the image.
FROM ghcr.io/astral-sh/uv:0.13.4 AS uv

FROM debian:trixie-slim@sha256:62bd423aeee912639ca8ee681c2b762cb3e25137e0a92b20be455898a9f034ad

ARG WOKWI_CLI_VERSION=0.29.0
ARG ACTIONLINT_VERSION=1.7.12
ARG TARGETARCH

ENV DEBIAN_FRONTEND=noninteractive \
    LAB_IN_CONTAINER=1 \
    PLATFORMIO_CORE_DIR=/pio \
    UV_PROJECT_ENVIRONMENT=/opt/lab-venv \
    UV_PYTHON_INSTALL_DIR=/opt/uv-python \
    UV_LINK_MODE=copy \
    UV_COMPILE_BYTECODE=1 \
    PATH=/opt/lab-venv/bin:$PATH

# build-essential: native (host) tests; cppcheck: lint; git: firmware hash and checkout; zstd: CI cache.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential ca-certificates cppcheck curl git zstd \
 && rm -rf /var/lib/apt/lists/* \
 && git config --system --add safe.directory '*'

COPY --from=uv /uv /uvx /usr/local/bin/

# Python tooling (PlatformIO, esptool, ruff, clang-format, pytest, just) from the lockfile.
WORKDIR /opt/lab-src
COPY pyproject.toml uv.lock .python-version ./
RUN uv sync --frozen --no-install-project && pio --version && just --version

RUN case "$TARGETARCH" in amd64) arch=x64 ;; arm64) arch=arm64 ;; *) exit 1 ;; esac \
 && curl -fsSL -o /usr/local/bin/wokwi-cli \
      "https://github.com/wokwi/wokwi-cli/releases/download/v${WOKWI_CLI_VERSION}/wokwi-cli-linuxstatic-${arch}" \
 && chmod +x /usr/local/bin/wokwi-cli \
 && curl -fsSL "https://github.com/rhysd/actionlint/releases/download/v${ACTIONLINT_VERSION}/actionlint_${ACTIONLINT_VERSION}_linux_${TARGETARCH}.tar.gz" \
    | tar -xz -C /usr/local/bin actionlint \
 && wokwi-cli --version && actionlint --version

VOLUME /pio
WORKDIR /work
