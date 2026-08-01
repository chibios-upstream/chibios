# RT Mutex Review: Confirmed Defects and Proposed Fixes

Branch reviewed: `master` at `859a932f43`.

This is the short list. It excludes optional diagnostics, portability
observations, and other qualified items from `MUTEX-REVIEW.md`.

## PR classes

The classes below are the proposed change boundaries. Each class should be one
PR. The detailed entries retain stable defect numbers so review discussion and
tests can refer to them without renumbering.

1. Class A: RT core priority inheritance and S-class scheduling.

   Defects: 1 and 5.

   PR scope: correct effective-priority recomputation, factor the internal
   no-reschedule mutex-release operation, make public `chMtxUnlockS()` obey the
   S-class scheduling rule, and add focused PI and condition-wait atomicity
   tests. These changes touch the same mutex unlock paths and should be reviewed
   together.

2. Class B: kernel invariant validation.

   Defects: 2, 3, 7, and 8.

   PR scope: reject reserved user priorities, validate the mutex actually
   passed to unlock, reject thread exit while owning mutexes, and guard the
   recursive counter limit. This PR is assertion, validation, and negative-test
   work; it should not change valid scheduling behavior.

3. Class C: mutex and condition-variable API contracts.

   Defects: 4, 6, and 9.

   PR scope: document recursive condition-wait behavior, correct the S-class
   scheduling documentation and class check for `chMtxUnlockAllS()`, and replace
   the incorrect deadlock-freedom statement. Keep these contract changes
   separate from the core scheduler refactor.

4. Class D: NASA OSAL mutex object lifecycle.

   Defect: 10.

   PR scope: make `OS_MutSemDelete()` validate and dispose only its target
   mutex. This is independent of the RT core fixes and should have its own OSAL
   tests.

5. Class E: arbitrary-thread priority APIs.

   Defects: 11, 12, 13, and 14.

   PR scope: add or use a common arbitrary-target priority-change operation,
   preserve transitive inheritance, correct CMSIS priority conversion, fix
   NASA OSAL target comparison, and notify a remote owner core when required.
   This PR should follow Class A so it can reuse the corrected core priority
   machinery rather than duplicate it.

Recommended PR order: A, B, C, D, then E. Class E depends on A; the other
classes are logically independent, although B and C may need trivial conflict
resolution if they are developed concurrently with A.

## Detailed findings

1. `chThdSetPriority()` can discard an existing mutex donation.

   A mutex owner whose base and effective priorities are initially equal can
   lower its effective priority below an already queued waiter. This permits an
   intermediate-priority thread to preempt the owner. The simulator reproducer
   observed that inversion.

   Proposed fix: compute the effective priority as the maximum of the new base
   priority and the highest waiter on every owned mutex. Factor the calculation
   shared by `chThdSetPriority()`, `chMtxUnlock()`, and `chMtxUnlockS()` into one
   internal helper.

2. Public thread-priority paths accept reserved priorities.

   `NOPRIO` is zero and is the priority-queue header sentinel. `IDLEPRIO` is
   reserved for the idle thread, while `LOWPRIO` is explicitly the lowest user
   priority. `chThdSetPriority()`, `chThdCreateSuspendedI()`, and
   `chThdCreateStatic()` check only the upper bound.
   `chThdSpawnSuspendedI()` does not check the priority at all.

   The simulator reproducer called `chThdSetPriority(NOPRIO)` with assertions
   enabled. The value passed the check and the scheduler hung in
   `ch_pqueue_insert_behind()`: an element at priority zero can never be
   inserted before or after the zero-priority circular-list header.

   Proposed fix: require every user-created or user-adjusted thread priority to
   satisfy `LOWPRIO <= prio && prio <= HIGHPRIO`. Keep the lower-level
   `chThdObjectInit()` able to initialize the kernel's idle thread at
   `IDLEPRIO`. Add negative tests for both `NOPRIO` and `IDLEPRIO`.

3. Recursive foreign unlock mutates a mutex owned by another thread.

   `chMtxUnlock()` and `chMtxUnlockS()` validate
   `currtp->mtxlist->owner`, not `mp->owner`. If the foreign mutex recursion
   count is at least two, the invalid call silently decrements it and returns,
   even with assertions enabled. The owner's next unlock can then release the
   mutex one level early. The simulator reproducer confirmed this behavior.

   Proposed fix: assert `mp->owner == currtp` before changing `mp->cnt` in both
   unlock functions. Retain the LIFO assertion for the final release.

4. Condition-wait documentation omits the recursive-depth restriction.

   `chCondWaitS()` and `chCondWaitTimeoutS()` call `chMtxUnlockS()` once. At
   recursion depth greater than one this decrements the count but does not make
   the mutex available to other threads. The waiter sleeps while remaining the
   owner, so a signaller that needs the same mutex cannot proceed. The simulator
   reproducer confirmed this behavior.

   This observable behavior is consistent with POSIX recursive mutexes. POSIX
   condition waits do not release and reacquire a recursive mutex whose lock
   count is greater than one, and applications are advised not to use such a
   mutex depth with a condition variable. The ChibiOS implementation is
   therefore not defective on this point by itself; the defect is that its
   documentation promises to release the mutex without stating the recursive
   exception.

   Proposed fix: document a precondition that the current mutex must have
   recursion depth one and add a matching debug assertion. Alternatively,
   explicitly document the POSIX-style behavior, including that the mutex
   remains unavailable at greater depths. Fully unwinding and later restoring
   the recursion count would be a different API policy rather than a required
   correction.

