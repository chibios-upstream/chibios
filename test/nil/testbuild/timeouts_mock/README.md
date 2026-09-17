# NIL timeout regression fixture

Run from the repository root:

```sh
sh test/nil/testbuild/check_timeouts.sh
```

The full suite covers NIL-1 accounting and the accepted NIL-2 timing contract:
alarm programming must finish before the selected deadline. Missed deadlines
are diagnosed by assertions, not recovered by retries or intermediate alarms.
The NIL-1 subset remains available separately:

```sh
sh test/nil/testbuild/check_timeouts.sh --rebase-only
```

The earlier restart-only subset remains available:

```sh
sh test/nil/testbuild/check_timeouts.sh --restart-only
```

The full suite is expected to pass with assertions both enabled and disabled.

The fixture compiles the real NIL scheduler, semaphore, event and message
sources with a deterministic port boundary. It controls the counter,
compare register, register-write latency and nested-ISR interleavings.
Context switches are intercepted; this is not a hardware timing simulation.
The shell runner builds in a temporary directory and removes its executable
on exit. Host GCC (or a compatible compiler selected using `CC`), 32-bit C
libraries and GNU Make are required (`gcc-multilib` on Debian/Ubuntu).

This 21.11 backport uses the branch's SIMIA32 kernel type declarations and
its `stkalign_t`/`PORT_SETUP_CONTEXT` port interface at the mock boundary.
It runs as a 32-bit host executable with intercepted context switches, not
as the actual IA32 simulator. The `-m32` build matches the stable message API's
use of the 32-bit `msg_t` type to pass thread pointers.
The focused `.github/workflows/nil-regression.yml` runs this fixture; master's
larger mechanical-check workflow is not imported into the stable branch.

The matrix covers 16/32-bit time, periodic/tickless operation (delta 0/2/10),
and assertions enabled/disabled, with parameter and state checks enabled.
Cases include:

- Simultaneous expiration, priority selection and S-class rescheduling.
- Queue/semaphore and suspension cleanup, cancellation in both orders,
  and cancellation by a nested interrupt during the timeout scan.
- First timed wait after a long inactive interval, including deadline zero.
- Full-range intervals added while another timeout is active; maximum-length
  waits retain alarm-based accounting across rollover and interrupt latency,
  without intermediate accounting alarms.
- The representable all-ones distance and the overflowing zero distance;
  repeated rebases preserving queue, suspension and sleep deadlines, also
  across counter wraparound.
- Consecutive deadlines one tick apart, without forcing the minimum initial
  delay onto handler rearming.
- Counter progress during initial programming, earlier-alarm replacement,
  rebasing and handler execution that stays strictly before the next deadline.
  A counter change alone must not trigger a diagnostic. Counter-read counts
  also verify that the new checks add no reads when assertions are disabled.
- Handler latency, scan overruns and compare-write overruns, including all
  four programming paths and rollover. Both equality with the programmed
  compare and passage beyond it must trigger a diagnostic.
- Overdue timeouts processed during rebasing and the alarm-start contract
  clearing a stale pending source before the kernel can receive it again.
- Infinite waits alongside finite waits.
- Alarm-started state after initialization, insertion, cancellation, handler
  execution and restart; cancellation alone must not clear the started flag.
- 1,000 ordinary and 1,000 mixed full-range four-waiter schedules per tickless
  configuration (16,000 in total in `--rebase-only` mode). Mixed schedules
  use staggered insertion and an independent 64-bit deadline model. Without
  simulated latency, their first deadline is after all four insertions;
  the named overdue-insertion case separately exercises pending-IRQ rebasing.

The out-of-budget cases use the fixture's halt hook and a separate `setjmp`
target to intercept the expected assertion halt: 36 cases per assertion-enabled
tickless configuration, 144 in total. An unexpected halt fails the executable
immediately. No intentional halt was added to the generated board regression
suite. NIL reports the asserting function, not the assertion's remark string;
the tests also check the written alarm and write count to verify that execution
reached the selected programming path.

With assertions disabled, the same out-of-budget probes verify that no
diagnostic runs and that programming is not retried. They do not establish
correct timeout behavior outside the supported timing budget. Each probe
resets the kernel model before continuing.

The former recovery-only expectations (automatically handling arbitrary scan
and write delays, retries, and 8,000 additional mixed-delay schedules) are not
part of this contract. Current tests exercise diagnostic detection instead of
claiming recovery from missed deadlines.

## Time helpers, context diagnostics and source selection

The same runner also covers NIL-3 through NIL-5:

- `chTimeAddX()` wrapping is checked as a direct expression and as a wider
  static initializer, so destination narrowing cannot hide an incorrect sum.
  Tests also check the result type, constant-expression initialization and
  single evaluation of both arguments.
- S-class entry checks are tested from an unlocked thread and a locked ISR.
  I-class entry checks are tested from unlocked thread/ISR contexts. Immediate
  suspension and empty-reference resume are included. After every expected
  halt the fixture verifies that references, queue counts, thread state,
  timeouts and scheduler selection were not modified.
- NULL reference/queue parameters are rejected before dereferencing. These
  context/parameter tests add 20 intercepted halts per configuration, 240 in
  total. Parameter and state checking remain enabled independently of the
  assertion setting. The full suite therefore intercepts 384 expected halts
  when combined with the 144 deadline-skip probes above.
- Valid I-class resume/dequeue calls retain deferred scheduling; S-class
  wakeup/resume still reschedule. Immediate suspension and empty-reference
  resume remain valid in their documented contexts.
- `sources.mk` parses the real `os/nil/nil.mk` with smart build both enabled
  and disabled and verifies the four selected kernel sources. This catches
  source-list continuation regressions without needing an ARM compiler.

These tests are hand-written and independent of the generated NIL suite;
do not change the generated suite sources to maintain this fixture.
