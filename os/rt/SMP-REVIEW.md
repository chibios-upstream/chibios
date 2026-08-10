# RT SMP — Correctness Review

**Status:** issue #205 cross-check complete. Five implementation defects and
one API-contract/validation defect are confirmed. One ARMv8.1-M BASEPRI
portability question remains open. No correction has been applied yet. Stable
IDs are assigned so subsequent reviews can converge without renumbering
findings.

| | |
|---|---|
| Date | 2026-08-09 |
| Source snapshot | `origin/master` @ `0d8b3028a7`; the two commits ahead of the local master do not change the reviewed SMP code |
| Main scope | Instance startup, global kernel locking, per-instance scheduling, affinity, inter-core rescheduling, priority inheritance, system-state publication, cross-core halt propagation, and RP2 flash lockout |
| Ports | ARMv6-M/RP2040, ARMv8-M-ML-ALT/RP2350, and RISC-V Hazard3/RP2350 |
| Verification | Source, call-site, history, port-assembly, and issue #205 cross-check; optimized ARMv6-M and ARMv8-M SMP demo builds; comparison with the RISC-V durable halt protocol and the unmerged SMP simulator branch |

## Proposed PR classes

1. **Class A — instance startup and publication**

   Defects: SMP-1, SMP-3, and SMP-6.

   Scope: establish actual global-lock ownership during instance
   initialization, provide the missing ARM release/acquire publication pair,
   and make the initialized-owner requirement for affinity explicit and
   diagnosable.

2. **Class B — durable cross-core halt propagation**

   Defect: SMP-2.

   Scope: port the RISC-V durable per-target halt latch to the two ARM RP2
   ports, retaining the FIFO only as a best-effort wakeup hint.

3. **Class C — flash-lockout readiness**

   Defect: SMP-4.

   Scope: publish lockout readiness only when the core can actually service
   the FIFO interrupt, and pair publication and observation with the required
   memory ordering.

4. **Class D — RP2350 spinlock configuration validation**

   Defect: SMP-5.

   Scope: reject unsafe RP2350 spinlock overrides for both the kernel lock and
   the flash-lockout lock.

5. **Class E — merged SMP regression coverage**

   This is a coverage class, not another defect. Move or adapt the useful
   startup, lock-contention, coalesced-notification, remote-wakeup, and remote
   priority tests from `origin/chibios-smp-sim-dev`, then add the RP-specific
   full-FIFO halt and flash-lockout-startup cases. Use the existing IRQ-STORM
   test with system-state checking enabled to qualify BASEPRI behavior on new
   ARMv8.1-M cores; this is qualification work rather than a regression for a
   confirmed defect.

## Findings summary

| ID | Classification | Summary |
|---|---|---|
| SMP-1 | Confirmed startup/locking defect; critical | Instance initialization claims I-lock state but neither startup core owns the shared kernel spinlock |
| SMP-2 | Confirmed cross-core halt defect; high | ARM halt notification is discarded when the inter-core FIFO is full |
| SMP-3 | Confirmed publication defect; medium | ARM system-state handoff has no release/acquire memory barriers |
| SMP-4 | Confirmed startup/flash-lockout defect; high | ARM ports publish lockout readiness before the core can service lockout requests |
| SMP-5 | Confirmed configuration defect; medium when overridden | ARMv8-M RP2350 accepts spinlock numbers affected by RP2350-E2 |
| SMP-6 | Confirmed API-contract/validation defect; medium | Affinity accepts an uninitialized instance and can enqueue into an uninitialized ready list |

## Governing SMP model

RT implements pinned-thread SMP rather than migration. Each core owns one
`os_instance_t`, current thread, ready queue, and virtual-timer list. A
`thread_t::owner` selects the instance on which the thread may run. Shared
kernel operations are serialized by one port spinlock in addition to masking
local kernel-aware interrupts.

