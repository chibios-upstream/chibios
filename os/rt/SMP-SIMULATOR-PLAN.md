# SMP Simulator Implementation Plan

## Objective

Add an isolated two-core simulator for ChibiOS/RT using one POSIX pthread per
simulated core. The existing single-core simulator and its test infrastructure
must remain unchanged.

The work is divided into two deliverables:

1. A usable `SIMX86_64-SMP` port with a dedicated `posix-smp` HAL and a
   minimal smoke application.
2. A separate `test/rt_smp` suite targeting SMP-specific behavior.

Development takes place on the stable `chibios-smp-sim-dev` branch and
worktree. Pull requests are created from temporary branches so that the
development branch is not deleted when a pull request is merged.

## Design principles

- Use one host pthread per simulated CPU core.
- Keep ChibiOS threads as lightweight contexts scheduled within their owning
  simulated core.
- Support two cores initially because the current RT instance definitions
  provide `ch0` and `ch1`.
- Keep thread ownership fixed through `thread_descriptor_t.owner`.
- Serialize kernel operations using one shared host-side atomic spinlock.
- Model inter-core notifications as asynchronous, target-specific virtual
  IPIs.
- Make reschedule notifications level-triggered and coalescible.
- Preserve the existing simulator implementation until the SMP design has
  stabilized; factor common code only afterward.
- Keep the initial implementation x86-64-only. An IA32 variant can be
  considered later.

## Phase 1: Isolated development environment

Create:

```text
branch:   chibios-smp-sim-dev
worktree: chibios-smp-sim-dev
```

The branch starts from an updated `master` containing the arbitrary-thread
priority helper and its abstraction-layer users.

The existing `SIMX86_64`, POSIX HAL, and RT test targets are not modified by
the initial bring-up work.

### Completion criteria

- The stable branch and worktree exist.
- The worktree is based on the intended `master` revision.
- This plan is present under `os/rt`.

## Phase 2: SMP simulator port skeleton

Create:

```text
os/common/ports/SIMX86_64-SMP/
os/common/ports/SIMX86_64-SMP/compilers/GCC/port.mk
os/hal/ports/simulator/posix-smp/
```

Initially copy the existing simulator port instead of extracting shared
components.

The new port will:

- define `PORT_CORES_NUMBER` as two;
- use `SIMX86_64_SMP` in C identifiers and Doxygen group names;
- link using `-pthread`;
- provide the core-1 stack symbols expected by `chsys.c`;
- identify itself explicitly as the SMP simulator;
- remain selectable only from a dedicated build target.

### Completion criteria

- The original simulator builds without changes.
- The SMP simulator skeleton compiles and links independently.
- No existing test Makefile selects the new port implicitly.

## Phase 3: Host-core startup and per-core state

Use the original process thread as simulated core 0 and create one pthread for
simulated core 1.

Core 1 invokes an application-provided `c1_main()` function. It follows the
existing RP SMP startup model:

1. Wait for `ch_sys_running`.
2. Initialize `ch1` using `chInstanceObjectInit()`.
3. Unlock the new instance.
4. Enter its application or test activity.

Convert the following simulator state to thread-local storage:

- simulated core ID;
- interrupt status;
- ISR-context state;
- host pthread identity where required.

Add the required publication and acquisition barriers around system and
secondary-core startup.

### Completion criteria

- Both `ch0` and `ch1` initialize.
- Each pthread reports the correct simulated core ID.
- Threads explicitly owned by each instance execute only on that instance.
- Core initialization is repeatable without startup races.

## Phase 4: Shared SMP kernel locking

Implement a shared kernel spinlock using lock-free atomics. A pthread mutex
must not be used in the interrupt path.

`port_lock()` performs:

1. Mask the virtual IPI signal on the local host pthread.
2. Acquire the shared kernel spinlock.
3. Service any pending local preemption.
4. Return in the normal ChibiOS locked state.

`port_unlock()` performs:

1. Release the shared kernel spinlock.
2. Unmask the virtual IPI signal.

Use acquire/release memory ordering around the spinlock and notification
state. Preserve the normal ChibiOS rule that the kernel lock can be
transferred across a context switch and is released by the resumed execution
path.

### Completion criteria

- Both cores repeatedly enter kernel critical sections without corruption.
- Debug state checking remains enabled and passes.
- Simultaneous kernel entry does not deadlock.
- Lock ownership remains correct across context switches.

## Phase 5: Signal-driven virtual IPIs

This phase is the main feasibility gate.

`port_notify_instance()` publishes a pending reschedule for the target core
then sends a target-specific signal:

```c
atomic_store_explicit(&preemption_pending[id], true,
                      memory_order_release);
pthread_kill(core_thread[id], SIM_SMP_IPI_SIGNAL);
```

The signal handler behaves as an otherwise empty inter-core interrupt:

1. Enter through `CH_IRQ_PROLOGUE()`.
2. Record or preserve the pending condition.
3. Leave through `CH_IRQ_EPILOGUE()`.
4. Execute the local preemption epilogue under the shared kernel spinlock.

The implementation follows the existing Hazard3 preemption model:

- pending state is per-core and level-triggered;
- multiple notifications may be coalesced;
- the pending flag is cleared before a context switch;
- the reschedule decision is made on the target core;
- the POSIX signal frame is preserved on the preempted ChibiOS stack;
- a shared alternate signal stack is not used.

