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

The foundation does not make VFS thread-safe. Callers still serialize shared
mutable state until the local synchronization work below is implemented.

## Required behavior

Use no global VFS operation mutex. Each layer protects its own mutable
state internally. A local mutex must be released before calling another
driver, a node virtual method, a callback, or reference release/disposal.
Do not replace that mutex with an operation-wide semaphore or busy gate that
keeps the same outer serialization across delegation.

Use short critical sections for small updates, and local mutexes for work
such as CWD copying and mount-table searches. Never sleep, allocate storage,
or call external code inside a system critical section. Avoid nesting local
mutexes: snapshot one object's state and release its lock before entering
another object.

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

## Proposed synchronization responsibilities

| State | Protection and scope |
| --- | --- |
| Root CWD and overlay mount table | One optional metadata mutex per overlay object, inherited by root; local copying, lookup, and updates only |
| Root temporary paths | Exclusive ownership of one guarded-pool element |
| Node and scratch pools | Existing internally synchronized pool APIs |
| Reference counts | Existing OOP critical sections; a safe non-dispatching pin operation for descriptor lookup where necessary |
| Newlib descriptor table | Short critical sections for lookup/pinning, insertion, and detachment; I/O and disposal outside |
| Sandbox descriptor table | Existing lifecycle ownership where sufficient; short local protection only where concurrent access is supported |
| Leaf filesystem and node state | The leaf implementation or underlying library's own synchronization; no inherited VFS mutex |
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

1. **Specify operation and ownership contracts; audit leaf readiness.**

   Inventory CWD, routing tables, directory cursor/phase, descriptor entries,
   reference transfer, node positions, and driver/library shared state.
   Identify every delegation and disposal boundary and what must survive it.

   Proposed handle rule: distinct open handles may run concurrently. Callers
   serialize operations on the same open file/directory unless its driver
   explicitly guarantees concurrent use. Duplicated descriptors share a handle.
   Each active user owns a reference; closing another owned reference is safe.
   Unlike the preserved global-lock prototype, this contract does not promise
   ordinary same-handle serialization, and must be documented before integration.
   A local cursor lock alone cannot serialize backing `first`/`next` calls.

   Audit actual leaf implementations, not just the wrapped library's advertised
   capabilities. FatFS tests currently set `FF_FS_REENTRANT=0`; evaluate native
   synchronization and mount/unmount constraints. LittleFS bindings enable
   `LFS_THREADSAFE`; verify hooks, configurations, and lifecycle operations.
   Audit CHFS/cache interactions, stream forwarding, ROMFS positions, and
   dynamic/compressed sessions and callbacks. Keep unsupported sharing explicit.

   Completion: a concrete state/lock inventory, documented handle contract,
   and identified prerequisites for each supported leaf configuration.

2. **Protect root and overlay metadata locally.**

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
   commit. Concurrent successful changes commit in completion order. Handle
   lazy CWD allocation outside the mutex and recheck before installing storage;
   avoid leaking competing allocations from the monotonic core allocator.
   If necessary, use a reclaimable allocation or change CWD storage explicitly.

   Give overlay enumeration an explicit mounts/backing/end phase instead of
   using a mutable mount count as the backing-start marker. Copy mount names
   while protected and access the backing directory after unlocking. Define
   enumeration during mount changes as a live view that may skip/repeat entries
   but must remain memory-safe and must not restart backing iteration because
   the mount count changed. Apply the handle contract from step 1.

   Completion: metadata has matching reader/writer protection; no local mutex
   surrounds delegation, cleanup, or a pool wait.

3. **Make leaf synchronization sufficient without the global mutex.**

   Implement the prerequisites found in step 1 using native library locking
   or protection of each driver's own state. Audit open/close, mount/unmount,
   callbacks, stream access, and error cleanup as well as read/write. Two roots
   sharing one leaf must share that leaf's synchronization domain. For FatFS,
   account for shared volumes and library-global state across wrapper objects.

   Do not add a generic VFS wrapper mutex around another driver's calls. Any
   callback must enter without a caller-owned VFS metadata mutex. Audit native
   library/backend lock ordering separately so internal synchronization does
   not reintroduce dependencies on upper-layer VFS state.

   Completion: every configuration offered as concurrently usable has a
   supported leaf contract. Optional untested backends are listed explicitly;
   removing the global lock is not itself evidence that they are thread-safe.

4. **Separate descriptor ownership from I/O.**

   Newlib lookup must acquire a node reference atomically with inspecting the
   table slot; release the table protection before node calls. Close detaches
   the pointer under protection and releases it afterward. Preserve the
   existing read/write pins across concurrent close and descriptor reuse.

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
All six local synchronization steps remain pending. The next step is the
operation/ownership contract and leaf-driver readiness audit.

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
- LittleFS tests were not run because the external library is unavailable.
  Build products were cleaned.
