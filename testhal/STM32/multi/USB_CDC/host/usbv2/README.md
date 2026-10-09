# HAL USBv2 host regression

This harness compiles the actual HAL USB frontend, USBv2 LLD, and in-tree
STM32H563 CMSIS header. OSAL, RCC, peripheral registers, and packet memory
are backed by host test scaffolding. No firmware is flashed.

Run from this directory, `CHIBIOS=` selects another tree:

```sh
make -j4
make negative
make clean
```

Seven variants enable normal/fast copies, isochronous endpoints, both
options together, synchronous wait support, the EP0 worker-thread API,
and disabled OSAL debug checks. AddressSanitizer and UndefinedBehaviorSanitizer
are enabled with fatal diagnostics and `-Wall -Wextra -Werror`. Native
execution outside a ptrace sandbox may be necessary for LeakSanitizer.

## Coverage

- OUT packet sizes 1 through 64 in the ordinary variants and 1 through
  1023 in the isochronous variants. Allocations are compared with capacity
  independently decoded from RXBD0, including the 62/63 and 64/65 boundaries.
- A following endpoint must not overlap the first endpoint's receive
  capacity. Both isochronous buffer descriptors must point at the same
  correctly sized reservation, preserving the existing overlapping-buffer
  policy.
- Endpoint disable/reinitialization preserves EP0 descriptors, buffers,
  and register configuration, and reuses only the non-EP0 area.
- SETUP destinations at all four byte alignments, packet-copy sizes
  0 through 128, fast-copy tails, and buffer sentinels.
- Multi-packet bulk IN/OUT completion through the real HAL frontend and
  callbacks; endpoint events already served (VTTX/VTRX clear, a stale ISTR)
  are ignored; an OUT packet larger than the room left in the transfer is
  truncated to it (AddressSanitizer guards the buffer end).
- Isochronous counters: an OUT packet is counted in the buffer DTOG_RX does
  not select (RM0481, Table 610); an IN packet sets both counters, the
  endpoint being always valid, and discards the event of a token already
  answered.
- Isochronous endpoints with no transfer active: an IN token completes
  nothing and leaves no packet, a sent packet is followed by zero-length
  packets, an OUT packet is neither copied to the last transfer's buffer
  nor counted in it.
- Suspend/wakeup state transitions, including the existing line-state
  rejection of spurious wakeups. The wakeup callback checks that SUSPEN
  has already been cleared.
- Reset with stale suspend/SOF and endpoint-completion snapshots. The real
  HAL reset invalidates endpoint pointers; the LLD must not use them afterward.

The allocation follows the block-size encoding documented by ST in
[RM0487, USB receive buffer descriptors](https://www.st.com/resource/en/reference_manual/rm0487-stm32u3-series-armbased-32bit-mcus-stmicroelectronics.pdf).
The wakeup handling was also checked against
[ST's STM32H5 PCD driver](https://github.com/STMicroelectronics/stm32h5xx-hal-driver/blob/main/Src/stm32h5xx_hal_pcd.c).

## Before/after probes

```sh
make probe PROBE=pma
make probe PROBE=setup
make probe PROBE=reset
make probe PROBE=wakeup
```

Override `LLD=/absolute/path/to/USBv2` and `BUILDDIR=/absolute/build/path`
to use another driver revision without reusing the patched binaries.
All four probes fail against the unchanged HAL LLD at `901fe18742`:
allocation mismatch, an unaligned SETUP store, a stale suspend event after
reset, and a wakeup callback with SUSPEN still set. All four pass against
the patched LLD, as does the complete seven-variant suite.

## Negative controls

`negative_controls.py` copies the HAL sources, reverts one fix at a time and
runs four variants; every mutation must fail the regression (a build failure
does not count). Covered: receive buffer rounding, reset snapshot, SUSPEN on
wakeup, OUT truncation, stale IN and OUT events, and the isochronous fixes
(IN idle completion, repeated packets, single counter, stale event; OUT idle
copy, counter selection).

## Firmware builds

The existing USB CDC test builds with `USE_COPT=-Werror` for
`stm32h563zi_nucleo144`, `stm32g0b1re_nucleo64`, and `stm32u385rg_nucleo64`.
From `testhal/STM32/multi/USB_CDC`, for example:

```sh
make -f make/stm32h563zi_nucleo144.make clean
make -j4 -f make/stm32h563zi_nucleo144.make USE_COPT=-Werror
make -f make/stm32h563zi_nucleo144.make clean
```

H563 also passes with `USE_OPT='-O2 -g -fomit-frame-pointer' USE_LTO=no
USE_SMART_BUILD=no` added to the build command. Clean each target after use.

## Limits and scope

The host storage does not emulate write-one-to-clear/toggle registers, USB
bus timing, actual PMA arbitration, enumeration, or a scheduling RTOS: the
data toggle resets on endpoint initialization and CLEAR_FEATURE(ENDPOINT_HALT)
are not covered, they were verified on hardware. The endpoint service helper
is called directly for packet-completion tests; the real top-level ISR is
used for reset and wakeup tests. The worker-thread variant checks its
frontend integration, not real blocking-thread behavior.

The driver fixes this harness was written for are in master; the
isochronous checks follow the fixes verified with the USB_AUDIO speaker on
H563. The USBv1 harness is in `../usbv1`.
