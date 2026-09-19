# VFS ownership and local synchronization audit

Step 1 of the [local locking plan](local_locking_plan.md), completed on
2026-09-19 against `27f0ad644e`. This document specifies the intended contract
and records implementation gaps at that baseline. The linked plan tracks
which gaps have since been addressed. This audit does not certify the VFS as
thread-safe; steps 2 through 6 implement and validate the missing parts.

The audit covers the in-tree root, overlay, streams, ROMFS, FatFS, and LittleFS
implementations, CHFS's current skeleton, OOP references, pools, and the newlib
and sandbox adapters. The available FatFS source is R0.14b, revision 86631,
with ChibiOS extensions. `ext/littlefs` is absent, so LittleFS library behavior
and runtime concurrency remain unverified; its wrapper and HAL bindings were
inspected. Template and unfinished drivers have no concurrency guarantee.

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

The future `VFS_CFG_USE_MUTUAL_EXCLUSION` option controls only VFS-owned local
mutex fields/helpers. Disabled helpers are empty macros with no corresponding
mutex storage. Callers then supply serialization for those metadata domains.
It does not disable kernel pool/reference protection or configure a leaf
library's native synchronization. No global acquire/release API is introduced.

## State and protection inventory

| State and source | Current protection | Required protection or ownership |
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
| FatFS volume/library state; LittleFS `lfs`, `mounted`, `cfgp` | Configuration-dependent native locks; lifecycle gaps | Native domain shared by all callers, exclusive lifecycle phase, and the leaf prerequisites below. |
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
| Native FatFS/LittleFS to storage hooks | Native lock/lifecycle requirements, backend identity, and backend buffer lifetime. No inherited upper VFS metadata mutex. Native lock reentry is not implicitly supported. |

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

### FatFS

[drvfatfs_impl.inc](drivers/fatfs/drvfatfs_impl.inc) uses global native volume
routing: ordinary operations do not derive the volume's synchronization from
the `vfs_fatfs_driver_c` object. Two wrappers can access the same volume. A
per-wrapper mutex would therefore leave shared native state unprotected.

The inspected R0.14b `ext/fatfs/source/ff.c` uses `lock_fs()` with a per-volume
sync object when `FF_FS_REENTRANT` is enabled. The existing
[fatfs_syscall.c](../various/fatfs_bindings/fatfs_syscall.c) implements that
revision's `ff_cre_syncobj`, `ff_del_syncobj`, `ff_req_grant`, and
`ff_rel_grant` hooks using one semaphore per volume. The simulator configuration,
all five RT-VFS-FATFS configurations, and both L4R9 sandbox configurations
currently set `FF_FS_REENTRANT=0` and `FF_FS_LOCK=0`. Their successful functional
tests/builds are not concurrency validation.

Prerequisites for step 3:

- Enable native reentrancy for configurations offered for simultaneous access
  to one volume, link the matching hooks, and provide kernel semaphore support.
  The FatFS simulator target currently adds `ff.c` only; it must also link the
  synchronization hooks for such tests. Keep `FF_USE_LFN` at 0, 2, or 3:
  R0.14b rejects static LFN scratch (`FF_USE_LFN=1`) with native reentrancy.
- Serialize native mount/unmount/format and volume registration as lifecycle
  operations; no live nodes or calls may overlap them. `ffdrvMount()` queries,
  allocates/registers and sometimes frees a `FATFS`; `ffdrvUnmount()` unregisters
  then frees it. The native `f_getfs()` extension reads `FatFs[]` without a lock.
  Native per-volume file locking does not make these sequences safe.
- Audit cross-volume native globals before advertising multi-volume concurrency.
  R0.14b has a shared `Fsid` changed on actual mount, optional `Files[]` share
  tracking, and optional `CurrVol`; `lock_fs()` itself only locks one volume.
  Eager mounts in the exclusive lifecycle phase and `FF_FS_RPATH=0` avoid some
  races; media-triggered remount and shared file-table updates still need an
  explicit supported policy or native fixes. A VFS-wide wrapper lock is not
  the solution.
- Respect native duplicate-file/remove restrictions. `FF_FS_REENTRANT` protects
  filesystem internals; `FF_FS_LOCK` is a separate sharing check. With the
  latter disabled, callers must enforce the native file-sharing restrictions.
  Enabling it does not resolve the cross-volume table issue above.
