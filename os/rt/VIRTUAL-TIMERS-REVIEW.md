# RT Virtual Timers — Correctness Review

**Status:** second cross-check complete. Nine behavioral defects and two
documentation defects are confirmed. The normal one-shot callback-rearm pattern
is correct and is not a finding. The report retains stable IDs while further
cross-checks continue.

| | |
|---|---|
| Date | 2026-08-03 |
| Branch / commit | `chibios-kernel-dev` @ `a874f3591c` |
| Main scope | `chvt.c`, `chvt.h`, delta-list helpers, periodic and tickless operation, continuous reload, callback-time mutation, SMP instance ownership |
| Also read | VT storm test, standard RT VT tests, timer port contracts, SMP RP2 alarm binding, VT/reload history from 2015 through 2024 |
| Verification | Source invariants, history, tree-wide use audit, and fixed-width arithmetic traces; no source build was required for this review-only pass |

## Proposed PR classes

The classes below are proposed change boundaries, not yet a merge order. The
three Class A defects share the callback/reload state machine and should be
fixed together so that one correction does not conceal another.

1. **Class A — callback rearming and continuous reload**

   Defects: VT-1, VT-3, and VT-4.

   Scope: recognize callback-side rearming, calculate tickless reload insertion
   against the current list base, bound the skipped-deadline recovery path, and
   add periodic and tickless callback-mutation tests.

2. **Class B — tickless range, configuration, and query APIs**

   Defects: VT-5, VT-6, VT-10, and VT-11.

   Scope: make next-event reporting saturating and adaptive-delta-aware, report
   unrepresentable near-full-range tickless delays through RFCU and saturate
   them to the furthest representable deadline, make the current-delta getter
   obey its no-suffix locking contract, and reject unrepresentable static delta
   settings.

3. **Class C — SMP ownership and callback dispatch**

   Defects: VT-2 and VT-7.

   Scope: define and assert same-instance timer affinity without enabling
   remote timer manipulation, and preserve the expired callback and argument
   before dropping the kernel lock.

4. **Class D — API documentation**

   Defects: VT-8 and VT-9.

   Scope: correct the initialization, continuous-callback, and reload-setter
   contracts. This class should follow the callback-state decision in Class A.

## Safe changes to take first

Here, "safe" means that the change does not alter normal deadline ordering,
continuous-timer phase calculations, delta-list coordinates, or physical alarm
ownership. Keep the changes independently reviewable and pair each behavioral
change with a focused regression test.

1. **VT-8 and VT-9 — documentation only (implemented)**

   Correct the initialization, continuous restart, and callback-only reload
   setter documentation. There is no source behavior change.

2. **VT-2 — snapshot callback dispatch under the lock (implemented)**

   Copy both `func` and `par` to automatic locals before dropping the kernel
   lock, then invoke the saved values. This restores the earlier dispatch
   invariant without changing timer insertion, removal, reload, or alarm
   programming.

3. **VT-1 — suppress duplicate automatic insertion (implemented)**

   After the callback, enter the automatic reload path only when `reload` is
   nonzero and the timer is still disarmed. This is the narrow corruption fix:
   an armed timer is an explicit callback-side replacement and must never be
   inserted again. Test normal continuous reload, continuous-to-one-shot rearm,
   and continuous-to-continuous rearm in periodic and tickless modes.

   Do not yet change reset's treatment of `reload`. The continuous-rearm-then-
   reset override case remains open until its API semantics are fixed together
   with the rest of the callback/reload state machine.

4. **VT-10 — lock the no-suffix current-delta getter (implemented)**

   Acquire the system lock around the mutable tickless `lastdelta` read. The
   periodic constant-return path remains unchanged. The tree currently has only
   two call sites and both use the no-suffix API from thread context.

5. **VT-5 — make the read-only timer-state query saturating (implemented)**

   Use `lastdelta`, saturate deadline addition at `TIME_INFINITE`, and saturate
   elapsed-time subtraction at zero. This changes only a query result and does
   not modify the timer list or alarm.

6. **VT-6 — report and saturate unrepresentable enqueue intervals (implemented)**

   As a second-stage bounded change, add the selected RFCU overflow event and
   saturate to `TIME_INFINITE` in both insertion paths. Representable delays are
   unchanged. Land this after the query fixes because it deliberately changes
   exceptional-path behavior and can invoke an application runtime-fault hook.

