# RT Mutexes — Correctness / Priority-Inheritance Review

**Status:** fifth cross-check complete — the **RT core behavior has converged**
and no finding changed classification in round 5. Maintainer clarification
established that public S-class functions must reschedule internally and must not
return leaving a higher-priority thread ready. That resolves finding 4 in favor of
the current implementation and exposes the same stale contract plus a missing
reschedule in `chMtxUnlockS()` (finding 10). Findings 3, 5, and 9 still require
API-policy choices. Related abstraction-layer defects are recorded separately and
are not claimed to share the core fix.

Findings 0, 2, 3, and 10 are confirmed by executed reproducers, not by reading
alone; see Part 6 for the recipe and output.

Round 5 verified every round-4 claim against the sources (all four hold, one of
them correcting an earlier round-3 statement of mine — see finding 9), closed off
the obvious-but-broken alternative remedy for finding 10, and ran a tree-wide audit
of the clarified S-class rule. **Audit result: `chMtxUnlockS()` is the only
violation in RT and oslib** — see "Scope of the S-class rule" under finding 10. No
new defects were found in round 5; the finding set is believed complete for this
scope.

| | |
|---|---|
| Date | 2026-07-30 |
| Branch / commit | `master` @ `859a932f43` ("XHAL: remove redundant MMC SPI module switch (#157)") |
| Scope | `os/rt/src/chmtx.c`, `os/rt/include/chmtx.h` and everything the PI walk touches |
| Also read | `chschd.c`/`chschd.h`, `chcond.c`, `chmsg.c`, `chsem.h`, `chobjects.h`, `chthreads.c`, `chlists.h`, `chsys.h`, `test/rt/source/test/rt_test_sequence_008.c` |
| Verified on | `test/rt/testbuild` (SIMX86_64 host simulator), `CH_DBG_ENABLE_ASSERTS=TRUE` |

All line numbers below are anchored to `859a932f43`. Re-verify them if the tree moved.

---

## Part 1 — Verified portions of the PI core

The mutex dependency walk and unlock-time deboost algorithm were checked by
hand and hold under their normal invariant:

```
effective priority = max(real priority, all current mutex donations)
```

The walk itself is sound, but `chThdSetPriority()` can violate that invariant;
see finding 0. The findings in Part 2 are therefore not all peripheral.

- **Transitive boost** — `chmtx.c:194-247`. The owner→waiter chain walk is correct;
  the `continue`-on-`WTMTX` / `break`-otherwise structure makes it exactly
  O(nesting depth) as documented.
- **Cycle termination** — the walk cannot spin on an AB-BA deadlock. Each iteration
  either exits or sets a node's `prio` to the fixed value `currtp->hdr.pqueue.prio`,
  so a revisited node fails the `<` test. `currtp` appearing in the chain also
  terminates it immediately (`prio < prio` is false). Finite threads → finite walk.
- **Deboost math** — `chmtx.c:391-406`. `mtxlist` is popped *before* the scan, so the
  released mutex's own waiters correctly do not contribute. Queues are
  priority-ordered and re-sorted on every boost (`ch_sch_prio_insert`), so
  `queue.next` really is the maximum.
- **No lost boost on the empty-queue release path** — `chmtx.c:425-427`. Skipping the
  priority recomputation there is safe, *not* a bug: under the LIFO unlock constraint
  every remaining boost source is a waiter on a still-owned mutex, and mutex waits
  have no timeout (`chSchGoSleepS`, not `...TimeoutS`), so a waiter can never
  silently vanish from a queue.
