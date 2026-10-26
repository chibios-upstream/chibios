# Spec: SB Virtual Shared Memory (VSHM), draft

Specification draft started 2026-10-26 from the design notes in
`vshm.md`, nothing implemented yet. A mechanism of shared memory among
sandboxes: a sandbox creates a shared memory, other sandboxes attach to it
and take turns using it, one at a time, to exchange messages with the
creator. Unnamed SMs are private, the way a sandbox allocates memory at run
time, with attributes such as coherent for DMA buffers.

It takes the role of VSRV, parked in branch `park/sb-vsrv`, with a lower
level and simpler model: no host-defined services, no per-client slices.
Most of the VSRV host infrastructure carries over, see
[Reuse from VSRV](#reuse-from-vsrv).

Related: [note_sb_lifecycle.md](note_sb_lifecycle.md) (producer contract,
PIDs), [note_sb_isolation_security.md](note_sb_isolation_security.md)
(copy-in-once, TOCTOU),
[note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md) (switched
region tables).

## Concepts

- A **shared memory** (SM) is created by a sandbox, its **creator**. Its
  memory is host-owned, never part of a sandbox's own memory, allocated on
  creation.
- SMs are **reference-counted**: the creator and every attached sandbox
  hold a reference. The memory is released when the last reference goes, a
  terminating sandbox releases its references in its cleanup phase.
- An SM is **named**, others can attach to it by name, or **unnamed**,
  private to its creator, see [Private SMs](#private-sms).
- **Attributes**, set at creation, select the memory the SM is allocated
  from and its MPU mapping, see [Attributes](#attributes).
- The SM metadata, in host memory, holds the name, the creator and the PID
  of its creating execution, size, attributes, maximum hold time, creator
  VRQ and the access state.
- **Groups.** Each sandbox is assigned a group ID (GID, 0 to 31) in its
  configuration, and a mask of friendly GIDs: a sandbox can attach to the
  SMs created by sandboxes whose GID is in its mask. Permissions are
  enforced by the configuration only, the host needs no knowledge of the
  software the sandboxes run.
- **Access.** The clients compete for the SM: one client at a time, the
  **holder**, has it mapped and uses it; the others hold references only
  and wait in a FIFO. Access is granted for at most the hold time set by
  the creator.
- **Message box.** One per SM, used by the holder: a request word and a
  reply status, see [Messages](#messages).

## Transaction cycle

```text
 client SB                  host (VSHM)                     creator SB
 ---------                  -----------                     ----------
                                                      <---- create("log",
                            allocates, maps the SM           size, hold,
                            in the creator                   vrq)
 attach("log") -----------> GID check, reference
 request(h) --------------> queued; when free: maps the
                            SM in the client, arms the
                            hold timer
 VRQ "granted" <----------
 writes the SM
 send(h, msg) ------------> unmaps the SM from the client,
                            stops the timer,
                            request word -> box ---------> VRQ "message"
                                                            receive(),
                                                            reads SM,
                                                            writes SM
                            maps the SM in the client,
                            re-arms the timer,
 VRQ "answer" <------------ status <--------------------- reply(status)
 result(h), reads the SM
 ... more messages ...
 release(h) --------------> unmaps the SM from the client,
                            stops the timer, grants the
                            next waiter
 detach(h) ---------------> reference released
```

1. **Attach**: the client takes a reference to the SM by name, after the
   GID check. Nothing is mapped, no MPU region is used.
2. **Request access**: the client joins the FIFO of the SM. When the SM is
   free the host grants it to the first waiter, maps the SM in it, arms the
   hold timer and raises "granted" on the client VRQ.
3. **Messages**: the holder writes the SM and sends a message, the SM is
   unmapped from it and stays with the creator until the answer, when it is
   mapped again; the holder then reads the answer from the SM. It can
   exchange several messages while it holds access.
4. **Release**: the client releases access, in its own syscall: the SM is
   unmapped from it, the timer is stopped, the next waiter is granted. The
   client keeps its reference and can request access again.
5. **Detach**: the client releases its reference, also releasing access or
   leaving the queue if needed.

## Forced release

The creator sets the maximum hold time at creation, it counts the time the
holder has the SM mapped. The host arms a virtual timer when access is
granted, stops it at a send and re-arms it at the answer for the remaining
time; on expiry, in the timer callback:

- the SM is unmapped from the holder: the entry of its regions table, and
  the MPU registers if the holder is the thread running when the timer
  fires;
- the holder gets "access lost" on its VRQ;
- the next waiter is granted.

No message is in flight at a forced release: the SM is never taken from the
creator while it works on a message, its processing time is not counted, a
slow creator stalls its own clients only.

After a forced release the operations of the old holder return an error,
it is no longer the holder; touching the SM faults and terminates the
sandbox, the cleanup then releases its references. A client keeping access
beyond the hold time is a misbehaving client, the creator and the other
clients are not stalled by it.

## Messages

- The holder sends one request word: a fixed message, `0xFFFFFF00` to
  `0xFFFFFFFF` (the code in the low byte), or a pointer into the SM to a
  command structure of at least 8 bytes, the first word the command, the
  second its real size.
- The host checks only that a pointer is inside the SM, aligned, with room
  for the 8 bytes; it never reads the SM.
- Sending unmaps the SM from the holder, in its own syscall, the reply maps
  it again. While the creator works on a message it is the only one with
  access: it uses the command structure in place, no copy is needed
  against the holder rewriting it. The creator still validates it, the
  content comes from the client. A holder touching the SM before the
  answer faults.
- The reply is a status code, any result data is written in the SM.
- One message in flight at a time, the reply slot is reserved with the
  request.

## Events

- **Creator VRQ**, chosen at creation: client attached, client detached,
  message received.
- **Client VRQ**, chosen at attach: access granted, answer ready, access
  lost (forced release), creator lost.

Flags accumulate in the VRQs until delivered, events carry no data. Every
raise is bound to the PID of the target execution
(`sbVRQRaisePidS()`/`sbVRQRaisePidI()`), a restarted sandbox never gets an
event meant for its previous execution.

## Lifecycle

- **Client termination**: its references are released; as holder its
  access is released and the next waiter granted, as waiter it leaves the
  queue. With a message in flight the SM stays with the creator until the
  reply, which is discarded, then the next waiter is granted; the same for
  a release or a detach with a message in flight.
- **Creator termination**: its reference is released and the SM becomes
  orphaned: the name is removed, the holder, the waiters and the attached
  clients get "creator lost", further requests and messages fail, a
  message in flight gets no answer and the SM is not mapped again. The
  memory stays until the last reference is released, so no client faults
  because its creator went away.
- **Destroy** by the creator: same as its termination for that SM.
- A stale client writing the SM, believing an older creator execution is
  there, only reaches memory nobody else uses; the memory is reused only
  after the last reference is released.

## Private SMs

An unnamed SM is private to its creator, the way a sandbox allocates memory
at run time:

- no attach, no access requests, no message box, no hold time, no events;
  the reference of the creator is the only one;
- it is mapped in the creator, one switched region, until it is destroyed
  or the creator terminates, then its memory is released;
- the host services accept pointers into the private SMs of the calling
  sandbox, so they can be buffers of VIO operations, DMA included;
- switched regions are few, those left unused by the sandbox configuration
  out of 4 on ARMv7-M: a sandbox needing many buffers creates one private
  SM and sub-allocates it itself.

A private SM can be destroyed while a host driver still transfers into it,
a DMA started by a VIO operation: its memory must not go to another
sandbox meanwhile, see the open questions.

## Attributes

Set at creation, for named and private SMs alike, they select the memory
pool and the MPU attributes of the mapping. Memory never changes attributes,
each attribute has its own pool.

- **coherent**: non-cacheable memory, for the buffers of DMA operations. A
  sandbox can't do cache maintenance, the registers are privileged, and the
  host drivers transferring by DMA on its behalf do none. The mapping uses
  the non-cacheable encoding of `SB_REG_IS_NOCACHE_DATA`, TEX/C/B on
  ARMv7-M, MAIR index 2 on ARMv8-M. On cores without data cache it is
  normal memory.

The host CPU must not reach coherent memory through the cache, mismatched
attributes lose coherency:

- in the syscalls of the sandbox its table is loaded, host accesses go
  through its region and are non-cacheable; from other contexts, host
  threads, ISRs, another table loaded, they go through the default memory
  map, cacheable;
- a static non-cacheable region over the pool is no way out. On ARMv7-M the
  switched regions are 0 to N-1, the lowest priority, a static region
  overrides the sandbox mapping: the `STM32_NOCACHE` region of the H7/F7
  HAL, region 6 with AP RW/RW, would open the whole area to every sandbox,
  a privileged-only one would deny the sandbox its block. On ARMv8-M
  overlapping regions fault;
- so the host clears coherent memory at allocation with cache maintenance
  of the range, and then touches it only inside the syscalls of its
  sandbox; the drivers access it by DMA only.

## MPU

- The creator maps each of its SMs, one switched region per SM, for the
  life of the SM in its execution.
- A client uses one switched region only while it holds access; attached
  references use none.
- Mappings are written in `mpuregs[]` only, `regions[]` keeps the static
  configuration, `sbStart()` rebuilds clean tables. The pointer checks of
  the host syscalls accept `regions[]` and the private SMs of the caller,
  they reject pointers into named SMs.
- Mapping at grant and at the answer is done by the host in a client that
  is not running, it only adds access. Removing access happens at send and
  at release, in the client's own syscall, or at forced release, where a
  fault of the holder is the accepted outcome.
- Per message the cost is a live MPU update in the client at send and a
  table entry written at the answer, loaded by the next switch to the
  client.
- On SMP hosts every change runs on the core of the sandbox, see
  [Multicore](#multicore).
- ARMv7-M regions are a power of two in size, at least 32 bytes, aligned to
  their size; ARMv8-M needs 32 bytes alignment of base and size.

## Multicore

On SMP hosts a sandbox thread belongs to one core, and everything touching
it must run there: its MPU registers are loaded only on that core, virtual
timers are per core, `sbVRQTriggerS()`/`sbVRQTriggerI()` assert the core.
VSHM operations cross cores: a release grants a waiter elsewhere, a reply
answers a client elsewhere, a send signals the creator elsewhere.

- Changes in the caller's own syscall stay direct: the unmap at send and
  at release, the creator mapping at creation.
- Changes for another sandbox are posted to its core: the pending work is
  recorded in its state and a per-core host thread is woken there with the
  RT cross-core wakeup, `chSchReadyI()` notifying the instance. The thread
  maps the SM, arms the hold timer and raises the VRQs on that core. The
  target is not running while the thread runs, a mapping is a table write,
  loaded at the switch back.
- The hold timer is armed on the core of the client, the forced release
  runs there and can update the live registers.
- The cleanup of a terminated client is posted to its core too, it stops
  its hold timer.
- On a single core, and for a target on the same core, posting is a direct
  call, the semantics are the same.

The alternative, the client collecting the grant and the answer with its
own syscalls and mapping there, still needs cross-core events and leaves
the time between grant and collection out of the hold timer.

## Configuration

Per sandbox, set while it is stopped, as the VIO configuration:

- GID and mask of friendly GIDs;
- right to create SMs, and the memory it can create them from, one pool
  per attribute, see the open questions;
- storage of its handle table.

## Reuse from VSRV

From `park/sb-vsrv`, mostly as it is:

- `sbMapRegion()`/`sbUnmapRegion()`, with variants for a sandbox that is
  not running (grant) and for the timer callback (forced release), the
  region encoding of `sbhost.c` for the attributes;
- sandbox PIDs and `sbVRQRaisePidS()`/`sbVRQRaisePidI()`, `CH_RET_ESRCH`;
- handle tables, pointer check of the request word, fixed messages and
  command header, generation checks, termination cleanup hooks;
- the syscall and fastcall codes with sub-codes, the testsb harness and the
  NIL-MULTI demo structure.

## Open questions

1. **Memory at creation**: pools per sandbox, one per attribute, set in
   configuration, give a quota, so that a sandbox can't exhaust the memory
   of the others, and confine a DMA outliving a private SM to the memory of
   its own sandbox. A host-wide pool would need the asynchronous operations
   to hold a reference instead. An orphaned named SM stays charged to its
   creator across a restart, until the last client detaches. ARMv7-M
   alignment makes a general heap wasteful.
2. **Clearing between holders**: the next holder sees what the previous one
   left; clearing at grant, as an attribute, or trust within the GID.
3. **Waiting with a timeout**: guest-side, with the client VRQ alarm and a
   cancel of the request, so no host timer is needed except the hold
   timer.
4. **Creator regions**: one per SM, a creator with several SMs could use a
   pool mapped once.
5. **Right to create**: any sandbox with memory to create from, or a
   configuration flag.
6. **Waiters order**: FIFO, or by priority.
7. **Multicore**: posting to the core of the target is not VSHM specific,
   it gives SB cross-core VRQs in general; a host thread per core and a
   cross-core wakeup in the grant and answer latency.
8. **More attributes**: read-only for the clients of a named SM, clear on
   grant.
9. **DMA into named SMs**, for zero-copy pipelines: the DMA can outlive the
   access and the reference of the client that started it.
10. **NIL hosts** have no virtual timers, the hold timer needs another
    mechanism there.
