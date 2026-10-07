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

The 29 OTG variants cover both controllers, FS/HS/ULPI settings, stepping 1,
configuration tables, BASEPRI, synchronization disabled and eleven real U5
registry/CMSIS selections. Sensing-enabled and no-sensing configurations cover
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
- Invalid host endpoint addresses and directions, including accesses beyond
  the OUT shadow array under UBSan, with teardown idle and in every phase.
- CLEAR_HALT DATA0 reset for bulk/interrupt endpoints, including deferred OUT.
- Stop cancellation of EP0 waiters even with synchronization disabled.
- All 15 safety wait predicates/widths: completion during preemption, genuine
  timeout, counter wrap and the final returned register value. EP0 abort checks
  both registers again after expiry.
- Existing transfer splitting, FIFO copies/capacity, PHY delays, startup and
  runtime faults, SOF/resume behavior and asynchronous ISO IN recovery.

## Separate follow-ups

Classic HAL OTGv1 also omits the DATA0 reset when clearing endpoint halt.
Backport that fix separately; this development update changes XHAL only.
High-speed/ULPI and stuck-handshake fault coverage here is modeled, not
hardware qualification.