Defer VT-3 and VT-4 because they change tickless reload scheduling and ISR exit
behavior. Defer the remaining VT-1 reset semantics and the VT-7 diagnostic owner
lifetime until the callback state transitions are specified and tested as a
whole. Defer VT-11 until the exact maximum legal static delta, including the
alarm retry increment boundary, is stated explicitly.

## Findings summary

| ID | Classification | Modes | Summary |
|---|---|---|---|
| VT-1 | Partially fixed, high | periodic and tickless | Continuous callback-side rearming inserts the same timer twice |
| VT-2 | Confirmed concurrency defect | periodic and tickless, especially SMP | Callback function and argument are loaded after the kernel lock is dropped |
| VT-3 | Confirmed | tickless | A callback-created list base makes automatic reload late |
| VT-4 | Confirmed, high | tickless | Continuous timers with no future reload margin can keep the timer ISR from returning |
| VT-5 | Fixed arithmetic defect | tickless | `chVTGetTimersStateI()` can wrap instead of reporting a bounded interval |
| VT-6 | Fixed contract defect | tickless | Near-full-range delays could expire early when the list was nonempty |
| VT-7 | Confirmed, high | SMP | Timer operations use the caller's instance without recording the timer's owner |
| VT-8 | Confirmed documentation defect | all | `chVTObjectInit()` incorrectly says `chVTSetI()` needs no initialization |
| VT-9 | Confirmed documentation defect | all | Continuous callback and reload-setter documentation contradicts behavior |
| VT-10 | Fixed API atomicity defect | tickless | `chVTGetCurrentDelta()` read mutable state without locking |
| VT-11 | Confirmed configuration defect | tickless | `CH_CFG_ST_TIMEDELTA` can narrow to an invalid runtime value |

## Confirmed behavioral defects

### VT-1 — continuous callback-side rearming inserts an armed node again

Both ticker implementations unlink the expired timer and set
`vtp->dlist.next` to null before invoking its callback (`chvt.c:535-541` and
`chvt.c:578-594`). This is what makes callback-side timer reuse possible. The
tree already relies on that behavior for variable one-shot intervals: the VT
storm sweepers and several drivers call `chVTSetI(vtp, new_delay, ...)` from the
callback.

After a continuous callback returns, however, both ticker implementations test
only `vtp->reload` and unconditionally insert the timer (`chvt.c:543-546` and
`chvt.c:596-647`). If the callback called `chVTSetContinuousI(vtp, ...)`, that
call already inserted the same delta-list node and left `reload` nonzero. The
automatic path inserts it a second time.

The equal-deadline case is particularly direct. `ch_dlist_insert()` reaches the
already-linked node as its insertion point, so the duplicate check inside the
strict `<` scan does not fire (`chlists.h:626-639`). Inserting the node before
itself turns its links into a self-loop and disconnects the list header.

Rearming with `chVTSetI()` is correct: it sets `reload` to zero, so the automatic
reload is suppressed. Changing the next period with
`chVTSetReloadIntervalX()` is also correct while the timer remains disarmed in
its callback. The primary defective case is rearming the object while leaving
it continuous. There is a related override case: a callback can continuously
rearm the object and then reset that newly armed timer. Reset unlinks it but
leaves `reload` nonzero, so the original expiration's automatic path starts it
yet again. The explicit final reset does not win.

**Implemented corruption fix:** after the callback, automatically reload only
if `reload` is nonzero **and the timer is still disarmed**. An armed timer
represents an explicit callback-side replacement and is left untouched. This
prevents duplicate insertion in both periodic and tickless operation.

**Remaining VT-1 decision:** explicit reset must also cancel a pending
automatic reload. The small representation-compatible solution is to clear
`reload` when an armed timer is reset. If preserving the reload value across
reset is an intended contract, add a callback-generation or override marker
instead. The relevant state transitions are:

- one-shot callback to one-shot rearm;
- continuous callback to one-shot rearm with a variable delay;
- continuous callback to continuous rearm with a new delay;
- continuous rearm followed by reset in the same callback;
- one-shot callback converted to continuous with
  `chVTSetReloadIntervalX()`.