The ready queues remain per-instance, but wait queues, mutex ownership chains,
the SMP registry, and RFCU state can connect objects belonging to both cores.
Consequently, local interrupt masking is not an I-lock in SMP: the shared
spinlock is part of every valid I/S-class critical section.

Remote readying is intentionally a two-part operation. The initiating core
modifies the target ready queue while holding the shared lock and sends a
reschedule notification. The target handles the notification and evaluates
preemption only after acquiring the same lock. This makes the notification a
wakeup hint; the ready queue is the authoritative state.

## Confirmed defects

### SMP-1 — instance initialization does not own the SMP kernel lock

`chInstanceObjectInit()` documents that the instance is in I-lock state after
initialization. Its debug initializer sets `lock_cnt` to one, and its statistics
initializer starts critical-section timing on the assumption that the final
`chSysUnlock()` will close a real critical section.

The implementation never acquires the port lock. It registers the instance in
`ch_system.instances[]`, initializes port state and the ready/timer lists,
inserts the main and idle threads into the system-wide SMP registry, invokes
`CH_CFG_OS_INSTANCE_INIT_HOOK()`, and readies the idle thread. The RP secondary
entry points only arrive with local interrupts masked; `chSysWaitSystemState()`
does not acquire the shared spinlock, and all three RP `port_init()` paths leave
the spinlock untouched.

Both startup flows then call `chSysUnlock()`, whose SMP `port_unlock()`
unconditionally releases the hardware spinlock. The primary publishes
`ch_sys_running` immediately before this call, so the secondary may begin its
own initialization before the primary has completed that nominal unlock.

This creates several manifestations of one root defect:

- secondary registry insertion can race normal registry scans and insertion
  on the primary;
- an early-published instance pointer can expose a ready list that has not yet
  been initialized;
- once the primary is running, it can acquire the kernel spinlock while the
  secondary is still initializing, after which the secondary's final
  `chSysUnlock()` releases the primary's lock; RP hardware spinlocks do not
  enforce releasing-core ownership; and
- the instance initialization hook runs with the debug state claiming an
  I-lock but without cross-core exclusion.

The unmerged `origin/chibios-smp-sim-dev` port independently recognizes that
startup unlock cannot be treated as a normal spinlock release. It implements a
special primary/secondary rendezvous for the first unlock. That avoids this
failure in the simulator but does not change the three merged RP ports.

**Proposed fix:** establish a real startup lock protocol before publishing an
instance. Both the primary and secondary paths must participate; adding a raw
lock only to `c1_main()` is insufficient because the primary stores
`ch_sys_running` before its final unlock and could release the secondary's new
lock. The generic initialization path can acquire a startup-capable port lock
which does not depend on `currcore`, or each SMP port can provide an equivalent
first-unlock rendezvous. Hold that exclusion through registration, hooks, and
idle-thread insertion, then let the existing final `chSysUnlock()` release it.

Add a startup test which deliberately delays the secondary instance hook while
the primary performs registry and kernel-lock operations. Instrumented ports
should also diagnose a release by a core which does not own the lock.

### SMP-2 — ARM cross-core halt notification is not durable

Both ARM RP2 ports define `PORT_SYSTEM_HALT_HOOK()` as an unconditional write
of `PORT_FIFO_PANIC_MESSAGE` to `SIO->FIFO_WR`. Their own comment explains that
the write is performed without waiting for space. On RP2, a write while the
FIFO is full is not queued; it sets the write-on-full status and discards the
word. The receiving core reacts only if it reads the halt token, so it can
continue executing indefinitely when that write is discarded.

This is not equivalent to the best-effort reschedule notification. Ready-queue
state remains authoritative after a coalesced reschedule hint, but there is no
other authoritative ARM halt indication for the receiving core to observe.

The RISC-V RP2350 port already contains the correct model:

- a per-target `port_panic_pending[]` latch is stored with release ordering;
- the FIFO token is sent only when space is available and is merely a wakeup
  hint;
- the receiver checks the latch on handler entry and after draining the FIFO,
  closing the drain/send race; and
