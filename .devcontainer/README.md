# Dev Container for ChibiOS

This folder contains the Visual Studio Code Dev Containers configuration for this repository.

## Files

- `devcontainer.json`: VS Code container definition (image build, extensions, post-create hook).
- `Dockerfile`: Ubuntu-based image with ARM embedded build/debug tools.
- `post-create.sh`: Post-create setup script executed by VS Code after container creation.

## What the container provides

- ARM GCC cross compiler (`arm-none-eabi-gcc`).
- Multi-arch GDB exposed as `arm-none-eabi-gdb`.
- OpenOCD.
- Python 3 and pip, plus workflow Python dependencies from `tools/workflows/requirements.txt`.
- Common build tools (`make`, `build-essential`, `gawk`, `git`).

## Usage

1. Open the repository in VS Code.
2. Run "Dev Containers: Reopen in Container".
3. Wait for image build and post-create script completion.

After that, project builds can be executed from the integrated terminal, for example:

```sh
cd testhal/RP/RP2040/XIP-VALIDATION
make -j$(nproc)
```