5. `chMtxUnlockS()` can return while a higher-priority thread is ready.

   It can lower the current thread's inherited priority, make the selected
   waiter ready, and return without rescheduling. This violates the S-class
   requirement that the function reschedule internally and not leave a
   higher-priority thread ready. The simulator reproducer reaches the scheduler
   state check at `chSysUnlock()`.

   Proposed fix: factor mutex release into an internal no-reschedule helper.
   Public `chMtxUnlockS()` and `chMtxUnlock()` should invoke the helper and
   reschedule as required. Condition waits must call the internal helper so that
   release and enqueue remain atomic.

6. `chMtxUnlockAllS()` has a stale scheduling contract and no class check.

   The implementation correctly calls `chSchRescheduleS()`, but its
   documentation says that callers must reschedule. It is also the only public
   S-class mutex function without `chDbgCheckClassS()`.

   Proposed fix: remove the stale no-reschedule postcondition, document the
   internal reschedule, and add `chDbgCheckClassS()` at function entry.

7. A thread can exit while still owning mutexes.

   `chThdExitS()` neither releases owned mutexes nor rejects the operation.
   Waiters remain blocked on an owner in `CH_STATE_FINAL`. If a dynamically
   allocated thread is subsequently disposed, mutex owner pointers can refer to
   the released thread object.

   Proposed fix: document that a thread must own no mutexes when it exits and
   assert `currtp->mtxlist == NULL` in `chThdExitS()`. Do not release mutexes
   automatically because protected state may have been abandoned midway through
   an update.

8. Recursive mutex acquisition can overflow `cnt_t`.

   Both recursive acquisition paths increment the signed recursion counter
   without a limit check. `cnt_t` can be 8, 16, or 32 bits depending on the
   target. Crossing the representable limit breaks the positive-count
   invariant.

   Proposed fix: define and document a maximum recursion depth and check before
   incrementing. If release builds must handle exhaustion, choose an explicit
   API policy because the current lock functions have no error result.

9. Mutex documentation incorrectly claims that reverse unlock order prevents
   deadlock.

   Two threads can lock `A` then `B` and `B` then `A`, respectively, and
   deadlock without ever violating reverse unlock order.

   Proposed fix: state that reverse unlock order supports the owned-mutex stack
   and priority recomputation. Document that applications still require a
   consistent global lock-acquisition order to prevent lock-order deadlocks.

10. NASA OSAL `OS_MutSemDelete()` deletes a specific mutex by unlocking all
    mutexes owned by the caller.

    The function calls parameterless `chMtxUnlockAllS()`, so it can release
    unrelated mutexes, fail to release a target owned by another thread, or hand
    a contended target to a waiter immediately before returning the object to
    the pool.

    Proposed fix: validate the target mutex directly, require it to be unowned
    and uncontended, dispose that object, and only then return it to the pool.
    Do not use `chMtxUnlockAllS()` in this path.

11. Arbitrary-thread priority setters do not preserve transitive priority
    inheritance.

    CMSIS `osThreadSetPriority()` and NASA OSAL `OS_TaskSetPriority()` directly
    change a blocked thread's priority and reorder that one wait queue. If the
    target is blocked on a mutex, raising its priority does not propagate the new
    donation through the owner chain, and lowering it does not recompute and
    remove stale donations from that chain.

    Proposed fix: provide a kernel-internal arbitrary-thread priority-change
    operation that preserves effective priority, reorders the target in its
    current priority queue, and propagates both increases and decreases through
    every `WTMTX` owner in the dependency chain. Make both abstractions use it.

12. CMSIS thread-priority conversion is inconsistent between create, set, and
    get.

    Thread creation maps a CMSIS priority with `NORMALPRIO + tpriority`.
    `osThreadSetPriority()` writes the signed CMSIS enum directly into the RT
    priority, while `osThreadGetPriority()` subtracts in the opposite direction.
    A set/get round trip therefore does not preserve the requested priority.

    Proposed fix: centralize checked CMSIS-to-RT and RT-to-CMSIS conversion
    helpers. Use `NORMALPRIO + cmsis_priority` when setting and
    `rt_priority - NORMALPRIO` when getting.

13. NASA OSAL `OS_TaskSetPriority()` compares the requested priority with the
    caller instead of the target.

    If the caller already has the requested RT priority, the function returns
    success before validating or changing a different target thread.

    Proposed fix: validate and resolve the target first, then compare
    `tp->realprio` with the converted requested priority. Continue through the
    common priority-change path whenever the target differs.

14. Arbitrary-thread priority setters do not reschedule a remote core when
    lowering its current thread.

    In SMP mode, the CMSIS and NASA OSAL setters can target a thread in
    `CH_STATE_CURRENT` on another core. Lowering that thread can make an existing
    ready thread on that core higher priority, but the setters only call
    `chSchRescheduleS()` on the caller's core. The target core is not notified.

    Proposed fix: when an arbitrary priority change affects a remote current
    thread, call `chSysNotifyInstance(tp->owner)` so that its owner core
    reschedules. Put this behavior in the common kernel operation proposed for
    defect 11.
