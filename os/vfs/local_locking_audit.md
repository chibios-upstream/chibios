# VFS ownership and local synchronization audit

Step 1 of the [local locking plan](local_locking_plan.md), completed on
2026-09-19 against `27f0ad644e`. This document specifies the intended contract
and records implementation gaps at that baseline. The linked plan tracks
which gaps have since been addressed. This audit does not certify the VFS as
thread-safe; steps 2 through 6 implement and validate the missing parts.

The audit covers the in-tree root, overlay, streams, ROMFS, FatFS, and LittleFS
implementations, CHFS's current skeleton, OOP references, pools, and the newlib
and sandbox adapters. The available FatFS source is R0.14b, revision 86631,
with ChibiOS extensions. LittleFS was unavailable at the initial audit; step 3
now also validates the locally available 2.10.x library. The FatFS/LittleFS
sections below have been updated for wrapper-owned synchronization, replacing
the initial native-reentrancy prerequisites. Template and unfinished drivers
have no concurrency guarantee.

## Operation and ownership contract

1. **Synchronization is internal and local.** Upper VFS metadata protection
   covers only copying, selecting, and updating owned state. Release it before
   FS or node virtual methods, reference methods, disposal, backend calls,
   callbacks, and blocking allocation. Do not substitute an operation-wide
   semaphore or busy flag. Native leaf/backend synchronization has its own
   scope and must not depend on a caller holding an upper VFS lock.

2. **Separate handles are conditionally concurrent.** Distinct nodes may be
   operated on concurrently when their leaf and shared backend support it.
   This includes open, close, stat, and namespace operations, not only I/O.
   Different nodes, FS wrapper objects, or roots do not imply independent
   volumes, media, stream cursors, or callback state. Backend restrictions on
   opening, modifying, or removing the same file still apply.

3. **One handle requires caller serialization.** Callers serialize operations
   on the same open file/directory, including stat, seek, position queries,
   control, stream-interface access, and directory first/next, unless that
   driver explicitly permits overlap. Duplicated descriptors refer to the
   same node and share its position. Multi-call sequences such as seek/read
   need caller ordering. This also excludes recursive use of an active handle
   from a callback unless explicitly supported. No upper VFS mutex is added
   around these calls to impose ordering.

4. **A reference protects lifetime, not operation ordering.** Each independent
   node user owns a reference for the entire operation, including waits and
   interface-adapter calls. A copied pointer or an FD lookup is not a retained
   reference. Another owner may release its reference while an operation has
   its own; final disposal cannot occur until the operation relinquishes its
   reference. An open transfers one reference on success. An adapter either
   takes ownership of that reference or releases it on insertion failure.
   Borrowed interface pointers do not extend node lifetime.

5. **FS lifetime remains external.** Nodes borrow their immutable owner;
   `vfsNodeGetOwner()` does not retain an FS object. Roots and overlays borrow
   backing/mounted FS objects. The application keeps each FS and its backend
   alive through all operations and final node disposal. Unregistering removes
   a route; it neither cancels nor drains operations already using that route.
   Destroying the FS afterward needs application-controlled quiescence.

6. **Publication and lifecycle are separate from normal access.** Initialize
   before publication. Keep backing pointers, prefixes, ROM descriptions,
   stream tables, backend configurations, and the default-root pointer fixed
   during active use. Mount names remain immutable while registered; local
   readers must finish comparison/copying under the metadata lock. Mount,
   unmount, format, backend stop/reconfiguration, and FS disposal require an
   exclusive lifecycle phase with no affected active operations or live nodes.
   Concurrent hot unmount is outside this migration's contract. Future APIs
   supporting it will need explicit admission, draining, and media lifetimes.

7. **Temporary storage has one owner.** A root operation reserves one pair
   before any metadata lock, resolves its inputs locally, and keeps the pair
   through all delegated work and cleanup. Borrowed path/data pointers remain
   valid and unmodified by their owners until the call returns. Callees cannot
   keep those pointers afterward. No call chain retaining a pair may wait for
   a second pair, including indirectly through callbacks. Root-under-root and
   callback reentry into allocating root operations are therefore unsupported.
   ELF relocation and sandbox directory scratch follow the same rule.

