# HAL USBv1 double-buffered bulk regression

This harness compiles the actual HAL USB frontend and USBv1 LLD. Before
compiling, `epr_access.py` rewrites every `EPR` and `ISTR` access of
`hal_usb_lld.c` and `stm32_usb.h` into calls to a model of the peripheral.
The model implements the toggle and write-zero-to-clear bits, the
interrupt status derived from the endpoints, and the flow control of the
endpoints. A host model and an application drive two unidirectional bulk
endpoints with randomized traffic. No firmware is flashed. `../usbv2_dbl`
compiles the same regression against the USBv2 LLD.

Run from this directory, `CHIBIOS=` selects another tree:

```sh
make -j4
make SEEDS="1 30000"
make negative
make clean
```

## Peripheral model

The flow control follows the behavior measured on STM32G474. The test
firmware drove the endpoints directly and logged the endpoint register at
each transaction:

- A double-buffered endpoint is blocked when DTOG equals SW_BUF.
- The condition is evaluated when SW_BUF is written and at the end of the
  transactions. It is not evaluated at the end of the first transaction
  after setting DBL_BUF: the peripheral executes one transaction more, and
  the parity of the buffer flags allows one more after that. DTOG, STAT
  and DBL_BUF writes do not evaluate it.
- The STAT field of a blocked endpoint reads as NAK. The stored status
  does not change at the end of the transactions.
- Clearing DBL_BUF keeps the blocking condition: the endpoint stays
  blocked in the single-buffered mode, also after disabling it. Without
  DBL_BUF, a SW_BUF write leaving SW_BUF different from DTOG clears it and
  no write sets it.
- A single-buffered endpoint goes in NAK state after each transaction.
- The interrupt status reports the double-buffered and isochronous
  endpoints first.

## Scenarios

Each seed runs three scenarios, with a random regime: the weights of the
host transactions, of the interrupts and of the application, the
probability of a transaction completing at each register access of the
driver, and the probability of the high priority handler preempting the
low priority one.

- `stream`: an IN stream of transfers from 0 to 1000 bytes, and an OUT
  stream of host transfers from 0 to 300 bytes, with short packets and
  zero-length packets, received in transfers of 1 to 8 packets. The
  callbacks restart the transfers or leave it to the application thread.
- `clear`: the host also clears the halt of either endpoint at random
  times, it resets its data toggle and does not use the endpoint
  meanwhile.
- `stall`: the application stalls idle endpoints, the host clears them.
- `reconfig`: the host selects the configuration again while the
  endpoints are idle, they are disabled and initialized again without a
  bus reset. The summary counts these in the clears.

Checks:

- The host receives exactly the packets of the IN transfers, in sequence
  and with the expected data toggle; nothing more is sent afterwards.
- The OUT transfers deliver exactly the packets acknowledged to the host.
  The only exception is a held packet: received in the double-buffered
  entry state while no transfer is active, and discarded when the halt is
  cleared.
- Every run completes, there is no deadlock.

Variants:

- `dbl`, `dbl_fast`, `dbl_iso`, `dbl_release`: double buffering on the
  dense PMA, with fast copies, with isochronous support, without OSAL
  debug checks.
- `dbl_shared`: a single interrupt vector, as on STM32F0/L0.
- `dbl_sparse`: the sparse PMA of STM32F1/F3.
- `single`, `single_shared`, `single_sparse`: the same scenarios with
  double buffering disabled.

## Negative controls

`make negative` reverts one driver behavior at a time in a temporary copy
of the sources and checks that the regression fails:

- entry with one buffer or both packets in one buffer;
- the IN entry transfer not waiting for both packets, or counting a late
  event twice;
- OUT entry serving a late packet twice or releasing the buffer owned by
  the peripheral;
- packets received without a transfer not held, or not served when a
  transfer starts;
- a start releasing a buffer over a held packet or over a packet not
  served yet;
- STALL computed from the status read;
- leaving the double-buffered mode without writing SW_BUF, the endpoint
  stays blocked;
- initialization without clearing the blocking condition of a previous
  double-buffered use;
- the stored status or the data toggle kept when leaving the
  double-buffered mode;
- the IN buffer pointer not rewound, the packet not moved, or the first
  entry packet not accounted when the halt is cleared;
- a transfer in progress stopped by clearing the halt.

## Limits

The model does not cover enumeration, EP0, bus reset or suspend, or the
actual bus timing. Transactions on the cleared endpoint do not complete
while the halt is being cleared. The driver handles one event not yet
served at that point, a case the harness does not reach.