Run the same state-transition tests in periodic and tickless configurations.

### VT-2 — callback function and argument are fetched after unlocking

The ticker marks the timer disarmed and then drops the kernel lock before
evaluating `vtp->func(vtp, vtp->par)` (`chvt.c:535-540` and
`chvt.c:578-592`). A higher-priority interrupt, or another core after the global
lock is released, can legally observe the disarmed state and rearm the object.
`chVTDoSetI()` and `chVTDoSetContinuousI()` overwrite both `func` and `par`.
The pending expiration can consequently invoke the newly installed callback
immediately and lose the callback that actually expired.

Earlier VT code saved the callback in a local before dropping the lock. That
local disappeared when armed state moved from `func != NULL` to the delta-list
null sentinel; the need to preserve the dispatch target did not disappear with
it.

**Implemented fix:** copy both `func` and `par` to locals while the timer is still
protected by the kernel lock, then invoke the saved values after unlocking.
Combine this with the ownership decision in VT-7; preserving the callback alone
does not define cross-instance object lifetime.

### VT-3 — callback-created tickless base shifts the next continuous deadline

The tickless ticker correctly records the expired logical deadline in local
`lasttime` before invoking the callback. It also explicitly notes that the
callback can modify `vtlp->lasttime` (`chvt.c:574-589`). The automatic reload
path nevertheless calculates `delta` with `nowdelta` measured from the old,
local expiration deadline (`chvt.c:600-646`).

The failing sequence is:

1. A continuous timer is the sole timer and expires at time 100.
2. Its callback adds another timer at time 103. Because the list was empty,
   `vt_insert_first()` changes the list base to 103.
3. The callback returns at time 105; the continuous reload is 20.
4. The code correctly calculates 15 ticks remaining to the phase deadline at
   120, but stores delta 20 (`5 + 15`) in the list now based at 103.
5. The continuous timer therefore expires at 123, three ticks late.

The error is the distance between the old expiration base and the base created
inside the callback. The same issue occurs if a callback empties a previously
nonempty list and then starts it again.

**Proposed fix:** use elapsed time from the expired deadline only to calculate
the phase-preserving remaining `delay`. Immediately before insertion, calculate
a separate base delta from the **current** `vtlp->lasttime` to the captured
`now`. Alternatively, factor an enqueue helper that accepts the already-read
`now` and always translates a delay relative to that time into the current
list's coordinate system.

### VT-4 — zero remaining reload time can make the tickless ISR unbounded

When callback execution has already exceeded a continuous timer's reload, the
default RFCU path records `CH_RFCU_VT_SKIPPED_DEADLINE` and assigns zero to
`delay` (`chvt.c:605-619`). Equality reaches the same zero delay through the
normal subtraction path, without recording a fault. If another timer remains
in the list, the continuous timer is inserted at the current logical time
(`chvt.c:628-646`). The outer `while` immediately sees it as due and calls it
again before leaving `chVTDoTickI()`.

One overdue timer can cause a long catch-up burst while another future timer
keeps the list nonempty. Two continuous timers whose callbacks consume at least
their reload are sufficient for an unbounded alternating sequence: each timer
is reinserted already due, and the list never reaches the empty-list path that
returns after programming a physical minimum delay. With strictly overdue
callbacks and debug assertions enabled, the explicit assertion halts first;
with normal release assertions disabled, the RFCU recovery path continues into
this loop. Exact-deadline equality follows the unbounded path even with the
assertion enabled because the asserted `<=` condition still holds.

The comment says recovery proceeds with a minimum delay, but zero is only
raised to the physical minimum in the empty-list special case. It is not a
minimum delay in the nonempty case.

**Proposed fix:** when `nowdelta >= reload`, schedule the timer no earlier than
`now + vtlp->lastdelta`, reprogram the alarm, and leave the current timer
handler invocation. This gives due timers another interrupt opportunity without
repeatedly invoking the same callback in one ISR. Add tests with two continuous
timers whose callbacks advance simulated time exactly to and then past their
reload; verify bounded callbacks per interrupt and the RFCU record for the
strictly skipped case.

### VT-5 — `chVTGetTimersStateI()` wraps at both ends of its arithmetic

