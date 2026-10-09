# Classic HAL OTGv1 host regression

Run `make` in this directory, `make negative` for the negative controls,
then `make clean`. Use `make CHIBIOS=<tree>` to test another worktree; set
`ASAN_OPTIONS=detect_leaks=0` if the host's LeakSanitizer is noisy.

The test targets the classic OTGv1 port of the XHAL fixes, merged in #408.
It supersedes the retired stub-based test whose notes remain in `../otgv1`.

## What is real

The test compiles, unchanged, the classic USB driver `os/hal/src/hal_usb.c`
(control-transfer state machine, `default_handler`, EP0 worker API), the
STM32 OTGv1 LLD and `os/hal/src/hal_safety.c`. The U5 HS PHY helpers are
extracted verbatim from `os/hal/ports/STM32/STM32U5xx/hal_lld.c`.

Only the platform is modeled in `hal.h`:

- OSAL: lock state checked on every call (thread, ISR, X-class), a single
  thread whose `osalThreadSuspendS()` runs a test hook that plays the host
  until another context resumes it, a one-shot hook before a thread lock
  (an interrupt preempting the thread there), debug halts recorded only
  where a fault is expected.
- NVIC enable/disable/pend (both clear a pending request, as on ARMv7-M),
  RCC enable/disable/reset counters; an RCC reset clears the registers.
- Registers: two `stm32_otg_t` blocks. Self-clearing commands (core reset,
  FIFO flushes, global OUT NAK set/clear, IN/OUT endpoint disable, the
  isochronous OUT even/odd frame commands) complete
  only when the driver reads the timeout counter or performs a polled
  delay, or when a test lets time pass; each counter read is 10 us.
  Stuck commands, a busy AHB and endpoint disables that never end are
  injectable.
- W1C acknowledgment and RX FIFO popping are not emulated: tests supply each
  event (GINTSTS, DAINT, DIEPINT/DOEPINT, GRXSTSP) right before raising the
  interrupt and drop them afterwards. GONAKEFF follows the global OUT NAK
  status as a level. The interrupt handler only runs if an unmasked source
  is pending and the vector is enabled. RX entries are delivered through
  the ISR's FIFO handler, SETUP payloads are placed in the driver's packet
  buffer.

## Coverage

On every enabled controller, through the real core:

- Start/stop: clocks, vector priority and ownership, initial interrupt
  masks, no pull-up before `usbConnectBus()`, VBUS registers per stepping
  and PHY, ULPI clock gates, restart from READY does not reset the core,
  stop disconnects and masks. Each core reset handshake times out (AHB idle
  before/after, CSRST stuck), `usbStart()` returns `HAL_RET_HW_FAILURE`,
  releases clocks and vector, and can be retried. PHY delays before and
  after the core reset and after each FIFO flush, at four clock rates.
- Enumeration: bus reset, ENUMDNE turnaround, GET_DESCRIPTOR (short, ZLP
  after a full packet, multi-packet, truncated), SET_ADDRESS, SET/GET
  CONFIGURATION, GET_STATUS, stall of an unsupported request and recovery.
  FIFO layout after reconfiguration, EP0's FIFO stays reserved.
- SETUP ordering: an abandoned IN data stage is neither refilled nor
  completed while a SETUP is pending, it is disabled and flushed before the
  new request; a status IN or OUT completed before the SETUP is delivered
  first (the end callback runs once); an aborted OUT data stage never
  reaches the old request.
- Bulk: TX FIFO space gating, hardware-sized chunks with one callback
  (IN and OUT, above the packet count limit), ZLP, short packets, excess
  data drained (AddressSanitizer guards the buffer end).
- IN restart: a transfer started by the IN callback is filled in the
  completion interrupt (XFRC with the TX FIFO empty), as the old driver did;
  a callback that does not restart, or that disables the endpoints, leaves
  nothing to fill.
- Waiters (USB_USE_WAIT): an IN or OUT callback that disables the endpoints
  removes their configuration before the frontend resumes the waiter; the
  waiter is resumed once, with MSG_RESET by the disable, and the frontend
  does not read the configuration again.
- OUT teardown on reconfiguration: staged configuration and receive restart,
  old packets drained, global OUT NAK, EPDIS/EPDISD, release, SOF masked
  again, halt set/clear on the staged configuration, timeout reported as a
  fault.
- Suspend: transfers cancelled, refill stopped, stale completions ignored
  after wakeup, lazy TX FIFO flush on reuse, reuse of an endpoint whose
  disable never ended fails closed, EP0 waits for a fresh SETUP.
- Remote wakeup: RWUSIG during the sleep, the stale SOF acknowledged,
  resume detected by SOF.
