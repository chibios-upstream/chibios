# RT-SB-DYNAMIC-RAMBOX-MULTI

Multi-target XHAL sandbox host requiring no SD card. The host dynamically loads
`/bin/msh.elf`, connects stdin/stdout/stderr to the SIO-backed POSIX TTY at
`/dev/ttyS0`, waits for shell termination, finalizes the sandbox and restarts it.
Before each launch, `pttyReset()` restores terminal defaults and discards pending
input, EOF records, output, signals and flow-control state.

`msh` and interactive `sbsh` save terminal attributes before each prompt and
temporarily disable canonical input and driver echo. Their own editors handle
Backspace, Ctrl-U, and arrow-key history; CR or LF submits a command. Ctrl-D
exits only on an empty command line; on a nonempty line it does not submit it.
The saved attributes are restored before parsing or executing commands and on
reader EOF/error. A terminal preparation/restoration failure stops the shell.

Commands therefore inherit the pre-editing mode, normally the canonical
defaults established by the host. Intentional attribute changes made by a
command persist across subsequent prompts. Mode changes do not flush input.
Sandbox `tcgetattr()` and `tcsetattr()` use validated host VFS controls.
Plain streams keep the shell editor without terminal attribute changes.
The shell and commands emit LF text line endings, expanded to CRLF by the
TTY's preserved output processing. Plain-stream output also uses LF.

## Build

From this directory:

```sh
make -j8 mkfs
make -j8
# Or select one target:
make -j8 -f make/stm32g474re_nucleo64.make mkfs
make -j8 -f make/stm32g474re_nucleo64.make
# Or build the H5 target using the same ROMFS:
make -j8 -f make/stm32h563zi_nucleo144.make
```

Requires the normal ARM GCC toolchain and Python 3 for `tools/mkromfs`.
The explicit `mkfs` target builds and stages every deployable sandbox ELF
listed in `os/sb/apps/common/manifest.mk`, then generates `source/bin_romfs.c`
and `source/bin_romfs.h`. The ELFs are embedded unchanged, including the
relocation sections required by the sandbox loader. No manual staging or card
preparation is needed.

Run `mkfs` before the first firmware build and again after changing applications
or the manifest. Use separate make invocations as shown above: `autobuild.mk`
discovers the generated source/header when the firmware makefile is read.
Normal builds compile the existing image without rebuilding the apps.
Firmware outputs are `build/<target>/ch.elf`, `ch.hex` and `ch.bin`.

The generated sources are shared by all targets. `make clean` retains them.
Repeating `mkfs` leaves unchanged files untouched; removed manifest entries
disappear from the next image.

```sh
make clean
# Optional: also clean the shared sandbox app build products.
make -C ../../../os/sb/apps clean
```

## STM32G474RE Nucleo-64

- Console: ST-LINK virtual COM port, LPUART1 on PA2/PA3 (AF12), 38400 baud, 8N1.
  Disable local echo in the terminal emulator; the shell editor or canonical
  TTY supplies echo.
- LED: the board's green LED blinks while the host runs.
- Flash: the complete app set and host share the 512 KiB internal flash.
- RAM: the local linker script uses the contiguous 128 KiB SRAM1/SRAM2/CCM
  mapping. The default 48 KiB extra shell heap request, plus the shell image
  and environment, rounds to a 64 KiB MPU region at `0x20010000`.
  The default device linker script's 80 KiB heap region is too small for this
  aligned allocation. No DMA peripherals are used by this demo.
- The shell and commands execute within that same sandbox region. Larger
  applications, nested shells and editor buffers are limited by its remaining RAM.

## STM32H563ZI Nucleo-144

- Console: ST-LINK virtual COM port, USART3 on PD8/PD9 (AF7), 38400 baud, 8N1.
  Disable local echo in the terminal emulator, as for the G474 target.
- LED: the board's green LED blinks while the host runs.
- Uses the ARMv8-M Mainline alternate port with syscalls and one switched MPU
  region. The target runs without TrustZone, as in the existing H563 demos.
- The standard `STM32H563xI.ld` linker script provides 2 MiB of flash and a
  contiguous 640 KiB SRAM heap/data region; no fixed sandbox partition is needed.
- The default extra shell heap request is 128 KiB, plus the shell image and
  environment. ARMv8-M sandbox allocations use 32-byte alignment, without the
  G474's power-of-two region constraint. Commands and editor buffers share this
  sandbox allocation; adjust `PORTAB_SHELL_HEAP_SIZE` in the target's `portab.h`
  if more space is needed.
- The same ROMFS and `/dev/ttyS0` interface are used on both targets. Each target
  makefile provides `mkfs`; only one invocation is needed for the shared image.

## File systems and TTY checks

`/bin` is a read-only ROMFS containing all 13 current deployable apps:
`cat`, `chedit`, `cmp`, `cp`, `head`, `hexdump`, `ls`, `msh`, `sbsh`,
`sleep`, `stat`, `systime` and `wc` (each with an `.elf` suffix).
`/dev` exposes `ttyS0` and `null`. There is no writable file system;
commands which create or modify files cannot do so in `/bin`.

Suggested on-board checks:

1. `ls /bin`: check the complete command set; run `systime` and `stat /bin/msh.elf`.
2. Run `cat`, type a line and Enter, then press Ctrl-D on an empty line.
   The command should receive EOF and return to the shell.
3. Run `cat`, type text without Enter and press Ctrl-D: pending text should be
   delivered without including the Ctrl-D byte. Press Ctrl-D again to finish.
4. Exit the shell, including after using an app that changes terminal settings,
   and check that commands in the restarted shell inherit canonical input and
   echo. At the shell prompt, check Up-arrow history and Backspace editing.
5. Run `cat /dev/null` and `hexdump /dev/null`: both should return immediately
   without output. `sbsh -c "echo discarded > /dev/null"` should also return
   without command output; null-stream writes succeed and discard all data.

## Host-side shell regression tests

Run from this demo directory:

```sh
make -C ../../../os/sb/apps/msh/test check
make -C ../../../os/sb/apps/msh/test clean
```

These compile the actual shell code against native test shims and cover
editing, history, EOF/error restoration, interrupted terminal calls and reads,
plain streams, and preservation of noncanonical settings. A shared Linux
pseudo-terminal test verifies command-mode restoration using native `cat`,
Ctrl-D at both prompts and command input, and persistent `stty` changes.
`make -C ../../../os/sb/apps/sbsh check` runs the same PTY checks for `sbsh`.
ARM ELF execution and board hardware are not exercised by these native tests.

## Adding targets

Add `make/<target>.make` and a self-contained `cfg/<target>/` with the RT,
XHAL, MCU, sandbox and VFS configurations plus `portab.c/h`. Define the console
SIO, LED and extra shell heap request in `portab.h`. Select an appropriate
startup, board and linker script in the target makefile. Include
`$(CHIBIOS)/tools/mk/autobuild.mk` with the source modules and
`$(CHIBIOS)/tools/mk/romfs.mk` at the end to expose `mkfs`. Check both flash
capacity for the full ELF set and the MPU alignment constraints on dynamically
allocated RAM. The top-level Makefile discovers target `.make` files
automatically.