In tickless mode the function calculates:

```
(first_delta + CH_CFG_ST_TIMEDELTA) - elapsed
```

without saturation (`chvt.h:238-244`). Two independent wrap cases follow:

- A first delta of `TIME_INFINITE`, which the VT APIs explicitly accept as a
  normal interval, plus a configured delta of two becomes one in 32-bit
  arithmetic. With zero elapsed time the function reports one tick rather than
  the full-range interval.
- If interrupt or lock latency makes `elapsed` greater than the tolerated
  deadline, subtraction wraps to a very large interval rather than reporting
  zero.

The expression also uses the static `CH_CFG_ST_TIMEDELTA`. Since commit
`ac16028605`, the effective safe delta is `vtlp->lastdelta` and can increase at
runtime. The query can therefore disagree with the alarm policy even when no
arithmetic wraps.

**Implemented fix:** calculate the tolerated deadline with saturating addition,
subtract elapsed with saturation at zero, and use `vtlp->lastdelta`. The
documented tolerance saturates at `TIME_INFINITE` rather than wrapping. Added
focused tickless tests for a first delta equal to `TIME_INFINITE` and for a
query held past the tolerated deadline.

### VT-6 — near-full-range tickless timers can expire early

When a tickless list is nonempty, `vt_enqueue()` expresses a new delay relative
to the older list base by adding the elapsed `nowdelta` (`chvt.c:236-245`). If
that addition wraps, it stores `delay` itself as a delta from the old base. The
source comment accurately states the consequence: the timer triggers
`nowdelta` ticks early.

For a 32-bit example with list base 100, current time 200, and requested delay
`TIME_INFINITE`, the stored delta from the base is 4294967295. Relative to the
current time, the actual remaining interval is 4294967195: exactly 100 ticks
short. The callback contract says that `TIME_INFINITE` is allowed as a normal
time specification and does not document early expiry.

This is not the physical-timer chunking case. `VT_MAX_DELAY` correctly limits a
single hardware alarm when `sysinterval_t` is wider than `systime_t`; VT-6 is
the logical list-coordinate overflow before alarm programming.

**Implemented fix:** detect the unsigned addition overflow, report it through
the dedicated `CH_RFCU_VT_INTERVAL_OVERFLOW` fault, and saturate the
list-relative delta to `TIME_INFINITE`, the furthest deadline representable
from the current list base. If the application runtime-fault hook returns, this
provides the least-early representable fallback; strict applications can halt
from the hook. When VT RFCU collection is disabled, a debug assertion is used
as the fallback, consistently with the other VT runtime-fault paths.

The shared checked-add helper is used by both ordinary enqueue and automatic
continuous reload insertion. The timer APIs now document that a tickless delay
is exact only while adding the age of the current list base is representable;
otherwise the RFCU event is raised and the deadline is saturated. A focused
generated test inserts the boundary timer after the list base has aged and
checks both the fault mask and saturated fallback coordinate. An
RFCU-disabled, assertions-enabled build verifies the diagnostic fallback is
compiled.

### VT-7 — SMP timer operations have no owner but manipulate the current list

Each `os_instance_t` owns a separate `vtlist`, and the SMP RP2 tickless port
binds a separate hardware alarm to each core. `virtual_timer_t` contains no
instance owner (`chobjects.h:72-89`). Set, reset, remaining-time query, and tick
all select `currcore->vtlist`; alarm operations likewise select
`currcore->core_id` (`chvt.c:333-394`, `chvt.c:479-520`, and
`chcoresmp_timer.h:54-95`). No public timer API documents same-instance
affinity.

An armed timer can therefore be passed to `chVTReset()` or `chVTSet()` by a
thread on another core while correctly serialized by the global kernel lock.
The intrusive node still points into the original list, so unlinking can partly
operate on that list, but all header identity and alarm decisions use the
caller's list.

The smallest destructive case is the last timer on the original core. In
periodic mode, reset adds its delta to the original list header and then
restores the **caller's** header to `TIME_INFINITE`, leaving the original header
corrupted. In tickless mode it additionally fails to stop the original core's
alarm. `chVTGetRemainingIntervalI()` simply scans the wrong list and reaches
its "timer not in list" assertion. Re-setting an armed timer on another core
first performs the wrong reset and then migrates the damaged object.

