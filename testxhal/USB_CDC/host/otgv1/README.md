# XHAL OTGv1 host regressions

Development-only harness; do not import it into `master` or stable branches.
Run from this directory with `make -j8`, then `make clean`. The harness uses
the repository containing it by default. To check another worktree:

```sh
make clean
make -j8 CHIBIOS=/path/to/chibios-worktree
make clean
```

Requirements: a C compiler supporting AddressSanitizer/UndefinedBehaviorSanitizer,
POSIX process/shared-memory APIs, make, awk and Python 3. LeakSanitizer cannot
run under a ptrace-based sandbox; run the sanitizer tests outside that sandbox.
Clean before changing CHIBIOS because generated HLD extracts are cached.

The 30 OTG variants cover both controllers, FS/HS/ULPI settings, stepping 1,
configuration tables, BASEPRI, synchronization disabled, isochronous support
disabled and eleven real U5 registry/CMSIS selections. All variants but
`no_iso` enable `STM32_USB_USE_ISOCHRONOUS`; in `no_iso` the isochronous tests
are compiled out, the remaining endpoints are bulk ones, and an isochronous
endpoint is rejected by a debug assertion (counted, not fatal) without
touching its registers. Sensing-enabled and no-sensing configurations cover
all three steppings, including ULPI and integrated HS PHYs at FS and HS.
The registry checks cover 35 device selections and
reject missing FIFO capacities and missing safety counter hooks.
Eight additional PHY variants compile the actual U5 platform start/stop helpers
and shared safety waits against each integrated-HS CMSIS device header. The
helpers and timeout constant are extracted from the selected CHIBIOS tree.

The actual LLD and safety sources are compiled, and selected generated HLD
functions are extracted verbatim by `extract_hld.awk`. The helper process only
models reset/flush/IN-disable self-clearing bits. Tests supply FIFO-pop and
write-one-to-clear transitions explicitly; this is not a USB bus emulator.

Regressions include:

- Stepping-specific VBUS bypass, normal hardware sensing, stale overrides,
  reserved stepping-1 GOTGCTL bits, ULPI PHY selection and sensing settings
  preserved across repeated connect/disconnect cycles.
- U5 PHY booster readiness immediately, after delay and after deadline-crossing
  preemption; genuine timeout, counter wrap, power-field rollback without
  overwriting unrelated changes, untouched clocks/registers and retry. The
  driver-level check uses an inaccessible OTG pointer during PHY failure to
  catch use of the normal shutdown path before the core is clocked.
- EP0 status/SETUP ordering, queued SETUP packets, aborted IN data and pending
  SETUP gating of starts/stalls.
- OUT global-NAK/disable/release ordering, deferred configuration and receive
  starts, stale packets/events, independently completing endpoints, temporary
  SOF subscription, cancellation and stuck-handshake timeouts.
- Lazy IN cleanup: suspend stops refill and requests disable without waiting
  or flushing, including EP0 and pending ISO recovery. A single 16-bit mask
  rejects stale XFRC/TXFE/ISO failure events before and after wakeup; poisoned
  transmit pointers catch use of returned buffers. New IN starts flush only
  after EPENA clears, and premature reuse fails closed. EP0 waits for a fresh
  SETUP to flush. Suspend uses the actual HLD, including waiter cancellation.
- Reconfiguration: one bounded deadline for all old IN transmitters precedes
  flushing and overlapping replacement FIFO allocation. Tests cover locked
  thread and ISR callers (the public API is I-class), two endpoints completing
  at different times, counter wrap and completion during preemption across the
  deadline, stuck disable/flush faults, reset/stop cleanup and EP0 isolation.
- Invalid host endpoint addresses and directions, including accesses beyond
  the controller endpoint limit under UBSan, with teardown idle and in every phase.
- Stall/clear writes discard sampled hardware commands; CLEAR_HALT resets
  DATA0 for bulk/interrupt endpoints, including deferred OUT. A later deferred
  stall preserves that pending DATA0 command in the software shadow.
- Stop cancellation of EP0 waiters even with synchronization disabled: inject
  reset/suspend/wakeup at drvStop's unlock before interrupts are masked, then
  exercise a high-priority worker before STOP is published. It must not park
  again. Start asserts rather than silently forgetting a leftover waiter.
- Two additional probes execute the actual CDC/audio EP0 workers extracted
  from their demo sources. They require exit with MSG_RESET on STOP/STOPPING
  cancellation, 100 ms sleeps on hardware failure until the application stops
  the driver, and immediate retry on bus-reset cancellation while running.
  These scripted checks verify blocking and exit policy, not
  preemptive scheduling; the LLD/HLD harness alone does not catch worker spins.
- All 15 safety wait predicates/widths: completion during preemption, genuine
  timeout, counter wrap and the final returned register value. EP0 abort checks
  both registers again after expiry.
- Existing transfer splitting, FIFO copies/capacity, PHY delays, startup and
  runtime faults, SOF/resume behavior and asynchronous ISO IN recovery.

