# HAL USBv1 host regression

This harness compiles the actual HAL USB frontend and USBv1 LLD with the
in-tree STM32G474 or STM32F303 CMSIS header. OSAL, RCC, NVIC, peripheral
registers, and packet memory are backed by host test scaffolding. No
firmware is flashed.

Run from this directory, `CHIBIOS=` selects another tree:

```sh
make -j4
make negative
make clean
```

Ten variants: normal and fast copies, isochronous endpoints, both options
together, synchronous wait support, the EP0 worker-thread API and disabled
OSAL debug checks on the dense PMA (two 16-bit words per 32-bit location,
STM32G4/L4/F0); normal and fast copies and isochronous endpoints on the
sparse PMA (one 16-bit word per 32-bit location, STM32F1/F3). The PMA is
mapped below 4 GB, the LLD computes descriptor addresses in 32 bits.
AddressSanitizer and UndefinedBehaviorSanitizer are enabled with fatal
diagnostics and `-Wall -Wextra -Werror`.

## Coverage

- OUT packet sizes 1 through 64 in the ordinary variants and up to the PMA
  room in the isochronous variants. Allocations are compared with the
  capacity decoded from COUNT_RX: every byte the hardware can write is
  reserved, including the 32-byte block rounding above 62 bytes. A
  following endpoint does not overlap it, both isochronous descriptors
  point at the same reservation.
- Endpoint disable/reinitialization preserves EP0 descriptors, buffers,
  and register configuration, and reuses only the non-EP0 area.
- SETUP destinations at all four byte alignments, packet-copy sizes
  0 through 128 with fast-copy and halfword tails, and buffer sentinels.
- Multi-packet bulk IN/OUT completion through the real HAL frontend and
  callbacks; an OUT packet larger than the room left in the transfer, a
  full packet at the end of a transfer or one shorter than a packet, is
  truncated to the transfer (AddressSanitizer guards the buffer end).
- Endpoint events already served (CTR_TX/CTR_RX clear) are ignored.
- Isochronous counters: an OUT packet is counted in the buffer DTOG_RX does
  not select; an IN packet sets both counters and discards the event of a
  token already answered.
- Isochronous endpoints with no transfer active: an IN token completes
  nothing and leaves no packet, a sent packet is followed by zero-length
  packets, an OUT packet is neither copied to the last transfer's buffer
  nor counted in it.
- Suspend/wakeup through the low priority handler, including the line-state
  rejection of spurious wakeups; FSUSP is cleared before the wakeup event.
- Reset with stale suspend/SOF and endpoint-completion snapshots: the
  handler stops after the reset, the old snapshot is not served. The reset
  leaves the ERR interrupt masked, on a floating bus it fires continuously.

## Negative controls

`negative_controls.py` copies the HAL sources, reverts one fix at a time and
runs six variants; every mutation must fail the regression (a build
failure does not count). Covered: ERR interrupt mask, receive buffer
rounding, byte-wise SETUP copy, reset snapshot, OUT truncation, stale IN and OUT events, and the
isochronous fixes (IN idle completion, repeated packets, single counter,
stale event; OUT idle copy, counter selection).

## Limits and scope

The host storage does not emulate the write-one-to-clear and toggle bits of
EPR or the ISTR flags derived from the endpoints, USB bus timing, actual
PMA arbitration, enumeration, or a scheduling RTOS: the high priority
isochronous handler and the data toggle resets on endpoint initialization
and CLEAR_FEATURE(ENDPOINT_HALT) are not covered. The endpoint service
helper is called directly for packet-completion tests; the low priority
handler is used for reset and wakeup tests.
