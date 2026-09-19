# VFS limitations and follow-up work

Initial audit: 2026-09-19, against `2117dd204b` on `chibios-vfs-dev`.
Update this inventory as behavior changes; retain item IDs when recording a
resolution and link the implementation and relevant regression coverage.

This document distinguishes constraints of the selected native filesystem or
storage model from behavior that can be addressed in VFS, its wrapper drivers,
or its libc adapters. A native constraint can sometimes be mitigated by VFS,
but that may require a different open-handle model or additional persistent
metadata, rather than another mutex.

This is a compatibility inventory, not a claim of full POSIX conformance.
Read-only filesystems, configured resource limits, `EXDEV` across filesystems,
and directory enumeration without a snapshot are not inherently POSIX bugs.

Related documents:

- [Supported POSIX I/O contract](posix_io_contract.md): required behavior,
  error choices and backend decisions.
- [POSIX I/O implementation plan](posix_io_plan.md): migration progress and
  simulator/ARM/hardware validation. Steps 1 through 4 are complete at this
  audit; the host Newlib migration is next.
- [Local locking audit](local_locking_audit.md): ownership, synchronization,
  callback and filesystem-lifetime requirements.
- [Open points](open_points.md): other implementation and maintenance work.

## 1. Intrinsic filesystem and backend constraints

These constraints belong to the current backend or its native API. They do
not mean that every possible emulation is impossible. The last column records
the boundary between a backend constraint and possible VFS mitigation.

| ID | Backend | Constraint | VFS mitigation or boundary |
| --- | --- | --- | --- |
| I-FAT-01 | FatFS | Independent opens of the same file are only supported when all are read-only. A conflicting writer can invalidate native per-handle state. A wrapper mutex does not make separate `FIL` caches coherent. | Enable and size `FF_FS_LOCK` to reject conflicts. `dup` shares one native handle and is supported. General POSIX independent writable opens would require a coherent open-file model, not just serialized native calls. |
| I-FAT-02 | FatFS | Native rename does not replace an existing different destination. Open objects must not be removed or renamed. | Preserve `EEXIST`/sharing errors. Unlink-then-rename is not an atomic replacement and can lose the destination on failure; do not use it as a transparent emulation. POSIX replacement and unlink-while-open need substantially different backend/lifetime support. |
| I-FAT-03 | FAT naming and metadata | Names follow FAT case/encoding rules; long-name support depends on FatFS configuration. FAT does not provide Unix UID/GID, permission/link identity semantics, or POSIX timestamp precision/timezone information. | Expose native capabilities honestly. Full Unix metadata would need an additional storage convention. UTC interpretation and incomplete metadata propagation are separate VFS issues, listed below. |
| I-FAT-04 | FatFS seeking | Native read-only seeks cannot position beyond EOF. Writable `f_lseek` can extend the file immediately and leaves the new contents undefined. | This is a native API constraint, but exposing it directly is addressable in VFS: see FAT-02. It does not force the public API to use identical semantics. |
| I-LFS-01 | LittleFS | Conflicting independent writable opens are not a supported VFS use of the current native cached-handle model. An existing two-append-handle probe lost the first writer's update. | VFS can reject conflicts, or redesign shared file state. No rejection registry exists today. A per-instance mutex alone is insufficient; duplicates of one handle are supported. |
| I-LFS-02 | LittleFS metadata | The native file information does not supply Unix ownership/permissions, timestamps, or a per-file allocated-block count. Native custom attributes are available, but need an application-defined schema for such metadata. | Attribute exposure and a metadata convention are possible VFS work; reliable values must not be fabricated from file size or device geometry. |
| I-ROM-01 | ROMFS | Embedded raw/compressed contents and the published namespace are read-only. Dynamic callbacks provide data, not a writable filesystem namespace. | Writes, creation, deletion, rename and mkdir/rmdir require a writable backend or a separately designed writable layer. Read-only operation itself is intentional. |
| I-STREAM-01 | Stream/device backends | A sequential device may have no seekable position, persistent files, or meaningful file size. Shared device I/O, terminal state and error reporting depend on the supplied interface. | VFS cannot manufacture regular-file semantics for an arbitrary device. Random-stream position/error limitations and per-open state are addressable interface work; see STM-01 through STM-03. |

FatFS sharing restrictions are independent of `FF_FS_REENTRANT` and of the VFS
mutex option. The writable L4R9 sandbox demos currently use `FF_FS_LOCK=16`.
LittleFS supports separate instances; their mounted storage areas must not
overlap, and shared hardware still requires device-level synchronization.

