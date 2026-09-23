# STM32 OTGv1 regressions for stable-21.11.x

Adapted backport of master commit
`1551e90420a58f01bc1b9ea4e25028f69790109b`
([PR #333](https://github.com/chibios-upstream/chibios/pull/333)).

Run `make` in this directory, then `make clean`.

## Stable-branch adaptation

The EP0 configuration, IN/OUT state union and SETUP buffer are private to
each enabled controller. Unlike the master implementation, they are not
added to `USBDriver`: the public headers, structure layout and USB API are
unchanged. Single-controller builds allocate only that controller's storage.

Disabling nonzero endpoints preserves EP0 operation, its interrupt sources
and its TX FIFO reservation. PHY settling before core reset and waits after
core reset/RX flush/TX flush use one-microsecond polled delays scaled from
`SystemCoreClock`. The legacy 21.11.x embedded-FS/ULPI startup sequence and
board configuration interface are retained.

## Regression coverage

The tests compile the actual 21.11.x HAL USB headers and OTGv1 LLD, with
AddressSanitizer and UndefinedBehaviorSanitizer enabled. Five variants cover
both controllers, OTG1 only, OTG2 only, synchronous waits, and external ULPI
in high-speed mode. The newer master EP0-thread option is not present in
21.11.x and is not part of this matrix.

- Legacy startup and controller-specific EP0 configuration selection.
- Cross-controller SETUP isolation and independent IN/OUT state, including
  resetting one controller while the other has an active transfer.
- Repeated disable/reinitialize cycles with active-IN, active-OUT and idle
  EP0: preserve EP0 registers, pending interrupts, FIFO-empty masking and
  FIFO allocation; disable only nonzero endpoints; keep FIFO reuse bounded.
- Delay placement before/after core reset and after RX/TX FIFO flushes,
  including 48, 168 and 520 MHz CPU clocks and upward rounding at a
  fractional-MHz frequency.

Negative controls were checked against temporary copies of the driver:
sharing the SETUP buffer fails the isolation assertion, removing EP0's FIFO
reservation fails the allocator assertion, and removing the pre-reset delay
fails the delay-count assertion. The unmodified backport passes all five
variants.

OSAL and upper-layer callbacks are stubs. A helper process models only
self-clearing reset/flush commands in shared-memory registers. FIFO popping,
write-one-to-clear interrupt effects and endpoint-disable handshakes are not
emulated. These tests are not USB protocol or physical timing validation.

## Firmware builds

From `testhal/STM32/multi/USB_CDC`:

```sh
make -j5 -f make/stm32h743zi_nucleo144.make USE_COPT=-Werror
make -f make/stm32h743zi_nucleo144.make clean
```

The same default build was checked for F407 Discovery, H723 Nucleo144 and
L4R5 Nucleo144. H723 additionally enables assertions, parameter checks and
the state checker through `UDEFS`. All use their existing configurations;
no templates or generated configurations were changed.

H743 also builds with `USB_USE_WAIT=TRUE`, `USE_SMART_BUILD=no`,
`USE_LTO=no`, and `USE_OPT='-Og -ggdb'`, and separately with
`BOARD_OTG2_USES_ULPI` and `STM32_USE_USB_OTG2_HS=TRUE`. The ULPI build is
compile coverage, not a ready-to-run HS CDC fixture.

Style checks pass on the changed C/H files. The production USB headers are
byte-for-byte unchanged from the stable branch base.

The stable backport has not been flashed. The successful H723 cold-start
and CDC smoke test reported for master PR #333 is not hardware validation
of this adapted 21.11.x build. Dual-controller hardware isolation, repeated
SET_CONFIGURATION, ULPI timing, and repeated cold/warm resets remain to be
tested on hardware.
