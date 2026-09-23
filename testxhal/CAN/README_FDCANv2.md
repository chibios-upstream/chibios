# STM32H7 XHAL FDCANv2

The import adds the H7 message-RAM variant of FDCAN to XHAL, using the
existing CAN frontend and base-driver lifecycle. HAL FDCANv2 and XHAL
FDCANv1 are not changed.

## Driver behavior

- Classic CAN and CAN FD, with one logical transmit mailbox (the hardware
  FIFO) and two receive mailboxes (FIFO0 and FIFO1).
- FDCAN1/2 on H743 and H7A3, and FDCAN1/2/3 on H723, with compact RAM
  allocation for any enabled-instance combination. All enabled instances
  share the 2560-word message RAM; disabled instance numbers leave no holes.
- Shared RCC ownership: starting or stopping one controller does not reset
  or disable an active peer. Startup handshakes have bounded, thread-context
  waits and release the clock reference on failure.
- Classic DLC 9..15 is limited to eight data bytes, including when the
  controller also accepts FD frames. Remote classic frames copy no payload.
- Standard/extended range, dual-ID, and classic-mask filters, with count and
  identifier validation. Replacement clears unused entries. The extended
  filter mode mask is corrected to include both EFT bits.
- ISR callbacks run outside system locks. TX/RX waiters, cancellation, FIFO
  overflow, bus-off, warning, passive, and protocol-error notifications use
  the existing frontend facilities.

Stop/start is required to change hardware configuration. Configuration
validation/selection does not wait or touch hardware in X-class context.
With `CAN_USE_CONFIGURATIONS` enabled, a NULL start selects configuration
zero; otherwise supply an explicit configuration.

Set `CAN_USE_SLEEP_MODE` to `FALSE`: sleep/wakeup is not implemented and
enabling it produces a compile-time error. Time-triggered CAN, dedicated RX
buffers, and a TX event FIFO consumer API are not provided. Leave frame
`EFC` clear. Bus-off recovery is an application decision; stop/start is
available, not automatic recovery. Reapply filters after restarting.

Serialize `canSTM32SetFilters()` with lifecycle operations and quiesce
external traffic before replacing filters: the hardware reads filter RAM
concurrently, so replacement is not atomic. RX notification uses a FIFO
watermark of one; consumers must drain the notified FIFO.

The RAM layout and initialization/clock-stop sequence were checked against
ST reference manuals RM0433 (H743), RM0468 (H723), and RM0455 (H7A3).

## Host regression tests

From this directory:

```sh
make -j4 -C host/fdcanv2
make -C host/fdcanv2 clean
```

The harness compiles the actual CAN frontend, LLD, and IRQ fragments using
the in-tree H723/H743/H7A3 CMSIS headers. AddressSanitizer and
UndefinedBehaviorSanitizer are enabled. Its 11 variants cover all seven
nonempty H723 instance combinations, configurations enabled,
synchronization disabled, H743, and H7A3.

Checks include RAM bounds and peer isolation, all DLC values in classic/FD
modes, remote frames, both RX FIFOs, filter encoding/replacement/rejection,
IRQ vectors and masking, callback lock state, queue wake/reset behavior,
abort requests, bounded startup failures, and successful restart afterward.

The hardware and scheduler are mocked. These tests do not establish real
FIFO/interrupt timing, W1C register behavior, physical bus behavior, or
concurrent-thread correctness.

## Board loopback fixture

Build from this directory, selecting one target:

```sh
make -j4 -f make/stm32h743zi_nucleo144.make USE_COPT=-Werror
make -j4 -f make/stm32h723zg_nucleo144.make USE_COPT=-Werror
make -j4 -f make/stm32h7a3ziq_nucleo144.make USE_COPT=-Werror
```

Clean using the same makefile and `clean`. Each target owns its configuration
headers under `cfg/`; assertions, parameter checks, and the state checker
are enabled. CAN instance and priority options already exist in the H7
templates. The new headers were processed by the configuration updaters;
global H7 MCU configuration regeneration was verified to be idempotent.

The fixture uses internal loopback (no external transceiver required),
starts all available controllers, checks classic and FD frames with DLC
0..15 through standard and extended filters into both FIFOs, and stops
earlier controllers while later ones continue. Bit timing is for internal
loopback, not a prescribed external bus bitrate.

Debugger-visible results:

- `can_test_stage`: 1 = classic, 2 = FD, 3 = completed.
- `can_test_result`: `0x13579BDF` = passed, `0x2468ACE0` = failed.
- `can_test_failure`: the failed check number in `main.c`.

## Validation status

Passed all 11 sanitizer host variants and seven `-O2 -Werror` ARM builds:

- H743, H723, H7A3: all available instances, smart build and LTO enabled.
- H743 FDCAN2-only, H723 FDCAN3-only, H723 FDCAN1+3: full build, no LTO.
- H735 `RT-XHAL-STM32-MULTI` demo: CAN disabled, full build, no LTO.

No firmware was flashed or executed on a board for this import. Internal
loopback, external transceiver tests, bus-off/recovery, and real concurrent
traffic still require hardware validation. Build outputs were cleaned.