Native evidence is in the vendored FatFS
[sharing notes](../../ext/fatfs/documents/doc/appnote.html),
[seek documentation](../../ext/fatfs/documents/doc/lseek.html), and
[LittleFS API](../../ext/littlefs/lfs.h). The LittleFS conflicting-open probe and
the chosen portable contract are recorded in [the I/O contract](posix_io_contract.md).

## 2. Limitations addressable in VFS

Status meanings:

- **Open**: behavior is established by source review or existing coverage;
  the improvement is not implemented. This does not imply a dedicated runtime
  regression test exists for every item.
- **Needs regression**: source review identified a likely compatibility bug;
  reproduce it in a targeted test before changing behavior.
- **Design limit**: intentional current scope or configuration choice;
  changing it requires an explicit design decision.
- **Planned**: already assigned to an implementation-plan step.
- **Unimplemented**: placeholder code, not usable filesystem functionality.

### Shared API and object model

| ID | Status | Current limitation | Possible work |
| --- | --- | --- | --- |
| VFS-01 | Open | Offsets and sizes use signed 32-bit `vfs_offset_t`; no supported large-file interface beyond `INT32_MAX`. Some FatFS stat/directory-size conversions still narrow larger native values without checking. | Add consistent overflow rejection first; assess a 64-bit API and host/guest ABI separately. |
| VFS-02 | Open | No common `fsync`/`fdatasync` or arbitrary `truncate`/`ftruncate` operation. Open-time `O_TRUNC` is supported. | Add explicit node/path methods and backend capability/error contracts. FatFS and LittleFS already have native sync/truncate primitives. |
| VFS-03 | Open | Final reference disposal discards native close/flush errors. Closing a descriptor can precede final disposal when another reference or operation pin exists. | Design status-returning flush/close behavior without breaking reference ownership. A successful descriptor close currently does not confirm durable storage. |
| VFS-04 | Design limit | No Unix credentials/permission enforcement, umask, chmod/chown, or timestamp-update API. Creation mode is ignored by current adapters/backends; supported metadata is partial. | Decide the supported permission/metadata model and which backends can persist it. Opened read/write access is enforced and must not be confused with Unix permission enforcement. |
| VFS-05 | Design limit | No hard-link/symlink API, `openat` family, `pread`/`pwrite`, advisory file locking, or filesystem-space query interface. | Add individual capabilities with backend support or explicit unsupported results; do not imply that adding a wrapper makes every backend support them. |
| VFS-06 | Design limit | Open supports access mode, append, create, truncate, exclusive, directory and CLOEXEC flags. General nonblocking/synchronized-write flags and full `fcntl` are absent; only SET/CUR/END seeks are exposed. | Extend flags and operations together with their actual semantics. Unknown flags are currently rejected. |
| VFS-07 | Design limit | CLOEXEC is stored but there is no POSIX process-image replacement/inheritance boundary. Returning sandbox ELF calls are not exec. | Consume descriptor flags only when a real exec/inheritance facility is implemented; this extends beyond FS drivers. |
| VFS-08 | Design limit | Lifetime pins do not serialize shared-handle cursor operations. In particular, `vfsIOSeek` uses separate set-position/get-position calls. Optional leaf mutexes do not make every compound operation atomic. | Define any stronger open-handle atomicity contract and corresponding leaf operations. Preserve the rule that upper metadata locks do not span driver calls. |
| VFS-09 | Design limit | Directory operations provide first/next iteration but no POSIX `telldir`/`seekdir` cookies. Sandbox getdents requires room for the largest configured record before advancing the iterator. | Add cursor/pending-entry support if smaller buffers or directory-position APIs are required. Current conservative capacity checks prevent entry loss. |
| VFS-10 | Design limit | FS objects/backends have caller-managed lifetimes. Native mount/unmount/format require no affected active operations or live nodes. No general busy-reference or hot-removal protocol exists. | Add explicit lifecycle tracking/admission rules if dynamic teardown is required. Node reference counting alone does not retain a filesystem object. |
| VFS-11 | Design limit | Paths, names, scratch-pool capacity, descriptor tables and mount-table capacity are configured bounds. With VFS mutual exclusion disabled, callers retain shared-state serialization responsibilities. | Keep limits/configuration requirements documented; enlarge or redesign only for a stated use case. Bounds and optional synchronization are not automatically POSIX defects. |

Sources: [FS interface](codegen/vfs_drivers.xml),
[node interface](codegen/vfs_nodes.xml),
[I/O context](codegen/vfs_io.xml), and
[synchronization contract](local_locking_audit.md).

### FatFS wrapper