- per-core initialization forces the FIFO interrupt if a halt was latched
  during startup.

**Proposed fix:** implement the same durable-latch protocol in ARMv6-M and
ARMv8-M-ML-ALT using an aligned shared word and the port's supported memory
barriers. Preserve the entry and post-drain checks as well as the startup
check. Add a target test that fills the outbound FIFO, invokes the halt hook,
and verifies that the other core still reaches its local halt path.

### SMP-3 — ARM system-state publication lacks memory ordering

`chSysInit()` supports a port `PORT_SYSTEM_STATE_RELEASE()` operation before
storing `ch_sys_running`; `chSysWaitSystemState()` supports the matching
`PORT_SYSTEM_STATE_ACQUIRE()` after observing the requested state. The RISC-V
Hazard3 port and the SMP simulator define these operations. Neither merged ARM
RP2 port defines them.

Declaring `ch_system.state` volatile makes the polling load occur, but it does
not publish all preceding non-volatile initialization to another core. The
current ARM startup path also does not acquire the kernel spinlock, so its DMB
operations cannot incidentally provide the missing handoff.

**Proposed fix:** define the ARM release/acquire hooks with `__DMB()` (including
the compiler barrier supplied by CMSIS). Keep the explicit state handoff even
if SMP-1 is corrected by a spinlock rendezvous because
`chSysWaitSystemState()` is itself the documented publication API, not merely
an implementation detail of `chInstanceObjectInit()`.

### SMP-4 — flash-lockout readiness is published too early

The two ARM `__port_smp_init()` functions enable the local FIFO vector and then
immediately set `port_lockout_ready[current] = true`. This function is called
near the beginning of `chInstanceObjectInit()`, before ready-list, virtual
timer, debug/statistics, current-thread, registry, user-hook, and idle-thread
initialization.

More importantly, startup still has PRIMASK or BASEPRI masking the FIFO
interrupt. Interrupts become serviceable only at the final `chSysUnlock()`.
The readiness flag therefore means only that the NVIC enable bit has been
written, not that the core can acknowledge a lockout request.

`rpEflBeforeXipOff()` explicitly waits for this flag and then starts the FIFO
handshake. A valid primary flash operation can observe `true` while the
secondary remains in a long instance hook or debug stack fill. The secondary
cannot run the FIFO handler, the 100ms handshake expires, and the system halts
with a lockout timeout.

**Proposed fix:** publish readiness at the final transition to an
interrupt-serviceable instance, not inside early `port_smp_init()`. A
port-specific instance-initialization-complete hook or guarded first-unlock
path can perform the publication immediately before enabling interrupts, with
release ordering. The getter needs the corresponding acquire semantics. Add a
test that holds the secondary in its instance hook for longer than the lockout
timeout while the primary requests its first flash operation; the primary
must wait for true readiness rather than begin a doomed handshake.

### SMP-5 — ARMv8-M RP2350 does not reject E2-affected spinlocks

The RISC-V RP2350 SMP header documents RP2350-E2 and rejects a
`PORT_SPINLOCK_NUMBER` which can be falsely released by unrelated SIO writes
on affected mask revisions. The ARMv8-M-ML-ALT RP2350 header checks only the
numeric range and that the two configured locks differ.

The defaults, kernel lock 31 and lockout lock 30, are in the safe range. A
supported configuration override can nevertheless select an affected lock
for either `PORT_SPINLOCK_NUMBER` or `PORT_LOCKOUT_SPINLOCK_NUMBER`, making
kernel exclusion or flash-lockout serialization unreliable.

**Proposed fix:** apply the RISC-V RP2350-E2 safe-set check to both ARM lock
configuration macros. Add preprocessing/build cases for one accepted safe
override and rejected unsafe overrides of each lock.

### SMP-6 — remote affinity accepts an uninitialized instance

