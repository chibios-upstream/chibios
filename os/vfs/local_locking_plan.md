# VFS local locking migration plan

Updated on 2026-09-19 for a restart from master with a reusable foundation.

The completed global-lock solution is preserved on the local branch
`backup/vfs-global-locking-20260919` at `b8ec66b840`.
The original migration plan is preserved separately on
`backup/vfs-local-locking-plan-20260919` at `4e48e4ff11`. The previous
`mutual_exclusion_plan.md` and `mutual_exclusion_design.md` remain on those
backup branches as records of the global-lock implementation and validation.

Development restarts on `chibios-vfs-dev` from fetched `origin/master` at
`97faf9c40c`. The foundation restores paired buffers, root-owned prefixes,
read-only routing, combined root opens, path-boundary fixes, and independent
ELF/directory scratch fixes. The global mutex API, implicit locking scopes,
and atomic mutex/pool waiter are not carried forward. Configuration files
are regenerated for buffer pairs, without introducing a mutex option yet.

The foundation alone does not make VFS thread-safe. Steps 2 through 4 now
supply optional metadata/leaf mutexes and descriptor ownership protection.
Caller integration and final validation remain before concurrent use of the
full stack is supported.

## Required behavior

Use no global VFS operation mutex. Each layer protects its own mutable
state internally. An upper metadata mutex must be released before calling
another VFS driver, a node virtual method, a callback, or reference
release/disposal.
Leaf FatFS/LittleFS wrapper mutexes protect their native-library calls,
including storage callbacks and I/O waits.
Do not replace an upper mutex with an operation-wide semaphore or busy gate that
keeps the same outer serialization across delegation.

Use short critical sections for small updates, and local mutexes for work
such as CWD copying and mount-table searches. Never sleep, invoke a
thread-context allocator, or call external code inside a system critical
section. Blocking allocations stay outside metadata mutexes. The bounded,
nonblocking `chCoreAlloc()` call for initial CWD storage runs under the root
metadata mutex, preventing duplicate allocations. Avoid nesting local mutexes:
snapshot one object's state and release its lock before entering another object.

Preserve the paired-buffer union, combined scratch view, root-owned prefix,
read-only overlay/ROMFS routing, and allocation/use/release within one function.
Reserve a pair before taking metadata locks. A reservation protects temporary
paths while another operation or driver runs; it does not protect CWD or mounts.
Retain the prohibition on waiting for a second pair while holding the first,
including through callbacks. Root-under-root routing remains unsupported.

File systems remain caller-owned and must outlive active operations and nodes.
Unregistering a mount does not establish that its file system is idle. Mount
names must be copied while protected or remain valid until readers finish.
Keep prefixes/backing pointers immutable after publication unless an explicit
synchronized reconfiguration API is introduced.

## Synchronization responsibilities

| State | Protection and scope |
| --- | --- |
| Root CWD and overlay mount table | One optional metadata mutex per overlay object, inherited by root; local copying, lookup, and updates only |
| Root temporary paths | Exclusive ownership of one guarded-pool element |
| Node and scratch pools | Existing internally synchronized pool APIs |
| Reference counts | Existing OOP critical sections; a safe non-dispatching pin operation for descriptor lookup where necessary |
| Newlib descriptor table | Short critical sections for lookup/pinning, insertion, and detachment; I/O and disposal outside |
| Sandbox descriptor table | Existing lifecycle ownership where sufficient; short local protection only where concurrent access is supported |
| Leaf filesystem and node state | Optional singleton FatFS/per-instance LittleFS wrapper mutex; other leaves follow their documented backend contract |
| Shared open directory/file operation ordering | Explicit handle contract, independent of reference-count safety |

Introduce `VFS_CFG_USE_MUTUAL_EXCLUSION` as the global option controlling
VFS-owned local mutex storage and operations, defaulting to `FALSE`. Disabled
local acquire/release helpers are empty macros and objects contain no optional
mutex fields. Existing kernel pool/reference synchronization remains active.
When disabled, callers must serialize otherwise unprotected shared state.
Native filesystem/backend synchronization retains its own configuration.

The XML schema supports conditional fields and regular methods. Use those
facilities; do not modify generated class layouts by hand. Do not put a mutex
in the abstract FS class merely because some implementations need one.

## Implementation steps

