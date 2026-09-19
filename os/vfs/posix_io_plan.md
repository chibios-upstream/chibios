# VFS I/O API and sandbox POSIX behavior plan

Created on 2026-09-19. Status: steps 1 through 4 complete; step 5 is next.

Make `vfs_io_c` the main application interface for paths and descriptors,
shared by Newlib bindings and sandboxes. Target the POSIX behavior needed by
existing sandbox applications, with explicit limits for embedded backends.
Build on the I/O class prototype committed at `8265033984` and the completed
[local locking plan](local_locking_plan.md).

## Architecture and scope

- Keep one `vfsIOOpen()` returning an integer descriptor for files or
  directories. `opendir()` and `fdopendir()` remain libc directory-stream
  functions. Keep typed FS/node operations for drivers and specialized users.
- An I/O object has caller-provided descriptor storage and a borrowed root
  pointer. The root and backing drivers outlive all operations and nodes.
  Sharing a root shares CWD; give sandboxes separate roots when they need
  independent CWD state.
- Keep descriptor publication, reference acquisition and detachment inside
  short critical sections. Call drivers, wait for scratch and release references
  outside them. Preserve local leaf locking and optional mutex storage.
- Reserve a descriptor before a potentially creating/truncating open. Retain
  the node for the whole operation, including waits and multi-call sequences.
  Existing shared-handle serialization and filesystem lifetime rules still apply.
- Retain `vfsInit()` and `vfs.h` as initialization and umbrella/configuration
  interfaces. Remove operation wrappers and the global `vfs_root` after migrating
  their users. Newlib gets an explicitly configured default I/O context because
  its syscall signatures do not carry one; each sandbox embeds its own context.
- Keep encoded ChibiOS errors internally; libc adapters translate them into
  the appropriate return value and `errno`.

## Findings to address

These were the initial audit findings. Steps 2 through 4 now address the VFS
and sandbox issues; Newlib migration remains step 5. The sandbox uses the shared
context, while Newlib still has its own protected descriptor table.

Sandbox `libdir.c` and shell globbing already pass `O_DIRECTORY | O_CLOEXEC`.
Current VFS flag definitions omit both. FatFS and LittleFS flag translators
reject some useful combinations; streams ignore flags. FatFS uses
`FA_OPEN_APPEND` when opening, but its write wrapper just calls `f_write()`.
Audit this against append-after-seek and repeated-write requirements.

`fdopendir()` currently closes the supplied descriptor on allocation failure
and does not check its type. Sandbox directory records need a shared alignment
and bounds contract. A too-small first getdents buffer can consume an entry
before returning an error. Newlib currently stubs seek/isatty and rejects
directories in fstat.

There is no sandbox POSIX exec syscall. Host `sbExecStatic`/`sbExecDynamic`
start a stopped sandbox with explicitly registered descriptors. Guest
`sbRunElfAt()` calls an ELF program and returns to the caller. Neither should
automatically close the caller's descriptors as if replacing its process image.

## Implementation sequence

### 1. Define and test the supported contract - complete

The [supported behavior and regression contract](posix_io_contract.md) records
the open/path/error matrices, backend decisions and focused acceptance cases.
The ARM host/guest flag ABI was checked against the installed toolchain, with
Linux simulator differences recorded. Runtime cases are assigned to the steps
that implement their behavior; this completion covers specification and audit.

- Record expected results for existing/missing files, directories, read-only
  mounts and streams across access/create/exclusive/truncate/append flags.
  Include descriptor exhaustion, invalid descriptors and unsupported flags.
- Use the target C library's flag values; verify host/sandbox agreement rather
  than importing Linux constants into the ARM ABI.
- Specify empty paths, relative paths, prefixes and trailing slashes before
  normalization erases distinctions. In particular, a trailing slash must
  preserve the requirement that the target be a directory.
- Separate required behavior from backend limits: creation permissions/umask,
  rename replacement, unlinking open files, multiple writable opens and reporting
  errors from final native close. Current open adapters discard the creation
  mode, and reference disposal cannot return a close error. Do not imply these
  features become POSIX-compliant through table consolidation.
- Keep the initial scope to existing sandbox operations. Full fcntl, openat,
  fork/exec, symlinks and Unix permission enforcement are separate work.

Deliverable: a supported-behavior/error matrix and focused regression cases,
plus explicit decisions for any backend limitation exercised by current apps.

### 2. Implement one open operation with correct type routing - complete

Common validation now precedes descriptor reservation and root scratch waits.
Directory flags and trailing separators select directory lookup before file
mutation. Root stat also preserves the trailing-directory requirement, and
empty root paths fail instead of resolving to CWD. Flags are checked in the
enabled leaf wrappers, with corrected FatFS/LittleFS combinations, read-only
ROMFS handling and stream capability checks.

