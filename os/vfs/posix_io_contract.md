# VFS I/O supported behavior and regression contract

Step 1 of the [POSIX I/O plan](posix_io_plan.md), completed on 2026-09-19.
This specifies the target of steps 2 through 7; it does not claim that the
current prototype or adapters already implement every requirement below.

Steps 2 and 3 implement open routing, flag validation, descriptor flags,
shared access/append state, transfer/seek checks and native namespace type checks.
Generated tests cover real FatFS/LittleFS and ROM/stream backends, including
reserved descriptors and concurrent close/reuse. Adapter migrations remain
pending. See the plan for checks and ARM storage measurements.

## Boundary and ownership

`vfs_io_c` is the descriptor/path API. FS and node APIs remain available to
drivers and clients that need retained nodes. Each published descriptor owns
one node reference. An operation pins that reference before leaving the table
critical section and releases it after all driver work. Close detaches the
slot before releasing the reference. A root association is borrowed and stable
during use; sharing a root shares CWD and mounts.

Native errors remain encoded ChibiOS results inside VFS. Sandbox and Newlib
adapters return libc results and set errno on failure. Success need not clear
errno. Guest memory validation remains in the sandbox adapter. Kernel callers
must supply valid memory; a checked NULL argument returns `EINVAL`, while an
invalid guest memory range/string returns `EFAULT` at the syscall boundary.

No outer operation mutex is introduced. Preserve the local-locking rules,
including driver calls and disposal outside metadata locks/critical sections,
one scratch pair per operation, and caller serialization of compound operations
on the same open handle. Duplication protects lifetime, not concurrent cursor
transactions. With VFS mutexes disabled, callers retain the existing shared-
state serialization responsibility.

## Required sandbox workloads

| Caller | Required behavior |
| --- | --- |
| `user/lib/libdir.c`, `apps/sbsh/glob.c` | Read-only directory open with `O_DIRECTORY \| O_CLOEXEC`, batched enumeration and close |
| `apps/sbsh/main.c` | Read script with `O_RDONLY \| O_CLOEXEC`; descriptor survives a returning ELF call |
| `apps/sbsh/execute.c` redirection | Read input; create/truncate or append output; dup/dup2 save and restore standard descriptors |
| `apps/sbsh/execute.c` pipeline temporary files | `O_RDWR \| O_CREAT \| O_EXCL \| O_TRUNC \| O_CLOEXEC`; retry name on `EEXIST`; rewind; close before unlink |
| `apps/cp`, `apps/chedit` | Read source, create/truncate destination, handle short transfers/errors |
| Shell namespace commands | CWD, stat, mkdir/rmdir, unlink, rename and meaningful failure reporting |
| Registered stdin/stdout/stderr | Retained host nodes with defined access capabilities and terminal controls |

Paths above are relative to `os/sb`. All in-tree sandbox uses of open flags
were inspected. No additional flag is required by these callers.

## Open and flag contract

One `vfsIOOpen()` returns descriptors for either files or directories. Typed
opens remain below this API. The supported flag set is `O_ACCMODE`, `O_APPEND`,
`O_CREAT`, `O_TRUNC`, `O_EXCL`, `O_DIRECTORY` and `O_CLOEXEC`.

The following matrix assumes a writable filesystem, sufficient resources,
valid arguments, existing parent directories and no concurrent namespace change.
`R`, `W`, `RW` mean the three access modes; `C`, `X`, `T`, `A`, `D` mean create,
exclusive, truncate, append and directory. Success starts at offset zero;
append affects writes, not the initial read position.

| Flags | Existing regular file | Missing final component | Existing directory |
| --- | --- | --- | --- |
| R | Open | `ENOENT` | Open directory |
| W or RW | Open, preserve data | `ENOENT` | `EISDIR` |
| R + D | `ENOTDIR` | `ENOENT` | Open directory |
| W/RW + D | `ENOTDIR` | `ENOENT` | `EISDIR` |
| R/W/RW + C | Open, preserve data | Create | `EISDIR` |
| R/W/RW + C + X | `EEXIST` | Create exclusively | `EEXIST` |
| W/RW + T, optionally C | Truncate | `ENOENT`, or create with C | `EISDIR` |
| W/RW + A, optionally C | Open for append | `ENOENT`, or create with C | `EISDIR` |
| RW + C + X + T + CLOEXEC | `EEXIST` | Create, record descriptor flag | `EEXIST` |

