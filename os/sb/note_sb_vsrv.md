# Spec: SB Virtual Services (VSRV), draft

Specification draft started 2026-10-19, nothing implemented yet. A
mechanism for a sandbox to expose named services to other sandboxes, with
host-enforced access rules and handles that become invalid when the
serving sandbox stops.

Service data is exchanged through memory areas that the host shares between
a server and its clients. The host maps them on demand, as switched MPU
regions, when a sandbox publishes or opens a service. It never copies
service data: it manages names, handles, mappings, notifications and the
lifecycle. Earlier variants are summarized in
[Alternatives considered](#alternatives-considered).

VSRV stands for Virtual SeRVices, see [Naming](#naming).

Related: [note_sb_lifecycle.md](note_sb_lifecycle.md) (producer contract,
finalization), [note_sb_async_vfs.md](note_sb_async_vfs.md) (asynchronous
host ABI, guest threading model),
[note_sb_isolation_security.md](note_sb_isolation_security.md) (copy-in-once,
TOCTOU), [note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md)
(switched region tables). Implementation status will be tracked in
[open_points.md](open_points.md).

## Requirements

1. A sandbox, the *server*, exposes one or more *services*, each identified
   by a name.
2. Other sandboxes, the *clients*, subscribe to a service by name and obtain
   a *handle*, used for all further operations.
3. Only the sandboxes allowed by the host may serve a given name, and only
   the allowed clients may subscribe to it. The rules are defined by the
   host, never by the guests.
4. When the server exits, crashes or withdraws the service, every handle held
   by the clients becomes invalid. A stale handle never refers to a later
   instance of the service, also after the server restarts and publishes the
   same name again.
5. Neither side can stall the other: a slow, blocked or faulty server must
   not block its clients, a misbehaving client must not block the server or
   exhaust its resources.
6. Isolation is preserved: a sandbox reaches the memory of another one only
   through the shared areas mapped by the host, never its private memory;
   addresses returned to a sandbox are always inside its own mappings; the
   existing lifecycle contract holds.

Non-goals for the first version: services provided by the host itself, name
patterns or service discovery beyond exact lookup, multiple servers for the
same name, removal of a mapping by anyone but its owner, hosts without
switched MPU regions (`PORT_SWITCHED_REGIONS_NUMBER == 0`).

## Design constraints from the existing SB

- **One host thread per sandbox.** Guest multithreading is a sub-scheduler
  of green threads over that thread, so a blocking syscall stalls the whole
  guest. As in the async VFS design, no VSRV operation blocks and the
  notifications are VRQs. Guest runtimes rebuild blocking calls per green
  thread; bare-metal guests can wait with `__sb_vrq_wait()`.
- **Lifecycle.** A terminating sandbox goes through `STOPPING`, the host
  quiesces every producer that can reach it, then `sbFinalize()` releases its
  memory. Notifications between two sandboxes are producers, so their
  teardown is part of the termination cleanup, and handles carry a
  generation (the lifecycle note asks for generation-aware endpoints when a
  producer cannot be bound to one execution).
- **Switched MPU regions.** Each sandbox has a table of
  `PORT_SWITCHED_REGIONS_NUMBER` regions in `mpuregs[]`, built at `sbStart()`
  from the static `regions[]` set with `sbSetRegion()`, and loaded on
  switch-in when it differs from the active one. Code and data use one or
  two of them, the others are free for VSRV, see
  [MPU budget](#mpu-budget).
- **Shared memory is not trusted.** A shared area can change at any time:
  the peer sandbox can preempt the reader, also while the reader is inside a
  host syscall. Validate-then-use on such memory is a double fetch (point 1
  of the isolation note), so the host never reads shared areas, and the
  receiving guest validates the messages, see the memory model.

## Model

```text
 client SB               host (VSRV)                    server SB
 ---------               -----------                    ---------
                                                  <---- publish("log", min)
                         maps the area in the server
 open("log") ----------> policy check, takes slice j,
                         maps its block in the client,
                         handle
 writes slice j
 notify(h) ------------> flag j on the service VRQ ---> VRQ, R1 = flags
                                                        reads slice j,
                                                        writes slice j
 VRQ, server flag <----- flag on the handle VRQ <------ notify(svc, j)
 reads slice j
 close(h) -------------> unmaps slice j from the client

      [ slice 2 | free | slice 0 |  slice 1  ]   service area, host memory,
                                                 mapped whole in the server,
                                                 slice j in its client
```

- A **service** is a host object: name, server sandbox and the PID of the
  publishing execution, allowed clients, state (`OFFLINE`/`ONLINE`),
  generation and its area.
- The **area** of a service is host-owned memory, never part of a sandbox's
  own memory, divided in one block of the maximum slice size per slice.
- A **PID** identifies one execution of a sandbox, `sbGetPidX()`: a new
  one is assigned at each start, it is zero while the sandbox is stopped.
  VSRV stores the PID next to every sandbox pointer and compares it before
  acting on the sandbox, see [Invalidation](#invalidation).
- A **slice** is the attachment of one client to the service, identified
  by an index j, 0 to 29, used for the notifications. Slice j always uses
  block j of the area and the client maps the whole block, the slice size
  is defined by the service. Slice j is `FREE` (not attached),
  `ATTACHED` (to one client, with the service generation at attach time) or
  `RESET` (released by its client while the server instance that served it
  is still running, see [Invalidation](#invalidation)).
- A **handle** is a small integer in a per-client host table, like a file
  descriptor. The table entry stores the service, the slice and the service
  generation seen at open time, and the region of the client holding the
  slice mapping. The slice records the client sandbox and its PID.
- A **notification** is a VRQ flag: a doorbell that carries no data. Flags
  accumulate in the VRQ until delivered, so a flood of notifications
  coalesces and can't make the host queue or allocate anything. The layout
  of the messages inside a slice is defined by each service.
- Both sides **notify** each other, the server a client through its slice
  index, a client the server through its handle. Each side chooses the VRQ
  receiving its own notifications: the server at publish, for the service,
  the client at open, for the handle. The host only sets flags on it, the
  other VSRV events (requests pending, reply, slice change, service lost)
  are flags on the same VRQ. The number is checked against the VRQs of the
  sandbox (`CH_RET_EINVAL`); avoiding collisions with the guest's other
  VRQs, the alarm or VIO ones, is up to the guest.

### Memory model

- **Attach on demand.** Publish maps the whole area in a free region of the
  server, open maps one slice in a free region of the client. A free region
  is a switched region unused in the static `regions[]` and not holding
  another mapping.
- **Only the owner changes its table.** A mapping is created and removed
  only inside a syscall of the sandbox owning the table: publish and
  withdraw for the server, open and close for the client. That table is the
  active one, so the `mpuregs[]` entry and the MPU registers are written in
  the same critical section, entry first (the switch code compares table
  pointers and doesn't reload a modified active table). This also holds on
  multicore hosts, the owner runs on the core doing the change.
- **No revocation because of the peer.** When the server stops, the client
  mappings stay until each client closes its handle; the slices are host
  memory and remain valid. Removing them would make a client touching its
  slice at the wrong time fault and be terminated, a crash of the server
  would kill its clients (requirement 5).
- **Released by the client.** A slice is released when its client
  detaches, by close or termination, whether the server is alive or not.
- **Static configuration untouched.** Mappings are written in `mpuregs[]`
  only, `regions[]` keeps the static configuration:
  - a terminated sandbox drops its mappings with its table, `sbStart()`
    rebuilds a table without them;
  - the pointer checks of the host syscalls (`sb_is_valid_read_range()`,
    `sb_check_string()` and the others) only look at `regions[]`, so every
    host syscall rejects a path, an iovec or a buffer inside a slice with
    `CH_RET_EFAULT`, no TOCTOU on shared memory in the host;
  - `sb_locate_data_region()` never sees slices, the system block and the
    heap are never placed there.
- **No finalization race.** Slices aren't sandbox memory, `sbFinalize()` of
  a dynamic server or client never frees memory mapped by another sandbox.
- **No allocator.** Slice j always uses block j of the area, taking a free
  slice index is the allocation and the slice table, in host memory, is the
  free list. An allocator keeping headers or free lists inside the area, a
  ChibiOS heap or memory pool over it, would hand host metadata to the
  server, which maps the whole area.
- **The host never reads slices.** It only clears them: a block when its
  slice is attached, the blocks of the free slices when the service is
  published, so no data of a previous client or server instance is
  visible.
- **Validation by the receiver.** The messages in a slice are validated by
  the receiving side only, server or client. The host doesn't read, check or
  interpret them and knows nothing of their format. The receiver reads each
  field once and validates the copy (offsets, lengths, indexes), assumes
  the peer can rewrite the slice at any moment, and handles a malformed
  message as an error of the peer, never as a fault of its own. A guest
  helper implementing a standard request/reply layout can hide this, see
  the client operations.

### MPU budget

- Mappings use the switched regions left free by the static configuration.
  `SB_CFG_NUM_REGIONS` is at most 4, `PORT_SWITCHED_REGIONS_NUMBER` at most 4
  on ARMv7-M and 8 on ARMv8-M. With code and data in two regions, 2 regions
  are left; 3 for dynamic sandboxes, which keep code and data in one region.
  Hosts using VSRV raise `PORT_SWITCHED_REGIONS_NUMBER` from the usual 2.
  On ARMv8-M the `SB_CFG_NUM_REGIONS` limit could be raised.
- A server uses one region per published service, a client one per open
  handle. The limit is on the services served and used at the same time,
  not on the services a sandbox can use over its run: with 2 free regions a
  sandbox can serve one service and use another one, or use two.
- ARMv7-M regions are a power of two in size, at least 32 bytes, and
  aligned to their size, with 8 subregions: the slice size is a power of
  two and the area is aligned to it, so every block is a valid client
  region. The area also has to fit one region of the server: a power of
  two, or a larger region with subregions disabled.
- ARMv8-M only needs 32 bytes alignment of base and size: the slice size
  is a multiple of 32 and the area is aligned to 32 bytes.
- An area `get_mpu_settings()` can't represent is a host configuration
  error, caught by the definition check or at the first publish.
- `PORT_SWITCHED_REGIONS_NUMBER` is global, every switch between two
  sandboxes loads all the regions: 2 words more per region on the burst
  load, a second burst on ARMv8-M above 4 regions.
- On single-core hosts the caches are coherent and slices can be cacheable;
  DMA into slices isn't considered.

## Policy

Two levels, both set by the host application: the service definitions bind
names to a server and its allowed clients, the sandbox settings grant each
sandbox the right to publish and to attach at all. An operation needs both.

### Service definitions

The host application defines the services statically, as it configures VIO:

```c
/* Area of the "log" service: one block per slice.*/
static uint8_t log_area[4][256] __attribute__((aligned(1024)));
static vsrv_slice_t log_slices[4];
static sb_class_t * const log_clients[] = {&sbx2, &sbx3, &sbx4, NULL};

static const vsrv_service_def_t log_def = {
  .name      = "log",
  .server    = &sbx1,
  .clients   = log_clients,
  .area      = (uint8_t *)log_area,
  .slicesize = 256U,            /* Power of two on ARMv7-M.*/
  .slices    = log_slices,      /* Slice objects storage.*/
  .nslices   = 4U               /* Clients attached at the same time.*/
};
static vsrv_service_t log_service;

  /* Before starting the server and the clients.*/
  sbVsrvObjectInit(&log_service, &log_def);
```

`sbVsrvObjectInit()` binds a service object to its definition and
registers it, the service is known by name; in debug builds it checks the
definition: unique name, slices, slice size and area alignment, area
disjoint from the static regions of the server and of the clients. The
module itself is initialized by `sbHostInit()`. The storage
of the slice objects comes from the application like the handle tables, so
a definition costs only what it uses.

- No mapping is configured by the application: the area is given to the
  server and the slices to the clients by publish and open. Access to the
  memory follows from the policy checks.
- The host defines the slice size and how many clients can be attached
  at the same time. The server states at publish the smallest slice its
  protocol works with, publish fails if the host slice is smaller: a host
  configuration not matching the server image is caught before the service
  goes online.
- The area is `nslices` blocks of `slicesize`, each slice has its own
  block: an open never fails for lack of memory and memory never moves
  between clients.
- Areas must be disjoint from each other, from every private region of any
  sandbox and from host data. A host check of the definitions, run before
  starting the sandboxes, verifies this and the MPU representation in debug
  builds.
- Names are compared exactly, length limited (for example 15 characters),
  copied and checked like the VFS paths, so a name in shared memory is
  rejected.
- Publishing only turns a defined service `ONLINE`. A sandbox can't create a
  name that the host did not define, and can't serve a name defined for
  another sandbox.
- One handle per client and service, so a client can't take all the
  slices away from the other allowed clients.
- Host state is static: a service object and the state of its slices per
  definition, a handle table per sandbox
  from its settings. No heap allocation.
- At most 30 slices per service, one notification flag each.

### Sandbox settings

Each sandbox gets a VSRV configuration, as it gets a VIO one with
`sbSetVirtualIO()`:

```c
static vsrv_handle_t sbx2_handles[2];

static const vsrv_conf_t sbx2_vsrvconf = {
  .flags    = VSRV_CONF_ATTACH, /* May open services, not publish.*/
  .handles  = sbx2_handles,     /* Handle table storage.*/
  .nhandles = 2U                /* Handles open at the same time.*/
};

  sbSetVirtualServices(&sbx2, &sbx2_vsrvconf);
```

- `VSRV_CONF_PUBLISH` allows publishing, `VSRV_CONF_ATTACH` allows opening;
  a sandbox can have both, a server can be a client of another service.
- Set while the sandbox is `STOPPED`, like the other sandbox settings. The
  default is `NULL`: no VSRV access, every VSRV call returns
  `CH_RET_EACCES`. A sandbox that never needs services can't even probe the
  names.
- The settings belong to the sandbox object: an image loaded later in the
  same sandbox, by `sbExecDynamic()` for example, has the same rights.
- `nhandles` bounds the open handles, a full table returns `CH_RET_EMFILE`
  at open; the table storage comes from the application, the sandbox object
  doesn't grow with it.
- Mappings are bounded by the free switched regions anyway, see
  [MPU budget](#mpu-budget).
- The VSRV host code is enabled by `SB_CFG_ENABLE_VSRV` in `sbconf.h`.

## Server operations

- **Publish** `(name, minsize, vrq)`: the caller must have
  `VSRV_CONF_PUBLISH`, be the defined server and the service `OFFLINE`, the
  host slice size can't be smaller than `minsize` (`CH_RET_EINVAL`). Maps
  the area in a free region of the caller (`CH_RET_EMFILE` if none), clears
  the free slices, the service becomes `ONLINE` and the generation is
  incremented. `vrq` receives the notifications: flag j when the client of
  slice j notifies, flag 30 when requests are pending, flag 31 when a slice
  changes state. Returns a service id, base and size of the area.
- **Slices** `(service)`: returns the masks of the slices attached to
  clients of this server instance and of the slices in `RESET`.
- **Client** `(service, j)`: returns the identity of the client on slice j,
  its index in the definition, not a pointer, its PID, and base and size of
  the slice. A PID different from the one seen before means a new execution
  of the client.
- **Notify** `(service, j)`: sets the server flag on the handle VRQ of the
  client on slice j. A slice not attached to a client of this server
  instance, or a client that went away, returns `SB_VSRV_RET_STALE`.
- **Reset done** `(service, j)`: the server no longer uses slice j after
  its client went away, the slice goes from `RESET` to `FREE`; a slice not
  in `RESET` returns `CH_RET_EINVAL`.
- **Withdraw** `(service)`: same effect as the server termination for that
  service, then unmaps the area from the caller.

## Client operations

- **Open** `(name, vrq)`: the caller must have `VSRV_CONF_ATTACH`, a free
  entry in its handle table (`CH_RET_EMFILE` otherwise), be an allowed
  client without a handle on the service, the service `ONLINE`. Takes a
  `FREE` slice (`CH_RET_EBUSY` if none), maps its block in a free region of
  the caller (`CH_RET_EMFILE` if none), clears it, the slice becomes
  `ATTACHED`, the server gets the slice change flag. Returns the handle,
  base and size of the slice. `vrq` receives the server notifications
  (flag 0), the "service lost" event (flag 1) and the replies (flag 2).
- **Notify** `(handle)`: checks the handle (see invalidation), sets flag j
  on the server VRQ. A stale handle, or a server that went away, returns
  `SB_VSRV_RET_STALE`.
- **Close** `(handle)`: unmaps the slice from the caller and releases it,
  whether the server is alive or not. If the server instance that served
  the slice is still running (service `ONLINE`, same generation), the slice
  goes to `RESET` and the server gets the slice change flag, otherwise it
  goes to `FREE`. The handle is freed.
  Close also accepts stale handles.

## Synchronous messages

Commands go from a client to the server as synchronous messages, as with
`chMsgSend()`, `chMsgWait()` and `chMsgRelease()` in RT: the client sends a
request, the server receives it and replies. They are synchronous for the
calling green thread only, the host operations never block. The server
never sends messages to its clients, it notifies them, and event data goes
in the slice; so the server never waits for a client.

- A **request** is one word: a pointer to a **command structure** in the
  client's slice, or a **fixed message**, see below. The slice is mapped
  at the same address in the client and in the server, so the pointer is
  used as it is. A **reply** is a status code, any result data is written
  by the server in the command structure or in buffers it points to.
- The host passes the request word and the status between the registers
  of the two sides and doesn't interpret them.
- Each handle has at most one request in flight and a reply slot reserved
  with it, so a reply never fails for lack of space. The host state per
  slice is a pointer, a status and a request state: `IDLE`, `PENDING`
  (queued), `RECEIVED` (taken by the server), `REPLIED`.
- Pending requests are queued per service in FIFO order, the queue links the
  slices, no allocation.

### Command structures

A command structure starts with a common header, followed by the fields
defined by the command:

```c
typedef struct {
  uint32_t      cmd;            /* Command code.*/
  uint32_t      size;           /* Size of the whole structure.*/
} vsrv_cmd_header_t;
```

Fields can point to other data, buffers or strings, which must be in the
same slice. The structure and the data it points to belong to the server
from send until the result, the client doesn't touch them meanwhile.

The host checks only the request word: a request that is not a fixed
message must point inside the slice of the sender, with room for the
command header, otherwise send fails with `CH_RET_EFAULT` (decided
2026-10-20, defense in depth, the check is a few compares). The host never
reads the structure. The server still validates everything:

- the request pointer and every pointer in the structure must be inside the
  slice of the *requesting client*, not just inside the service area: the
  server maps all the slices, a client pointing into another client's slice
  would make the server read or write that client's data;
- alignment and sizes, including the header `size` against the structure
  expected for `cmd`;
- copy-in-once: the client can still rewrite its slice while the server
  works, by mistake or on purpose, so the server copies the header and the
  pointers out of the slice first, validates the copies and only uses them.
  Bulk data can be used in place when its content changing is harmless to
  the server.

`vsrvIsValidRange()` in `sbuservsrv.h` checks a range against a slice.

### Fixed messages

Request words from `0xFFFFFF00` to `0xFFFFFFFF` are fixed messages, not
pointers: that range is the system area of the Cortex-M memory map, never a
slice. A fixed message is a code in the low byte, without structure and
without slice access, so it needs no validation beyond the code; simple
commands such as start, stop or flush need no command structure and the
reply status is their only result.

- `0xFFFFFF00-0xFFFFFF7F`: generic VSRV messages, see below.
- `0xFFFFFF80-0xFFFFFFFF`: defined by each service.

The server tells them apart before any validation:
`VSRV_MSG_IS_FIXED(msg)`.

### Operations

Client side:

- **Send** `(handle, msg)`: the handle must be `IDLE` (`CH_RET_EBUSY`
  otherwise) and a request that is not a fixed message must point inside the
  slice of the handle, 4 bytes aligned, with room for the command header
  (`CH_RET_EFAULT` otherwise). Stores the request word, queues the request,
  raises the "request pending" flag on the server VRQ.
- **Result** `(handle)`: if the request was replied returns the server
  status, the handle goes back to `IDLE`; a request still queued or
  received returns `SB_VSRV_RET_PENDING`, no request `CH_RET_EINVAL`.
- **Cancel** `(handle)`: removes a `PENDING` request from the queue. A
  `RECEIVED` request can't be cancelled (`CH_RET_EBUSY`), the client waits for
  the reply or closes the handle; a green thread giving up on a timeout leaves
  the handle busy until the reply.

Server side:

- **Receive** `(service)`: dequeues the oldest pending request, returns its
  slice index and the request word, not validated; the slice goes to
  `RECEIVED`. An empty queue returns a VSRV code.
- **Reply** `(service, j, status)`: slice j must be `RECEIVED` (`CH_RET_EINVAL`
  otherwise). Stores the status, raises the "reply" flag on the client VRQ. If
  the client went away meanwhile, the reply is discarded and a stale code
  returned.

A call in a guest runtime is: fill the command structure in the slice,
send, wait for the handle VRQ on the calling green thread, take the status
and read the results from the structure. Clients with several threads on
the same service serialize on the handle, or the service uses rings in the
slice driven by notifications.

### Commands and status codes

Generic fixed messages, every server answers NOP and INFO:

| Message | Name | Reply status |
|---|---|---|
| `0xFFFFFF00` | `VSRV_MSG_NOP` | `CH_RET_SUCCESS`, liveness and round trip |
| `0xFFFFFF01` | `VSRV_MSG_INFO` | service protocol version, major in bits 16-30 |
| `0xFFFFFF02` | `VSRV_MSG_RESET` | `CH_RET_SUCCESS`, the server drops its state for the slice, the slice layout restarts |

The `cmd` codes of the command structures are all defined by each service.

The reply is a `msg_t`: `CH_RET_SUCCESS`, a non-negative result defined by
the command (a count, for example), or a `CH_RET_xxx` error. No new codes
are defined:

| Status | Meaning |
|---|---|
| `CH_RET_SUCCESS` | done |
| `CH_RET_ENOSYS` | unknown or unsupported command |
| `CH_RET_EINVAL` | invalid structure or arguments |
| `CH_RET_EFAULT` | a pointer or a range outside the client's slice |
| `CH_RET_EOVERFLOW` | the data doesn't fit the slice |
| `CH_RET_EBUSY` | busy, retry later |
| `CH_RET_EACCES` | refused by the service for this client |
| `CH_RET_EIO` | internal failure of the service |

Services can use other `CH_RET_xxx` values for their own commands.

## Invalidation

Each service has a generation, incremented when it goes `ONLINE`. A handle
is valid only while the service is `ONLINE` and the generation stored in the
handle equals the current one. Every client operation checks it; a stale
handle returns `SB_VSRV_RET_STALE` and can only be closed.

PIDs cover the sandboxes: before raising a flag on a sandbox, notification,
reply, request pending, slice change or service lost, the host compares the
PID stored with the sandbox pointer (server PID in the service, client PID in
the slice) with the current one, and drops the event if they differ,
`sbVRQRaisePidS()`/`sbVRQRaisePidI()` returning `CH_RET_ESRCH`; an operation
of a sandbox, a server notifying a client that restarted for example, then
returns `SB_VSRV_RET_STALE`. A restarted server or client never gets an
event meant for its previous execution, also if an event raced with the
termination cleanup. The service generation is still needed because a
server can withdraw and publish again within the same execution, with the
same PID.

On server termination (exit, fault, abort), within the `STOPPING` cleanup
and before `sbFinalize()` is allowed:

1. each service of that server becomes `OFFLINE`, so every handle becomes
   stale at once;
2. the clients of the attached slices get the "service lost" flag on their
   handle VRQ, so the waiting green threads wake up; `PENDING` and
   `RECEIVED` requests are dropped, their result is the stale error; the
   slices stay attached and mapped until each client closes;
3. the `RESET` slices become `FREE`, the server is gone;
4. the server mapping goes with its table.

After a restart the server publishes again, the new generation keeps the old
handles stale, the clients close them and open new ones. Slices still
attached to stale handles are given out again only after their client
closes them.

On client termination, within its `STOPPING` cleanup, its mappings go with
its table, its slices are released as by close and its handles are freed.
Close and termination remove a `PENDING` request from the queue; for a
`RECEIVED` one the server's reply is discarded.

`RESET` doesn't hold back the client, which has already released the slice
and its region; it only delays the reuse of the slice. The server instance
that served the previous client could still be writing a reply in it, and
that reply would land in the slice of the next client: a leak between
clients, or a new client taken for the previous one. The server ends
`RESET` with reset done. A server that never does only keeps that slice
busy, a withdraw or the server termination frees it.

The generation counter is 32 bits; a wrap requires about 4 billion restarts
and is considered acceptable, to be confirmed. PIDs are positive 32 bits
values and restart from one after the largest, a reuse requires about 2
billion sandbox starts.

## Limits and fairness

- Fixed host state per slice, one request in flight per handle: a client
  can't flood the server, neither side can make the host allocate or hold
  more.
- A client can only reach its own slice, and holds at most one slice per
  service.
- Requests are served in FIFO order across clients. Notifications arrive in
  one VRQ flags word and their order is up to the server.
- A server that never answers only stalls its own service; clients can wait
  with a timeout (VRQ alarm) on their green threads.

## Scheduling

Each sandbox keeps its own host priority. A high priority client waiting for
a low priority server is a priority inversion, accepted in the first version
and documented: the system designer assigns priorities. Priority donation to
the server sandbox while it serves higher priority clients is an open
question, the green-thread model makes it coarse (the whole server sandbox
would be boosted). The request queue tells the host which clients wait for
the server, which a donation scheme could use; the queue could also be
ordered by client priority, as `CH_CFG_USE_MESSAGES_PRIORITY` does for RT
messages.

## ABI sketch

Guest API in `os/sb/user/sbuservsrv.h`, host in `os/sb/vsrv` (`sbvsrv.h`,
included by `sb.h` when `SB_CFG_ENABLE_VSRV` is `TRUE`, `FALSE` by default
while VSRV is new, `sbvsrv.c` and `sbvsrv.mk`), codes in
`os/sb/common/sbsysc.h`.

- One syscall and one fastcall code, as VIO does, with sub-codes in R0
  bits 0-7 and a single numbering space:
  - `SB_SYSC_VSRV` (231): `SB_VSRV_PUBLISH`, `SB_VSRV_WITHDRAW`,
    `SB_VSRV_OPEN`, `SB_VSRV_CLOSE`. They change MPU mappings, copy names
    or clear slices.
  - `SB_FASTC_VSRV` (103): `SB_VSRV_SEND`, `SB_VSRV_RESULT`,
    `SB_VSRV_CANCEL`, `SB_VSRV_NOTIFY`, `SB_VSRV_RECEIVE`,
    `SB_VSRV_REPLY`, `SB_VSRV_NOTIFYC`, `SB_VSRV_SLICES`,
    `SB_VSRV_CLIENT`, `SB_VSRV_RESETDONE`. Constant time, no guest memory
    access.
- R0 is built with `VSRV_CALL(subcode, id, j)`: the handle, the service or
  the VRQ number (publish and open) in bits 24-31, the slice index in bits
  16-23. R1-R2 carry the other parameters: the request word for send, the
  status for reply.
- Results come back in R0-R3: R0 is the status, a handle, a service id or
  a slice index; receive returns the request word and result the server
  status in R1; publish, open and slices return two more values in R1-R2,
  client three in R1-R3 (base, size, PID), through the new
  `__syscall1rrrr()`, `__syscall2rrrr()` and `__syscall3rrrr()` macros of
  `sbuser.h`.
- Return codes: `CH_RET_xxx` plus `SB_VSRV_RET_STALE` (handle of a service
  instance that is gone), `SB_VSRV_RET_PENDING` (no reply yet) and
  `SB_VSRV_RET_EMPTY` (no request queued), values outside the Posix-mapped
  range like the VIO ones.
- VRQ flags `SB_VSRV_FL_xxx`. Server VRQ: `SB_VSRV_FL_SLICE(j)` for
  j = 0..29, `SB_VSRV_FL_REQUESTS` (30), `SB_VSRV_FL_SLICES` (31). Client
  VRQ: `SB_VSRV_FL_NOTIFY`, `SB_VSRV_FL_LOST`, `SB_VSRV_FL_REPLY`.
- Limits: `SB_VSRV_NAME_MAX` (15), `SB_VSRV_MAX_SLICES` (30).
- Guest API: server `vsrvPublish()`, `vsrvWithdraw()`, `vsrvReceive()`,
  `vsrvReply()`, `vsrvNotifyClient()`, `vsrvGetSlices()`,
  `vsrvGetClient()`, `vsrvResetDone()`; client `vsrvOpen()`,
  `vsrvClose()`, `vsrvSend()`, `vsrvSendFixed()`, `vsrvResult()`,
  `vsrvCancel()`, `vsrvNotify()`; `vsrvIsValidRange()` for the server
  checks. The command header, the `VSRV_MSG_xxx` fixed messages and macros
  and `VSRV_VERSION()` are in the same header, the host doesn't use them.
- Host interface: `vsrv_service_def_t`, `vsrv_service_t`,
  `vsrv_slice_t`, `vsrv_conf_t`, `vsrv_handle_t`, the `VSRV_CONF_xxx`
  flags, `sbVsrvObjectInit()`; `sbSetVirtualServices()`
  in `sbhost.h` and a `vsrvconf` pointer in `sb_class_t`, next to `vioconf`.
- Host code in `os/sb/vsrv/sbvsrv.c`: `sb_sysc_vsrv()` and
  `sb_fastc_vsrv()` bound with `SB_SVC_HANDLER()`, `__sb_vsrv_cleanup()`
  hooked in the exit and abort paths next to `__sb_vio_cleanup()`,
  `__sb_vsrv_init()` called by `sbHostInit()`. Mappings through
  `sbMapRegion()` and `sbUnmapRegion()` of `sbhost.c`, raises through
  `sbVRQRaisePidS()` and `sbVRQRaisePidI()`.
- Adding codes is not an ABI break, `SB_ABI_VERSION` stays unchanged.

## Security notes

- Handles are host table indexes with generations, they can't be forged and
  are meaningful only for the sandbox owning the table.
- Policy is checked at publish and open, sandbox settings and service
  definitions, and memory is mapped only as their result; each operation
  checks the handle state. Without settings a sandbox has no VSRV access
  at all.
- The host never reads shared memory and the host syscalls reject pointers
  into it, so there is no host-side TOCTOU. Message validation, guest-side
  TOCTOU included, is the receiver's responsibility.
- Requests are fixed messages or pointers into the client's slice,
  validated by the server against that slice only, copy-in-once included;
  replies are status codes. The host passes both without interpreting them.
- Slices are cleared when attached and when the service is published, no
  data crosses instances or clients.
- A server sees the slices of all its clients, so the clients trust it with
  the data they send. A client sees only its own slice.
- Mappings are memory, never device and never executable.

## Open questions

1. Static definitions only, or dynamic services (names created at run time
   within host-defined limits)?
2. Region budget: raising `SB_CFG_NUM_REGIONS` on ARMv8-M.
3. One-way broadcasts from a server to all its clients: notify each slice,
   or a single "notify all" operation.
4. Host syscalls accepting shared buffers for pure data (a `write()` from a
   slice) with DMA semantics, instead of rejecting them.
5. More than one request in flight per handle, a few reply slots, if
   serializing a multithreaded client limits the throughput; rings in the
   slice driven by notifications are the alternative.
6. Forced detach of a client by the server or the host: it removes a
   mapping from a table that isn't active (safe on a single core, the next
   switch-in reloads it) but the client faults if it touches the slice, so
   probably only together with terminating the client.
7. Slice sizes per client in the definition, for services whose clients
   have different needs.
8. Priority donation.
9. Host-provided services through the same API, and host access to sandbox
   services.
10. A copy-based fallback for hosts without switched regions.

## Alternatives considered

**Copies through host slots**, the first draft of 2026-10-19. The host
reserved request/reply slots per service, copied requests from the client
and replies from the server, and touched each sandbox's memory only inside
that sandbox's own syscalls. TOCTOU was closed by construction and no MPU
region was needed, but each direction took two copies, the host held
storage and state per request, and the size of messages was limited by host
memory.

**Static mappings**, the second draft. The host application mapped each
slice with `sbSetRegion()` before starting the sandboxes, one slice per
allowed client. It had no run-time MPU update, but the host configuration
had to know which client uses which service, a region of each client was
taken for each service it might ever use, every allowed client needed its
own slice, and a new region attribute was needed to keep the slices out of
the pointer checks. Attaching on demand ties regions and slices to the open
handles only.

**Slice size requested by the client**, with a minimum set by the server
at publish and a maximum set by the host, rounded up for the MPU. With one
block per slice the request no longer saved memory, the slice size is now
defined by the service.

**Buddy allocator** for the slices, in the variable size revision. Blocks
of power of two sizes, split and merged, fit the ARMv7-M MPU rules and could
overcommit the area, but they fragment like a heap: scattered small slices
can block a larger request while enough memory is free, and the allocator
needs its own map. With the area sized for one block of the maximum size
per slice, which the spec recommended anyway, a fixed block per slice uses
the same memory with no allocator and no fragmentation.

**Mailboxes**, for the commands. Two host mailboxes per slice, one message
each way, the server could send messages to its clients too. A reply could
find the client mailbox full, so every server needed retry logic, requests
needed tags to match their replies, the server found work by scanning
flags, and a server sending to a slow client could be held up by it.
Synchronous messages reserve the reply slot with the request, match
implicitly, queue the requests in the host and keep the server from ever
waiting for a client.

**Register messages**, the first form of the synchronous messages: a
command and two arguments in registers, a status and two values back. Small
commands needed no slice access, but every service had to squeeze its
commands into three words and point to the slice for anything larger.
Pointers to command structures in the slice carry commands of any size in
one word, leave the results in the slice and reduce the reply to a status.

## Naming

**VSRV, Virtual SeRVices**, chosen 2026-10-19: the "V" of the other
virtualized SB subsystems, VIO, VFS and VRQ. Guest wrappers are `vsrvXxx()`,
host types `vsrv_xxx_t` as VIO does with `vio_xxx_t`, host functions
`sbVsrvXxx()` and `sbSetVirtualServices()`, codes `SB_FASTC_VSRV` and
`SB_SYSC_VSRV` with `SB_VSRV_xxx` sub-codes.

Other candidates considered: Gates (endpoint metaphor), ISC (Inter-Sandbox
Calls), NSR (Named Service Registry).