FatFS per-handle append positioning/write was brought forward from step 3 so
accepting append without create cannot produce incorrect writes. Its initial
read offset remains zero. Step 3 consolidates this state with shared open flags and adds descriptor
flag storage. At the step-2 commit, CLOEXEC was accepted and stripped at routing
boundaries without being recorded in the table.

Validation: 19 flag combinations against existing files, missing names and
directories on each real FatFS/LittleFS backend; concurrent exclusive creators;
append after seeks; native trailing-slash mutation rejection; 31 root/ROMFS
routing cases; stream flags/capabilities; reservation validation and cleanup.
Simulator suites passed with leaf mutexes/debug checks enabled, with kernel
mutexes/condition variables disabled, and with root support disabled. The
STM32G474 switched sandbox and STM32L4R9 dynamic FatFS sandbox demos compiled
and linked without warnings. These ARM checks are builds, not hardware runs.
XML schema, repeat-generation and changed-line style checks passed.

- Add `VO_DIRECTORY`; handle it at the common routing layer, before any file
  creation or truncation. Strip flags that belong to the descriptor layer before
  passing flags to native filesystems.
- Read-only open can return a file or directory. Requiring a directory on a
  regular file returns `ENOTDIR`; requesting write access on a directory returns
  `EISDIR`. Do not let the current file-first fallback bypass flag validation.
- Define explicit policy for unspecified flag combinations, such as
  `O_DIRECTORY | O_CREAT`, and reject unsupported semantics consistently.
- Correct native flag translation, including plain write-only opens and append
  without create where supported. Keep exclusive-create checks inside the
  native operation; a preliminary stat does not make creation atomic.
- Preserve reservation cancellation and reference cleanup. Validate flags and
  table capacity before side effects; audit post-open failure paths, including
  admission of custom node implementations.
- Apply the contract's read-only and stream capability rules. Preserve empty-
  path and trailing-directory distinctions through normalization.