**Preferred solution:** define timer affinity as same-instance-only and add an
`os_instance_t *owner` field to `virtual_timer_t` only when
`CH_DBG_ENABLE_ASSERTS` is enabled. The field is diagnostic state only: no
functional decision may depend on it. This gives constant-time detection with
no timer-size or execution overhead when assertions are disabled.

Initialize the diagnostic owner to null, set it to `currcore` when arming, and
assert `owner == currcore` before reset, replacement, remaining-time query, or
other owner-sensitive mutation. Clear it after an explicit reset. Keep it set
while the expiration callback is executing so that another core cannot claim
the temporarily disarmed object; after the callback, clear it if the timer
remains disarmed and retain or restore it if the timer is explicitly or
automatically rearmed. All owner-field accesses must be under the same
`CH_DBG_ENABLE_ASSERTS` conditional as the field itself.

Do not operate on the owning instance's list or alarm from a foreign core.
Document that an armed or callback-active timer may only be manipulated from
its owner instance; a fully disarmed timer has no affinity and may subsequently
be armed on another instance. Add assertion-enabled SMP tests that arm on core
zero and attempt query, reset, re-set, and callback-window rearm from core one,
plus a test that resets on the owner before safely arming the disarmed object on
the other core.

### VT-10 — `chVTGetCurrentDelta()` does not implement no-suffix locking

`chVTGetCurrentDelta()` has the ordinary no-suffix API name but directly reads
`currcore->vtlist.lastdelta` without acquiring the system lock
(`chvt.h:510-525`). `lastdelta` is mutable: the tickless alarm compensation
paths increase it from interrupt context (`chvt.c:198-203`, with the analogous
update in `vt_set_alarm()`). It is not declared volatile or atomic.

This is observably unsafe in a supported data model where
`CH_CFG_INTERVALS_SIZE` is 64 and the target CPU has 32-bit natural accesses. A
thread-context read can be interrupted between the words of a concurrent
64-bit update and return a value that was never stored. SMP adds a second
unlocked writer/read interleaving. The nearby `chVTGetSystemTime()` and
`chVTGetTimeStamp()` no-suffix APIs acquire the lock; only their X/I-class
forms assume the caller supplies the required context.

**Implemented fix:** `chVTGetCurrentDelta()` now acquires and releases the
system lock around the mutable tickless `lastdelta` read. The periodic
constant-return path remains lock-free. The tree has no locked-context caller,
so no additional I- or X-class API was introduced.

A generated tickless test exercises the thread-context getter and verifies the
adaptive delta is not below its configured minimum. The test is also compiled
with 64-bit intervals for a 32-bit Cortex-M4 target, covering the data model in
which the original unlocked read could tear.

### VT-11 — `CH_CFG_ST_TIMEDELTA` can narrow to zero or one

The configuration check rejects negative values and the literal value one, but
does not verify that `CH_CFG_ST_TIMEDELTA` fits the configured system-time and
interval types (`chvt.h:42-44`). Initialization then explicitly casts the
option to `sysinterval_t` (`chvt.h:541-543`).

For example, with 16-bit intervals, a configured value of 65536 passes the
preprocessor check and becomes zero at runtime. A value of 65537 becomes one,
even though one is explicitly invalid. The preprocessor still selects all
tickless code because it tests the original positive macro, so the compiled
mode and the runtime minimum delta disagree. When intervals are wider than
`systime_t`, a value that fits only the interval type can also reach
`chTimeAddX()` as a physical alarm delay that does not fit system time.

**Proposed fix:** reject any nonzero delta that is not representable as both a
runtime `sysinterval_t` margin and a physical `systime_t` alarm distance, and
exclude reserved or sentinel boundary values needed by the retry increment.
Add accepted tests at zero, two, and the chosen maximum plus rejected tests at
one and immediately above each representable boundary.

## Documentation defects

### VT-8 — the object-initialization note names the wrong setter

