# esp32-lab-dev: the whole toolchain for the agent's shell, the devcontainer and CI.
# Pinned here (base and uv by digest, binaries by sha256), in pyproject.toml/uv.lock, or in
# platformio.common.ini. apt packages float within Debian trixie point releases (see
# docs/decisions.md). Bump with a decisions.md line. PlatformIO packages live in /pio, not here.
FROM ghcr.io/astral-sh/uv:0.13.4@sha256:9f130421e930bfbdc563c00740466603f8723158e90297d07957c2294640262b AS uv

FROM debian:trixie-slim@sha256:62bd423aeee912639ca8ee681c2b762cb3e25137e0a92b20be455898a9f034ad

ARG WOKWI_CLI_VERSION=0.29.0
ARG ACTIONLINT_VERSION=1.7.12
# sha256 of the release assets (actionlint from its checksums.txt; wokwi-cli has none, so these
# were recorded when 0.29.0 was pinned).
ARG WOKWI_SHA256_x64=2d1cf522c07a358e4de632c9d5c324fd70ce049bf66e54d58c584700ef9bdcf9
ARG WOKWI_SHA256_arm64=fd7b450ed1c844f837af9e93fbefedca8eb0b42ea7b6ddffd62910ccca43eb04
ARG ACTIONLINT_SHA256_amd64=8aca8db96f1b94770f1b0d72b6dddcb1ebb8123cb3712530b08cc387b349a3d8
ARG ACTIONLINT_SHA256_arm64=325e971b6ba9bfa504672e29be93c24981eeb1c07576d730e9f7c8805afff0c6
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

RUN case "$TARGETARCH" in \
      amd64) arch=x64; wokwi_sha=$WOKWI_SHA256_x64; lint_sha=$ACTIONLINT_SHA256_amd64 ;; \
      arm64) arch=arm64; wokwi_sha=$WOKWI_SHA256_arm64; lint_sha=$ACTIONLINT_SHA256_arm64 ;; \
      *) exit 1 ;; \
    esac \
 && curl -fsSL -o /usr/local/bin/wokwi-cli \
      "https://github.com/wokwi/wokwi-cli/releases/download/v${WOKWI_CLI_VERSION}/wokwi-cli-linuxstatic-${arch}" \
 && echo "$wokwi_sha  /usr/local/bin/wokwi-cli" | sha256sum -c - \
 && chmod +x /usr/local/bin/wokwi-cli \
 && curl -fsSL -o /tmp/actionlint.tgz \
      "https://github.com/rhysd/actionlint/releases/download/v${ACTIONLINT_VERSION}/actionlint_${ACTIONLINT_VERSION}_linux_${TARGETARCH}.tar.gz" \
 && echo "$lint_sha  /tmp/actionlint.tgz" | sha256sum -c - \
 && tar -xzf /tmp/actionlint.tgz -C /usr/local/bin actionlint && rm /tmp/actionlint.tgz \
 && wokwi-cli --version && actionlint --version

VOLUME /pio
WORKDIR /work