The `VFS_CFG_USE_MUTUAL_EXCLUSION` option controls only VFS-owned local
mutex fields/helpers. Disabled helpers are empty macros with no corresponding
mutex storage. Callers then supply serialization for those metadata and leaf domains.
It does not disable kernel pool/reference protection or configure a leaf
library's native synchronization. No global acquire/release API is introduced.

## State and protection inventory

| State and source | Protection at the step 1 baseline | Required protection or ownership |
| --- | --- | --- |
| Root `path_cwd`, resolution, `getcwd`, `chdir` in [drvroot_impl.inc](drivers/root/drvroot_impl.inc) | Private operation pair; CWD itself is unprotected | Root's inherited optional metadata mutex for copying/snapshot/commit only. Rename resolves both inputs from one CWD. Never carry the mutex into validation or node disposal. |
| Lazy CWD storage in `vfsRootChangeCurrentDirectory()` | `chCoreAlloc()` followed by an unprotected pointer assignment | Establish storage without racing monotonic allocations. The selected design keeps one lazy buffer per root and protects the bounded, nonblocking `chCoreAlloc()` call with the metadata mutex. This prevents competing allocations; storage remains permanent rather than requiring a separate pool. |
| Overlay `next_driver`, `names[]`, `drivers[]` in [drvoverlay_impl.inc](drivers/overlay/drvoverlay_impl.inc) | Register/unregister use system sections; lookup and enumeration do not | One matching reader/writer metadata mutex. Copy names while protected. Select both rename routes under one lock at each layer, then delegate. Preserve existing length and duplicate-name checks. |
| Root/overlay backing pointer, root prefix, default `vfs_root` | Raw borrowed pointers | Immutable after publication; owner controls lifetime and any stopped-state reconfiguration. |
| Overlay directory `index`, `overlaid_root` | Per-node fields; backing phase inferred from current mount count | Caller serializes the handle. Add explicit mounts/backing/end phase; take the metadata lock only to copy mount entries. Backing iteration and final release happen outside it. |
| Shared path/scratch pairs in [vfsbuffers.c](src/vfsbuffers.c) | Guarded pool plus exclusive ownership | Already sufficient for storage. Allocate before local locks; release on every exit. Reservation does not protect CWD, route tables, or handles. |
| Driver node/info/FS pools | `chPoolAlloc()`/`chPoolFree()` synchronize their free lists | Already sufficient for allocation. Initialization is before publication; an allocated node is private until returned. Pool safety does not protect allocated node fields. |
| Node `references` in [oop_referenced_object.c](../common/oop/src/oop_referenced_object.c) | Default addref/release use `oopLock()`; final disposal follows unlock | Retain a valid reference before using a node. Concurrent builds require the ChibiOS/OSAL protection, not `OOP_USE_NOTHING`. Reference overrides must preserve the declared lifetime contract. |
| Node owner, mode, VMT and returned interfaces in [vfs_nodes.xml](codegen/vfs_nodes.xml) | Initialized before publication; built-in methods do not replace owner/mode | Immutable identity plus a retained node reference. Interface pointers are borrowed views of the same handle. |
| Reference-count assertions in [vfs.c](src/vfs.c) | Direct reads outside the OOP critical section | Step 5 must remove or safely synchronize the diagnostic reads; owning one reference prevents disposal but does not prevent other owners changing the count. |
| Newlib `fds[]` in [syscalls.c](../various/newlib_bindings/syscalls.c) | No table lock and no operation pins in this branch | Atomic lookup plus lifetime pin, insertion, and detach. Keep I/O, virtual addref/release, and disposal outside table protection. An in-flight operation must continue on its pinned node after close/reuse. |
| Sandbox `io.vfs_nodes[]` in [sbposix.c](../sb/host/sbposix.c) | Lifecycle ownership; ordinary lookup uses the table reference | One guest thread owns its active table. Host registration is stopped-state only. Detach entries before release on close/replacement/cleanup; preserve transferred references. See adapter details below. |
| ROMFS cursors, positions, sessions; streams directory cursor | Per-open mutable state without operation locking | Same-handle caller serialization. Shared immutable descriptors need no mutex; shared callback/stream state needs its own backend contract. |
| FatFS volume/library state; LittleFS `lfs`, `mounted`, `cfgp` | Configuration-dependent native locks; lifecycle gaps | Optional singleton FatFS/per-instance LittleFS wrapper mutexes, exclusive lifecycle phase, and the leaf requirements below. |
| CHFS cache and prospective FS metadata | Cache ownership exists; FS operations are stubs | Cache reservation protects one object only. Actual filesystem transactions, lifecycle, and callback order require design with the future implementation. |