These core choices follow [POSIX open](https://pubs.opengroup.org/onlinepubs/9799919799/functions/open.html).
Local policy adds the following precise validation rules:

- Invalid access encodings, unknown flag bits, X without C, T with R, and
  D combined with C or T return `EINVAL` before delegation. Some combinations
  have unspecified/undefined POSIX behavior; this is our chosen policy.
- R + A is accepted; writes still fail the access check. A and T together on
  W/RW truncate once at open and append on subsequent writes.
- CLOEXEC belongs to the descriptor; D belongs to routing. Neither is passed
  to native file-open flag translation. Access and append state belong to the
  shared open handle, not stat permission bits.
- Read-only creation is supported on writable regular-file backends. A native
  library's need for temporary write access during creation must not grant
  write capability to the returned descriptor.
- Validate arguments/flags, then reserve the lowest available descriptor before
  driver work. A full table returns `EMFILE` without create/truncate or a scratch
  wait. Driver failure cancels the reservation. Local allocation failures use
  `ENOMEM`; a native open-object limit can use `ENFILE`.
- Native exclusive create must decide existence and creation together. A stat
  followed by an independently locked create is not sufficient.

When several independent failures coexist, there is no general promise of
which error wins. Tests isolate each condition except the specified validation
and reservation ordering. Native storage failure after mutation is not a
transactional rollback guarantee; prevent avoidable local failures after mutation.

## Paths and namespace operations

| Input or operation | Required result or rule |
| --- | --- |
| Empty path | `ENOENT`; never interpret it as CWD |
| Relative path | Resolve from one CWD snapshot belonging to the associated root |
| Absolute path | Resolve from the logical root; backing prefix is not exposed |
| `/`, repeated separators, `.` and `..` | Preserve existing VFS normalization and confinement at logical root |
| `file/`, including a create request | `ENOTDIR` for an existing regular file; never truncate it |
| `missing/` passed to open, including C | `ENOENT`; never create a regular file |
| `dir/` | Same directory requirement as D; flags must still be validated |
| Path/component too long | `ENAMETOOLONG`, including insufficient space after adding the backing prefix |
| Missing ordinary parent component | `ENOENT`; existing regular-file parent gives `ENOTDIR` |
| chdir to file/missing path | `ENOTDIR`/`ENOENT`; retain previous CWD |
| getcwd with insufficient output space | `ERANGE`; do not expose a truncated path as success |
| unlink of directory | `EISDIR` (chosen VFS convention); leave it intact |
| rmdir of regular file | `ENOTDIR`; leave it intact |
| rmdir of nonempty directory | Fail without removing contents; see FatFS error limitation below |
| mkdir of existing entry | `EEXIST` |
| rename across different backing FS objects | `EXDEV`, without modifying either side |
| rename with identical resolved source/destination | Success if the source exists; no removal |

Keep the trailing-directory requirement until leaf resolution. Mutation type
checks must be performed together with mutation under the leaf lock; an upper
stat followed by a separately locked remove is insufficient. Both FatFS and
LittleFS use a native combined file/directory removal primitive (FatFS's
`f_rmdir` aliases `f_unlink`). Their wrappers now check type and perform removal
under the same native-operation lock.

VFS currently normalizes dot components lexically. This work does not promise
POSIX component-by-component checks for paths such as `missing/../file` or
`file/../other`, nor symlink resolution. Record this as a path-model limitation;
do not use those paths to assert full POSIX traversal compliance. Backend name
rules, including FatFS case behavior, also remain backend-specific.

The operation separation follows [unlink](https://pubs.opengroup.org/onlinepubs/9699919799/functions/unlink.html)
and [rmdir](https://pubs.opengroup.org/onlinepubs/009696799/functions/rmdir.html);
the EISDIR choice is a VFS convention rather than a claim of exact POSIX errno
compliance. Cross-filesystem behavior follows [rename](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html).

## Descriptors, data and directory streams

| Operation/condition | Contract |
| --- | --- |
| Invalid, closed or pending descriptor | `EBADF` for ordinary descriptor operations; a pending slot is not published |
| dup | Lowest free descriptor, same open handle/cursor/status, independent descriptor flags |
| dup2 | Validate source before replacing destination; same-fd succeeds unchanged; release displaced reference outside critical section |
| CLOEXEC on dup/dup2 | Clear on a newly created duplicate; preserve on same-fd dup2 |
| Reserved destination for install/dup2 | `EBUSY`, an explicit local concurrency extension |
| Read of W handle / write of R handle | `EBADF`, regardless of filesystem permission bits |
| Byte read/write of directory | `EISDIR`, including zero-length requests (local policy) |
| Zero-byte I/O on a valid, suitably opened file | Zero without entering the driver; NULL buffer allowed |
| Short transfer / EOF | Return actual byte count / zero; do not turn a partial result into a full-count success |
| Count beyond signed return range | `EINVAL` before narrowing/delegation; adapters also reject negative signed counts |
| Seek on regular file | Return resulting offset; invalid origin/negative result gives `EINVAL`; unrepresentable result gives `EOVERFLOW` |
| Seek on directory / non-seekable stream | `EISDIR` / `ESPIPE`; directory enumeration uses its own cursor interface |
| Seek beyond EOF | Backend limitation unless verified; no silent success with a different requested position |
| stat/fstat | Work for files and directories; initialize all output fields, report type and available size accurately |
| isatty/control | Consult terminal capability; a character device is not necessarily a terminal |
| Directory enumeration on file | `ENOTDIR` |
| Different directory opens | Independent cursors; duplicates share a cursor; enumeration is not a namespace snapshot |
| fdopendir | Validate fd/type, take ownership only on success; failure leaves caller's fd open |
| opendir allocation failure | Close its internally opened fd, preserve the original allocation error |
| closedir | Release stream storage and its descriptor exactly once |
| readdir EOF | NULL without setting a new errno; errors return NULL with errno set |

For fdopendir, all admitted directory handles must be readable; the initial
subset has no write-only/O_PATH directory handles and does not require a new
fcntl syscall just to validate this property. Query type without advancing the
cursor. The [GNU directory-stream contract](https://sourceware.org/glibc/manual/2.25/html_node/Opening-a-Directory.html)
describes the ownership behavior.

Sandbox getdents is a project ABI, not a POSIX function. Pin the same directory
for the full batch, including scratch allocation. Use an aligned record stride,
bounded NUL-terminated names and initialized padding; copy safely to unaligned
guest memory. Reject a buffer smaller than the maximum supported record before
advancing the iterator. This conservative minimum avoids adding a pending-entry
allocation. Verify `DIR_BUF_SIZE` against the configured maximum. Short batches
are valid; the next call must not lose or repeat an entry.

Duplication follows [POSIX dup](https://pubs.opengroup.org/onlinepubs/9799919799/functions/dup.html).
Access/transfer rules use [read](https://pubs.opengroup.org/onlinepubs/9799919799/functions/read.html)
and [write](https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html).
Append must locate EOF at each write under the leaf operation lock, even after
a seek. Do not inherit FatFS's initial seek-to-end as the public read offset.
ROMFS and read-only FatFS reject past-EOF seeks with `ENOTSUP`. Writable
FatFS may extend the file during seek and reports `ENOSPC` if allocation clamps
the result. LittleFS permits past-EOF positions; its wrapper explicitly returns
to EOF before append writes, even from a later position. Random-stream clamping
returns `ENOTSUP`; restoring the previous position depends on the backend.
Seek beyond EOF and concurrent shared-handle atomicity remain limits relative
to [POSIX lseek](https://pubs.opengroup.org/onlinepubs/9799919799/functions/lseek.html).

## Explicit backend decisions

| Area | Decision for this migration |
| --- | --- |
| Creation mode/umask | Keep existing embedded behavior: open's mode is ignored; no Unix user/group permission enforcement or umask is claimed. No syscall ABI extension for mode in this migration. Document this for callers passing 0600/0644/0666. |
| Read-only regular-file backing | Reads and directory operations succeed. Existing R+C behaves as R; existing C+X gives EEXIST. Creation of a missing file or an actual write/truncate request gives EROFS. R+A does not request a write. |
| Static streams namespace | Existing named streams can be opened with supported access; C does not create a missing name (EROFS). C+X on an existing name gives EEXIST. Character-stream A/T have no file-position/content-truncation effect. Regular random streams lacking append/truncate support reject those requests with ENOTSUP. |
| Stream access | Enforce requested access against registered capabilities. Backend stream objects can still alias across opens; independent offsets and serialization depend on that backend. |
| Rename replacement | Use native LittleFS replacement where supported. FatFS keeps EEXIST for an existing different destination. Do not emulate replacement with unlink-then-rename, which can lose the destination on failure. Shell mv reports that failure. |
| Open-file unlink/rename | Not portable in this subset. Callers close all references first, as the shell's pipeline cleanup already does. LittleFS-specific behavior is not a common guarantee. |
| FatFS simultaneous native opens | Multiple read-only opens are allowed; any conflicting writable open is unsupported. dup shares one FIL and is allowed. A wrapper mutex does not make multiple FIL caches coherent. |
| LittleFS simultaneous native opens | Conflicting independent writable opens of one file are unsupported: each native handle caches its own file size and data state. A native probe of two append handles produced `seedB` after A then B, losing A. Use duplicates of one handle or close/reopen with external serialization. No wrapper registry rejects this misuse. |
| FatFS misuse rejection | Writable sandbox deployments must enable FF_FS_LOCK with capacity for their simultaneously open native files/directories, so conflicting opens/remove/rename fail instead of corrupting data. This is independent of FF_FS_REENTRANT and VFS mutexes. Current SB demos set it to zero and need configuration updates in step 4. General non-sandbox use may retain zero under the documented caller restrictions. |
| FatFS error detail | Keep EACCES for FR_LOCKED/FR_DENIED unless a precise native distinction is available. In particular, nonempty rmdir may return EACCES instead of ENOTEMPTY. Native capacity exhaustion is ENFILE. |
| Final close errors | The current void reference-disposal interface cannot propagate native close/flush errors, especially when a pin delays final disposal. Do not claim successful descriptor close confirms durable storage. A status-returning flush/close design is separate work. |
| Metadata | File type/size and initialized fields are required. Permission bits, inode identity, timestamps and link counts are limited to available native information; no fabricated POSIX permission enforcement. |
| CLOEXEC | Store the flag, but current returning ELF calls and host sandbox startup are not POSIX exec. Do not close shell-owned descriptors there. Actual exec/inheritance is outside this migration. |
| Unsupported operations | No new fork/exec, openat, full fcntl, symlink, directory seek cookie or filesystem permission layer. Unknown open flags fail validation rather than being silently masked. |

FatFS restrictions are documented in its [application note](https://elm-chan.org/fsw/ff/doc/appnote.html#dup)
and [rename API](https://elm-chan.org/fsw/ff/doc/rename.html). The vendored copies
under `ext/fatfs/documents/doc` confirm the same restrictions. LittleFS's native
remove/rename contract is in `ext/littlefs/lfs.h`; append/cache behavior was
checked against `ext/littlefs/lfs.c` and a RAM-backed native probe. No external dependency is
modified by this step.

## Flag ABI verification

Preprocessed `<fcntl.h>` using the installed ARM GNU 14.3.1 compiler in host
and sandbox include contexts, and using GCC for the Linux simulator. All ten
ARM definitions match between host and guest. Representative differences:

| Flag | ARM Newlib | Linux simulator |
| --- | --- | --- |
| O_ACCMODE | 0x3 | 0x3 |
| O_APPEND | 0x8 | 0x400 |
| O_CREAT | 0x200 | 0x40 |
| O_TRUNC | 0x400 | 0x200 |
| O_EXCL | 0x800 | 0x80 |
| O_DIRECTORY | 0x200000 | 0x10000 |
| O_CLOEXEC | 0x40000 | 0x80000 |

R/W/RW are 0/1/2 in both environments. These values are audit observations,
not constants to copy into VFS. Define VO aliases from the build's libc headers.
The current SB ABI passes flags through unchanged; host and guest must be
built with compatible headers. Linux tests cannot validate ARM numeric values.
Repeat the probe when changing toolchains or feature-test macro settings.

## Focused regression cases and implementation ownership

Each case below defines a fixture and an observable result. Cases marked
specified are acceptance cases for their owning step, not newly passing tests.
Existing coverage refers to the current prototype's generated suites 010/011
in `test/vfs/configuration.xml`; it is not native-filesystem conformance evidence.

| ID | Fixture, action and assertions | Status / owning step |
| --- | --- | --- |
| ABI-01 | Preprocess the ten open macros in ARM host/guest contexts; compare expansions; contrast simulator values | Verified in step 1 |
| OPEN-01 | Existing file, directory and missing name; run every open-matrix row; assert result, returned node type and unchanged data on rejection | Specified / 2 |
| OPEN-02 | Invalid/unknown bits and local-policy combinations, including D+C and R+T; assert no native open/create/truncate and no leaked slot | Specified / 2 |
| OPEN-03 | Fill the table; attempt C/T open; assert EMFILE and no driver invocation; free a slot and assert reuse | Existing 011; extend to real files / 2 |
| OPEN-04 | Race two C+X creates for one absent name; exactly one succeeds, one gets EEXIST, no truncation; repeat with shell temporary-file flags | Specified / 2 |
| PATH-01 | Empty, relative, prefixed, root, trailing-slash and overlong paths; verify matrix errors and no create/truncate through a trailing slash | Specified / 2 |
| PATH-02 | Failed chdir preserves CWD; shared roots share CWD, separate roots do not; insufficient getcwd space returns ERANGE | Root sharing existing 011; remaining cases specified / 2 |
| NODE-01 | Read W, write R, zero-byte transfers, short transfer, EOF, excessive count; assert errno/byte count and no incompatible driver call | Implemented / 3; direct nodes and I/O context |
| NODE-02 | Open RW+A on nonempty file; initial read starts at zero; seek to zero, write, then write again; both writes extend file; repeat through dup | Implemented / 3, real FatFS/LittleFS, including dup |
| NODE-03 | SET/CUR/END seeks, invalid origin, negative/overflow offset and character stream; assert result and cursor; reject unsupported beyond-EOF behavior explicitly | Implemented / 3; native, ROM and stream boundaries |
| FD-01 | Dup shares cursor; new duplicate clears CLOEXEC, same-fd dup2 preserves it; invalid source leaves destination and flags intact | Implemented / 3, ownership 010 and native shared cursor |
| FD-02 | Suspend every node operation, close/reuse fd, resume; operation stays on pinned old node; repeat driver errors and reference overflow | Existing 010/011; retain through migrations |
| NS-01 | unlink empty directory, rmdir regular file, rmdir nonempty directory; assert failure and unchanged namespace; remove correct types successfully | Implemented / 3, real FatFS/LittleFS |
| NS-02 | Rename to new name, itself, existing destination and another FS; assert documented backend result and preservation of destination on failure | Implemented / 3, native rename and root EXDEV |
| RO-01 | ROM/read-only backing: R, R+A, existing R+C, existing C+X, missing C and write/truncate; assert documented results without mutation | Specified / 2 |
| STREAM-01 | Registered read/write capabilities, D on device, missing C, existing C+X, A/T on character versus random stream, isatty on non-TTY character device | Specified / 2-3 |
| DIR-01 | fdopendir invalid fd/regular file/allocation failure; original fd remains owned; opendir allocation failure releases its internal fd; closedir closes once | Specified / 4 |
| DIR-02 | Mixed name lengths, unaligned output, maximum-length record, too-small buffer, EOF/error and multiple batches; rejected buffer consumes nothing; no uninitialized padding | Specified / 4 |
| SB-01 | Guest invalid string/range, signed length conversion, fstat of directory and terminal control; verify libc result/errno and memory validation | Specified / 4 |
| SB-02 | Shell input/output/append redirection, glob, script calling ELF and spooled pipeline; stdio restored, temporary files closed/removed, caller descriptors survive returning ELF call | Specified / 4 |
| FAT-01 | FF_FS_LOCK enabled: distinct read-only opens work, conflicting write/remove/rename reject; dup remains usable; native lock-table exhaustion reports ENFILE | Specified / 4 |
| LIBC-01 | Newlib seek/tell, file/directory fstat, TTY/non-TTY, buffered stdio and errno translation through configured context | Specified / 5 |

When implementing, put generated regression cases in the test XML and validate
the schema before regeneration. Use real native backends for their flag and
namespace behavior, and guest libc for directory-stream ownership. This step
performed source and ABI checks only; no runtime behavior was changed or newly
claimed as passing.
