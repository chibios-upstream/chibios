# RT-SB-DYNAMIC-RAMBOX-MULTI

Multi-target XHAL sandbox host requiring no SD card. The host dynamically loads
`/bin/msh.elf`, connects stdin/stdout/stderr to the SIO-backed POSIX TTY at
`/dev/ttyS0`, waits for shell termination, finalizes the sandbox and restarts it.
Before each launch, `pttyReset()` restores terminal defaults and discards pending
input, EOF records, output, signals and flow-control state.

`msh` detects the typed TTY through `isatty()` and uses its canonical defaults:
the driver handles echo and line editing, LF ends a command, and an empty Ctrl-D
record exits the shell. The shell's own arrow-key history/editor remains available
on plain serial streams, but is not active on the canonical TTY. The shell and
commands emit LF text line endings, expanded to CRLF by the TTY driver.
Plain-stream output also uses LF, without application-side translation.
Sandbox termios calls are not yet wired up, so this detection does not query or
change terminal attributes; TTY descriptors are expected to use canonical input
and the default output processing.

## Build

From this directory:

```sh
make -j8 mkfs
make -j8
# Or select one target:
make -j8 -f make/stm32g474re_nucleo64.make mkfs
make -j8 -f make/stm32g474re_nucleo64.make
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
  Disable local echo in the terminal emulator; the TTY supplies echo.
- LED: the board's green LED blinks while the host runs.
- Flash: the complete app set and host share the 512 KiB internal flash.
- RAM: the local linker script uses the contiguous 128 KiB SRAM1/SRAM2/CCM
  mapping. The default 48 KiB extra shell heap request, plus the shell image
  and environment, rounds to a 64 KiB MPU region at `0x20010000`.
  The default device linker script's 80 KiB heap region is too small for this
  aligned allocation. No DMA peripherals are used by this demo.
- The shell and commands execute within that same sandbox region. Larger
  applications, nested shells and editor buffers are limited by its remaining RAM.

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
   and check that the restarted shell has canonical input and echo restored.

## Host-side shell regression tests

Run from this demo directory:

```sh
make -C ../../../os/sb/apps/msh/test check
make -C ../../../os/sb/apps/msh/test clean
```

These compile the actual shell code against native test shims and cover
canonical records, EOF, overflow, interrupted/error reads, plain-stream editing
and history, and LF output independently of descriptor type. A Linux
pseudo-terminal test checks driver echo/editing and Ctrl-D with the shell's
actual builtins. ARM ELF execution and board hardware are not exercised by
these native tests.

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