1. **Specify operation and ownership contracts; audit leaf readiness — complete.**

   The [step 1 audit](local_locking_audit.md) records the concrete inventory,
   ownership/handle contract, delegation boundaries, and leaf prerequisites
   against `27f0ad644e`. This completes the specification and source audit;
   implementing or validating concurrent use remains in the later steps.

   Inventory CWD, routing tables, directory cursor/phase, descriptor entries,
   reference transfer, node positions, and driver/library shared state.
   Identify every delegation and disposal boundary and what must survive it.

   Handle rule: distinct open handles may run concurrently when the leaf and
   shared backend support it; stream nodes can alias the same cursor. Callers
   serialize operations on the same open file/directory unless its driver
   explicitly guarantees concurrent use. Duplicated descriptors share a handle.
   Each active user owns a reference; closing another owned reference is safe.
   Unlike the preserved global-lock prototype, this contract does not promise
   ordinary same-handle serialization, and must be documented before integration.
   A local cursor lock alone cannot serialize backing `first`/`next` calls.

   Audit actual leaf implementations, not just the wrapped library's advertised
   capabilities. The baseline FatFS tests set `FF_FS_REENTRANT=0` and LittleFS
   bindings force `LFS_THREADSAFE`; evaluate synchronization, hooks,
   configurations and lifecycle constraints. Step 3 replaces the initial
   native-reentrancy prerequisite with wrapper-owned mutexes.
   Audit CHFS/cache interactions, stream forwarding, ROMFS positions, and
   dynamic/compressed sessions and callbacks. Keep unsupported sharing explicit.

   Completion: a concrete state/lock inventory, documented handle contract,
   and identified prerequisites for each supported leaf configuration.

2. **Protect root and overlay metadata locally — complete.**

   Implemented one conditional overlay mutex inherited by root, matching
   mount-table reader/writer protection, local CWD snapshots/commit, paired
   rename route selection, and explicit mounts/backing/end enumeration.
   `VFS_CFG_USE_MUTUAL_EXCLUSION` defaults to `FALSE`; disabled helpers are
   empty macros and no mutex field is present. Enabled builds require kernel
   mutex support. The template and all 24 configurations have been updated.

   Each root retains one CWD pointer, initially `NULL` for `/`. Its first
   successful `chdir` allocates a full path buffer with `chCoreAlloc()` under
   the metadata mutex; subsequent changes reuse that buffer. Core allocation
   is bounded and nonblocking, so this prevents competing allocations without
   another pool or a recheck protocol. Storage remains permanent, as with the
   original core allocation model; root disposal does not reclaim it. No
   operation pair is retained persistently. Prefixes and backing pointers
   retain the immutable-publication contract.

   Implementation requirements:

   Add the conditional metadata mutex and private local helpers in the overlay
   XML/implementation; root reuses that mutex. Protect reads and writes of the
   mount table with the same mechanism, replacing the current writer-only
   system sections as appropriate. Rename selects both routes under one local
   lock at each routing layer. Save selected FS pointers before unlocking.

   Root reserves its pair before locking. Resolve relative inputs using a
   consistent CWD while holding the root metadata lock, then work on the private
   paths. Rename uses one CWD for both inputs. Combined open retains its logical
   absolute input across file/directory fallback. Release the metadata mutex
   before every delegated call; mapping changes between calls are permitted,
   with the same lifetime rules as the preserved implementation.

   `getcwd` copies under the metadata lock. `chdir` resolves a candidate,
   unlocks, validates it and releases the validation node, then relocks to
   commit. Concurrent successful changes take effect in commit order. Keep
   the initial CWD check, nonblocking core allocation, pointer installation,
   and copy under that mutex. Failed validation leaves CWD unallocated or
   unchanged. With local locking disabled, callers serialize these operations.

   Give overlay enumeration an explicit mounts/backing/end phase instead of
   using a mutable mount count as the backing-start marker. Copy mount names
   while protected and access the backing directory after unlocking. Define
   enumeration during mount changes as a live view that may skip/repeat entries
   but must remain memory-safe and must not restart backing iteration because
   the mount count changed. Apply the handle contract from step 1.

   Completion: metadata has matching reader/writer protection; no upper metadata
   mutex surrounds delegation, cleanup, or a pool wait.