| ID | Status | Current limitation | Possible work |
| --- | --- | --- | --- |
| FAT-01 | Needs regression | `f_write` may return `FR_OK` with zero bytes written when storage is full. The wrapper currently returns that zero for a nonzero request instead of reporting `ENOSPC`. | Reproduce a full-volume zero-progress write; preserve successful partial counts, but report no-progress space exhaustion accurately. |
| FAT-02 | Open | Public seek exposes the native behavior in I-FAT-04: read-only past-EOF seeks return `ENOTSUP`; writable seeks may grow the file immediately and do not zero the gap. | Consider a logical per-open offset, deferred extension and zero-filling on a later write. POSIX seek alone must not extend a regular file. |
| FAT-03 | Open | `FR_LOCKED` and `FR_DENIED` become `EACCES`; nonempty rmdir can therefore return `EACCES` instead of `ENOTEMPTY`. | Improve errno distinctions where native state permits a reliable check within the same leaf operation. Do not split validation and mutation across locks. |
| FAT-04 | Open | Path stat supplies modification time, but node stat does not. FAT wall-clock fields are interpreted as UTC; no timezone policy is configurable. | Improve handle metadata reporting and define a configurable time convention if required. Native timestamp precision remains limited. |
| FAT-05 | Design limit | One module mutex serializes the singleton and all native volumes, including storage waits. Separate wrapper objects are not independent exclusion domains. | Retain this model unless a native-global-state audit supports a finer design. Do not release the mutex while non-reentrant native state is active. |

Source: [FatFS wrapper](drivers/fatfs/drvfatfs_impl.inc).
The [native write contract](../../ext/fatfs/documents/doc/write.html) documents
short counts on a full volume. FAT-01 has not yet been reproduced by a targeted
runtime test; the earlier successful flag/append tests do not cover it.

### LittleFS wrapper

| ID | Status | Current limitation | Possible work |
| --- | --- | --- | --- |
| LFS-01 | Open | Unsupported conflicting independent writers are not rejected; native cache incoherence remains possible despite serialized calls. | Add admission checks or a coherent shared-file design, following I-LFS-01. Preserve valid independent read-only opens and duplication. |
| LFS-02 | Open | Native custom attributes are not exposed. VFS reports type, size and preferred block size, but not timestamps, ownership or allocated blocks. | Expose attributes or establish a metadata schema if needed; distinguish absent native information from fields merely omitted by an adapter. |
| LFS-03 | Design limit | No portable VFS guarantee is made for removal/rename of open objects. Native LittleFS behavior must not be generalized to FatFS or other drivers. | Specify and test any stronger backend-specific contract before advertising POSIX open-object lifetime semantics. |
| LFS-04 | Design limit | One mutex serializes each instance's native operations and storage waits. Native callbacks must not reenter VFS; direct native calls must not overlap wrapper operations. | Keep this contract explicit. Independent instances on disjoint storage may progress concurrently. |

Source: [LittleFS wrapper](drivers/littlefs/drvlittlefs_impl.inc).
Native rename replacement and positioning beyond EOF are supported; they are
not missing features. Shared sync/truncate omissions are tracked as VFS-02.

### ROMFS and streams

| ID | Driver | Status | Current limitation and possible work |
| --- | --- | --- | --- |
| ROM-01 | ROMFS | Open | Seeks beyond EOF return `ENOTSUP`. Read-only storage does not require this restriction: a logical offset could permit the seek and return EOF on subsequent reads. |
| ROM-02 | ROMFS | Design limit | No timestamps or stable inode/link identity are represented. Add optional descriptor metadata if needed; immutable storage cannot support runtime metadata mutation. |
| ROM-03 | ROMFS | Design limit | Dynamic/compressed callbacks synchronize their own shared state and keep borrowed descriptors/data alive. VFS does not serialize arbitrary callbacks; they must obey the scratch and reentry contract. |
| STM-01 | Streams | Open | Separate opens alias the registered backend. Random-stream cursors may therefore be shared across distinct VFS nodes. Independent regular-file offsets require per-open sessions or positional backend I/O. |
| STM-02 | Streams | Open | Random-stream seek returns a position without a separate error result. The wrapper detects clamping and attempts restoration, but cannot reliably distinguish every backend failure from a valid position. Fixing this requires an OOP stream-interface change. |
| STM-03 | Streams | Open | Regular random streams reject writable append/truncate requests; stat reports size zero. Add backend capabilities for size, resizing and append before exposing those semantics. Character-device append/truncate flags have no regular-file effect. |
| STM-04 | Streams | Design limit | The namespace is static. Missing-name creation returns `EROFS`, while unlink/rename/mkdir/rmdir inherit `ENOSYS`. Decide whether to retain this capability distinction or make immutable-namespace errors consistent; dynamic device creation is a separate feature. |
| STM-05 | Streams | Design limit | Sequential devices are not seekable. Input consumption, write/control ordering, terminal behavior and backend lifetime are shared-device responsibilities. General nonblocking/readiness APIs are not provided by this wrapper. |