`chVTObjectInit()` says explicit initialization is unnecessary because
`chVTSetI()` initializes the object (`chvt.c:268-273`). `chVTSetI()` first calls
`chVTResetI()`, which reads `dlist.next` to decide whether the timer is armed
(`chvt.h:297-300` and `chvt.h:342-346`). Calling it on an uninitialized automatic
object therefore reads an indeterminate pointer and can attempt to unlink an
invalid list node. The setter's own precondition correctly requires prior
initialization.

The note appears to mean `chVTDoSetI()`, whose precondition is that the object is
not armed and whose implementation overwrites all fields needed before
insertion.

**Proposed fix:** name `chVTDoSetI()` in the note and state explicitly that the
ordinary `chVTSetI()`/`chVTSet()` forms require an initialized object because
they support replacing an armed timer.

### VT-9 — continuous and reload-setter contracts contradict the implementation

The public inline documentation for `chVTSetContinuousI()` and
`chVTSetContinuous()` says the timer is disabled after the callback and can be
disposed or reused (`chvt.h:391-393` and `chvt.h:419-421`). The implementation
and the lower-level continuous setter correctly say it is restarted. The public
text is copied from the one-shot API and is false for a callback that makes no
state change.

`chVTSetReloadIntervalX()` also says it does nothing outside a timer callback,
but the inline implementation writes `reload` unconditionally
(`chvt.h:448-463`). The API is intended exclusively for use from the callback
of the timer passed as `vtp`; the copy/paste error is the claim that calls from
other contexts "do nothing." Such calls are outside the supported contract, so
their incidental implementation effect must not be documented as API behavior.

**Selected solution:** documentation changes only. Document the actual
continuous restart behavior and the supported callback overrides: zero reload
stops, one-shot rearm replaces the automatic reload, continuous rearm replaces
it once VT-1 is fixed, and changing `reload` changes the next phase interval.
State clearly that `chVTSetReloadIntervalX()` may only be called from the
callback of `vtp`. Within that callback, zero suppresses automatic reload and a
nonzero value selects the next reload interval, including turning a one-shot
callback into a continuous timer. Its X-class suffix describes the locking
context in which the callback can invoke it; it does not permit use outside the
callback. Remove the false "does nothing" statement, but do not add runtime
context enforcement or otherwise change behavior for VT-9.

## Behaviors checked and not classified as defects

- Variable one-shot rearming from the callback is valid and works in both
  ticker implementations because `chVTSetI()` clears `reload`.
- Calling `chVTSetReloadIntervalX(vtp, 0)` in the active callback correctly
  stops a continuous timer.
- Equal-deadline timers are represented by a zero delta following the first
  timer and are consumed in the same ticker invocation. No FIFO ordering is
  promised for equal deadlines.
- Normal reset preserves the delta-list cumulative deadline by transferring the
  removed node's delta to its successor. Restoring the header after removing the
  last timer is intentional in same-instance operation.
- `chVTGetRemainingIntervalI()` saturates an overdue tickless timer at zero.
- Hardware-alarm chunking through `VT_MAX_DELAY` prevents a wide interval from
  being passed directly to a narrower `systime_t` alarm. This is distinct from
  VT-6's now-handled logical list-coordinate overflow.
- A sole continuous timer that misses its deadline takes the empty-list path,
  programs a physical minimum delay, and returns. VT-4 requires another timer
  to keep the list nonempty.

## Test gaps exposed by the review

The standard continuous-timer test only increments a counter. The VT storm test
has extensive one-shot self-rearming and a continuous timer, but its continuous
callback does not mutate timer state and its full-range wrapper deadline is not
checked. Neither suite covers:

- continuous rearm followed by reset in the same callback;
- a sole continuous callback that starts another timer;
- two continuous callbacks that overrun their periods;
- runtime execution of VT-6's RFCU-disabled assertion fallback;
- callback-target replacement during the unlocked dispatch window;
- timer operations from a different SMP instance;
- runtime execution of the 64-bit current-delta getter test on a 32-bit
  tickless target;
- rejected `CH_CFG_ST_TIMEDELTA` narrowing configurations.

These should become focused regression tests rather than being folded only into
the long-duration VT storm.

## Implemented fixes

1. **VT-8 — object initialization documentation**

   Corrected `chVTObjectInit()` documentation to name `chVTDoSetI()` and
   `chVTDoSetContinuousI()` as the forms that initialize a disarmed object
   during insertion. Documented that the replacing `chVTSet*()` forms require
   an initialized object because they first inspect and possibly reset it.

