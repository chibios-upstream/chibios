# Note: SB Lifecycle and Restart Protocol

Design proposed 2026-07-15 and implemented 2026-07-23. This note addresses
sandbox termination, restart, and late asynchronous VRQ producers that are
not necessarily owned by VIO, such as host worker threads, custom timers, DMA
completion paths, or board-specific IRQ sources.

Any future lifecycle work is tracked only in [open_points.md](open_points.md).

## Problem

The previous VRQ acceptance check relied on the state of the host thread. The
thread object is embedded in `sb_class_t` and reused on restart, so thread
state is not a sandbox lifecycle boundary. Once a replacement thread had
been spawned, an old producer saw a non-terminated thread and could not be
distinguished from a producer belonging to the new execution.

The built-in VIO paths avoided this in normal operation by removing callbacks
and stopping drivers during sandbox exit or abort, and the alarm virtual
timer was reset. This was not a complete host contract: an application could
add other producers whose lifetime was independent of VIO and which could
still reference the sandbox after its thread had terminated.

Making every producer carry a sandbox generation would solve the ambiguity,
but would spread generation awareness through all callbacks and worker APIs.
The implemented model instead uses an explicit lifecycle and a synchronous
producer-quiescence contract.

## State machine

```text
UNINIT -- object init --> STOPPED -> STARTING -> RUNNING -> STOPPING
                            ^                                  |
                            +-------- explicit finalize -------+
```

- `UNINIT`: the sandbox object has not been initialized; zero-initialized
  static storage has this state and no sandbox operation is permitted.
- `STOPPED`: no producer may target the sandbox; a start operation is
  permitted.
- `STARTING`: the thread and VRQ state are being prepared; VRQs are rejected.
- `RUNNING`: the sandbox may accept VRQs.
- `STOPPING`: the guest has exited or aborted and producer teardown is in
  progress; VRQs are rejected. This is a durable state, not an automatic
  transition to `STOPPED`.

`sbObjectInit()` is the only transition from `UNINIT` to `STOPPED`. There is
no transition back to `UNINIT`; object disposal is outside this execution
lifecycle.

Sandbox exit and abort must set `STOPPING` before disabling VIO callbacks,
resetting timers, or notifying the host. Start APIs must accept only
`STOPPED`, set `STARTING` before reusing thread or VRQ state, and enter
`RUNNING` only after the new context is ready to start. Failed starts return
the object to `STOPPED` after cleaning any partial setup.

All lifecycle transitions and the corresponding VRQ-state changes must be
performed under the system lock. Thread state remains relevant when choosing
how to inject a VRQ into a running thread, but it no longer decides whether
the sandbox execution owns the event.

## Explicit synchronization and finalization APIs

Termination leaves the sandbox in `STOPPING`. The host must then:

1. Call `sbSync()` while the sandbox is `RUNNING` or `STOPPING` to wait until
   its thread reaches the final state.
2. Disable custom IRQ and DMA completion sources, cancel pending operations,
   and stop or join workers that can reference the sandbox or its memory.
3. Call `sbFinalize()`.

`sbSync()` wraps `chThdSync()`. The original thread reference remains owned
by `sb_class_t`, so synchronization does not invoke the thread dispose
callback and dynamic sandbox memory remains valid while the host quiesces
external producers. Calling it while the sandbox is `RUNNING` blocks until
termination changes the lifecycle to `STOPPING`; calling it after that
transition waits only if the thread has not reached its final state yet. It
returns with the lifecycle still in `STOPPING`.

`sbFinalize()` is preferable to a public state setter. It represents the
host's assertion that every external producer is quiescent. It verifies that
the sandbox thread has terminated and that the lifecycle is `STOPPING`,
releases the thread reference owned by `sb_class_t`, and changes the lifecycle
to `STOPPED`. Releasing that reference is the dynamic sandbox-memory release
boundary. A subsequent start is rejected until finalization succeeds.

For a sandbox using only built-in VIO and the alarm timer, the existing
termination cleanup already quiesces its producers, so the host can finalize
immediately after `sbSync()` returns.

## VRQ producer contract

`sbVRQTriggerI()`, `sbVRQTriggerS()`, and flag updates must have no effect
unless the sandbox state is `RUNNING`. Flag setting and triggering are
normally performed within one system-lock interval; a combined raise helper
could make this contract harder to misuse, but is not required by the state
model.

The protocol deliberately does not identify producer generations. Its
correctness depends on the host not calling `sbFinalize()` until all old
producers are synchronously quiescent. If a producer cannot be cancelled,
drained, detached without retaining any sandbox reference, or joined, that
producer needs a separate generation-aware endpoint; lifecycle state alone
cannot distinguish its old event after a new execution reaches `RUNNING`.

## Required validation

- Normal guest exit and fault/abort both enter `STOPPING` before cleanup.
- Zero-initialized static storage reports `UNINIT`; start, finalization, VRQs,
  and configuration are rejected until `sbObjectInit()` enters `STOPPED`.
- VRQs and flags raised in `UNINIT`, `STOPPING`, `STOPPED`, and `STARTING` are
  ignored.
- `sbSync()` can be entered from `RUNNING` or `STOPPING` and returns in
  `STOPPING`.
- `sbSync()` does not release the controller-owned thread reference or dynamic
  sandbox memory.
- Start is rejected before explicit finalization.
- Finalization is rejected while the thread is live or from the wrong state.
- A custom worker retaining sandbox memory is stopped and joined before
  finalization; its late completion is rejected in `STOPPING`, and the joined
  worker cannot deliver into the replacement execution.
- Dynamic sandbox memory remains valid until every memory-touching producer
  has been quiesced.