Kernel allocation details matter: [chmempools.c](../oslib/src/chmempools.c)
calls a pool's growth provider under the system lock. Existing VFS providers
are `chCoreAllocAlignedI()` or `NULL`; keep them I-class and nonblocking.
Do not replace one with a thread allocator or FS callback. Upper VFS code
must call the ordinary pool APIs outside its own system sections.

## Delegation and disposal boundaries

| Boundary | State that must survive after releasing metadata protection |
| --- | --- |
| Root path operation to `vfsFS*()` | One private pair, selected borrowed FS pointer, and caller-provided FS lifetime. The callee may sleep. |
| `vfsRootOpen()` file-to-directory fallback | Preserve the normalized logical input in the second half; rebuild the delegated path in the first. CWD is not reread. Routes may change between attempts; this is not a namespace transaction. |
| `chdir` directory validation and release | Preserve the logical candidate through both open and `roRelease()`. Commit only after success and cleanup. Concurrent changes take effect in commit order, which need not match return scheduling. |
| Overlay to child FS, including `__ovldrv_open_root()` | Snapshot FS pointer and borrowed path. A successfully opened backing directory reference belongs to the new merged node. No metadata mutex reaches child open or error cleanup. |
| Overlay `first`/`next` to backing directory | Caller owns and serializes the merged node; it owns the backing node reference. Copy mount names before unlocking. Mount changes produce a live view that can skip/repeat entries, but cannot corrupt memory or restart backing iteration. |
| Final node release to disposal | OOP counter section has ended. Descriptor entries are already detached. Overlay disposal can release another node; FatFS/LittleFS close native handles; ROMFS may invoke a user close callback. |
| ROMFS `stat`/`open`/`read`/`close` and open-failure cleanup | Descriptor lifetime and applicable per-open session. Distinct-session callbacks can overlap; callback-owned shared state needs synchronization. A path operation or getdents caller may still own a pair. |
| Streams read/write/seek/control | Retained VFS node and borrowed backend interface lifetime. Backend may block and may be shared by other nodes. |
| ELF relocation and sandbox `getdents` to node methods | Scratch pair retained across I/O/iteration; node methods and their callbacks cannot wait for another pair. |
| Native FatFS/LittleFS to storage hooks | Leaf wrapper mutex held through native calls and I/O waits; optional backend locking follows it. No inherited upper VFS metadata mutex. Callback reentry into VFS is unsupported. |

## Leaf readiness

### ROMFS

[drvromfs_impl.inc](drivers/romfs/drvromfs_impl.inc) routes borrowed paths
without modifying them or allocating another pair. The tree, directory/file
descriptors, names, and raw data are read-only during use. Directory indices
and file `position`, `size`, and `session` are per node. Raw files are suitable
for distinct-handle concurrency with immutable backing data and synchronized
node allocation/references; the full root/adapter path still needs later steps.

The built-in stored and PackBits chunk backends use an immutable descriptor
as their session and local decode state; [packbits.c](../common/utils/src/packbits.c)
does not introduce shared decode scratch. Custom dynamic/compressed backends
must support overlapping operations on distinct sessions and calls to shared
`stat`/`open` arguments. Close may run during another session's operation or
during a failed-open cleanup. They must not retain borrowed paths or acquire a
second VFS pair. The VFS does not serialize arbitrary user callbacks.

Prerequisites for claiming support: document callback/backend capabilities,
exercise distinct raw/compressed handles, and probe callback/final-disposal
entry with no upper metadata lock. No ROMFS-wide operation mutex is needed.