Deliverable: tested routing and flag behavior through root, overlay and enabled
leaves, without introducing separate descriptor-returning file/directory opens.
The baseline follows [POSIX open](https://pubs.opengroup.org/onlinepubs/9799919799/functions/open.html).

### 3. Complete descriptor and open-handle semantics - complete

Descriptor storage is now `vfs_descriptor_t` (owned node and independent flags).
Open records CLOEXEC atomically; flag get/set, dup, dup2, close and reuse preserve
the documented ownership rules. Explicit constructor flags initialize shared
access/append state for every leaf and host-created file node. Common read/write
checks also cover direct node callers and reject incompatible zero-byte calls.
FatFS's temporary append field has been removed.

Both writable native wrappers position append writes under their own lock.
LittleFS needs explicit positioning as well: native append alone preserves a
cursor beyond EOF. A native two-handle probe produced `seedB`, not `seedAB`,
when separately opened append handles wrote A then B; independent conflicting
writable handles remain unsupported on both backends. Duplicates share a single
native handle and are tested. This step adds no registry or upper lock.

Checked seek arithmetic rejects negative/overflowing targets before mutation.
ROMFS and read-only FatFS reject past-EOF seeks with ENOTSUP. Writable FatFS may
extend at seek time; LittleFS supports past-EOF positions. Random streams are
checked for clamping and report failure instead of a false success. Native
count conversions are bounded. Unlink/rmdir type checks and mutation share one
leaf lock, FatFS identical-name rename verifies existence, and LittleFS reports
its native nonempty-directory and transfer errors through the proper VFS codes.

ARM storage measurements: context remains 16 bytes with root support or 12
without, entries grow from 4 to 8 bytes, and the common file node grows from
16 to 20 bytes. FatFS replaces its private append storage. No new heap objects
or mutexes are introduced.

Validation covers descriptor flags and reuse, reserved-slot invisibility,
wrong access including read-only creation, zero/short/EOF transfers, excessive
counts, signed seek boundaries and stream clamping, duplicated append (including
LittleFS past-EOF seeks), typed removals and rename behavior on real FatFS and
LittleFS. Simulator variants include native wrappers with mutexes/debug checks,
no kernel mutexes/condition variables, and no root support. STM32G474 switched
and STM32L4R9 dynamic FatFS sandbox demos build and link; these are build checks,
not hardware runs. XML schemas, deterministic regeneration, changed-line style
and the OOP_USE_NOTHING compile check pass.

- Replace pointer-only slots with a small descriptor-entry structure containing
  the node reference and descriptor flags. Record `O_CLOEXEC` atomically when
  publishing an open descriptor. Measure the ARM storage cost before finalizing
  the layout; keep storage caller-owned and avoid a separate heap object.
- Store access mode and required status flags with the shared open handle,
  separate from stat permission bits. Initialize them consistently in all leaf
  constructors, including nodes installed directly by the host. Check read/write
  access and return `EBADF` for an incompatible access mode.
- Preserve shared node/cursor ownership on dup. A new duplicate clears
  close-on-exec; dup2 of a descriptor onto itself preserves it. Invalid-source
  dup2 leaves the destination intact. Document the existing `EBUSY` result for
  a destination reserved by an in-flight open as a local concurrency extension.
  See [POSIX dup](https://pubs.opengroup.org/onlinepubs/9799919799/functions/dup.html).
- Implement append at each write within the leaf's native-operation lock,
  including after seeks. Verify native support for distinct handles to the same
  file; a wrapper mutex alone does not fix stale per-handle native metadata.
  Document/reject unsupported backend cases rather than claim atomic append
  there. See [POSIX write](https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html).
  FatFS append/write and native append regression coverage are already present
  from step 2; consolidate its append field when adding common open-handle flags.
- Preserve short transfers, EOF, accurate seek results and non-seekable errors;
  validate count/offset conversions. Keep same-handle compound-operation limits
  explicit instead of adding an upper mutex around driver calls.
- Enforce unlink/rmdir type restrictions under the leaf mutation lock and verify
  rename behavior against the backend decisions in the contract. Native combined
  removal APIs must not erase the distinction between files and directories.
- Retain close-on-exec metadata, but document that current sandbox ELF calls do
  not implement exec. A future process-image replacement/inheritance operation
  must consume that metadata at its actual boundary. Do not close shell script,
  glob or redirection descriptors on a returning ELF call. POSIX associates this
  behavior with [exec](https://pubs.opengroup.org/onlinepubs/9799919799/functions/exec.html).

Deliverable: ownership/duplication/flag tests, wrong-access tests, append-after-
seek tests, and a documented memory and concurrency contract.

### 4. Migrate sandboxes and repair directory-stream behavior - complete

Each sandbox embeds the shared context and descriptor entries. Initialization,
registration, syscalls and cleanup use that context; stopped-state lifecycle
rules and borrowed root lifetime remain in force. Registration now reports
errors and consumes the supplied reference only on success. Guest range/string
checks remain in the adapter; stat output supports unaligned guest buffers.

Getdents pins one directory before waiting for scratch and keeps it for the
whole batch. Records preserve field offsets, use an inode-aligned stride and
zero padding, and can be copied to unaligned output. A first buffer smaller
than the maximum configured record is rejected before cursor advancement.
Guest directory streams validate records, preserve fd ownership on failure,
and distinguish EOF from errors. Shell globbing reads headers by byte copy.

Terminal detection uses the existing VFS terminal control operation through
new sandbox operation 19; operations 1 through 18 retain their encodings.
Host and guest must be rebuilt together for the updated shared directory ABI
and terminal syscall. ARM Newlib measurements: inode size 2, name offset 5,
maximum record 38 bytes with NAMELEN_MAX=31, and DIR size 524. Sandbox I/O storage
is 16 + 8*N bytes instead of 4 + 4*N: an increase of 60 bytes for 12 descriptors.

Both writable L4R9 sandbox demos enable FF_FS_LOCK=16 (12 guest descriptors
plus four host/loader objects). Their VFS name limit is now 31, regenerated with
the updater: the shell's temporary filename needs 19 characters, exceeding the
previous limit of 15. Native FatFS reentrancy remains independent.

Validation compiles production host dispatch/range checks and guest libc,
parser, glob and execution code into the simulator, replacing the ARM trap and
program entry with a host fixture. Tests cover ownership/allocation failures,
multiple directory batches, padding/alignment, partial errors/EOF, scratch waits
with close/reuse, invalid guest memory/counts, full-table mutation rejection,
terminal capability, globbing, input/output/append redirection, spooled pipeline
cleanup, and CLOEXEC survival across a returning program call. The shell has no
here-document syntax; its temporary-document creation/rewind/cleanup primitive
is covered. FatFS tests additionally check native conflicts and lock exhaustion.

FatFS and LittleFS suites pass with mutexes/debug checks enabled and with kernel
mutexes/condition variables disabled. Root-disabled builds pass. STM32G474
switched and both STM32L4R9 FatFS sandbox hosts, plus the ARM sbsh guest, compile
and link. XML schema, deterministic regeneration and changed/new-source style
checks pass.

Hardware follow-up: the STM32G474RE multi-target sandbox demo was rebuilt with
fresh guest images and run through OpenOCD on port 3333 and its 38400-baud ST-LINK
console. A temporary guest probe passed 207 checks with VFS mutexes both disabled
and enabled, with kernel assertions/checks/state checking active. It covered
TTY/non-TTY capability, unaligned stat/getdents output, record padding and bounds,
80-entry enumeration across batches, EOF, fdopendir errors, independent directory
cursors, dup/shared offsets, signed seek, access checks, descriptor exhaustion
and reuse, and invalid/out-of-range guest pointers through actual SVC dispatch.
A ROMFS script continued after real ELF calls, retaining its CLOEXEC descriptor.
A deliberate write to protected host RAM terminated only the sandbox with
status 000F0004; the host restarted it and the probe passed again. Normal exit
and restart also passed. Interactive listing, globbing and /dev/null redirection
passed. The normal ROMFS was regenerated twice identically, removing temporary
fixtures, and the standard demo restored on the board. Native writable-file and
spooled-pipeline tests remain simulator coverage: this G4 demo has no writable
filesystem. The checked-in multi-demo ROMFS now contains rebuilt guest binaries.

- Embed `vfs_io_c` plus fixed-capacity entries in sandbox I/O state. Delegate
  registration, lookup, open/close/dup, path operations and cleanup to it.
  Preserve stopped-state registration, startup-failure cleanup and root lifetime.
- Keep guest address/range/string validation and libc structure conversion in
  the sandbox adapter. Preserve syscall numbers and encodings; coordinate any
  necessary ABI extension with guest headers and bindings.
- For getdents, pin one directory node across the scratch wait and entire batch.
  Define aligned record lengths, bounds, terminators and initialized padding in
  the shared ABI. Support unaligned guest buffers without unaligned typed stores.
  Check output capacity before consuming an entry; define the minimum supported
  buffer size if the iterator cannot retain a pending entry. Return partial
  batches and EOF consistently.
- Validate fdopendir's descriptor/type before success. Transfer ownership only
  on success; preserve the descriptor on failure. Have opendir close its own
  descriptor on stream-creation failure while preserving errno. Initialize all
  stream fields and make closedir release exactly once. This follows the
  [directory-stream ownership contract](https://sourceware.org/glibc/manual/2.25/html_node/Opening-a-Directory.html).
- Test through guest libc: directory open/read/close, invalid and regular-file
  descriptors, allocation failure, EOF/error distinction, small buffers, shell
  globbing, redirection and here-document creation. Audit isatty through the
  terminal control operation rather than equating every character device to a TTY.
- Enable and size FatFS native open-object checks (FF_FS_LOCK) in writable
  sandbox configurations, independently of native reentrancy, and test rejection
  of conflicting open/remove/rename operations. No VFS lock registry is added.

Deliverable: sandbox descriptor-table duplication removed and the actual guest
directory/descriptor workflows passing through the shared class.

### 5. Migrate Newlib to a configured I/O context

- Replace its private descriptor table with the shared class. Define explicit
  context binding and initialization before stdio use; retain existing capacity
  configuration and make standard-descriptor installation unambiguous.
- Delegate open/close/read/write/seek/stat and path operations, preserving libc
  errno conversion. Implement real lseek/fstat/isatty behavior, including
  directories and terminal checks. Audit integer widths against target Newlib.
- Test buffered stdio and direct descriptor use, invalid descriptors, short I/O,
  full-table opens, initialization and the build without VFS bindings.

Deliverable: one descriptor-table implementation used by both adapters.

### 6. Migrate remaining callers and remove the old application API

- Give shell/xshell an I/O context; use integer descriptors rather than casting
  node pointers to descriptors. Account for recursive directory traversal when
  sizing their table and handle exhaustion correctly.
- Update demos to initialize and bind contexts. Keep ELF loading and HTTP
  bindings on direct FS/node methods where their retained-node model is useful;
  replace their calls to redundant wrappers with those methods.
- Migrate tests and documentation, then remove the 19 operation wrappers in
  `vfs.c/h` and the exported default root. Keep initialization, configuration,
  umbrella includes and low-level class interfaces.

Deliverable: repository-wide call-site checks find no remaining removed API
users, and public examples show the new I/O context model.

### 7. Validate the integrated behavior

- Run focused descriptor and POSIX-behavior tests with mutexes enabled and
  disabled, debug/state checks enabled, and descriptor-only root-disabled builds.
  Preserve the OOP_USE_NOTHING exclusion and one-pair scratch exhaustion tests.
- Exercise real FatFS/LittleFS configurations and streams/ROMFS where relevant;
  mocks alone do not verify native flag translation or append behavior.
- Build affected sandbox applications, host demos, Newlib, shells and HTTP
  bindings. Run guest libc/application scenarios on a suitable target or harness;
  report compile-only coverage separately when runtime access is unavailable.
- Validate edited XML against its schema, regenerate code/tests, and confirm a
  second generation is unchanged. Update configuration templates/updaters only
  if configuration changes; run updaters sequentially. Check changed-line style
  and whitespace, then clean build products.

Deliverable: recorded results and remaining backend limitations. Each preceding
step should include its focused tests and remain independently buildable; this
last step verifies integration rather than postponing all testing until the end.