The pending flag is also checked after `port_lock()` acquires the shared
spinlock. This closes the race in which the target masks the IPI just before
signal delivery. Because the sender modifies the remote ready list while
holding the same spinlock, the target observes the completed modification and
reschedules before normal critical-section processing can later reach the
`chSysUnlock()` priority-order assertion.

The per-thread interrupt stack allowance must be checked against the actual
POSIX signal-frame requirements and increased if necessary.

### Feasibility tests

- Remotely ready a higher-priority thread while a lower-priority target thread
  is CPU-bound.
- Lower the priority of a current thread running on the other core.
- Deliver an IPI while the target has simulated interrupts masked.
- Coalesce repeated notifications without losing a required reschedule.
- Repeat preemption and resumption enough times to verify signal frames and
  signal masks unwind correctly.
- Exercise notification while both cores contend for the kernel spinlock.

If direct switching through the signal epilogue is not reliable, add an
x86-64-specific interrupt-return trampoline before proceeding.

### Completion criteria

- A busy remote core is preempted promptly.
- Masked IPIs are handled before the target proceeds through a stale
  scheduling state.
- No priority-order assertion is triggered.
- Repeated IPIs do not leak signal frames or leave the signal masked.

## Phase 6: Dedicated `posix-smp` HAL

Keep SMP HAL changes isolated under:

```text
os/hal/ports/simulator/posix-smp/
```

Initially:

- maintain system-timer state per core;
- let each idle core service its own virtual timers;
- assign serial input and console interrupts to core 0;
- make `port_wait_for_interrupt()` wake for a local virtual IPI;
- keep unrelated simulated peripherals single-owner.

Asynchronous timer-signal preemption is a possible later enhancement. SMP
inter-core notifications are asynchronous from the first usable version.

### Completion criteria

- Sleeping threads work independently on both cores.
- Timer state on one instance does not consume or delay the other instance's
  tick.
- Console and serial activity do not race between host cores.

## Phase 7: Minimal SMP smoke application

Add a dedicated build target selecting `SIMX86_64-SMP` and `posix-smp`.
Do not run the complete single-core RT suite from this target.

The smoke application covers:

- core-0 and core-1 initialization;
- one explicitly owned thread on each instance;
- cross-core wakeup;
- remote ready-list preemption;
- remote current-priority changes;
- basic shared spinlock contention;
- repeated virtual IPI delivery.

Assertions, parameter checking, and system state checking remain enabled.

### Completion criteria

- The smoke test completes repeatedly without hangs.
- There are no priority-order or lock-state assertions.
- Remote readying and priority changes cause execution on the target core.
- Existing single-core simulator tests still build and run unchanged.

## Phase 8: Dedicated SMP test suite

After the port is stable, create:

```text
test/rt_smp/
  configuration.xml
  config.fmpp
  rt_smp_test.mk
  source/test/
  testbuild/
```

Use host atomics only for initial orchestration and test barriers. The RT
primitive being tested must not also be the mechanism required to keep the
test harness operational.

All ChibiOS test threads are explicitly assigned to `ch0` or `ch1`.

### Test sequence 1: Instances and ownership

- Core identification.
- `ch0` and `ch1` initialization.
- Explicit thread ownership.
- Per-instance current and ready lists.
- Per-instance timer state.

### Test sequence 2: Inter-core notifications

- Remote readying.
- Idle-core wakeup.
- Busy-core preemption.
- Notification while masked.
- Notification coalescing.
- Notification during simultaneous kernel entry.

### Test sequence 3: Remote priority changes

- Remote ready-thread reordering.
- Remote current-thread priority raising.
- Remote current-thread priority lowering.
- Competing ready threads.
- Priority changes while waiting.

### Test sequence 4: Cross-core synchronization

- Semaphores.
- Events.
- Messages.
- Thread queues.
- Termination and wait operations.

### Test sequence 5: Cross-core mutex inheritance

- Single-owner donation.
- Transitive owner chains spanning both cores.
- Competing donations.
- Priority lowering and effective-priority restoration.
- Unlock and waiter reordering.

### Test sequence 6: Shared global facilities

- Registry traversal.
- RFCU operations.
- Global object lifetime.
- Simultaneous object creation and release.

### Test sequence 7: Stress

- Randomized remote wakeups.
- Repeated remote priority mutations.
- Kernel spinlock contention.
- Virtual IPI storms.
- Long-running masked and unmasked transitions.

Use an external watchdog for test execution so deadlocks fail
deterministically.

## Phase 9: Documentation and CI

Document:

- the pthread-per-core model;
- signal and spinlock interactions;
- the fixed two-core limit;
- ownership requirements;
- simulated peripheral ownership;
- timer and preemption limitations;
- supported host platforms.

Add a new explicit SMP CI target only after the smoke application and initial
SMP suite are stable. Existing simulator jobs remain unchanged.

## Pull request sequence

1. `SIMX86_64-SMP`, `posix-smp`, virtual IPIs, and the minimal smoke target.
2. Basic `test/rt_smp` sequences for instances, notifications, and priority
   changes.
3. Cross-core synchronization, priority inheritance, stress tests,
   documentation, and CI integration.

