# HAL OTGv1 PHY timing, EP0 ownership and FIFO preservation regressions

Run `make` in this directory, then `make clean`.

The test compiles the actual HAL USB headers and OTGv1 LLD. It checks that
EP0 configuration, transfer state, and incoming SETUP storage belong to each
driver, retaining the existing per-controller IN/OUT union and keeping the
upper-layer `setup[8]` buffer separate.

Dual-controller checks exercise incoming SETUP packets and opposite-direction
transfers on both controllers, including resetting one while preserving the
other's state. The matrix covers OTG1 only, OTG2 only, both controllers, wait
support, and the EP0-thread structure layout. AddressSanitizer and
UndefinedBehaviorSanitizer are enabled.

Reconfiguration checks cover repeated endpoint disabling and reinitialization
on every enabled controller. They verify that the allocator retains EP0's TX
FIFO reservation, nonzero TX FIFOs do not overlap it, EP0 control/transfer-size
registers and pending interrupts are preserved, and only nonzero endpoints and
their interrupt sources are disabled. Active-IN, active-OUT, and idle EP0 cases
also check that EP0's FIFO-empty interrupt mask is preserved.

PHY timing checks verify a delay before core reset, another after reset
completion, and waits after RX/TX FIFO flush completion. Each delay is one
microsecond, scaled from `SystemCoreClock`. Tests cover 48, 168 and 520 MHz,
plus a fractional-MHz frequency to check upward rounding. Shared reset
counters distinguish the pre-reset delay from the post-reset delay.

OSAL and upper-layer callbacks are stubs. Shared-memory registers and a helper
process model only self-clearing core reset and FIFO flush commands. This is
not USB protocol, interrupt-timing, or hardware validation. Endpoint-disable
handshakes are not modeled; interrupt registers record W1C writes without
emulating their effects.

## Validation

- All five host variants pass with sanitizers enabled.
- Using the original LLD from commit `c0e6bc76be`, the dual-controller test
  fails because SETUP reception on the second controller overwrites the first
  controller's packet.
- With the per-controller storage fix alone, the reconfiguration regression
  fails because endpoint disabling loses the EP0 FIFO reservation. Both fixes
  are needed for the full suite to pass.
- USB CDC builds pass with `USE_COPT=-Werror` for
  `stm32f407_discovery`, `stm32h743zi_nucleo144` (both controllers enabled),
  and `stm32l4r5zi_nucleo144` (OTG1 only).
- H743 also builds with `USB_USE_WAIT=TRUE`, smart build and LTO disabled,
  and `USE_OPT='-Og -ggdb'`. The same non-LTO build at `-O2` encounters an
  unrelated `-Wmaybe-uninitialized` warning for `interval` in
  `test/rt/source/test/rt_test_sequence_003.c`.

## H723 hardware smoke test (2026-09-23)

The HAL CDC target was flashed and verified on a NUCLEO-H723ZG using OTG2
with the embedded FS PHY, `-O2`, LTO, assertions, parameter checks, and the
state checker enabled. The original 1 MB flash image was backed up before
programming.

Before the timing fix, normal startup hung waiting for `GRSTCTL.CSRST` to
clear, before EP0/FIFO initialization. A debugger pause immediately before
writing `CSRST` allowed the same firmware to start and enumerate.
Temporarily selecting HSI48 instead of PLL3Q did not clear an already-stuck
reset. ST describes the required
10-PHY-clock delay between PHY selection and core reset in its
[USB reset FAQ](https://community.st.com/stm32-mcus-60/faq-troubleshooting-a-usb-core-soft-reset-stuck-on-an-stm32-151577).

The fix inserts a one-microsecond polled delay before asserting core reset
and replaces the fixed CPU-cycle waits after reset and FIFO flushes with
one-microsecond waits. These use the CPU clock, not the H7's slower AHB clock.
The optimized H723 image passes 520 cycles to each delay at 520 MHz.

After flashing the timing fix, without a breakpoint in USB initialization:

- CDC shell `info` and `threads` commands succeeded.
- `USBD2` reached `USB_ACTIVE`, configuration 1, with no kernel panic.
- EP0 configuration and SETUP pointers referenced the driver's own fields.
- FIFO allocations, in words: RX 0..255, EP0 TX 256..271, EP1 TX 272..303,
  EP2 TX 304..307; `pmnext` was 308.

The updated driver also builds with `-Werror` for F407. All five host variants
pass with the timing assertions and sanitizers enabled.

Automated OpenOCD reset/reconnect attempts were inconsistent: some completed
enumeration and CDC traffic, while others did not produce a new host USB
device within 20 seconds. In those timeouts `CSRST` was clear, not stuck as
before the fix. The reason for the reconnect failures is not established;
repeated-reset validation remains pending.

A subsequent user-performed cold power cycle passed: the device enumerated
as `0483:5740`, serial `800`, and three successive CDC `info`/`threads`
exchanges succeeded through separate serial opens. The responses identified
the NUCLEO-H723ZG, kernel 8.0.0, and the patched firmware build. This check
used only host USB enumeration and serial I/O, with no debugger connection,
register access, or reset commands. It validates one cold start, not repeated
power-cycle reliability or the earlier debugger-reset reconnect behavior.

This is only a startup/CDC smoke test, not hardware validation of endpoint
reconfiguration. Repeated SET_CONFIGURATION testing remains pending because
the host USB device node is not writable by the user. Cross-controller EP0
isolation also remains host-test-only: H723 has only one OTG controller.
