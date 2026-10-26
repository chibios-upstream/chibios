# Spec: SB Virtual Shared Memory (VSHM), draft

Specification draft started 2026-10-26 from the design notes in `vshm.md`,
reworked the same day, nothing implemented yet. VSHM is a reference-counted
shared memory mechanism, named or anonymous, in the spirit of POSIX
`shm_open()` and `mmap()`: a sandbox creates a shared memory, others open
it by name, each maps it in its own address space and uses it. VSHM gives
memory only, no messaging and no synchronization: messages are
[VMSG](note_sb_vmsg.md), coordination is the protocol of the users. The
first draft, with exclusive access per transaction, a message box and a
hold timer, is in the history of this note.

Together with VMSG it takes the role of VSRV, parked in branch
`park/sb-vsrv`; the host infrastructure carries over, see
[Reuse from VSRV](#reuse-from-vsrv).

Related: [note_sb_vmsg.md](note_sb_vmsg.md) (messages),
[note_sb_lifecycle.md](note_sb_lifecycle.md) (producer contract, PIDs),
[note_sb_isolation_security.md](note_sb_isolation_security.md)
(copy-in-once, TOCTOU),
[note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md) (switched
region tables).

## Concepts

- A **shared memory** (SM) is created by a sandbox, its **creator**, with a
  size and attributes fixed at creation. Its memory is host-owned, never
  part of a sandbox's own memory, allocated and cleared on creation.
- An SM is **named**, others can open it by name, or **anonymous**,
  creator only: the way a sandbox allocates memory at run time.
- SMs are **reference-counted**: create and open take a reference, close
  releases it. The memory is released when the last reference goes, a
  terminating sandbox releases its references in its cleanup.
- **Mapping** is separate from references: a sandbox maps an SM it holds a
  reference to, one switched region of its own, and unmaps it. There is no
  address translation, the SM has the same address in every sandbox
  mapping it: pointers into the SM are valid in all of them.
- The SM metadata, in host memory, holds the name, the creator and the PID
  of its creating execution, size, attributes, the access granted to the
  others and the reference count.
- **Groups.** Each sandbox is assigned a group ID (GID, 0 to 31) in its
  configuration, and a mask of friendly GIDs: a sandbox can open the SMs
  created by sandboxes whose GID is in its mask. Permissions are enforced
  by the configuration only, the host needs no knowledge of the software
  the sandboxes run.
- **Access.** The creator sets at creation the access of the others,
  read-write or read-only; an opener can map read-only an SM it could
  write.
- **No synchronization.** Concurrent access is the business of the users,
  as for POSIX shared memory; a receiver copies once what it validates.

## API

Guest calls, with their POSIX counterparts:

| Call                                | POSIX                               |
|-------------------------------------|-------------------------------------|
| `create(name, size, attrs, access)` | `shm_open(O_CREAT\|O_EXCL)`, size   |
| `create(NULL, size, attrs, 0)`      | `mmap(MAP_ANONYMOUS)`               |
| `open(name, mode)`                  | `shm_open()`                        |
| `map(h, mode)`, returns the base    | `mmap(MAP_SHARED)`                  |
| `unmap(h)`                          | `munmap()`                          |
| `close(h)`                          | `close()`                           |
| `unlink(name)`, creator only        | `shm_unlink()`                      |

- Handles are entries of a per-sandbox handle table, with generation
  checks.
- `map()` maps the whole SM at its fixed address, the address argument of
  `mmap()` is ignored and the offset is zero. The guest newlib bindings
  can implement `shm_open()`, `mmap()`, `munmap()` and `shm_unlink()` on
  top.
- `close()` of a mapped handle unmaps it first.

## Names

- A name is registered at creation, unique in the host, and removed by
  `unlink()` or when the creator releases its reference, by `close()` or
  termination. Handles already open stay valid, the memory stays until the
  last reference is released.
- A creation with the same name after the removal makes a new SM. A
  sandbox still using the old one, believing an older creator execution
  is there, only reaches memory nobody else uses; the memory is reused
  only after the last reference is released.

## Anonymous SMs

An anonymous SM is the way a sandbox allocates memory at run time, with
attributes such as coherent for DMA buffers:

- only the creator holds a reference, it is released by `close()` or
  termination, then the memory is released;
- the host services accept pointers into the anonymous SMs mapped by the
  calling sandbox, so they can be buffers of VIO operations, DMA included;
- switched regions are few, those left unused by the sandbox configuration
  out of 4 on ARMv7-M: a sandbox needing many buffers creates one
  anonymous SM and sub-allocates it itself.

An anonymous SM can be closed while a host driver still transfers into it,
a DMA started by a VIO operation: its memory must not go to another
sandbox meanwhile, see the open questions.

## Attributes

Set at creation, they select the memory pool and the MPU attributes of the
mappings. Memory never changes attributes, each attribute has its own pool.

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

- Mapping and unmapping happen only in the caller's own syscalls: the
  table entry and the live registers, on its own core. No sandbox ever
  changes the mappings of another and nothing is revoked: a mapping is
  removed only by its owner, or with its table, rebuilt by `sbStart()`.
- One switched region per mapped SM; references without a mapping use
  none.
- Mappings are written in `mpuregs[]` only, `regions[]` keeps the static
  configuration. The pointer checks of the host syscalls accept
  `regions[]` and the anonymous SMs mapped by the caller, they reject
  pointers into named SMs.
- Read-only mappings use the read-only access encoding, execute is never
  allowed.
- ARMv7-M regions are a power of two in size, at least 32 bytes, aligned to
  their size; ARMv8-M needs 32 bytes alignment of base and size. The size
  is rounded at creation.

## Lifecycle

- **Termination** of a sandbox: its references are released by its own
  cleanup, in `__sb_abort()`; its mappings disappear with its table.
- **Creator termination**: its reference is released and the name
  removed, the others keep their references and mappings, the memory stays
  until the last reference is released. No sandbox faults because another
  one went away.
- Releasing the last reference only returns the memory to its pool, no MPU
  work.

## Use with VMSG

- **Zero copy**: a client and a server map the same named SM, simple VMSG
  messages carry pointers into it, the server checks them against the SM
  bounds. The client data region is never opened, the server is trusted
  only with the SM.
- Who touches the SM content during a message is the protocol: the client
  leaves its buffers alone until the answer, the server copies once what
  it validates.

## Multicore

No cross-core work: the MPU changes are in the caller's own syscalls on its
own core, the reference counts, names and pools are under the system lock,
VSHM raises no events.

## Configuration

Per sandbox, set while it is stopped, as the VIO configuration:

- GID and mask of friendly GIDs;
- right to create SMs, and the memory it can create them from, one pool
  per attribute, see the open questions;
- storage of its handle table.

## Reuse from VSRV

From `park/sb-vsrv`, mostly as it is:

- `sbMapRegion()`/`sbUnmapRegion()`, the own-syscall variant, and the
  region encoding of `sbhost.c` for the attributes;
- sandbox PIDs for the creator metadata;
- handle tables, generation checks, the name registry, termination cleanup
  hooks;
- the syscall and fastcall codes with sub-codes, the testsb harness and the
  NIL-MULTI demo structure.

## Open questions

1. **Memory at creation**: pools per sandbox, one per attribute, set in
   configuration, give a quota, so that a sandbox can't exhaust the memory
   of the others, and confine a DMA outliving an anonymous SM to the
   memory of its own sandbox. A host-wide pool would need the asynchronous
   operations to hold a reference instead. An SM outliving its creator
   stays charged to it across a restart, until the last reference goes.
2. **Allocator**: fixed-size blocks per pool, sizes in configuration, or a
   general allocator; ARMv7-M alignment makes a general heap wasteful.
3. **Name persistence**: the name bound to the creator reference, as now,
   or persistent until `unlink()` as in POSIX, letting the content survive
   a creator restart, the name then holding a reference.
4. **Passing anonymous SMs** to another sandbox, a handle in a VMSG
   message, or anonymous strictly private.
5. **Map at open**: separate `map()`/`unmap()`, or a mapping per open
   handle, simpler but a region for every reference.
6. **DMA into named SMs**, for zero-copy pipelines: the DMA can outlive the
   mapping and the reference of the sandbox that started it.
7. **More attributes**.
8. **Events**: none, or the creator notified of opens and closes.