### Streams

[drvstreams_impl.inc](drivers/streams/drvstreams_impl.inc) has per-open nodes
and directory indices, but each file node points to the existing table entry.
Opening the same entry twice does not clone its `rstream`, `stream`, or `tty`.
Random-stream position belongs to that shared backend; different VFS nodes can
therefore still share a cursor. FIFO/TTY reads consume the same underlying
input, and write/control atomicity is the backend's responsibility.

Prerequisites: immutable table/names/interfaces, backend lifetime through all
nodes, and an explicit concurrency/ordering contract per published backend.
When its cursor or protocol is shared, callers must coordinate across all
aliases even if the VFS node pointers differ. Do not put an upper VFS mutex
around blocking stream or TTY calls. Directory enumeration alone needs only
per-handle serialization and the immutable table.

### FatFS — step 3 implementation

[drvfatfs_impl.inc](drivers/fatfs/drvfatfs_impl.inc) uses global native volume
routing. One optional module mutex now protects the singleton, including all
volumes, nodes, direct `FIL`/`DIR`/geometry reads, `f_getfs()`, and the
argument-less mount/unmount helpers. Separate wrapper objects cannot create
separate exclusion domains. The shared `Fsid`, optional `Files[]` and native
LFN scratch therefore remain serialized across wrapper calls.

Native `FF_FS_REENTRANT` is optional; the simulator validates the R0.14b
ChibiOS library with `FF_FS_REENTRANT=0` and `FF_FS_LOCK=0`. Native locks, when
configured, are acquired inside the wrapper domain. The wrapper takes its
mutex once for rewind/read and open/truncate/error-close. Node and info pools
are allocated before locking and freed after unlocking. The fixed filesystem
pool is nonblocking and is inspected under the singleton mutex during mount.
A failed immediate mount unregisters its object before returning it to the
pool; failed unmount must not return an object still registered with FatFS.

Mount/unmount and direct native formatting require an exclusive lifecycle
phase with no affected active calls or live nodes. Direct native APIs must
not overlap wrapper operations. Native duplicate-file/remove restrictions
remain: `FF_FS_LOCK` checks sharing rules; wrapper mutual exclusion does not
make prohibited sharing safe. The [disk bindings](../various/fatfs_bindings/fatfs_diskio.c)
forward to the configured block device. Other users of that device still need
a common device synchronization domain and safe start/stop/removal.

### LittleFS — step 3 implementation

The [VFS wrapper](drivers/littlefs/drvlittlefs_impl.inc) now has one optional
mutex per driver instance. It covers `lfs_t`, `mounted`, all path and node
operations (including close, tell, size, and rewind/read), and lifecycle calls.
Constructor/disposal require exclusive ownership. Configuration, buffers and
context remain fixed for the instance lifetime. Pools and final reference
cleanup are outside the native operation lock.

[littlefs.mk](../various/littlefs_bindings/littlefs.mk) no longer forces
`LFS_THREADSAFE`. The default supported VFS configuration uses its own wrapper
mutex with native reentrancy disabled. Optional native hooks remain supported;
the VFS demo/test configurations conditionally initialize them. The native
WSPI-LITTLEFS HAL test explicitly enables those hooks to preserve its coverage.

Leaf mutexes span native calls, storage callbacks and waits. They cannot be
released while non-reentrant library state is active. Storage callbacks must
not reenter VFS. Independent filesystem instances on independent storage can
progress concurrently. Multiple instances sharing a flash device need a
common device synchronization domain in addition to separate instance locks.
The standard [HAL binding](../various/littlefs_bindings/lfs_hal.h) requires an
exclusively owned device without native hooks; shared users can select native
hooks backed by working HAL exclusive access, or supply synchronized callbacks.
For EFL native hooks, `EFL_USE_MUTUAL_EXCLUSION` must be enabled. Lock/unlock
hook errors are now propagated instead of ignored. Device exclusion does not
make overlapping filesystem caches coherent: instances must use disjoint areas.