2. **VT-9 — continuous timer and reload-setter documentation**

   Corrected `chVTSetContinuousI()` and `chVTSetContinuous()` to state that the
   timer is restarted after its callback. Documented that
   `chVTSetReloadIntervalX()` may only be called from the callback of the timer
   passed as `vtp`, that zero suppresses automatic reload, and that a nonzero
   reload turns a one-shot callback into a continuous timer. No runtime behavior
   was changed.

3. **VT-2 — protected callback dispatch snapshot**

   In both periodic and tickless ticker paths, copied the expired timer's
   callback function and argument to automatic locals while holding the kernel
   lock, then invoked those saved values after unlocking. A concurrent rearm can
   no longer replace the dispatch target or argument of the expiration already
   being processed.

   Verification completed with `git diff --check`, the full periodic simulator
   RT and OSLIB test suites, and an STM32G474 tickless VT storm build. All checks
   passed and generated build artifacts were cleaned.

4. **VT-1 — duplicate automatic reload insertion guard**

   In both ticker paths, automatic reload now requires a nonzero reload value
   and a timer that is still disarmed after its callback. An explicit
   callback-side replacement is therefore left in place and is not inserted a
   second time.

   Added generated RT tests for continuous-to-continuous self-rearm and
   continuous-to-one-shot replacement, updating both `configuration.xml` and
   its checked-in generated source. XML validation, regeneration, and
   `git diff --check` passed. The full periodic simulator RT and OSLIB suites
   passed, including the new cases, and the STM32F407 tickless test-suite demo
   built successfully. Generated build artifacts were cleaned.

5. **VT-5 — saturating tickless timer-state query**

   `chVTGetTimersStateI()` now uses the adaptive `lastdelta` value, saturates
   tolerated-deadline addition at `TIME_INFINITE`, and saturates elapsed-time
   subtraction at zero. Normal representable query results are unchanged.

   Added generated tickless tests for maximum-interval addition and an overdue
   query, updating both `configuration.xml` and its checked-in generated source.
   XML validation, regeneration, and `git diff --check` passed. The full
   periodic simulator RT and OSLIB suites passed, and the STM32F407 tickless
   test-suite demo containing the new conditional test built successfully.
   Generated build artifacts were cleaned; execution of the new tickless-only
   test still requires a supported tickless hardware target.

6. **VT-6 — reported and saturated tickless interval overflow**

   Added `CH_RFCU_VT_INTERVAL_OVERFLOW` and a shared checked-add helper for
   conversion from a current-time delay to a tickless delta-list coordinate.
   Both ordinary insertion and automatic continuous reload use the helper.
   Overflow invokes the RFCU path and saturates at `TIME_INFINITE`; when VT
   RFCU collection is disabled, the same condition uses a debug assertion.

   Added API documentation for the exceptional tickless range contract, a
   generated RT test using an aged nonempty list, and VT storm recognition of
   the new fault bit. XML validation, regeneration, and `git diff --check`
   passed. The full periodic simulator RT and OSLIB suites passed, the normal
   STM32F407 tickless test-suite image and STM32G474 VT storm image built, and
   an RFCU-disabled/assertions-enabled STM32F407 variant built without
   warnings. Runtime execution of the focused tickless-only test still
   requires a supported hardware target. Generated build artifacts were
   cleaned.

7. **VT-10 — atomic current-delta getter**

   `chVTGetCurrentDelta()` now locks around the mutable tickless `lastdelta`
   read, preventing interrupt or SMP interleaving and torn wide reads. The
   periodic compile-time-constant branch remains unchanged and lock-free. No
   locked-context form was added because both tree call sites are ordinary
   thread-context calls.

   Added a generated tickless getter test, updating both `configuration.xml`
   and its checked-in generated source. XML validation, regeneration, and
   `git diff --check` passed. The full periodic simulator RT and OSLIB suites
   passed, the STM32G474 tickless VT storm built, and the STM32F407 tickless
   test-suite image built with 64-bit intervals on its 32-bit Cortex-M4 target.
   Runtime execution of the new tickless-only test still requires a supported
   hardware target. Generated build artifacts were cleaned.
