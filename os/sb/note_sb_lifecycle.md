# Note: SB Lifecycle and Restart Protocol

Design proposal, 2026-07-15. This note addresses sandbox termination,
restart, and late asynchronous VRQ producers that are not necessarily owned
by VIO, such as host worker threads, custom timers, DMA completion paths, or
board-specific IRQ sources.

## Problem

The current VRQ acceptance check relies on the state of the host thread. The
thread object is embedded in `sb_class_t` and reused on restart, however, so
thread state is not a sandbox lifecycle boundary. Once a replacement thread
has been spawned, an old producer sees a non-terminated thread and cannot be
distinguished from a producer belonging to the new execution.

The built-in VIO paths avoid this in normal operation by removing callbacks
and stopping drivers during sandbox exit or abort, and the alarm virtual
timer is reset. This is not a complete host contract: an application may add
other producers whose lifetime is independent of VIO and which can still
reference the sandbox after its thread has terminated.

Making every producer carry a sandbox generation would solve the ambiguity,
but would spread generation awareness through all callbacks and worker APIs.
The preferred model is instead an explicit lifecycle and a synchronous
producer-quiescence contract.

## Proposed state machine

```text
STOPPED -> STARTING -> RUNNING -> STOPPING
   ^                                  |
   +-------- explicit finalize -------+
```

- `STOPPED`: no producer may target the sandbox; a start operation is
  permitted.
- `STARTING`: the thread and VRQ state are being prepared; VRQs are rejected.
- `RUNNING`: the sandbox may accept VRQs.
- `STOPPING`: the guest has exited or aborted and producer teardown is in
  progress; VRQs are rejected. This is a durable state, not an automatic
  transition to `STOPPED`.

Sandbox exit and abort must set `STOPPING` before disabling VIO callbacks,
resetting timers, or notifying the host. Start APIs must accept only
`STOPPED`, set `STARTING` before reusing thread or VRQ state, and enter
`RUNNING` only after the new context is ready to start. Failed starts return
the object to `STOPPED` after cleaning any partial setup.

All lifecycle transitions and the corresponding VRQ-state changes must be
performed under the system lock. Thread state remains relevant when choosing
how to inject a VRQ into a running thread, but it no longer decides whether
the sandbox execution owns the event.

## Explicit finalization API

Termination leaves the sandbox in `STOPPING`. The host must then:

1. Disable custom IRQ and DMA completion sources.
2. Cancel timers and pending asynchronous operations.
3. Stop and join worker threads that can reference the sandbox or its memory.
4. Call an explicit API, provisionally named `sbFinalize()`.

`sbFinalize()` is preferable to a public state setter. It represents the
host's assertion that every external producer is quiescent, verifies that
the sandbox thread has terminated and that the lifecycle is `STOPPING`,
clears the VRQ state under the system lock, and finally changes the state to
`STOPPED`. A subsequent start is rejected until finalization succeeds.

The implementation must define the ordering between `sbWait()`, thread
release, dynamic sandbox-memory release, and `sbFinalize()`. Memory referenced
by an asynchronous producer must not be released before that producer has
been stopped or joined. This may require finalization to become the resource
release boundary, or require memory-touching subsystems to participate in
the termination cleanup before `sbWait()` releases the thread.

For a sandbox using only built-in VIO and the alarm timer, the existing
termination cleanup already quiesces its producers, so the host can finalize
immediately after observing termination.

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
- VRQs and flags raised in `STOPPING`, `STOPPED`, and `STARTING` are ignored.
- Start is rejected before explicit finalization.
- Finalization is rejected while the thread is live or from the wrong state.
- A custom worker is stopped and joined before finalization, then cannot
  deliver a completion into the replacement execution.
- Dynamic sandbox memory remains valid until every memory-touching producer
  has been quiesced.