- **Handoff** — `chmtx.c:408-423`. `chSchReadyI()` + `chSchRescheduleS()` instead of
  `chSchWakeupS()` is necessary (the unlocker has just lowered its own priority, which
  breaks `chSchWakeupS`'s precondition — see its assert at `chschd.c:407-409`); the
  comment at `chmtx.c:418-421` explains it correctly.
- **States deliberately ignored by the walk** — `CH_STATE_QUEUED`, `SUSPENDED`,
  `WTOREVT`/`WTANDEVT`, `SNDMSG`. Correct: those queues are FIFO
  (`chthreads.c:1159` uses `ch_queue_insert`) or the thread is in no queue at all, so
  raising `prio` needs no re-sort.
- **`chThdSetPriority()` interaction is not correct** — `chthreads.c:844-866`.
  Lowering the base priority across an already-present donor can lower the
  effective priority below that donor. See finding 0.
- **Recursive counter bookkeeping is only conditionally symmetric** — the `cnt`
  transitions balance while the value remains representable. There is no
  overflow protection, and balanced decrement/re-increment around a condition
  wait does not mean the mutex was actually released. See findings 3 and 9.

Worked scenarios that came out correct: simple boost/deboost; two mutexes with
waiters on both; three-thread transitive chain (H → T_mid → T_low) including the
`mtxlist` ordering after handoff; boost of a thread parked in `WTCOND` while owning a
second mutex; condvar timeout path.

---

## Part 2 — Findings

The identifiers are kept stable for future cross-check rounds. Findings 0 and
9 were added during the second cross-check; finding 10 was added during the
fourth. "Executed" means a reproducer was built and run on the host simulator
with assertions enabled (Part 6).

| ID | Cross-check result | Evidence | Summary |
|---|---|---|---|
| 0 | **Confirmed, high — converged** | executed | `chThdSetPriority()` can discard an existing donation |
| 1 | **Qualified — converged** | read + test 8.6.3 | Blocking self-lock is conventional; assert optional, and **not** in try-lock |
| 2 | **Confirmed — converged** | executed | Recursive foreign unlock mutates the wrong mutex, silently |
| 3 | **Confirmed — converged** | executed | Recursive condition wait sleeps while retaining ownership |
| 4 | **Confirmed documentation/class-check defect — converged** | read + maintainer contract | `UnlockAllS` implementation reschedules correctly; its `@post` is stale |
| 5 | **Confirmed — converged** | read | Thread exit can strand mutexes and enable dynamic-thread UAF |
| 6 | **Qualified, low — converged** | read | Layout coupling; second pass's fix is the better one |
| 7 | **Qualified, low — converged** | read | READY requeue violates helper/trace contract; SMP notification **is** needed |
| 8 | **Confirmed, documentation — converged** | read | Reverse unlock order does not prevent deadlock |
| 9 | **Confirmed, low — converged** | read | Recursive acquisition counter has no overflow limit |
| 10 | **Confirmed, high — converged** | executed + maintainer contract + tree-wide audit | `chMtxUnlockS()` can return with a higher-priority thread ready |

### 0. `chThdSetPriority()` can lower effective priority below an existing donor — `chthreads.c:844-866`

The required invariant for a mutex owner is:

```
effective = max(realprio, highest waiter on every owned mutex)
```

The current shortcut is:

```c
if ((currtp->hdr.pqueue.prio == currtp->realprio) ||
    (newprio > currtp->hdr.pqueue.prio)) {
  currtp->hdr.pqueue.prio = newprio;
}
currtp->realprio = newprio;
```

Consider an owner at base/effective priority 128 with a priority-127 waiter.
That waiter does not initially change the effective priority because 128 is
already higher. If the owner calls `chThdSetPriority(126)`, the first condition
is true and the effective priority becomes 126, even though the existing donor
requires 127.

**Trigger conditions** (both passes agree): the owner must be in the
*un-boosted* state `prio == realprio` — the first arm of the `if` — and lower its
base below a waiter already queued on one of its mutexes. The boosted arm is safe:
`prio` is left at the donation, which is ≥ every waiter by construction. So the
defect is confined to that one assignment.

**Executed reproducer** (`/tmp/pi-repro`, output in Part 6). Base 128, donor 127,
lowered to 100, plus an unrelated priority-120 thread — chosen to sit between the
new base and the donation, which turns the invariant violation into observable
inversion:

```
owner locked m1: effective=128 real=128
donor queued:    effective=128 real=128  (waiter head=127)
after SetPrio100: effective=100 real=100  -> expected effective 127
creating medium p=120 while still inside the critical section
  [medium p=120] RUNNING (preempted the mutex owner)
owner reached end of critical section, unlocking m1
  [donor  p=127] acquired m1
```

The priority-120 thread preempts the mutex owner, so the priority-127 donor waits
behind a lower-priority thread for an unbounded time. This is exactly the inversion
PI exists to prevent, reached through entirely valid API use.

Fix by recomputing from `newprio` and the highest-priority waiter on every mutex in
`mtxlist`, using the same queue-head rule as unlock-time deboost. Note that
`chmtx.c:391-402` and `chmtx.c:477-488` are already that scan, duplicated verbatim —
so the fix should factor it into one internal helper called from `chMtxUnlock()`,
`chMtxUnlockS()` and `chThdSetPriority()`, which removes the triplication rather than
adding a fourth copy. Add a regression test in which a thread lowers its base
priority across an already-queued donor.

**Related abstraction-layer setters require separate fixes, not just the core
recomputation helper.** `cmsis_os.c:185` and `osapi.c:2014` replicate the
conditional for arbitrary target threads, but changing the priority of a
thread blocked in `WTMTX` also has to propagate the changed donation through
the mutex-owner chain. Re-sorting only the target's wait queue is insufficient:
an increase can leave its owner under-boosted, while a decrease can leave the
owner and a transitive chain over-boosted.

There are additional independent defects in those setters:

- `cmsis_os.c:178-231` writes the CMSIS `osPriority` enum directly into an RT
  priority. Thread creation uses `NORMALPRIO + tpriority`, so the setter uses
  the wrong scale; `osThreadGetPriority()` at `cmsis_os.h:556-559` also
  subtracts in the opposite direction.
- `OS_TaskSetPriority()` at `osapi.c:2001` compares `rt_newprio` with the
  calling thread's priority and can return success without changing a
  different target thread.

These belong in separate abstraction patches. A helper that only recomputes a
target's donations from its own `mtxlist` does not solve arbitrary-thread
upstream donation/deboost propagation.

### 1. Non-recursive build: blocking self-lock has no diagnostic — qualified — `chmtx.c:179-252`

With `CH_CFG_USE_MUTEXES_RECURSIVE == FALSE`, `chMtxLockS()` on a mutex the caller
already owns takes the contended path: `tp = mp->owner` is `currtp`, so
`while (tp->hdr.pqueue.prio < currtp->hdr.pqueue.prio)` is immediately false, the
thread enqueues itself on its own mutex and calls `chSchGoSleepS(CH_STATE_WTMTX)` —
unwakeable, with **no diagnostic at any assert or hardening level**. This is the one
documented-illegal usage (`chmtx.c:48-52`) that has no check, while the recursive
build asserts on counter invariants throughout.

This is not necessarily a kernel correctness defect. A normal non-recursive
blocking mutex conventionally self-deadlocks. If ChibiOS wants stronger debug
diagnostics, the following assertion in `chMtxLockS()` is reasonable:

```c
  /* Is the mutex already locked? */
  if (mp->owner != NULL) {
#if CH_CFG_USE_MUTEXES_RECURSIVE == FALSE
    chDbgAssert(mp->owner != currtp, "mutex already owned");
#endif
```

Do **not** add the same assertion to `chMtxTryLockS()`. Its contract only says
that `false` means acquisition failed, and test 8.6.3 explicitly verifies that a
second try-lock by the owner returns `false` in the non-recursive build
(`rt_test_sequence_008.c:787-793`). An assertion there would break tested API
behavior.

*Round 3:* the try-lock objection is verified against the test source and accepted —
the earlier suggestion to assert in `chMtxTryLockS()` is **withdrawn**. Both passes
now agree: lock-side assert optional (it is consistent with how the kernel asserts on
other documented-illegal usage, but it fixes no defect), try-lock side unchanged.

### 2. Unlock asserts validate the wrong object — confirmed — `chmtx.c:370-371` and `chmtx.c:456-457`

```c
chDbgAssert(currtp->mtxlist != NULL, "owned mutexes list empty");
chDbgAssert(currtp->mtxlist->owner == currtp, "ownership failure");
```

The second assert restates a kernel invariant (the head of a thread's own `mtxlist`
is by construction owned by that thread) instead of checking the parameter `mp`. `mp`
is only validated at `chmtx.c:378` / `chmtx.c:464`, **inside** the
`if (--mp->cnt == (cnt_t)0)` block.

Consequence in the recursive build: `chMtxUnlock(mp)` on a mutex owned by **another**
thread with `mp->cnt >= 2` passes every assertion, decrements the victim's recursion
counter and returns. The victim's next unlock then releases the mutex one nesting
level early — ownership is handed to a waiter while the victim is still inside its
critical section. Silent even with full assertions enabled.

**Executed reproducer** (`/tmp/pi-repro2`, `CH_CFG_USE_MUTEXES_RECURSIVE=TRUE`,
assertions on). The offender owns an unrelated `m2` so that the `mtxlist != NULL`
assert passes, then unlocks a mutex it does not own:

```
owner took m1 twice: cnt=2
  [offender] calling chMtxUnlock(&m1) on a mutex it does NOT own
  [offender] returned with NO assertion. m1.cnt=1 owner=set
after offender:      cnt=1 (expected 2)
owner unlocked once: cnt=0 owner=NULL -> RELEASED EARLY (owner still expects to hold it)
```

Fix (both functions):

```c
chDbgAssert(mp->owner == currtp, "ownership failure");
```

which subsumes the intent of the existing check and catches the case.

### 3. `chCondWait*()` silently fails to release a recursively-locked mutex — confirmed — `chcond.c:245` and `chcond.c:333`

`chCondWaitS()` releases via `chMtxUnlockS(mp)`, which for `mp->cnt > 1` only
decrements the counter. The thread then sleeps in `CH_STATE_WTCOND` **still owning the
mutex**, so any thread that needs that mutex in order to reach `chCondSignal()`
blocks forever. The re-lock on the return path (`chcond.c:253`) increments the count
back, so the bookkeeping is symmetric — only the release never happens. The
documented behaviour is "Releases the currently owned mutex" (`chcond.c:219-221`).

**Executed reproducer** (`/tmp/pi-repro2`). A prober thread runs while the owner
sleeps in `WTCOND` after taking `m3` twice:

```
owner took m3 twice: cnt=2
  [prober] chMtxTryLock(&m3) while waiter sleeps in WTCOND -> FAILED (mutex still held by the sleeping thread)
  [prober] m3.owner==sleeper? YES  m3.cnt=1
owner woke from chCondWait, m3.cnt=2
```

The mutex is demonstrably still owned across the wait. In this reproducer the prober
signals the condition variable without needing the mutex; a realistic monitor, where
the signaller must hold the mutex, deadlocks outright.

There are two legitimate designs:

1. Prohibit recursive depth greater than one with a documented `@pre` and an
   assertion before the unlock.
2. Fully release the mutex, then restore the original recursion depth after a
   successful wait. The timeout path must continue to lose ownership as
   documented.

The first is the smaller change:

```c
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
  chDbgAssert(mp->cnt == (cnt_t)1, "recursively locked mutex");
#endif
```

Applies to both `chCondWaitS()` and `chCondWaitTimeoutS()`.

### 4. `chMtxUnlockAllS()`: stale documentation and missing class check — confirmed — `chmtx.c:517-518`, `chmtx.c:526`, `chmtx.c:552`

The `@post` reads "This function does not reschedule so a call to a rescheduling
function must be performed before unlocking the kernel", but line 552 calls
`chSchRescheduleS()`.

**Maintainer clarification in round 4 resolves the mismatch:** public S-class
functions are required to reschedule internally and must not return while a
higher-priority thread is ready. The implementation is correct; the `@post` is
stale and must be removed or replaced with wording that states the function
may reschedule internally.

Also `chMtxUnlockAllS()` is the only `@sclass` function in the file without
`chDbgCheckClassS()` (compare `chmtx.c:175`, `323`, `453`).

The reschedule moved into the S-class function in commit `1a2da63568` ("Fixed
bug #1076", 2020), when the API implementation was consolidated around
`chMtxUnlockAllS()`. The 2022 blame attribution is not the semantic origin.
*Round 3:* verified — `1a2da63568` (2020-03-20) rewrites 62 lines of
`chmtx.c`; the 2022 commit is the line-ending mass change that masked the
semantic blame.

The round-3 `chMtxUnlockS()` halt does not establish a caller obligation. Under
the clarified S-class rule it demonstrates a separate defect in
`chMtxUnlockS()` itself; it is now finding 10.

### 5. `chThdExitS()` does not reject owned mutexes — confirmed — `chthreads.c:702-737`

Nothing on the exit path touches `mtxlist`, and nothing documents or asserts a
prohibition. A thread exiting while owning a contended mutex leaves `mp->owner`
pointing at a `CH_STATE_FINAL` thread and its waiters permanently blocked. With
`CH_CFG_USE_DYNAMIC` the working area — which holds the `thread_t` — can then be
freed, after which any later PI walk dereferences freed memory: reads of
`tp->hdr.pqueue.prio` and `tp->state`, plus a **write** through
`tp->hdr.pqueue.prio` at `chmtx.c:200`.

Given the hardening posture of this code (`CH_CFG_HARDENING_LEVEL`, the `chSft*`
family), a use-after-free reachable from a plain application bug deserves at least

```c
chDbgAssert(currtp->mtxlist == NULL, "owned mutexes");
```

in `chThdExitS()`, plus a documented precondition. This is preferable to an
unconditional `chMtxUnlockAllS()`: automatic handoff can expose protected state
that the exiting thread left inconsistent, and the mutex API has no
owner-death result for the recipient.

`chThdObjectDispose()` already asserts that `mtxlist` is empty
(`chthreads.c:225`), but normal dynamic-thread release does not call it: the
standard heap/pool dispose callbacks free the working area directly. It therefore
does not prevent this use-after-free path. *Round 3:* line verified; the
assert-plus-precondition remedy is accepted over automatic release.

### 6. `WTCOND`/`WTSEM` re-enqueue reads the union through the wrong member — qualified — `chmtx.c:215-223`

```c
ch_sch_prio_insert(&tp->u.wtmtxp->queue, ch_queue_dequeue(&tp->hdr.queue));
```

In those two states the union holds `u.wtobjp` (a `condition_variable_t *`, set at
`chcond.c:249`) or `u.wtsemp` (a `semaphore_t *`), never `wtmtxp`. It works only
because `queue` happens to be the first member of all three object types
(`chsem.h:51-55`, `chcond.h`, `chmtx.h:56-66`).

This is layout-coupled and reads the union using a state-inappropriate typed
member, so it should be cleaned up. It is not exactly the failure mode of bug
#1301: in that bug, the queue back-pointer and message value could not coexist
in the union, so the required pointer was actually unavailable/corrupted.

The previously proposed `(ch_queue_t *)tp->u.wtobjp` expression is also not the
best common fix because WTSEM stores `u.wtsemp`. Prefer state-specific typed
access:

```c
case CH_STATE_WTCOND:
  ch_sch_prio_insert(&((condition_variable_t *)tp->u.wtobjp)->queue, ...);
  break;
case CH_STATE_WTSEM:
  ch_sch_prio_insert(&tp->u.wtsemp->queue, ...);
  break;
```

This removes the offset-zero dependency, so static offset assertions are not
needed. Treat this as portability and maintenance hardening unless a supported
target with incompatible pointer representation/layout is identified.

*Round 3:* both points accepted. The state-specific access is strictly better than
the `(ch_queue_t *)u.wtobjp` proposal — it eliminates the layout dependency instead
of merely documenting it, which also drops the need for static asserts. The #1301
distinction is correct: pre-fix, `u.wtmtxp` aliased the non-pointer `u.sentmsg`, so
the walk wrote through a garbage pointer; here the pointer is valid and only the
offset assumption is load-bearing. Shared root cause, different blast radius.

*Round 4 correction:* `osapi.c:2032` is uniform but not supporting evidence for
safety. Its `(ch_queue_t *)tp->u.wtobjp` access still relies on queue-at-offset-zero
and reads the union generically for states that store a typed member. It should
not be described as safer or more type-correct than state-specific access.

*Round 5:* correction accepted; the round-3 "more type-consistent" characterisation
of `osapi.c:2032` is **withdrawn**. Uniformity across the four states is not the
property that matters — eliminating the offset assumption is, and only
state-specific typed access does that. `osapi.c:2032` should be listed as a second
site needing the same treatment, not as a model.

### 7. `CH_STATE_READY` boost path abuses `chSchReadyI()` — qualified — `chmtx.c:234-241`

Re-sorting an already-ready thread goes through `chSchReadyI()`, which requires the
thread *not* to be ready (`chschd.c:68-70`) — hence the
`tp->state = CH_STATE_CURRENT` workaround under `CH_DBG_ENABLE_ASSERTS`. Two side
effects: `__sch_ready_behind()` emits a second `__trace_ready()` record for a thread
that never left the ready list (trace consumers see a transition that did not
happen), and in assert builds a non-current thread is transiently marked
`CH_STATE_CURRENT`.

A small internal reprioritization helper — dequeue + priority insertion, with
no fake state change and no READY trace record — removes both issues.

The helper must preserve the SMP notification behavior. A remotely-owned ready
thread whose priority was raised may now need to preempt the current thread on
that core, so `chSysNotifyInstance()` is not pointless.

*Round 3:* the SMP objection is correct and the earlier "pointless notification"
remark is **withdrawn**. Raising a remote READY thread's priority above that core's
current thread is precisely a case where the remote core must be prodded. The
new helper should reproduce the necessary notification logic without calling
the state-changing/tracing `chSchReadyI()` path it is intended to replace.

### 8. Documentation: the deadlock-freedom claim is false — confirmed — `chmtx.c:40-46`

> "Operating under this restriction also ensures that deadlocks are no possible."

LIFO unlock order does not prevent deadlock. `T1: lock(A); lock(B)` against
`T2: lock(B); lock(A)` deadlocks while never violating lock-reverse unlock order —
neither thread ever unlocks anything. The property that prevents this is a global
*lock ordering* discipline, which the docs do not state.

This claim is load-bearing for users designing locking schemes, so it should be
replaced with the actual guarantee (LIFO ordering is what makes the O(depth) deboost
scan in `chMtxUnlock()` valid) plus an explicit note that lock ordering is the
application's responsibility. Also `are no possible` → `are not possible`.

### 9. Recursive mutex counter can overflow — confirmed — `chmtx.h:64`, `chmtx.c:187`, `chmtx.c:332`

The recursion counter is a signed `cnt_t` and both recursive acquisition paths
increment it without a bound check. AVR uses `int8_t`
(`os/common/ports/AVR/compilers/GCC/chtypes.h:52`); generic 16-bit
configurations use `int16_t`, and current 32-bit configurations use
`int32_t` (`os/rt/include/chearly.h:81-116`). The first out-of-range
acquisition commonly converts the value to a negative number on narrow types;
signed overflow on wider types is undefined behavior.

*Round 5, mechanism verified:* the `chearly.h` citation is correct and the earlier
round-3 framing ("`int8_t` on AVR, `int32_t` on every other port", from an
incomplete grep) is **withdrawn**. `chearly.h:81-116` selects the width from
`PORT_ARCH_REGISTERS_WIDTH` (32 → `int32_t`, 16 → `int16_t`, 8 → `int8_t`) for any
port declaring `PORT_DOES_NOT_PROVIDE_TYPES`; ports supplying their own `chtypes.h`
override it (AVR `int8_t`; e200 and the simulators `int32_t`). So the 8-bit exposure
is a property of the register width, not of the AVR port specifically, and a
narrow-`cnt_t` port is reachable without touching AVR. This widens the case for
asserting rather than relying on the type being wide in practice.

At minimum, document the maximum recursion depth and assert before incrementing.
If release builds must be protected too, the representation or API behavior on
exhaustion needs an explicit design because the current lock API has no error
return.

### 10. `chMtxUnlockS()` can return with a higher-priority thread ready — confirmed — `chmtx.c:442-443`, `chmtx.c:449-511`

`chMtxUnlockS()` can lower the current thread's inherited priority, hand the
mutex to its highest-priority waiter using `chSchReadyI()`, and return without
calling `chSchRescheduleS()`. Its documentation explicitly tells callers to
reschedule, but that local contract conflicts with the clarified S-class rule:
public S-class functions must reschedule internally and must not return leaving
a higher-priority thread ready.

The round-3 reproducer directly demonstrates valid S-class usage halting in the
state checker:

```
owner boosted to 130 by the 130 waiter
now: chSysLock(); chMtxUnlockS(&m1); chSysUnlock();
HALT: chSysUnlock
```

The fix cannot simply add `chSchRescheduleS()` to the existing function because
`chCondWaitS()` and `chCondWaitTimeoutS()` call it while constructing the
atomic release-and-wait operation. Rescheduling between mutex release and
condition-variable enqueue would create a lost-wakeup window.

Recommended structure:

1. Factor the existing release operation into an internal, no-reschedule
   helper.
2. Make public `chMtxUnlockS()` call that helper and then reschedule when
   required.
3. Make `chMtxUnlock()` use the same helper under the system lock.
4. Make the condition-wait functions call the internal helper, enqueue the
   current thread, and enter sleep without an intervening reschedule.
5. Remove the stale no-reschedule documentation from the C and C++ public
   S-class APIs.

Tests 8.5.3 and 8.5.5 currently add an explicit `chSchRescheduleS()` after
`chMtxUnlockS()`. They should be changed to verify that public
`chMtxUnlockS()` satisfies the S-class rule on its own. The explicit
reschedule in `OS_MutSemGive()` becomes redundant after the kernel fix.

*Round 5, claims verified:*

- Tests 8.5.3 and 8.5.5 do call `chMtxUnlockS(&m1); chSchRescheduleS();` inside
  `chSysLock()`/`chSysUnlock()` (`rt_test_sequence_008.c`, steps at 663-732), and the
  coverage thread at `rt_test_sequence_008.c:196-197` does the same. Test 8.5.4
  deliberately omits it for `chMtxUnlockAllS()` — so the suite currently encodes the
  very asymmetry finding 10 removes. Useful nuance for the patch: those explicit
  calls do not have to be deleted for the fix to be correct. Once the function
  complies, `chSchRescheduleS()` on an already-compliant state is a no-op
  (`chschd.c:470` compares `firstprio > current prio`), so the churn is about making
  the tests *assert* the new contract, not about un-breaking them.
- `OS_MutSemGive()` at `osapi.c:1648-1649` does `chMtxUnlockS(mp);
  chSchRescheduleS();` — confirmed, and redundant after the fix by the same argument.
- The stale `@post` is replicated in the C++ wrapper twice, on **both** methods that
  wrap `chMtxUnlockS()`: `os/various/cpp_wrappers/ch.hpp` `unlockS()` (doc at ~2084,
  body at ~2089) and `unlockMutexS()` (doc at ~2114, body at ~2121). Step 5 must
  cover both.

**The obvious alternative remedy is also unsafe — do not take it.** If someone
"fixes" the lost-wakeup window by reordering `chCondWaitS()` to enqueue on the
condition variable *before* releasing the mutex, and the release then reschedules,
the result is worse: the thread sits in `cp->queue` while in `CH_STATE_READY`, so a
`chCondSignal()` from the new mutex owner calls `chSchWakeupS()` on an
already-ready thread. That trips `__sch_ready_behind()`'s `state != CH_STATE_READY`
assertion (`chschd.c:68-70`), and with assertions off it inserts the same thread into
the ready list twice — list corruption instead of a lost wakeup. Both orderings fail
for the same underlying reason: release-and-wait must remain atomic with respect to
other threads. That is why step 4 above is the only safe shape, and why the internal
helper (step 1) must stay reschedule-free.

#### Scope of the S-class rule (round-5 audit)

The clarified rule is what exposed finding 10, so it was applied to the whole
kernel: every call site that makes a thread ready in `os/rt/src` and `os/oslib/src`
was mapped to its enclosing function and classified.

| Category | Sites | Compliant? |
|---|---|---|
| I-class, caller reschedules by contract | `chCondSignalI`, `chCondBroadcastI`, `chEvtSignalI`, `chSemResetWithMessageI`, `chSemSignalI`, `chSemAddCounterI`, `chThdSpawnRunningI`, `chThdCreateI`, `chThdResumeI` | yes, by definition |
| API / S-class that reschedules or sleeps | `chMsgSend`, `chMtxLockS` (sleeps), `chMtxUnlock`, `chMtxUnlockAllS`, `chSemSignalWait`, `chThdExitS`, `chThdResumeS`, `chMBPostTimeoutS` (`chmboxes.c:242-243`), `chMBPostAheadTimeoutS` (`:365-366`), `chMBFetchTimeoutS` (`:488-489`) | yes |
| **S-class that returns with a higher-priority thread ready** | **`chMtxUnlockS()` (`chmtx.c:503`)** | **no — finding 10** |

`chMtxUnlockS()` is the single violation. Two consequences: the S-class work is
bounded to one function plus its two C++ wrappers, and the function is an outlier
rather than an instance of a kernel-wide convention — which removes the main reason
to hesitate over changing it.

### Related independent defect: NASA OSAL `OS_MutSemDelete()` — confirmed, high

`os/common/abstractions/nasa_cfe/osal/src/osapi.c:1597-1620` calls the
parameterless `chMtxUnlockAllS()` while deleting a specific `mp`, then marks
that mutex unused and returns it to the pool.

Consequences:

- It releases every mutex owned by the caller, including unrelated mutexes.
- It does not release the target when another thread owns it.
- If the caller owns a contended target, `chMtxUnlockAllS()` can hand it to a
  waiter and the deletion code immediately frees it.

This requires a separate target-specific validation/disposal fix. It must not
be included as a side effect of changing the kernel `UnlockAllS` contract.

---

## Part 3 — Open questions / decisions for the maintainer

The first three items are the remaining API-policy choices. The backport item
is a release-planning decision, not an unresolved correctness result.

1. **Finding 3: recursive condition waits** — prohibit depth greater than one,
   or implement full release and depth restoration? The assert plus documented
   precondition is the smallest coherent behavior.
2. **Finding 5: owner exit** — the recommended default is a documented
   prohibition plus an assertion. Automatic release should only be considered
   together with explicit owner-death semantics; otherwise waiters can observe
   protected state abandoned midway through an update.
3. **Finding 9: recursion limit** — is a debug-only maximum sufficient, or
   must release builds detect exhaustion? A void blocking-lock API cannot
   report this cleanly, so release-build behavior needs an explicit policy.
4. **Backport scope** — the previous 2/4/8-only list is incomplete.
   *Round 3, verified against `stable-21.11.x`:* the `chThdSetPriority()` shortcut is
   identical (`chthreads.c:607-612` there), `chCondWaitS()`/`chCondWaitTimeoutS()`
   release through `chMtxUnlockS()` the same way (`chcond.c:214`, `chcond.c:304`), and
   `mtxlist` appears exactly once in stable's `chthreads.c` (the initializer), so the
   exit path is unguarded there too. Round 4 also verified that stable contains
   the same WTCOND/WTSEM member access, READY requeue workaround, recursive
   counter increments, and non-rescheduling `chMtxUnlockS()`. Findings 0 and
   2-10 therefore apply to stable; finding 1 has the same behavior but remains
   optional diagnostics. Still make the decision finding-by-finding after the
   changes land on `master`.

## Part 4 — Test-coverage gaps noticed

`test/rt/source/test/rt_test_sequence_008.c` covers PI well (8.1-8.5 simple/complex
boost, priority return, handoff variants; 8.6/8.7 repeated locks; 8.8-8.10 condvars
including a priority-boost test). Not covered, and relevant to the findings:

- **Critical missing PI regression:** owner base/effective 128, priority-127
  waiter already queued, then `chThdSetPriority(126)`; effective priority must
  remain 127 (finding 0). Prefer the stronger form used by the round-3 reproducer:
  lower to 100 and add an unrelated priority-120 thread, so the test fails on
  observable inversion (medium thread preempting the owner) and not only on a
  priority readback.
- The non-recursive second `chMtxTryLock()` behavior is already covered and
  must continue to return `false`. A blocking self-lock test is inappropriate
  unless finding 1 is deliberately changed into a debug assertion and the test
  harness can expect a panic.
- No negative assertion test for unlocking another thread's recursive mutex
  with `cnt >= 2` (finding 2).
- No test for condition wait on a recursively-held mutex (finding 3).
- Test 8.5.4 correctly covers the intended internally-rescheduling
  `chMtxUnlockAllS()` behavior. Retain it while correcting the stale
  documentation and adding the class check (finding 4).
- No test for exit while owning a mutex, including the dynamic-thread disposal
  path (finding 5).
- No test exercising the `CH_STATE_WTSEM` arm of the PI walk
  (`CH_CFG_USE_SEMAPHORES_PRIORITY == TRUE` + a boosted semaphore waiter that owns a
  mutex) — finding 6's fragile path is untested in both arms. Note the default
  `test/rt/testbuild/chconf.h` has `CH_CFG_USE_SEMAPHORES_PRIORITY = FALSE` and
  `CH_CFG_USE_MUTEXES_RECURSIVE = FALSE`, so the suite never compiles that arm or the
  recursive paths behind findings 2, 3 and 9. Both are overridable via `XDEFS`
  (see Part 6) — a second test configuration is the cheapest way to close this.
- No trace-oriented test that distinguishes READY-list reprioritization from a
  genuine transition into READY (finding 7).
- No recursive counter-boundary test on a target with a small `cnt_t`
  (finding 9).
- Tests 8.5.3 and 8.5.5 explicitly reschedule after `chMtxUnlockS()` and
  therefore encode the stale caller-managed contract. Remove the explicit
  reschedule and verify that the public S-class function performs it
  internally (finding 10) — removal is what makes the test able to *detect*
  non-compliance, even though leaving it in would merely be a no-op once the
  function complies (see the round-5 note under finding 10). Add a condition-wait
  regression to ensure the internal no-reschedule release helper preserves atomic
  release-and-wait; the round-5 analysis shows both a lost wakeup and a
  double-ready-list insertion are reachable if that atomicity is broken, so the
  regression should cover the signal-from-the-new-owner race specifically.
- No focused OSAL test for deleting an unowned, owned, or contended mutex.
- No focused tests for changing an arbitrary blocked thread's priority through
  CMSIS OS or NASA OSAL, including upstream PI propagation and priority-unit
  conversion.

## Part 5 — Convergence and implementation plan

1. Re-anchor line numbers against the then-current `master` (this report is pinned to
   `859a932f43`).
2. Fix finding 0 first and add the priority-lowering regression. It is the
   only confirmed defect that directly breaks the PI invariant during valid
   API use. Land it as the shared-helper refactor so `chMtxUnlock()`,
   `chMtxUnlockS()` and `chThdSetPriority()` share one recomputation.
3. Fix finding 10 by separating internal no-reschedule mutex release from the
   public, internally-rescheduling S-class API. Update condition waits, tests,
   C++ documentation, and redundant caller reschedules together. The round-5
   audit bounds this to one kernel function plus the two `ch.hpp` wrappers —
   no other S-class function in RT or oslib needs touching. Do not "simplify"
   the condition-wait path by reordering enqueue and release; that alternative
   is unsafe for a different reason (see finding 10).
4. Patch finding 2 independently; it is localized and does not require an API
   design decision.
5. Resolve findings 3, 5, and 9 explicitly before coding them because each
   selects or tightens API behavior.
6. Patch finding 4 as documentation plus the missing class check.
7. Handle findings 6 and 7 as internal portability/trace cleanup, preserving
   the necessary SMP remote-core notification.
8. Patch finding 8 as documentation. Treat finding 1 as optional diagnostics,
   not as a prerequisite correctness fix, and preserve try-lock behavior.
9. Fix the NASA OSAL deletion bug separately from the RT mutex series.
   Treat arbitrary-thread priority changes in CMSIS OS and OSAL as a separate
   design: correct their independent conversion/target bugs and propagate
   donation changes through WTMTX owner chains.
10. Build the POSIX simulator in default and recursive configurations plus one
   ARM target, run test sequence 008 and new focused tests, then
   `tools/style/stylecheck.py` on the touched files (per `REVIEW.md` stages A-D).
11. Land on `master` first per `REVIEW.md`, then compare each accepted change with
   `stable-21.11.x` and make a finding-by-finding backport decision (Part 3 item 4).

For subsequent cross-checks, update the result table and the evidence under the
same stable finding ID. A finding converges when its source-level behavior,
contract interpretation, chosen remedy, and regression test all agree; a
passing existing suite alone is not evidence that an uncovered edge case is
correct.

**State of the cross-check after round 5.** Rounds 3-5 each changed the report:
round 3 executed reproducers and withdrew three claims, round 4 added the S-class
rule and finding 10, round 5 verified round 4 and found nothing new. Behaviour is
agreed on all eleven findings; only the Part 3 policy choices remain. If another
round is wanted, the useful remaining work is evidence, not discovery:

- Findings 5, 6, 7 and 9 are still read-only. Reproducers are feasible for 5
  (exit while owning, `CH_CFG_USE_DYNAMIC` + a pool/heap thread, watched under
  valgrind for the use-after-free) and 9 (`PORT_ARCH_REGISTERS_WIDTH == 8`, or a
  local `cnt_t` narrowing, to hit the counter boundary). Finding 6 is a static
  property and finding 7 is observable only through the trace buffer.
- The finding-0 variants in `cmsis_os.c` and `osapi.c` have never been executed;
  the upstream-propagation claim in particular is reasoned only.
- Nothing in Parts 1-2 is contested, so a round that only re-reads `chmtx.c` is
  unlikely to pay. Prefer starting the patch series (Part 5) and letting the
  regressions become the remaining evidence.

## Part 6 — Reproducer recipe

The host simulator builds out of the tree with no board hardware. Scratch dirs were
used so nothing lands in the repo; recreate them the same way:

```sh
CH=<path-to>/chibios-master
mkdir -p /tmp/pi-repro && cd /tmp/pi-repro
cp $CH/test/rt/testbuild/{Makefile,chconf.h,halconf.h,mcuconf.h} .
# write main.c, then:
make -j8 CHIBIOS=$CH XDEFS="-DCH_DBG_ENABLE_ASSERTS=TRUE"
./build/ch
```

`CHIBIOS` overrides the Makefile's relative `../../..`; `XDEFS` feeds extra `-D`
flags, and every relevant `chconf.h` switch is wrapped in `#if !defined(...)`, so
configuration can be varied without editing the file. Findings 2, 3 and 9 need
`-DCH_CFG_USE_MUTEXES_RECURSIVE=TRUE`; finding 6's WTSEM arm needs
`-DCH_CFG_USE_SEMAPHORES_PRIORITY=TRUE`.

To see the halt reason (finding 10), fill in `CH_CFG_SYSTEM_HALT_HOOK(reason)` in the
scratch `chconf.h` with a `fprintf(stderr, "HALT: %s\n", (reason)); _exit(42);` —
otherwise `chSysHalt()` spins and the run just times out.

The three reproducers used in round 3:

1. **Finding 0** — main at base 128 locks `m1`; donor thread at 127 blocks on it
   (no boost, 128 > 127); main calls `chThdSetPriority(100)`; main then creates an
   unrelated thread at 120 while still inside the critical section and prints
   priorities at each step. Buggy output: effective 100, and the 120 thread preempts.
2. **Findings 2 and 3** (`-DCH_CFG_USE_MUTEXES_RECURSIVE=TRUE`) — (a) main locks
   `m1` twice; an offender thread locks an unrelated `m2` (to satisfy the
   `mtxlist != NULL` assert) and calls `chMtxUnlock(&m1)`; print `m1.cnt` before and
   after, then have main unlock once and print `m1.owner`. (b) main locks `m3` twice
   and calls `chCondWait()`; a prober thread calls `chMtxTryLock(&m3)` and prints
   whether it acquired it and who `m3.owner` is.
3. **Finding 10** — main locks `m1`, a thread at 130 blocks on it (boosting main to
   130), then `chSysLock(); chMtxUnlockS(&m1); chSysUnlock();`. Halts in
   `chSysUnlock`.

The recipe above is not self-contained because it summarizes rather than
embedding the three `main.c` files, and the current sources live only in
`/tmp/pi-repro*`. Before those scratch directories are removed, promote each
reproducer into the RT test suite or preserve its complete source alongside
this report. Passing binaries and prose summaries are not durable regression
evidence.