## H723 suspend hardware evidence (2026-10-07)

NUCLEO-H723ZG, DBGMCU ID `0x10016483`, OTG2 with embedded FS PHY.
Compared the LLD at `54b742dc1b` with the array-backed IN retirement change on
`fix/xhal-otgv1-vbus`, using the same scratch CDC fixture and debug checks.
This historical hardware run predates both the compact bitmap checkpoint and
the subsequent lazy-flush simplification; it does not qualify those changes.
The fixture arms a 256-byte EP1 IN transfer with no host reads: 64 bytes
enter its 16-word FIFO and 192 bytes remain in the caller buffer.

Linux device-only runtime suspend keeps VBUS connected. OpenOCD Tcl reads
memory without halting the CPU. Two samples taken 1.25 seconds apart both
show Linux `runtime_status=suspended`, hardware `DSTS.SUSPSTS=1`, an
unchanged frame number, and `GOTGCTL=0x04cd0000` (B-session valid).

| During suspend | Old LLD | Pending IN retirement |
| --- | --- | --- |
| Driver state | USB_SUSPENDED | USB_SUSPENDED |
| EP1 DIEPCTL | `0xc0488040` (EPENA/EPDIS set) | `0x004a8040` (both clear) |
| DIEPEMPMSK EP1 bit | 1 | 0 |
| EP1 DTXFSTS | 0 words free | 16 words free (full depth) |
| in_disable_pending / in_disable_wait | Not present | 0 / 0 |
| Hardware-failure count / panic | 0 / NULL | 0 / NULL |

Resume returns to USB_ACTIVE without a bus reset. On the old LLD the next
transfer sends 64 stale `0xa5` bytes before the new payload. The patched LLD
returns only the new 256-byte pattern. The initial host check requested 512
bytes despite the raw HLD transfer not appending a ZLP; that check timed out
after receiving all 256 correct bytes and was corrected to request 256.

The optional EP0 case is also achievable: a vendor request times out on the
host before the fixture arms its delayed 64-byte reply. Before suspend,
EP0 EPENA=1 and DTXFSTS=0. During suspend, the patched LLD has EPENA=0,
DTXFSTS=16, no pending IN retirement and no hardware failure; the waiting
reply returns MSG_RESET. After resume a new device-descriptor request
completes normally, without bus reset or stale bytes.

This demonstrates that the patched disable/flush sequence completes while
suspended on this H723 FS core. No timeout-pausing workaround is required
by this observation; other PHYs/cores are not hardware-qualified by it.

## Lazy-flush hardware validation (2026-10-07)

The replacement was tested on the same H723, OTG2 embedded FS PHY, 520 MHz,
with assertions, parameter checks and the system-state checker enabled.
Timing counters exist only in a scratch copy of the LLD, not production.

- EP1 armed with 256 bytes, 64 bytes queued and no host reads. Device-only
  runtime suspend lasted over 1.25 seconds with VBUS present and a stationary
  frame counter. During suspend: USB_SUSPENDED, EPENA clear, DIEPEMPMSK zero,
  in_flush=7, and zero free words in EP1's 16-word FIFO (intentionally not
  flushed yet). No panic or hardware failure. Resume did not reset the bus;
  the first new transfer returned exactly the new 256-byte pattern, with no
  leading stale 0xa5 bytes.
- EP0 armed with a delayed 64-byte reply was also suspended for over 1.25
  seconds. During suspend EPENA cleared, the FIFO retained its queued data,
  refill was masked, and the waiter returned MSG_RESET. Subsequent SETUP
  cleaned EP0; the post-resume device descriptor completed without bus reset
  or hardware failure.
- 200 SET_CONFIGURATION requests ran concurrently with IN reads. Seventy-three
  teardowns found an enabled IN endpoint with FIFO data queued. Across 677
  bulk reads, 173312 bytes were received, with no host timeout/error or device
  hardware failure/panic. Maximum observed shared disable-poll interval:
  25513 cycles / 49.063 us. Maximum whole locked teardown, including command
  setup and FIFO flushes: 27624 cycles / 53.123 us. These are observed maxima
  for this run, not guaranteed worst-case bounds or HS/ULPI qualification.

The host fixture restored device PM policy and kernel interfaces, and restored
and verified the original 1-MiB flash image. The initial run stopped before
USB checks because LTO removed the clock symbol; an explicit volatile clock
probe corrected the fixture, and both repeated cases passed.

## EP0 worker lifecycle hardware validation (2026-10-07)

This historical run used the sleep-on-stop policy. It does not qualify the
subsequently selected exit-on-stop policy or worker recreation after joining.

NUCLEO-H723ZG, ID `0x10016483`, embedded FS OTG2, 520 MHz. Scratch wrappers
included the actual CDC/audio `main.c` workers unchanged, counting wait
returns and sleeps; production HLD/LLD sources were compiled directly.
Assertions, parameter checks and the system-state checker were enabled.
The EP0 worker ran at priority 130; a debugger-commanded lifecycle thread
ran at priority 127. OpenOCD sampled memory without halting during the tests.