3. **Protect external filesystems in their VFS wrappers — complete.**

   Implemented and validated with native reentrancy disabled. XML interfaces,
   the LittleFS binding, optional native-hook initializers, configuration
   template and all 24 configurations are aligned. The native HAL-only test
   explicitly enables its own LittleFS hooks.

   Wrapper synchronization follows `VFS_CFG_USE_MUTUAL_EXCLUSION`. FatFS is
   a singleton: one optional mutex in its module state protects every volume,
   node operation, and the argument-less mount/unmount helpers. LittleFS has
   one optional mutex per driver instance, protecting its `lfs_t`, mounted
   state, and all node operations through their owning FS. Disabled helpers
   are empty macros and no wrapper mutex storage remains.

   Native FatFS/LittleFS reentrancy is optional. Test FatFS with
   `FF_FS_REENTRANT=0` and LittleFS without `LFS_THREADSAFE`; remove the mandatory
   LittleFS build definition and make native lock-hook configuration optional.
   Cover open/close, stat/position queries (including native macros), I/O,
   directory rewind/read, compound open/truncate, and mount/format/unmount.
   Avoid recursive wrapper acquisition by using private locked helpers for
   compound operations. Pools and reference disposal remain outside leaf
   locks wherever they do not access native state.

   Upper metadata locks never span delegation. A leaf wrapper's operation
   mutex protects the entire external-library call, including storage callbacks
   and I/O waits; it cannot be dropped while non-reentrant native state is
   active. The order is leaf wrapper, then native/backend protection where
   configured. Storage callbacks must not reenter that filesystem or acquire
   upper VFS metadata locks. Direct native API access during VFS use must obey
   the same serialization domain; bypassing the wrapper is unsupported.

   Shared hardware still requires backend synchronization. LittleFS instances
   using one flash device must share its exclusive-access domain; distinct
   filesystem states must not mount the same storage concurrently. FatFS's
   singleton mutex covers shared native globals as well as its volumes, but
   external users of its block device still require device-level coordination.
   Mount/unmount/format require an exclusive lifecycle phase with no affected
   operations or live nodes; locking alone does not provide hot-unmount safety.

   Preserve the ROMFS immutable-data/per-session callback contract and stream
   backend sharing contract. CHFS remains an unfinished driver, outside the
   concurrency guarantee. Validate actual available library versions and
   deterministic competing operations, independent LittleFS instances, and
   progress outside a blocked leaf. Update XML and regenerate interfaces.

   Completion: supported wrappers provide their own optional synchronization
   without relying on native reentrancy. Remaining backend restrictions and
   unsupported configurations are documented explicitly.

4. **Separate descriptor ownership from I/O — complete.**

   Implemented short newlib table critical sections and a private I-class
   pin for standard OOP nodes. Read/write/fstat retain the selected node;
   close detaches before release and permits immediate descriptor reuse.
   Custom addref/release implementations are rejected with `ENOTSUP` at open,
   with virtual cleanup outside protection; custom disposal is supported.
   `OOP_USE_NOTHING` is rejected for VFS newlib builds. Table/reference critical
   sections remain active independently of the optional VFS mutex setting.
   No generic OOP API, VMT field, table mutex or additional table storage is
   introduced. Existing limited syscall behavior is otherwise preserved.

   Sandbox close/cleanup detach before release; dup2 retains and publishes
   the new reference before releasing the old one. Source review confirms
   one host thread owns the active table, VRQs defer during host syscalls,
   and lifecycle cleanup runs unlocked before stopped-state reuse. That
   table reference retains getdents nodes across its ordinary buffer wait.

   Implementation requirements:

   Newlib lookup must acquire a node reference atomically with inspecting the
   table slot; release the table protection before node calls. Close detaches
   the pointer under protection and releases it afterward. Add read/write
   operation pins that remain valid across concurrent close and descriptor reuse.
   The restarted foundation had no pins; these are now implemented locally,
   without restoring the preserved branch's global operation lock.

   `roAddRef()` is virtual and its default implementation already enters
   `oopLock()`. Do not call it inside `chSysLock()` or assume arbitrary addref
   overrides are safe under a local table mutex. Specify a non-dispatching
   reference-pin helper with the correct I-class contract for standard VFS
   nodes, or another ownership protocol if custom reference overrides must be
   supported. Never bypass override semantics silently. Final release and
   disposal always happen after leaving table protection.

   Confirm the sandbox's single-thread/lifecycle guarantees without adding a
   sandbox-wide VFS scope. Detach cleanup entries before releasing them.
   `getdents` continues to use the ordinary scratch allocator and
   retains its node according to the descriptor ownership contract.

   Completion: lookup/close/reuse tests pass without a VFS-wide scope and no
   descriptor lock reaches a node call or disposal.

