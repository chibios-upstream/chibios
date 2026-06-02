#!/usr/bin/env bash
set -euo pipefail

# Run from repository root even if VS Code starts from a different cwd.
cd "$(dirname "$0")/.."

# Install Python packages used by CI/workflow helper scripts.
python3 -m pip install --no-cache-dir -r tools/workflows/requirements.txt

# Quick toolchain sanity checks so container setup fails early if something is missing.
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
openocd --version
