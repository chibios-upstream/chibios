#!/usr/bin/env bash
set -euo pipefail

# Run from repository root even if VS Code starts from a different cwd.
cd "$(dirname "$0")/.."

# Initialize submodules (ext -> chibios-ext, tools/ftl -> chibios-ftl).
# A fresh container clone will not have them populated, which breaks any build
# that pulls in FatFS, lwIP, wolfSSL, littlefs, or the FTL code-gen templates.
git submodule update --init --recursive

# Install Python packages used by CI/workflow helper scripts.
python3 -m pip install --break-system-packages --no-cache-dir -r tools/workflows/requirements.txt

# Quick toolchain sanity checks so container setup fails early if something is missing.
# NOTE: the Ubuntu-packaged gcc-arm-none-eabi (whatever the current base image
# ships) may differ from the canonical ChibiStudio toolchain (14.3.1). Builds
# will generally succeed, but there may be minor version-specific differences
# (warning flags, optimization behavior, etc.).
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
openocd --version