Mount/unmount/format remain externally quiesced lifecycle operations. Idle open
nodes still prevent unmount because disposal must close their native handles.
The simulator tests both native-lock configurations, same-instance exclusion
at a controlled I/O wait, independent instances, rewind/read, and error exits.

### CHFS and unfinished drivers

[drvchfs_impl.inc](drivers/chfs/drvchfs_impl.inc) currently initializes pools
and an object cache, but mount, lookup, and I/O remain skeleton operations.
Its cache callbacks currently do not perform storage I/O. The planned behavior
in [drvchfs_spec.md](drivers/chfs/drvchfs_spec.md) is not implemented behavior.

[chobjcaches.c](../oslib/src/chobjcaches.c) protects its indexing and grants
exclusive ownership of individual cache objects; callbacks run outside its
system section. This does not supply FS transaction, directory, allocation,
or media-lifecycle synchronization. Those are prerequisites of implementing
CHFS, not evidence for declaring the existing skeleton concurrently usable.
Template/unfinished drivers remain excluded from supported concurrency.

## Descriptor ownership — step 4 implementation

Newlib now protects insertion, lookup/pinning and close/detach with short
`chSysLock()` sections. An occupied slot owns one node reference. Read/write
and fstat pin that node before leaving table protection and release it outside.
Close transfers the table's reference to a local variable and clears the slot
before release. An open can reuse the number during an earlier I/O or disposal;
the earlier operation keeps its selected node. No node call, virtual reference
method or disposal executes under descriptor-table protection.

The private `pin_descriptorI()` helper increments the standard OOP counter
without dispatch or nested locking. All built-in node VMTs use the standard
`__ro_addref_impl`/`__ro_release_impl` pair. Newlib rejects custom reference
methods at open with `ENOTSUP`, releasing the returned node through its own
virtual release method outside protection. This explicitly limits admission
instead of silently bypassing override semantics. Custom disposal remains
supported. `OOP_USE_NOTHING` is rejected for VFS newlib builds. The pin/table
critical sections remain active with `VFS_CFG_USE_MUTUAL_EXCLUSION=FALSE`,
independently of optional metadata/leaf mutexes.

The changes preserve existing newlib syscall behavior, including the limited
mode-only fstat and seek/isatty stubs; completing those APIs is a separate task.
Reference pins do not provide same-handle ordering or libc stream protection.

Sandbox has one host thread per `sb_class_t`. [sbhost.h](../sb/host/sbhost.h)
restricts `sbSetRoot()` and reference-transferring `sbRegisterDescriptor()` to
the stopped state. Guest syscalls own the active table; VRQ delivery is deferred
while the host executes a syscall. Normal exit/fault and failed-start paths
invoke cleanup outside system locks before stopped-state reuse. Host lifecycle
operations themselves require external serialization.

The table's reference therefore retains a directory throughout getdents,
including guarded-pool and backend waits, without an extra pin or table mutex.
Close and cleanup now detach entries before release. Dup2 retains its source,
replaces the destination, then releases the displaced reference. Sharing nodes
with the host or another sandbox still requires separate references and
same-handle ordering. Supporting host-side mutation of an active table would
require a separate pinning protocol; it is outside the supported contract.

Simulator tests compile the production newlib bindings with only a minimal
libc ABI shim and renamed symbols. Controlled read/write/disposal suspension
verifies close/reuse, success/error cleanup, original-node retention, and
unlocked driver/disposal entry. Invalid descriptors, directories, full tables
and custom-reference rejection are covered. Sandbox changes are source-audited
and ARM compile/link validated; no sandbox runtime test is claimed here.

## Step 1 completion and follow-up

The state inventory, handle/lifetime contract, delegation boundaries, and
leaf-specific prerequisites are now recorded. The same-handle contract is
also documented in the generated node interface and VFS architecture page.
This step makes no runtime synchronization changes and adds no concurrency
claims to the previously run functional tests.

Steps 2 through 4 now supply local metadata and leaf mutexes plus descriptor
ownership protection. Next is step 5: finish API contracts and caller
integration, followed by final validation. The complete VFS stack is not yet
advertised as concurrently usable.
