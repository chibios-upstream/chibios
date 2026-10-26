# Spec: SB messages with temporary mapping (VMSG), draft

Specification draft started 2026-10-26, nothing implemented, the name is
provisional. The message mechanism of SB, next to [VSHM](note_sb_vshm.md)
which gives shared memory; messages need no shared memory at all. A client
sends a fixed format message, a command
and four parameters, snapshotted by the host at send and copied in the
memory of the server at receive. Parameters can point to buffers in the
client memory: the server gets temporary access to the client data region
to read the requests and write the answers there, the access ends with the
reply.

The system calls are asynchronous: a blocking call would stop the whole
guest RTOS of the sandbox. A guest thread waiting for an answer, or a
server thread waiting for a message, blocks on the objects of its own RTOS,
woken from the VRQ handler.

Messages need no memory allocation and no reference counts on memory, and
every MPU change happens in the table of the server, in its own syscalls.
The price is trust: the server sees the client data region. Bulk data
without that trust goes through a VSHM shared memory mapped by both.

Related: [note_sb_vshm.md](note_sb_vshm.md) (shared memory),
[note_sb_lifecycle.md](note_sb_lifecycle.md) (producer contract, PIDs),
[note_sb_isolation_security.md](note_sb_isolation_security.md)
(copy-in-once, TOCTOU),
[note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md) (switched
region tables).

## Concepts

- An **endpoint** is published by a sandbox, its **server**, under a name,
  with the server VRQ. The endpoint metadata, in host memory, holds the
  name, the server and the PID of its publishing execution, the queue of
  the messages and the message being processed.
- **Groups.** Each sandbox is assigned a group ID (GID, 0 to 31) in its
  configuration, and a mask of friendly GIDs: a client can open the
  endpoints of servers whose GID is in its mask. Sending exposes the client
  memory to the server, the mask lists the servers the client trusts.
- A **message** has a fixed format, a command and four parameters, five
  words. The meaning of the command and of the parameters is the protocol
  of the server, the host never interprets them. Parameters can be values
  or pointers to buffers in the client data region.
- A **simple** message, values only, doesn't open the client space at all:
  nothing is mapped. A message with buffers opens it. The client chooses
  at send, with a flag of the send call, the command word stays to the
  protocol.
- The message is **snapshotted** by the host at send in a message slot of
  the client, in host memory, and copied at receive in the memory of the
  server, with the PID of the client and the bounds of the client data
  region. The server validates its own copy, the client can't change it
  any more.
- **Message slots**: each client has a configured number of slots, the
  messages it can have in flight. A slot holds the snapshot, the state and
  the status of the reply; the send returns its identifier, slot and
  generation.
- The **reply** is a status code, stored in the slot; answer data is
  written by the server in the client buffers pointed by the message.
- Sandboxes have disjoint memory and no address translation: a pointer into
  the client data region is valid in the server as it is.

## Message cycle

```text
 client SB                  host (VMSG)                     server SB
 ---------                  -----------                     ---------
                                                      <---- publish("log",
                            registers the endpoint           vrq)
 open("log", vrq) --------> GID check, handle
 fills msg and buffers
 send(h, &msg, flags) ----> snapshots msg in a slot,
 gets id, continues         queues it ---------------------> VRQ "queued"
                                                      <---- receive(h, &rx)
                            maps the client data region
                            in the server, XN forced,
                            copies the snapshot, PID
                            and region bounds in rx -----> validates rx and
                                                            the pointers,
                                                            reads buffers,
                                                            writes answers
                            unmaps the client region, <---- reply(h, status)
 VRQ "answer" <------------ status in the slot
 result(id), reads the
 answers, slot freed
```

The diagram shows a message with buffers; a simple message skips the
mapping and the region bounds.

1. **Publish**: the server creates the endpoint, with its VRQ, the name is
   registered.
2. **Open**: the client takes a handle to the endpoint by name, after the
   GID check, and chooses its VRQ for the answers.
3. **Send**: the host copies the five words of the message in a free slot
   of the client and queues it on the endpoint, the server gets "queued"
   on its VRQ. The call returns at once with the identifier of the
   message, or an error when no slot is free.
4. **Receive**: returns the first queued message, or none. For a message
   with buffers the host maps the client data region in the server. It
   copies the snapshot, the PID of the client and the bounds of the region
   in the receive structure of the server, in its own memory, checked
   against its `regions[]`. A simple message has empty bounds: the pointer
   checks of the server reject every pointer, a server expecting buffers
   answers with an error.
5. **Reply**: the host unmaps the client region, if mapped, stores the
   status in the slot and raises "answer" on the client VRQ. The server
   can receive the next message.
6. **Result**: the client reads the status by identifier, pending or the
   reply status, which frees the slot.

A server processes one message at a time, the mapping takes one switched
region. Simple messages involve no MPU change at all.

## Guest side

- A client thread sends, then blocks on an object of its own RTOS; the VRQ
  handler of the endpoint collects the results of the completed messages
  and wakes their threads. Blocking send, with or without timeout, is a
  guest library function.
- A server thread blocks the same way until "queued", then receives.
- Other guest threads keep running while messages are in flight, the
  sandbox is never stopped by the host.

## Mapping

- Only messages with buffers are mapped. The host doesn't know which
  parameters are pointers, it maps the client data region: the writable
  region holding the client data, stack and system block, the one
  `sbStart()` locates. Buffers elsewhere are out of reach, for example
  constants in the code region of a static sandbox, string literals
  included; see the open questions.
- The mapping is the entry of that region in the client `mpuregs[]`,
  copied in a free switched region of the server with execute never
  forced: a code and data region, as for ELF sandboxes, must not become
  executable in the server. Nothing is computed, the client configuration
  already satisfies the MPU constraints, and the attributes are those of
  the client, cacheability matches.
