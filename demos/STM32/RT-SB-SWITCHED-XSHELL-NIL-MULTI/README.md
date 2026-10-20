# RT-SB-SWITCHED-XSHELL-NIL-MULTI

Multi-target XHAL sandbox host running two static sandboxes with switched MPU
regions:

- SB1 runs the ChibiOS/RT XSHELL client (`demos/various/SB-CLIENT-RT-XHAL-XSHELL`).
  It owns the console UART through VIO UART unit 0, VRQ 8.
- SB2 runs the ChibiOS/NIL client (`demos/various/SB-CLIENT-NIL-XHAL`). It
  owns the board LED through VIO GPIO unit 0. Its standard input, output and
  error are connected to `/dev/null`.

The host restarts a sandbox when it terminates and reports the exit code on
its serial port. The host port can be shared with the SB1 console, the host
starts it again before reporting because the sandbox cleanup stops it.

Targets: STM32G474RE Nucleo-64, STM32H563ZI Nucleo-144.

## NIL sandbox behavior

The NIL client blinks the LED at 1 Hz from a NIL thread. The NIL kernel and
the XHAL run unprivileged inside the sandbox:

- The NIL system tick, 1 kHz, is a periodic host alarm delivered on VRQ 0
  and handled by the XHAL VIO ST driver.
- The LED is accessed through the XHAL PAL driver, which forwards the
  operations to VIO GPIO unit 0. VIO units are local to each sandbox, the
  host maps unit 0 of SB2 on the board LED.
- The NIL idle thread releases the CPU to the host until a VRQ is pending.

The client is built with the NIL state checker, parameter checks, assertions
and stack checks enabled. A failed check halts the NIL kernel, the LED stops
blinking. The sandbox does not terminate in this case, the halt reason can
be inspected with the `(SB2)` debug configurations in `nil.dbg_panic_msg`.

The RT XSHELL client also toggles VIO GPIO unit 0 from its blinker thread,
SB1 has no GPIO units assigned so those operations are ignored by the host.

## Virtual services

SB2 serves the "upper" service to SB1 through VSRV, see
`os/sb/note_sb_vsrv.md`: it converts a text in the client slice to upper
case.

- Host: the service definition with one slice of 256 bytes in host memory,
  SB2 can publish, SB1 can open with one handle. VSRV maps the area and the
  slice in the switched regions left free by code and data, the host uses
  4 switched regions (`PORT_SWITCHED_REGIONS_NUMBER`, `SB_CFG_NUM_REGIONS`).
- SB2: a second NIL thread publishes the service and serves the requests,
  woken by VRQ 20; the slices released by the client are acknowledged with
  reset done.
- SB1: the `vsrv` command, replies on VRQ 21:
  - `vsrv upper <text>`: command structure and text in the slice, the
    server validates both against the slice and replies the length.
  - `vsrv info`: protocol version and number of conversions.
  - `vsrv bench`: 1000 NOP round trips, about 41 us each on the G474.
  - `vsrv fault`: the server faults on purpose, the host restarts SB2 and
    SB1 gets "service lost"; the service is back after the restart.
  - `vsrv close`: closes the handle, opened on first use.
- A fault or an exit of SB1 with the handle open releases the slice, the
  restarted SB1 opens it again.

The same XSHELL client runs on hosts not defining the service, there the
`vsrv` command reports the error code of the call.

## Build

From this directory:

```sh
make -j8
# Or select one target:
make -j8 -f make/stm32g474re_nucleo64.make
make -j8 -f make/stm32h563zi_nucleo144.make
```

The default goal of each target makefile also builds the sandbox images,
from their own projects, using the makefiles matching the target's sandbox
slots. The host output is `build/<target>/ch.elf`, the sandbox images are the
`sb.elf` files under the client projects' `build/<slot>/` directories. The
sandbox projects always use their own configuration and build directories,
also when the host ones are overridden on the command line.

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
  the NIL sandbox image, execution stops at the sandbox `main()`.

The sandbox images are located relative to the project location using
`${workspace_loc:/RT-SB-SWITCHED-XSHELL-NIL-MULTI}`, no paths need to be
edited. Build the target first, the configurations do not build the sandbox
projects separately.

Without the IDE all three images can be flashed using OpenOCD, for example on
the G474 target:

```sh
openocd -f board/st_nucleo_g4.cfg \
  -c "program build/stm32g474re_nucleo64/ch.elf verify" \
  -c "program ../../various/SB-CLIENT-RT-XHAL-XSHELL/build/128k-08040000-16k-20018000/sb.elf verify" \
  -c "program ../../various/SB-CLIENT-NIL-XHAL/build/128k-08060000-16k-2001C000/sb.elf verify reset exit"
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
  - `demos/various/SB-CLIENT-NIL-XHAL/build/128k-08060000-16k-2001C000/sb.elf`

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

  The NIL image uses the first 16k of the SB2 RAM region.
- Sandbox images:
  - `demos/various/SB-CLIENT-RT-XHAL-XSHELL/build/128k-081C0000-16k-20000000/sb.elf`
  - `demos/various/SB-CLIENT-NIL-XHAL/build/128k-081E0000-16k-20004000/sb.elf`

## Adding targets

Add `make/<target>.make` and a self-contained `cfg/<target>/` with the RT,
XHAL, MCU, sandbox, VIO and VFS configurations plus `portab.c/h`. Define the
host serial port, the SB1 console SIO and the LED port and pad in `portab.h`.
Provide a linker script defining the host and the two sandbox regions, then
select the sandbox images matching those regions in the `sandboxes` rule of
the target makefile. Add a build configuration in `.cproject` and the launch
configurations under `debug/`. The top-level Makefile discovers target
`.make` files automatically.
