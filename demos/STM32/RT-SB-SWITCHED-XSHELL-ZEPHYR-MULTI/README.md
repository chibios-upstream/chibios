# RT-SB-SWITCHED-XSHELL-ZEPHYR-MULTI

Multi-target XHAL sandbox host running two static sandboxes with switched MPU
regions:

- SB1 runs the ChibiOS/RT XSHELL client (`demos/various/SB-CLIENT-RT-XHAL-XSHELL`).
  It owns the console UART through VIO UART unit 0, VRQ 8.
- SB2 runs the Zephyr client (`demos/various/SB-CLIENT-ZEPHYR`). It owns the
  board LED through VIO GPIO unit 0. Its standard input, output and error
  are connected to a null stream, it has no file system.

The host restarts a sandbox when it terminates and reports the exit code on
its serial port. The host port can be shared with the SB1 console, the host
starts it again before reporting because the sandbox cleanup stops it.

Targets: STM32G474RE Nucleo-64, STM32H563ZI Nucleo-144.

## Zephyr sandbox behavior

At start the Zephyr client runs a self-test of the port for about 2.2
seconds, the LED is on during the test. After the test:

- LED blinking at 1 Hz: all checks passed.
- LED blinking at 5 Hz: at least one check failed.

The result is also kept in the `test_result` structure of the client, it can
be inspected with the `(SB2)` debug configurations. The Zephyr kernel runs
unprivileged inside the sandbox:

- The Zephyr system timer is the host system time and the host alarm,
  delivered on VRQ 0, the kernel is tickless. Ticks elapsed while the sandbox
  was not running are recovered.
- The Zephyr idle thread releases the CPU to the host until a VRQ is
  pending.

The client is built with assertions and the stack sentinel enabled. A fatal
error terminates the sandbox, the host reports the exit code and restarts
it, so the LED goes back to steady on while the self-test runs again. The
exit code is `0xA8000000` ORed with the Zephyr fatal error reason.

The RT XSHELL client also toggles VIO GPIO unit 0 from its blinker thread,
SB1 has no GPIO units assigned so those operations are ignored by the host.

## Build

The Zephyr image requires the Zephyr tree in `ext/zephyr`, or in
`ZEPHYR_BASE`, and the Zephyr Python environment, see `ext/zephyr_readme.md`.
The environment in `ext/zephyr-venv` is used if present, else the active
one. From this directory:

```sh
make -j8
# Or select one target:
make -j8 -f make/stm32g474re_nucleo64.make
make -j8 -f make/stm32h563zi_nucleo144.make
```

The default goal of each target makefile also builds the sandbox images,
from their own projects, using the makefiles matching the target's sandbox
slots. The host output is `build/<target>/ch.elf`. The XSHELL image is the
`sb.elf` file under the client project's `build/<slot>/` directory, the Zephyr
image is `build/<slot>/zephyr/zephyr.elf` in its client project. The sandbox
projects always use their own configuration and build directories, also when
the host ones are overridden on the command line.

The FPU is not used by default. `make USE_FPU=hard` builds the host with FPU
support and the sandbox images for the FPU, a host built with FPU support
also runs sandbox images not using it. Clean the host and the sandbox
projects when changing this option.

`make clean` cleans the host only. The sandbox images are cleaned from their
own projects.

## Flashing and debugging

The launch configurations under `debug/` connect to an OpenOCD server
started separately on `localhost:3333`, one set for each target:

- `(<target>)(OpenOCD, Flash and Run)`: erases both sandbox slots, loads the
  two sandbox images and the host image, debugs the host.
- `(<target>)(OpenOCD, Just Run)`: debugs the host without loading.
- `(SB1)` and `(SB2)` variants: same, using the symbols of the XSHELL or of
  the Zephyr sandbox image, execution stops at the sandbox `main()`.

The sandbox images are located relative to the project location using
`${workspace_loc:/RT-SB-SWITCHED-XSHELL-ZEPHYR-MULTI}`, no paths need to be
edited. Build the target first, the configurations do not build the sandbox
projects separately.

Without the IDE all three images can be flashed using OpenOCD, for example on
the G474 target:

```sh
openocd -f board/st_nucleo_g4.cfg \
  -c "program build/stm32g474re_nucleo64/ch.elf verify" \
  -c "program ../../various/SB-CLIENT-RT-XHAL-XSHELL/build/128k-08040000-16k-20018000/sb.elf verify" \
  -c "program ../../various/SB-CLIENT-ZEPHYR/build/128k-08060000-16k-2001C000/zephyr/zephyr.elf verify reset exit"
```

## STM32G474RE Nucleo-64

- Console (SB1): ST-LINK virtual COM port, LPUART1 on PA2/PA3, 38400 baud,
  8N1.
- Host messages: USART1 on PA9/PA10 (Arduino D8/D2), 38400 baud, 8N1. A
  separate USB-serial adapter is required.
- LED (SB2): the board's green LED on PA5.
- Memory map, see `ld/stm32g474re_nucleo64.ld`:

  | Region | Address      | Size  |
  |--------|--------------|-------|
  | Host   | `0x08000000` | 256k  |
  | SB1    | `0x08040000` | 128k  |
  | SB2    | `0x08060000` | 128k  |
  | Host   | `0x20000000` | 96k   |
  | SB1    | `0x20018000` | 16k   |
  | SB2    | `0x2001C000` | 16k   |

- Sandbox images:
  - `demos/various/SB-CLIENT-RT-XHAL-XSHELL/build/128k-08040000-16k-20018000/sb.elf`
  - `demos/various/SB-CLIENT-ZEPHYR/build/128k-08060000-16k-2001C000/zephyr/zephyr.elf`

## STM32H563ZI Nucleo-144

- Console (SB1): ST-LINK virtual COM port, USART3 on PD8/PD9, 38400 baud,
  8N1.
- Host messages: same port as the SB1 console.
- LED (SB2): the board's green LED1 on PB0.
- Uses the ARMv8-M Mainline alternate port, the target runs without
  TrustZone.
- Memory map, see `ld/stm32h563zi_nucleo144.ld`:

  | Region | Address      | Size  |
  |--------|--------------|-------|
  | Host   | `0x08000000` | 1792k |
  | SB1    | `0x081C0000` | 128k  |
  | SB2    | `0x081E0000` | 128k  |
  | Host   | `0x20010000` | 576k  |
  | SB1    | `0x20000000` | 16k   |
  | SB2    | `0x20004000` | 48k   |

  The Zephyr image uses the first 16k of the SB2 RAM region.
- Sandbox images:
  - `demos/various/SB-CLIENT-RT-XHAL-XSHELL/build/128k-081C0000-16k-20000000/sb.elf`
  - `demos/various/SB-CLIENT-ZEPHYR/build/128k-081E0000-16k-20004000/zephyr/zephyr.elf`

## Adding targets

Add `make/<target>.make` and a self-contained `cfg/<target>/` with the RT,
XHAL, MCU, sandbox, VIO and VFS configurations plus `portab.c/h`. Define the
host serial port, the SB1 console SIO and the LED port and pad in `portab.h`.
Provide a linker script defining the host and the two sandbox regions, then
select the sandbox images matching those regions in the `sandboxes` rule of
the target makefile. Add a build configuration in `.cproject` and the launch
configurations under `debug/`. The top-level Makefile discovers target
`.make` files automatically.
