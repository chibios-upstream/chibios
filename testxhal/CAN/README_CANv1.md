# STM32L4+ CANv1 (bxCAN)

The XHAL CANv1 port supports CAN1 on the currently ported STM32L4+ platform:
three transmit mailboxes, two receive FIFOs and fourteen filter banks. It
does not enable CAN2/CAN3 or add other STM32 platforms. Classic HAL and the
vendor CMSIS headers are unchanged.

## Configuration and API

Enable `HAL_USE_CAN` and `STM32_CAN_USE_CAN1`. All four CAN1 vectors use
`STM32_IRQ_CAN1_PRIORITY`, owned by the shared `CAN/stm32_can1.inc`.
`STM32_CAN_REPORT_ALL_ERRORS` enables last-error-code interrupt reporting;
it defaults to FALSE because this can be IRQ-intensive.

Use `drvStart(&CAND1, &config)`, with `hal_can_config_t.mcr` and `.btr`.
The register helpers `CAN_BTR_BRP/TS1/TS2/SJW` take **encoded** values
(actual prescaler/segment length minus one). The CAN clock is PCLK1.
INRQ, SLEEP and RESET are driver-owned; reserved bits and SJW greater than
TS2 are rejected. There is no guessed default bitrate: a NULL initial
configuration requires a valid entry zero in `can_configurations`.
Hardware reconfiguration requires stop/start.

The driver bounds each startup acknowledgement wait to 250 ms and returns
`HAL_RET_HW_FAILURE` with CAN reset and its clock disabled on failure.
Leaving initialization requires bus synchronization; for normal bus use,
configure CAN RX/TX pins and provide a powered transceiver and a recessive
bus. This test's silent loopback mode needs neither.

Frames retain bxCAN's classic HAL layout: `DLC`, `RTR`, `IDE`,
`SID/EID`, and eight payload bytes. They are not the FDCAN frame layout.
DLC 9–15 transfers eight bytes, not a CAN-FD frame. RX remote-frame payload
bytes are not meaningful. Mailbox zero means any; TX mailboxes are 1–3 and
RX mailboxes 1–2 map to FIFO0/FIFO1.

`canSTM32SetFilters(&CAND1, count, filters)` replaces the complete filter
table **after start**, while READY, like the XHAL FDCAN API. The entries use
bxCAN's raw bank/mode/scale/FIFO/register fields. Unlike classic HAL's
four-argument setter there is no CAN2 bank-split argument. Zero entries
selects accept-all FIFO0. Duplicate/out-of-range banks are rejected before
register writes. Serialize with lifecycle operations and quiesce traffic
during replacement, because FINIT suspends reception. Restart resets the
table to accept-all, so reapply custom filters after every start.

Generic callbacks execute in ISR context, outside system locks. RX
notification is edge-style: drain the indicated FIFO until empty to rearm.
Sleep/wakeup use the common CAN API; manual wakeup records an event but
does not invoke an ISR callback from thread context. The accompanying HLD
fix wakes queued TX/RX waiters on both software and interrupt wakeup.
Only write-one-to-clear completion/status bits are acknowledged; abort and
FIFO-release bits are not replayed when clearing interrupts. Register
handling was checked against RM0432 Rev 8, chapter 55.

## Native regression tests

From this directory:

```sh
make -C host/canv1
make -C host/fdcanv2
make -C host/canv1 clean
make -C host/fdcanv2 clean
```

CANv1 compiles the real HLD, LLD and shared vectors against the bundled
L4R5/L4P5/L4S5 CMSIS headers. Eight variants cover synchronization and sleep
enabled/disabled, configuration tables and all-error reporting. The harness
uses ASan/UBSan and checks:

- Lifecycle, bounded startup failure/retry and balanced RCC enables/disables.
- All TX mailboxes, both RX FIFOs, identifiers, remote frames and DLC 0–15.
- Filter programming, validation and default restoration.
- Targeted W1C writes, TX error masks, overflow and disabled/stale interrupts.
- Callback context, queue wakeups, software/hardware wakeup and stop from sleep.

Registers and RTOS scheduling are mocked. This is not a peripheral simulator:
FIFO depth, hardware W1C side effects, arbitration, bus timing and electrical
behavior still require board tests.

## NUCLEO-L4R5ZI loopback target

```sh
make -f make/stm32l4r5zi_nucleo144.make -j8
make -f make/stm32l4r5zi_nucleo144.make clean
```

The standalone `canv1.c` fixture uses internal silent loopback and checks
standard/extended data and remote frames, DLC 0–8, all TX mailbox choices,
both RX FIFOs, rejection of unmatched identifiers, software sleep/wakeup,
and stop/restart. Assertions and the RTOS state checker are enabled.

Debugger observables:

- `can_test_result == 0x13579BDF`: passed.
- `can_test_result == 0x2468ACE0`: failed; inspect `can_test_failure`.
- `can_test_stage`: progress, with 7 indicating completion.

The target is build-verified, **not board-tested**. Bus-triggered wakeup,
real bus errors and external CAN traffic are not covered by this fixture.
Eclipse project `XHAL-CAN` includes this target and the existing three H7
FDCAN targets. Its default build uses the multi-target Makefile.