Sources: [ROMFS wrapper](drivers/romfs/drvromfs_impl.inc),
[ROMFS callback contract](codegen/vfs_driver_romfs.xml), and
[streams wrapper](drivers/streams/drvstreams_impl.inc).

### Root, overlay and unfinished drivers

| ID | Driver | Status | Current limitation and possible work |
| --- | --- | --- | --- |
| ROOT-01 | Root | Design limit | `.` and `..` are normalized lexically, without validating every intervening component. Paths such as `missing/../file` can differ from POSIX traversal. Full semantics require component-wise lookup rather than just string normalization. |
| ROOT-02 | Root | Needs regression | CWD is a stored pathname, not a retained directory identity. Renaming the CWD or an ancestor does not update it, and getcwd returns the stored text. Add rename/removal tests and decide how CWD identity should survive namespace changes. |
| ROOT-03 | Root | Design limit | One CWD belongs to each root object; sharing a root shares CWD. Use separate roots for independent contexts, or redesign the association if per-I/O-context CWD is required. |
| OVL-01 | Overlay | Design limit | Provides mount routing and a merged listing, not copy-up/whiteout behavior. A writable union filesystem would be a separate feature. |
| OVL-02 | Overlay | Design limit | Enumeration is live: mount changes can skip or repeat entries. Snapshot enumeration is not promised. Each open directory still requires caller ordering. |
| OVL-03 | Root/overlay | Design limit | Mapping changes borrow filesystem pointers; unregistering a mapping is not unmount/disposal. Cross-driver rename returns `EXDEV`, which is expected behavior. Lifetime handling is tracked in VFS-10. |
| CHFS-01 | CHFS | Unimplemented | Cache/pool infrastructure and a design specification exist, but mount, format, lookup, I/O and namespace operations do not implement an on-media filesystem. Some stubs return success without doing the work. Implement the filesystem and make unsupported paths fail explicitly. |
| TMPL-01 | Template | Unimplemented | Driver-author scaffold only, not a usable filesystem or a supported concurrency implementation. |

Sources: [root](drivers/root/drvroot_impl.inc),
[overlay](codegen/vfs_driver_overlay.xml),
[CHFS implementation](drivers/chfs/drvchfs_impl.inc), and
[CHFS design](drivers/chfs/drvchfs_spec.md).
ROOT-02 is a source-review finding, not a newly passed runtime regression.

### Libc and sandbox adapters

These gaps must not be attributed to the leaf filesystem when the driver
already supplies the required operation or information.

| ID | Adapter | Status | Current limitation and follow-up |
| --- | --- | --- | --- |
| LIBC-01 | Host Newlib | Planned | The old adapter's lseek returns zero without seeking; isatty always returns true; fstat rejects directories and reports limited metadata. Replace its private table and these stubs in step 5 of the POSIX I/O plan. These are not the current sandbox implementations. |
| LIBC-02 | Sandbox and host Newlib | Open | Available VFS optional metadata is not propagated fully into POSIX stat. Sandbox stat/fstat currently export mode, size and a synthetic link count; directory records use a placeholder inode number. Define conversion/validity policy and meaningful identity where available. |
| LIBC-03 | Sandbox | Design limit | The shared directory ABI uses aligned records and a maximum-record minimum buffer size. Host and guest must be rebuilt together. Directory-position extensions belong with VFS-09. |

Sources: [host Newlib](../various/newlib_bindings/syscalls.c),
[sandbox adapter](../sb/host/sbposix.c), and
[guest directory streams](../sb/user/lib/libdir.c).

## POSIX reference points

- [lseek](https://pubs.opengroup.org/onlinepubs/9799919799/functions/lseek.html):
  positioning beyond EOF must not itself grow a regular file; a later write
  must leave the gap reading as zero.
- [rename](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html)
  and [unlink](https://pubs.opengroup.org/onlinepubs/9699919799/functions/unlink.html):
  destination replacement and the lifetime of open, unlinked objects.
- [write](https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html):
  partial transfers and errors when no data can be written.

The inventory is not an implementation schedule. Resolve entries by updating
their status, linking the change, and recording the checks that establish the
new behavior; keep remaining native restrictions explicit.