Both applications completed five stop/restart cycles: three idle stops and
two during bulk IN/audio IN traffic. Before every stop the EP0 worker was
parked on its setup waiter. In all ten cases the worker observed MSG_RESET
while the driver was still STOPPING, then slept, allowing the lower-priority
thread to finish drvStop(). Readback showed STOP, ep0thread NULL and the
worker in CH_STATE_SLEEPING. Over a further 650 ms the control heartbeat
advanced while the worker slept at roughly 100 ms intervals. No orphan,
starvation, panic or USB_FLAGS_HW_FAILURE was observed.

All ten restarts re-enumerated and handled new SETUP requests. CDC verified
120 echo round trips across the run and 131280 bulk bytes before the two
traffic stops. Audio verified six two-second captures (96000 samples each),
including after the final restart: 440.000011 Hz, peak 8192, maximum adjacent
sample step 472, sine-fit residual RMS 0.481. The two deliberately interrupted
audio recordings reported host read errors when drvStop disconnected USB;
subsequent captures were clean.

Evidence and scratch sources: `tmp/otgv1-worker-hw.UQgrOF/hardware.log` in the
workspace alongside the worktrees. The first attempt stopped before testing
because a static debugger symbol was ambiguous; it restored the original
firmware. A uniquely named probe symbol fixed the fixture and the complete
repeat passed. The original 1-MiB flash image was restored and verified again
at the end (SHA-256 `3f095cb997c0aa5d01e38c11c513618e34b27577e67e0a39163758b281bd38a3`).
This qualified the earlier sleep-on-stop behavior on H723 FS, not HS/ULPI or
the still-deferred active-endpoint stall/ISO OUT recovery sequences.

## Exit-on-stop worker validation (2026-10-07)

The selected policy polls hardware failure with a 100 ms sleep and exits with
MSG_RESET on STOP/STOPPING. Both actual demo workers pass scripted checks for
fault polling followed by application stop, immediate exit on stop, and normal
bus-reset retries. All 30 LLD variants, eight PHY variants and 35 registry
selections pass. Target builds pass with `-Werror`: all 13 CDC targets, H723
audio with/without LTO and smart build, and H723 CDC without synchronization.
XML validates, and a second regeneration is unchanged.

On H723 FS, a priority-127 owner stopped and joined each priority-130 demo
worker, then restarted/rebound the driver and recreated the worker in the same
working area before reconnecting. Both demos completed five cycles (three
idle, two during IN traffic). Every worker exited while STOPPING; joining
returned MSG_RESET, the EP0 waiter cleared, and the thread remained FINAL
with no further waits or sleeps during a 650 ms stopped interval. The owner
heartbeat progressed, with no panic or hardware-failure event.

All ten restarts enumerated. CDC passed 120 echo round trips and checked
131280 bulk bytes before traffic stops. Six two-second audio captures retained
the clean 440 Hz tone (96000 samples each, sine-fit residual RMS 0.481).
Hardware failure polling itself is covered by scripted host tests, not physical
fault injection. HS/ULPI remain unqualified by this run.

Evidence: `tmp/otgv1-worker-exit-hw.9o8GMx/hardware.log` alongside the worktrees.
The host test continued through the Codex server restart and completed normally;
the original 1-MiB firmware was restored and verified, with the same SHA-256
as the preceding lifecycle run. Scratch wrappers only changed lifecycle
instrumentation; production demo workers and HLD/LLD were compiled unchanged.

## Separate follow-ups

The lazy IN retirement regressions require the matching source change in
the `fix/xhal-otgv1-vbus` worktree; use the CHIBIOS override above until that
change is imported.

The lazy-flush change starts from `ae2c9a1ad7` and leaves all OUT teardown code and
its register shadows unchanged. It adds only `uint16_t in_flush`, alongside
the existing ISO mask narrowed to 16 bits: no driver RAM increase over that
baseline (H723 356 bytes, L4R5 308 bytes). H723 smart/LTO CDC text is 31988
bytes versus 33036 for the earlier compact experiment. The 0--15 endpoint-mask range
is checked at compile time. No timer, restart queue or SOF subscription was
added for IN cleanup.

Suspend has no new polling. SETUP abort and FIFO flushing retain their
existing bounded waits. Reconfiguration remains I-class, including ISR use:
its IN-disable poll has a shared 1000-us deadline, followed by bounded FIFO
flushes. This is an explicit latency tradeoff, not a thread-only operation.
There is no assumed 20-ms grace period: reuse while EPENA remains set faults.

Classic HAL OTGv1 also omits the DATA0 reset when clearing endpoint halt.
Backport that fix separately; this development update changes XHAL only.
High-speed/ULPI and stuck-handshake fault coverage here is modeled, not
hardware qualification.