- Faults: reported once from the pended vector as a suspend; transmit and
  receive waiters get `MSG_RESET` and new waits return at once (with
  `USB_USE_WAIT`); no reconnect, wakeup or receive restart until a restart;
  a second detection is a no-op; `usbStop()` clears the pended vector. A
  fault while the bus is already suspended adds no event, the driver stays
  suspended and a parked EP0 worker is released by `usbStop()`.
- Endpoint numbers beyond the controller never reach registers, from the
  host (stalled by the core) or from the stall API (LLD bound); endpoint
  requests on unconfigured endpoints or directions, or with reserved
  address bits, are stalled by the core. ENDPOINT_HALT set/clear
  (DATA0 for bulk/interrupt, none for isochronous), no command bits
  replayed by stall, clear or disable, EP0 registers kept by disable.
- EP0 worker SET_ADDRESS: a SETUP arriving before the commit, between the
  commit and the status stage, or after the status stage but before the
  worker resumes, neither applies a stale address nor loses the new one.
- IN disable barrier on reconfiguration (as XHAL): every old IN endpoint
  stops before any TX FIFO is flushed or reassigned, under one deadline,
  from a locked thread or ISR, including a completion seen just past the
  deadline; a disable or flush that never ends is a fault; EP0 untouched.
- Isochronous IN missed-frame recovery retired only by EPDISD, cancelled by
  suspend, flush timeout as a fault.
- Incomplete isochronous OUT (RM0468): only transfers due in the frame that
  ended are recovered, the endpoint is disabled under the global OUT NAK and
  the missed frame reported once, with no data, after EPDISD; no further
  reports for an endpoint not rearmed; a completion racing the disable is
  delivered normally; a teardown takes over; a stuck disable is a fault.
- The 15 safety waits recheck the register after a preempted deadline;
  the EP0 abort does the same for both of its conditions. Frame number at
  full and high speed. Connect/disconnect from any context. EP0 storage is
  per controller and SETUP, transfers and resets of one controller leave
  the other's EP0 untouched. On stepping 1 the pull-up follows B-session
  sensing on the embedded PHY, the soft disconnect without VBUS sensing or
  with an external ULPI PHY (DCTL resets to zero on stepping 1).
- U5 HS PHY: booster ready, delayed or ready while preempted, timeout with
  rollback of the startup-owned fields, clocks, retry, counter wrap.

Variants: both controllers, OTG1 or OTG2 only, `USB_USE_WAIT`, EP0 worker
(`USB_USE_EP0_THREAD`), the core's late SET_ADDRESS path (USBv1, USBv2)
with the default handler and the EP0 worker, steppings 1 and 3, no VBUS
sensing (steppings 1 and 2), ULPI at full and high speed (also on
stepping 1), FIFO fill
BASEPRI with the sequence workaround; U5 PHY on eight HS parts. All of them
enable `STM32_USB_USE_ISOCHRONOUS`; `no_iso` disables it: the third endpoint
is a bulk one, the isochronous tests are compiled out, an isochronous
endpoint is rejected by a debug assertion without touching its registers or
staged state, and the incomplete transfer interrupts stay masked.
AddressSanitizer and UndefinedBehaviorSanitizer are enabled.

## Registry checks

`make registry` compiles the real classic registries for 78 device
selections (F1 connectivity line, F2, F4, F7, H7, L4, L4+, U5) and checks
the OTG instances present, their endpoint counts and FIFO capacities:
1.25 Kbytes for the OTG_FS cores, 4 Kbytes for the OTG_HS cores, both
STM32H7 instances being OTG_HS cores. A registry without a capacity is
rejected.

## Negative controls

`negative_controls.py` copies the HAL sources, reverts one fix at a time and
runs nine variants, including `no_iso`; every mutation must fail the
regression (a build failure does not count). Covered: isochronous endpoint
rejected and interrupts masked without isochronous support, safety recheck, fault reporting and
idempotence, lazy IN flush, EP0 SETUP gating, status before SETUP, EP0 abort
recheck, endpoint bound, CLEAR_HALT toggle, stepping-1 GOTGCTL, teardown
drain, SOF masking, connect while faulted, wakeup SOF acknowledge, HS frame
number, restart from READY, vector release on stop, receive start while
faulted, command replay on disable, early ISO retirement, fault state
cleared on start, ISO OUT parity check, global OUT NAK, late completion and
teardown takeover, U5 PHY rollback and bounded wait, IN disable barrier
(wait, deadline recheck, flush), IN restart refill in the completion
interrupt, stepping 1 pull-up control without sensing
or with ULPI; in the core, the IN and OUT callback macros reading the
endpoint configuration after the callback (thread and received size), the
endpoint request check (whole, reserved bits,
direction) and the EP0 worker SET_ADDRESS commit (sequence check, completion
from the status stage, commit dropped by a new SETUP).

## Limits

This is not hardware validation and does not model USB timing, bus
traffic, real concurrency or interrupt preemption other than at the hook
points. The EP0 worker runs one iteration at a time on the test's stack.
