# Note: Async VFS for Sandboxes (SB stall problem)

Design discussion outcome, 2026-06-11. Addresses the main limitation of
the guest-side threading model: a blocking POSIX call stalls the whole
sandbox.

Related: [note_sb_isolation_security.md](note_sb_isolation_security.md)
(margin 1, TOCTOU — the worker introduced here is a software instance of
the DMA case described there).

## Background: the guest threading model

Multi-threading inside an SB is provided by a **sub-scheduler inside the
sandbox** (green threads over the single host thread), driven
preemptively by VRQs with `SB_VRQ_ALARM` as the tick. Host-side
multi-threading per SB was assessed and rejected as too costly: it
requires migrating per-SB state to per-thread (`u_psp`, VRQ targeting),
a concurrency audit and locking of the FD table and host services
(use-after-free class risks in the host), process-semantics lifetime and
fault policy, and it destroys the mid-syscall exclusion defense (guest
code unable to run while a syscall is in flight), promoting copy-in-once
to a baseline requirement for all buffers.

The sub-scheduler keeps every host-side invariant intact at near-zero
cost: a green-thread switch is a guest-mode register swap (no SVC, no
MPU traffic), the SB remains one schedulable entity with one priority
(composes with core binding), and all defenses in the isolation note
hold unchanged.

## The problem

POSIX is a synchronous ABI and the ChibiOS VFS is synchronous down
through the drivers. A guest `read()` on a slow medium parks the host
thread in the kernel, and with it **every** green thread in the sandbox
— the sub-scheduler cannot run because its carrier thread is blocked.
VIO does not have this problem (born async, VRQ completion); VFS is the
synchronous holdout precisely because it is exposed as POSIX.

## Design: async host ABI, synchronous guest semantics

Keep POSIX as the guest-visible semantics; make the host ABI
asynchronous underneath; reconstruct the blocking illusion **per green
thread** in the guest runtime. Block the green thread, never the
carrier.

### Host side: submit / complete

- The syscall becomes **submit**: validate, enqueue, return immediately.
  The dispatcher never blocks; the syscall path gets *faster* than
  today.
- The blocking VFS call is executed by a **host worker thread** on the
  SB's behalf (rewriting the VFS/driver stack to be natively async is
  out of scope; the worker adapts the sync stack to the async boundary
  at minimal cost).
- **Completion is a VRQ**: one I/O-completion VRQ plus per-slot status,
  using the VRQ flags mechanism (`sbVRQSetFlagsI` is documented as "a
  fast way to transmit a (virtual) peripheral status" — this is exactly
  that). A small submission-slot table per SB lives next to
  `vfs_nodes[]` in `sb_ioblock_t`; slot count is the natural quota for
  in-flight operations per SB.
- VFS becomes, in effect, another virtual peripheral, unifying its model
  with VIO.

### Submission as an IRQ-like fastcall

With the IRQ-like fastcall design
([note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md) point 5),
submit does not even need to be a syscall: validate the slot, range-check
the buffer, copy the metadata, enqueue, `chThdResumeI(worker)` — exactly
ISR-shaped, never blocks, single exception. Status reads and cancellation
are trivial fastcalls too (cancel = I-class flag set + worker poke). The
hot I/O path then never enters the privileged dispatcher at all:
**fastcall = submit/status/cancel, worker = execution, VRQ = completion**.

- **Priority interplay gives both service models for free**: worker above
  the SB thread -> submit preempts into the worker immediately (doorbell
  with synchronous service); worker at the SB's priority (the default
  above) -> the guest keeps running, submits several slots, the worker
  drains when the SB yields or blocks — batching emerges naturally with
  no batching API. A per-SB tuning knob, not a design decision.
- **Backpressure**: slot table full -> the submit fastcall returns a
  try-again status immediately (I-class never blocks); the guest runtime
  yields on the completion VRQ.
- **Bounded copy-in caveat**: the control-plane copy happens inside the
  fastcall handler at SVC priority, so it must be small and bounded. For
  `read`/`write` it is nothing (pointer + length validation; data plane
  is DMA-semantics). For `open` the pathname copy is the cost — bounded
  by a config max (64-128 bytes, ~100 cycles) it is acceptable;
  otherwise path-heavy operations stay on the syscall path.

### Guest side

Libc wrappers keep the POSIX face: submit, yield to the sub-scheduler,
resume on the completion VRQ, return the status. Synchronous per green
thread; the SB never stalls. The only place the host thread legitimately
parks is `vrq_wait` when the whole guest is idle.

### Migration

The synchronous POSIX syscalls stay for compatibility. The async
submit/complete ABI is added alongside; guests opt in via their runtime.
The sync path remains appropriate for trivial/fast operations where a
stall is irrelevant.

## Security analysis

### The worker is a software DMA engine on guest memory

Once an operation runs concurrently with guest code, the mid-syscall
exclusion defense does not cover it: this is the "region writable by
both the sandbox and a peripheral/DMA engine" TOCTOU case of the
isolation note's margin 1, created in software. The discipline splits
cleanly:

- **Control plane — copy-in at submit, mandatory.** Pathnames and all
  metadata: `sb_check_string` / range checks, then a private privileged
  copy; the worker never re-reads guest memory for metadata. This is the
  copy-in-once contract applied at the submit boundary.
- **Data plane — DMA semantics.** Read/write buffers are range-validated
  at submit and then treated like a hardware DMA target: a guest that
  mutates its own buffer mid-operation corrupts its own data, with no
  host exposure. Identical semantics to real DMA; no copy needed for
  bulk data.

### Teardown with operations in flight

A new instance of the late-producer / teardown windows already tracked
in open_points "Host": the worker must never touch SB memory after
termination or restart. The worker holds private copies of all metadata
(it never needs SB memory except the final data copy-out / status
write), and performs a terminated/generation check, in a critical
section, immediately before that final access — extending the existing
`chThdTerminatedX` guard pattern. Operations must be either cancellable
or safely detachable (worker completes into nowhere).

### Worker topology: per-SB, not a shared pool

A shared worker pool creates **cross-SB head-of-line blocking**: one
sandbox's slow SD read delays another sandbox's I/O — a QoS/interference
channel between supposedly isolated guests, and a DoS lever (a malicious
SB saturating the pool with slow operations). With few SBs per system:

- **One worker per SB** (decision): timing isolation between SBs,
  trivial accounting, priority inheritance from the SB thread is
  well-defined. Total cost: one host thread + privileged stack per SB —
  a fixed, bounded cost, far below the per-guest-thread bill that made
  host-side multi-threading too costly.
- Fallback if even that is too much: a pool with strict per-SB queue
  limits, with the interference channel accepted and documented.

The worker inherits the SB thread's priority (avoids inversion against
other SBs' workers and host threads).

## Open items

- Define the submission-slot ABI (slot table layout, status codes,
  cancellation semantics) and which VFS operations get async variants
  first (`read`/`write` on block-backed files are the offenders;
  `open` involves path resolution and can also be slow).
- Decide the one-in-flight-per-FD question (simplest: yes, reject
  overlapping ops on the same descriptor).
- Specify restart semantics: in-flight slots at restart are cancelled or
  detached before the new instance starts.
- Guest runtime: green-thread wait/wake integration with the completion
  VRQ, and the idle policy (`vrq_wait`).