`chThdObjectInit()` copies any non-NULL descriptor `owner` into the new thread.
The creation/start paths subsequently insert that thread into
`owner->rlist.pqueue` and notify the owner, but neither the implementation nor
the public descriptor documentation requires or verifies that the owner is a
fully initialized, registered instance.

This matters during the supported asynchronous RP startup. `chSysInit()` can
return on core zero before core one's `chInstanceObjectInit()` has completed.
Creating a thread with affinity `&ch1` in that interval can enqueue into a BSS
or partially initialized ready queue. Depending on the exact interleaving, the
operation faults while following null queue links or the later
`ch_pqueue_init()` disconnects the already-READY thread permanently.

SMP-1 prevents access after an instance has been published but does not by
itself handle a primary operation which wins the kernel lock before the
secondary has registered at all.

**Proposed fix:** document that a non-NULL owner must name a fully initialized,
registered instance and assert that requirement while holding the kernel lock.
Do not make creation busy-wait implicitly. Applications that create remote
threads during startup need an explicit instance-ready synchronization point.
Add positive remote-affinity coverage and an assertion-only negative case that
does not run in the normal regression flow.

## Open portability question

### BASEPRI synchronization on ARMv8.1-M implementations

The ARMv8-M-ML-ALT port raises `BASEPRI` without an ISB in `port_lock()` and
`port_suspend()`, and its assembly context-load path restores `BASEPRI`
without one. Some Arm guidance shows an ISB after `MSR BASEPRI`, including a
special reentrant-interrupt sequence. That is not sufficient to classify the
port as defective:

- the [Armv8-M Architecture Reference Manual](https://developer.arm.com/documentation/ddi0553/latest/)
  specifies `BASEPRI` masking but does not state in that section that every
  `BASEPRI` write must be followed by an ISB;
- Arm's [ordinary BASEPRI critical-section example](https://developer.arm.com/community/arm-community-blogs/b/embedded-and-microcontrollers-blog/posts/cutting-through-the-confusion-with-arm-cortex-m-interrupt-priorities)
  uses `__set_BASEPRI()` without an ISB; and
- no equivalent single-core `SV#8` failure has been observed on the RP2350
  Cortex-M33, while an unconditional ISB in every RT lock and suspend path has
  a significant and measurable hot-path cost.

The deeper ARMv8.1-M Cortex-M52, Cortex-M55, and Cortex-M85 implementations
still merit explicit qualification. Their implementation could expose timing
not observed on Cortex-M33, but no applicable public TRM rule or erratum has
yet been identified which confirms that they require an ISB on these paths.
Until such evidence or a target failure exists, this is not a confirmed defect
and no unconditional barrier change is proposed.

[Issue #205](https://github.com/chibios-upstream/chibios/issues/205) remains an
SMP-specific observation. The `BASEPRI` value read at the eventual halt is not
the vector-entry value because `__dbg_check_enter_isr()` first calls
`port_lock_from_isr()`, which writes `BASEPRI` again. The successful spinlock
acquisition with local `lock_cnt == 1` demonstrates a physical/logical locking
mismatch and is relevant to SMP-1, but SMP-1 alone does not clear the local
`BASEPRI` mask. The complete chain behind the reported `SV#8` and `SV#4`
failures therefore remains unresolved; it must not be used as confirmation of
a missing-ISB defect.

**Qualification work:** run the existing IRQ-STORM test for long durations on
available Cortex-M52, Cortex-M55, and Cortex-M85 targets with
`CH_DBG_SYSTEM_STATE_CHECK` enabled. Its two kernel-aware timer interrupts and
mailbox worker chain repeatedly cross kernel lock and interrupt boundaries; a
kernel-aware interrupt admitted before a raised `BASEPRI` becomes effective
should be trapped by the existing ISR-entry state check. Some current
IRQ-STORM board configurations disable that check for performance, so the
target qualification configuration must explicitly enable it.

Only if IRQ-STORM reports a failure is special first-vector instrumentation
needed to distinguish a delayed `BASEPRI` effect from another lock-state
mismatch. Capture `BASEPRI` before `CH_IRQ_PROLOGUE()` together with the kernel
spinlock state and owning core. If a specific implementation then proves to
need synchronization, add a port/core conditional synchronization hook rather
than imposing the ISB cost on all ARMv8-M lock operations.

## Cross-checked non-defects

### Best-effort reschedule FIFO writes are valid on the current two-core ports

`port_notify_instance()` sends a token only if `FIFO_ST.RDY` reports space.
When the outbound FIFO is full, it already contains traffic for the only other
core. Draining that FIFO invokes an IRQ epilogue and therefore a preemption
check. If the receiver drains concurrently with the sender, the sender still
holds the shared kernel lock while changing the ready queue; the receiver
cannot schedule against the new state until it acquires that lock. Interrupt
level/pending behavior then supplies another epilogue if needed. No durable
per-notification counter is required.

This reasoning depends on the current exactly-two-core RP ports and on all
callers notifying only a remote owner. It should be revisited for a port with
three or more instances because the RP implementations ignore their `oip`
argument and always signal the other core.

### Notification before ready-list insertion is ordered correctly

`chSchReadyI()`, `chSchWakeupS()`, and `__sch_requeue_behind()` can notify a
remote owner before completing the ready-list mutation. The initiating core
holds the shared spinlock, and the target scheduler must acquire that lock
before inspecting the queue. The release/acquire barriers on the spinlock make
the completed mutation visible. Reordering the notification after insertion
is not required for correctness.

### Cross-core priority inheritance is correctly propagated

Mutex dependency walks and arbitrary-priority recomputation operate while the
global lock is held. A boosted or lowered READY thread is requeued in its
owner's list and the owner is notified. A remotely running thread is notified
when a priority decrease can require rescheduling. Raising the priority of an
already-running remote owner does not itself require a reschedule. Unlock-side
priority reduction occurs on the local owning thread and S-class mutex APIs
reschedule before returning.

### Context-switch lock transfer is internally consistent after startup

The ARMv6-M, ARMv8-M-ML-ALT, and Hazard3 interrupt-preemption paths retain the
shared lock across scheduler selection and release it in the continuation
which returns to an unlocked thread. A continuation resuming an S-class
operation retains the lock. The defects above concern how the initial
execution flows enter that model, not ordinary context-switch ownership.

### Per-instance virtual timers and pinned ownership are coherent

Threads do not migrate. Timed waits arm the virtual-timer list of the waiting
thread's owner, and the timeout callback runs on that instance. Normal
cross-core wakeup and timeout cancellation are serialized by the shared lock.
The existing multicore virtual-timer owner assertions correctly reject
cross-instance timer manipulation in assertion builds.

## Test coverage assessment

The merged `test/rt/testbuild` and `test/rt-ports/testbuild` configurations set
`CH_CFG_SMP_MODE` to false. The three RP demos are the only merged RT SMP
applications. They compile the ordinary RT tests but do not automatically run
an SMP test and the test thread descriptors default their owners to the local
instance.

`origin/chibios-smp-sim-dev` adds a useful two-core simulator test for startup,
shared-lock contention, remote semaphore wakeups, masked/coalesced
notifications, and remote current-thread priority changes. It is not in
master, does not exercise RP FIFO saturation or flash lockout, and uses a
simulator-specific first-unlock rendezvous which masks SMP-1 for that port.

Verification performed in this pass:

- `demos/RP/RT-RP2040-PICO`: optimized ARMv6-M SMP build with LTO passed;
- `demos/RP/RT-RP2350-PICO2`: optimized ARMv8-M-ML-ALT SMP build with LTO
  passed;
- both demo trees were cleaned after the builds;
- the Hazard3 source, assembly, startup, timer, notification, and durable-halt
  paths were inspected; a RISC-V cross compiler was not available in the
  current environment.

The successful builds verify configuration and code generation only. They do
not invalidate the startup interleavings, FIFO-full behavior, or readiness
timing described above.