5. **Finish local API contracts and caller integration.**

   Direct and convenience entry points use the same internal local
   synchronization. Do not introduce `vfsAcquire()`/`vfsRelease()` or public
   no-op compatibility functions that appear to protect multi-call transactions.

   Keep ordinary guarded-pool allocation. The clean baseline already excludes
   `vfs_buffer_take_wait_locked()`, `__vfs_pool_alloc()`, `__vfs_assert_locked()`,
   and the VFS dependency on `__mtx_unlock_no_reschedule()`. Audit direct
   reference-count assertions for unprotected counter reads.

   Update sandbox, ELF, HTTP, newlib, shell, demos, and tests together. Preserve
   combined scratch capacity, ELF record boundaries, `/sb1` prefixes, mount
   bypass, logical CWD, and per-root combined opens. Document callbacks using
   local ownership and lifetime rules.

   Update XML, generated interfaces, `vfsconf.h.ftl`, checks, and documentation.
   Run all 24 configuration updates sequentially, preserving existing values.
   The disabled option must remove VFS-local mutex storage and calls; verify
   enabled/disabled and kernel-mutex availability combinations.

   Steps 2 through 4 build on the foundation without a global contract.
   Validate the prerequisites and caller integration together before claiming
   concurrent use is supported. No partial stage should be described as the
   completed local-lock architecture.

6. **Validate concurrency and driver-boundary behavior.**

   Retain existing functional regressions for root prefixes, merged root
   listing, mount bypass, nested overlays, rename, path limits, and cleanup.
   Adapt the backup branch's global-lock tests into probes that verify caller
   metadata mutexes are not owned at driver/node/callback/disposal entry.

   Extend the foundation's controlled driver-suspension tests without adding
   driver-side release or acquisition of an upper-layer lock.
   Verify progress on the same root and unrelated roots while a leaf sleeps,
   CWD consistency, mount changes during routing, both rename paths, combined
   open fallback, and completion in different orders. Exercise initial CWD
   allocation races and live directory enumeration during mount changes.

   Run one/two/three-pair exhaustion and scratch contention in both directions;
   allocation now waits without any upper-layer mutex. Preserve full pool
   recovery on success and error. Validate typed directory scratch spanning
   both halves and ELF chunks with non-record-aligned combined capacity.

   Exercise descriptor close/reuse and final disposal during I/O, independent
   nodes sharing a leaf, and the declared same-handle restrictions. Retain the
   root-disabled, locking-disabled, recursive-kernel-mutex, and no-kernel-mutex
   configurations. Check object sizes and symbols for disabled mutex storage.

   Run the FatFS suite and available LittleFS tests, STM32G474 VFS/sandbox
   builds, and both prefix-using L4R9 SB demo builds. Keep physical-board results
   separate from simulator execution and compile/link checks.

   Validate edited XML, regenerate derived files, verify a second generation
   is identical, run whitespace checks, and clean build products.

## Status

Both backup branches have been created. The active branch has been realigned
to `97faf9c40c`, and the reusable foundation is restored without global locking.
Steps 1 and 2 are complete: [ownership and local synchronization audit](local_locking_audit.md),
followed by optional root/overlay metadata protection, one lazy CWD buffer per root,
and explicit directory enumeration phase. Step 3 is also complete: optional
FatFS singleton and LittleFS per-instance wrapper synchronization, with native
reentrancy optional. Step 4 now protects newlib descriptor ownership and
orders sandbox descriptor release correctly. Steps 5 and 6 remain pending;
API contracts and caller integration are next.

The step 1 source audit found native FatFS reentrancy disabled, LittleFS
mount-state and native-hook prerequisites, shared backend state behind streams
and callbacks, and unprotected newlib descriptor lookup/close. Step 3 resolves
the filesystem prerequisites through wrapper locking; native reentrancy is no
longer required. Step 4 addresses descriptor ownership. Backend sharing and
same-handle ordering retain their own requirements. The node interface and architecture page now
document the handle contract. Step 1 validation is documentation/source review,
XML schema validation, repeatable generation, and whitespace/link checks;
no runtime synchronization behavior changed.

Step 4 validation on 2026-09-19:

- Six simulator configurations passed with checks, assertions and the system
  state checker: metadata/leaf locking enabled; locking disabled; no kernel
  mutexes; enabled with both native filesystem wrappers and three pairs;
  recursive kernel mutexes with two pairs; and root disabled. The newlib
  tests run in the five root-enabled configurations; the root-disabled build
  verifies that the test shim and sequence are excluded correctly.
- The simulator compiles the production newlib bindings with only a small
  libc ABI shim and renamed exported symbols. Paused read/write tests verify
  that close/reuse preserves the original node and releases its pin on success
  and error. A paused final disposal permits another open/read on the reused
  descriptor. Entry checks detect inherited system or mutex protection.
- Invalid descriptor, directory, full-table and custom reference-method errors
  preserve cleanup and slot availability. Rejected custom release methods are
  invoked outside table protection; they are not bypassed by the pin helper.