- Native stat/position wrappers read `FIL`, `DIR`, and FS geometry fields
  directly. Their safety relies on same-handle serialization and a stable
  mounted volume. `first` combines rewind/read, and truncate-open combines
  open/truncate; native per-call locking does not promise atomic multi-call
  operations or namespace snapshots.
- The [disk bindings](../various/fatfs_bindings/fatfs_diskio.c) forward to the
  configured block device without adding a common operation lock. A shared
  physical device must supply suitable synchronization, including when native
  volumes have different locks. Its start/stop/removal remains a lifecycle
  concern. Update/test configuration through its template/updater when needed.

### LittleFS

[littlefs.mk](../various/littlefs_bindings/littlefs.mk) enables `LFS_THREADSAFE`.
The demo and test configurations wire `__lfs_lock`/`__lfs_unlock` from
[lfs_hal.c](../various/littlefs_bindings/lfs_hal.c) to
`flashAcquireExclusive()`/`flashReleaseExclusive()`. The domain is the actual
flash device, so configurations using it share that exclusive-access domain.
This is native backend protection, not an upper VFS metadata lock.

The [VFS wrapper](drivers/littlefs/drvlittlefs_impl.inc) reads `mounted` before
calling the library; mount/unmount/format check or modify it outside the native
lock. Merely adding a mutex around that boolean would not retain an active
filesystem through the subsequent library call. Disposers call the library
to close their native handles, so even idle open nodes prevent unmount.

Prerequisites: enforce the exclusive lifecycle phase, keep `cfgp` and its
buffers/context immutable, use one `lfs_t` state per mounted filesystem, and
require working native lock hooks for every concurrent configuration. For
EFL this includes `EFL_USE_MUTUAL_EXCLUSION=TRUE`; its disabled implementation
asserts/returns `FLASH_ERROR_UNIMPLEMENTED`, while the binding currently ignores
that result and reports success. Step 3 must validate that requirement or
propagate failures. A shared device lock cannot make two independent mutable
filesystem caches for the same storage coherent.

Once the external library is available, verify the actual lock coverage of
open/close, stat, read/write, seek/tell/size, mount/unmount/format, and all
storage callbacks; then run distinct-handle tests. Rewind/read is still a
compound operation covered by the caller's same-handle rule. LittleFS is not
yet validated for concurrent use in this worktree.

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

## Descriptor ownership and remaining integration requirements

Newlib currently scans/inserts `fds[]` without protection, uses bare pointers
for read/write/stat, and releases before clearing a close slot. Concurrent
close/reuse can invalidate an operation's node. The earlier plan's reference
to existing read/write pins applied to the preserved global-lock branch;
there are no such pins in the restarted foundation.

Step 4 must implement atomic lookup plus lifetime retention, atomic slot
insertion/detachment, and release outside protection. The generic virtual
`roAddRef()` cannot be called under `chSysLock()` (its default implementation
locks again), or under a table mutex that must not span node methods. All
built-in node VMTs inspected use the standard OOP addref/release pair. A
non-dispatching I-class pin may serve that explicitly supported reference
model; arbitrary overrides need an explicit compatible protocol or deferred
table-owned references. Do not silently bypass custom addref/release behavior.
Selecting and testing that mechanism is part of step 4, not completed here.

Sandbox has one host thread per `sb_class_t`. [sbhost.h](../sb/host/sbhost.h)
restricts `sbSetRoot()` and reference-transferring `sbRegisterDescriptor()` to
the stopped state. Guest syscalls own the active table; normal exit/fault and
failed-start paths invoke cleanup outside system locks before stopped-state
reuse. Host lifecycle operations themselves require external serialization.
Keep that ownership contract rather than adding a table mutex around syscalls.
Sharing a node with the host or another sandbox still requires separate
references and same-handle ordering. Detach close/dup2/cleanup slots before
disposal can call external code; retain new references before replacing old
ones. Supporting active host-side table mutation would require a new pinning
protocol, not just a short critical section around a pointer read.

## Step 1 completion and follow-up

The state inventory, handle/lifetime contract, delegation boundaries, and
leaf-specific prerequisites are now recorded. The same-handle contract is
also documented in the generated node interface and VFS architecture page.
This step makes no runtime synchronization changes and adds no concurrency
claims to the previously run functional tests.

Next is step 2: optional overlay/root metadata locking, safe CWD storage,
consistent route snapshots, and explicit overlay enumeration phase. Steps 3
and 4 must satisfy the leaf and descriptor prerequisites before the complete
VFS stack is advertised as concurrently usable.