- Only the table of the server changes, in its own receive and reply
  syscalls: the table entry and the live registers. The table of the
  client is never touched.
- `regions[]` of the server is unchanged: the pointer checks of the host
  syscalls of the server reject the client memory, a server can't make
  host drivers transfer into its clients.

## Trust

- The server sees the whole client data region: stack, heap, the system
  block with the VRQ state. It can control the client, so a client sends
  only to servers it trusts, the GID mask selects them. Servers don't
  trust clients. Simple messages expose nothing of the client.
- The message can't be rewritten, the server works on its own copy. The
  buffers can: the client keeps running while the server works, the server
  copies once what it validates from the buffers; a client rewriting its
  buffers hurts only itself.
- **Pointer checks by the server**: every pointer and size from the
  message must lie in the client region bounds given at receive. The
  memory of the server is mapped in the server too, a pointer into it
  would make the server read or write its own memory for the client. A
  guest helper does the check.
- **Narrower window**, later: mapping only the buffers needs the host to
  know them, the message would describe its buffers, for example flags in
  the command word marking pointer parameters with sizes, and the buffers
  must be MPU-mappable: ARMv8-M base and size aligned to 32 bytes, cheap;
  ARMv7-M a power of two aligned to its size, refined by eighths with
  subregions, wasteful. A region per buffer in the server.

## Events

- **Server VRQ**, chosen at publish: message queued, client lost.
- **Client VRQ**, chosen at open: answer ready, server lost.

Flags accumulate in the VRQs until delivered, events carry no data, the
state is read with receive and result. Every raise is bound to the PID of
the target execution (`sbVRQRaisePidS()`/`sbVRQRaisePidI()`).

## Cancellation

- A queued message can be cancelled by identifier, the slot is freed.
- A simple message being processed can be cancelled too, the late reply is
  discarded.
- A message with buffers being processed can't be cancelled, the client
  region is mapped in the server: the slot stays busy until the reply. A
  guest timeout only stops waiting.

## Lifecycle

- **Client termination**: queued messages are removed, simple messages
  being processed get their replies discarded. Messages with buffers being
  processed are the hard case: the client terminates while servers still
  have its region mapped, and its memory must not get a new execution, or
  another sandbox for dynamic sandboxes, while a server can reach it.
- **Release interval**: the servers get "client lost" and must undo the
  transaction within a fixed interval, a host configuration constant the
  same for every endpoint: stop touching the client region and reply, the
  reply is discarded. The client thread waits in `__sb_abort()` for those
  replies, at most the interval, with a thread timeout, so no virtual
  timer, NIL hosts included. The lifecycle stays `STOPPING` meanwhile,
  `sbWait()`/`sbFinalize()` are unchanged.
- At expiry the region is revoked from the servers still holding it, a
  fault there terminates the server, the accepted outcome for a server
  breaking the contract. The client then exits: a hung server never holds
  a terminated client longer than the interval.
- Revoking is a write in the table of the server: on the same core the
  server is not running, the client is, the table entry is enough; on
  another core it is posted to the core of the server, see
  [Multicore](#multicore), and the client waits for its completion.
- **Server termination**: the name is removed, the messages queued and
  being processed complete with an error status and "server lost" on the
  client VRQs, the handles of the clients become stale and fail. The
  termination of the server runs in its own `__sb_abort()`, which also
  releases a client waiting there for it: a crashed server doesn't hold
  its clients. Its table is rebuilt by `sbStart()`.
- A restarted server publishes a new endpoint; handles are bound to the PID
  of the server execution they were opened to, a client reopens.

## Multicore

The MPU changes of the message cycle are in the table of the server, in its
own syscalls, on its core; the client table is never touched. The events
cross cores, a send raises a VRQ on the server, a reply on the client, and
`sbVRQTriggerS()`/`sbVRQTriggerI()` assert the core: they need a
cross-core posting. The raise is recorded in the state of the target and a
per-core host thread is woken on its core with the RT cross-core wakeup,
`chSchReadyI()` notifying the instance; the thread raises the VRQ there. On
a single core, and for a target on the same core, posting is a direct call.
The same posting carries a revocation. The queue and the slots are under
the system lock.

## Reuse from VSRV

From `park/sb-vsrv`:

- `sbMapRegion()`/`sbUnmapRegion()`, the own-syscall variant, and the
  table-only write for revocation;
- sandbox PIDs, `sbVRQRaisePidS()`/`sbVRQRaisePidI()`, `CH_RET_ESRCH`;
- handle tables, generation checks, the name registry, termination cleanup
  hooks;
- the syscall and fastcall codes with sub-codes, the testsb harness and the
  NIL-MULTI demo structure.

Zero-copy bulk data without opening the client data region: a VSHM shared
memory mapped by client and server, simple messages pointing into it, see
[note_sb_vshm.md](note_sb_vshm.md).

## Open questions

1. **Release interval**: its value, and whether the server undoes with the
   reply or with a dedicated abort call.
2. **Slots**: number per client, storage in configuration as the handle
   table; result as a fastcall.
3. **Code region**: constants of a static sandbox, string literals, are out
   of reach; mapping the code region too, read-only, takes a second server
   region, otherwise clients copy them to the data region.
4. **Servers for simple messages only**: simple messages need no trust in
   the server, a second GID mask could open endpoints the client doesn't
   trust with its memory.
5. **Several messages in processing** per server, one region each, or one
   at a time.
6. **Narrower window**: buffer description in the message, policy and
   timing.
7. **Queue order**: FIFO, or by priority of the sending guest thread,
   which the host doesn't know.