- STM32G474 FatFS/newlib and dynamic sandbox demos compile/link with local
  locking, assertions, parameter checks and the state checker enabled.
  Sandbox lifecycle/VRQ ownership was source-audited; no sandbox or physical
  board runtime execution is claimed for this step.
- Test XML validation, repeatable regeneration and whitespace checks passed.
  Build products were cleaned.

Step 3 validation on 2026-09-19:

- Six combined FatFS/LittleFS simulator configurations passed with checks,
  assertions and the system-state checker: wrapper locking enabled; disabled;
  disabled with no kernel mutexes; enabled with optional LittleFS native hooks;
  enabled with recursive kernel mutexes and three buffer pairs; and enabled
  with root support disabled. FatFS uses `FF_FS_REENTRANT=0` throughout, and
  LittleFS native reentrancy is off except in the explicit native-hook variant.
- Controlled storage suspension verifies that another FatFS node waits for the
  singleton mutex, and another LittleFS operation waits for its instance mutex.
  A separate LittleFS instance on independent RAM storage creates, writes,
  seeks, reads and closes while the first instance is suspended. Directory
  rewind/read and error/lifecycle paths complete without recursive acquisition.
  Failed immediate FatFS mounts preserve registration/pool consistency.
- Disabled FatFS/LittleFS object files contain no mutex references. On the
  64-bit simulator, the FatFS module state is 824/856 bytes and LittleFS objects
  are 184/216 bytes with locking disabled/enabled. Abstract FS and FatFS wrapper
  objects remain 8 bytes in both configurations.
- STM32G474 FatFS and STM32L476 LittleFS VFS demos compile/link with wrapper
  locking enabled and native reentrancy disabled. The STM32L476 native
  WSPI-LITTLEFS HAL test compiles/links with native hooks explicitly enabled.
  These are compile/link checks, not physical-board execution.
- All 24 configurations were regenerated sequentially without changing option
  values. XML validation, repeatable VFS/test generation, and whitespace checks
  passed. Build products were cleaned.

Step 2 validation on 2026-09-19:

- Seven simulator configurations passed with checks, assertions, and the
  system-state checker: disabled/one pair; enabled/one pair; enabled/three
  pairs; enabled/two pairs with recursive kernel mutexes and a 128-character
  path limit; disabled with no kernel mutexes; enabled with root disabled;
  and enabled with FatFS. FatFS coverage here remains functional, not native
  concurrent-volume validation.
- The final lazy-core-allocation change was revalidated with five simulator
  variants: locking disabled, enabled with one and three pairs, recursive
  mutexes with two pairs and a 128-character path limit, and no kernel mutexes.
- Regressions verify CWD and mount-table readers/writers wait for the same
  mutex, metadata stays available during root buffer-pool waits, driver and
  disposal suspension permits another CWD change, initial CWD publication
  preserves commit order, mount changes do not restart backing enumeration,
  and queries/failed validation leave CWD unallocated. Competing initial
  chdir calls consume one CWD allocation; later changes reuse it without
  consuming further core memory, and different roots have separate storage.
- Driver/callback entry assertions detect leaked metadata mutex ownership.
  Disabled root/overlay object files contain no mutex references. On the
  64-bit simulator, FS/overlay/root sizes are 8/56/72 bytes disabled and
  8/88/104 enabled; only overlay and its root subclass gain the mutex.
- Invalid option values and enabling VFS mutexes without kernel mutex support
  are rejected. STM32G474 CHFS compiles/links with local locking enabled;
  no physical-board execution was performed.
- All 24 configurations were regenerated sequentially with prior values
  preserved. XML schema validation, repeatable VFS/test generation, and
  whitespace checks passed. Build products were cleaned.

Foundation validation on 2026-09-19:

- Simulator suite passed with checks, assertions, and system-state checking:
  one pair, three pairs, two pairs with `VFS_CFG_PATHLEN_MAX=128`, root disabled,
  kernel mutexes disabled, and FatFS enabled. Driver suspension tests use
  controlled scheduling to test path ownership, not metadata thread safety.
- STM32G474 CHFS and dynamic sandbox builds passed, as did both prefix-using
  L4R9 sandbox builds. These were compile/link checks, not board execution.
- An isolated probe of the actual ELF relocation loop passed under address
  and undefined-behavior sanitizers with 258-byte scratch capacity, 65 records,
  an empty section, allocation/read errors, and malformed section length.
- All 24 `vfsconf.h` files were updated sequentially with existing values
  preserved. Edited XML validated; VFS and test regeneration was repeatable.
- LittleFS tests were not run at the foundation stage because the external
  library was then unavailable. Step 3 supplies the current validation.
  Build products were cleaned.
