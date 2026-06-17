# Note: SB Isolation / Escape Resistance

Assessment of whether a thread running inside a sandbox can escape the
isolation, i.e. trigger arbitrary privileged code execution or crash the
supervisor. Covers the ALT ports (`os/common/ports/ARMv7-M-ALT`,
`ARMv8-M-ML-ALT`) and `os/sb/host`. Date: 2026-06-11.

Related: [note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md) —
the shared-memory region API interacts directly with point 1 below; the
security effects of the design decisions recorded there are collected in
the "Design-decision impacts" section at the end of this note.

## Defenses currently in place (sound)

- **Hardware-enforced entry stacking.** On `svc`, the exception frame is
  stacked using the current (unprivileged) PSP, so the MPU applies. A
  sandbox that points PSP at host memory faults (MemManage) during
  stacking, before any privileged instruction runs. The sandbox cannot
  redirect the entry frame into supervisor memory.
- **Privileged dispatcher never runs on a guest stack.**
  `__port_do_syscall_entry` (`host/sbsyscall.c`) builds the return context
  at `tp->waend - sizeof(extctx)` (kernel-owned privileged stack). The
  user PSP is only saved (`sbp->u_psp`), never trusted as a stack pointer.
- **Dispatch index cannot go OOB by construction.** Handler splits on
  `cmp r0, #0x80`; syscall path does `subs #128` into a 128-entry table.
  SVC immediates are one byte, so 0..127 and 128..255 map exactly onto the
  two 128-entry tables. Unimplemented slots default to `sb_undef_handler`
  returning `ENOSYS`. Caveat: "by construction" holds only because the
  number originates from a one-byte SVC immediate — the planned R12 ABI
  makes it a 32-bit guest-controlled value and requires an explicit clamp
  (see point 3 below).
- **Buffer arguments validated against the sandbox's own regions.**
  `sb_is_valid_read_range` / `sb_is_valid_write_range` / `sb_check_string`
  (`host/sbregions.c`) walk `sbp->regions[]`, require `sb_reg_is_memory`,
  and for writes require `sb_reg_is_writable`. Device regions are excluded
  from syscall buffers.
- **Fail-safe on error.** A VRQ frame that does not fit the sandbox stack
  is "doomed" via `vrq_set_doomed` (`host/sbvrq.c`), pointing the return PC
  at a kernel address outside all sandbox regions -> immediate fault ->
  termination. Faults taken in unprivileged thread mode are converted by
  `sbexc.S` (`.Lexit_thd`) into `chThdExit`/`__sb_abort`, so a sandbox
  crash is a clean termination, not a supervisor crash.
- **VRQ delivery defers while privileged — no guest code mid-syscall.**
  `sbVRQTriggerI`/`sbVRQTriggerS` check the privilege state and, when
  the sandbox thread is in privileged mode (a syscall in progress,
  running or blocked), do *not* push a guest context — they only wake the
  thread (`host/sbvrq.c`, the `chThdResumeI` branches); the guest frame
  is built at syscall exit by `__sb_vrq_check_pending`. A VRQ handler
  therefore cannot run between a syscall's validation and its use of the
  validated data: the single-threaded model holds for the entire syscall
  duration.
- **Privilege cannot be re-acquired through restored frames.** nPRIV
  lives in CONTROL, which is never restored from guest-influenced state,
  and exception return cannot clear it; freshly built VRQ frames get a
  sanitized `xpsr = 0x01000000` (`vrq_initctx`). The "make the kernel
  restore a guest-controlled frame" path cannot elevate.

## Margins / hardening points

### 1. TOCTOU / double-fetch (highest priority, gated by shared-memory API)

All range checks are validate-then-use. Within one SB this is safe: guest
code cannot run while a syscall is in flight (VRQ delivery defers while
privileged, see Defenses), so the validated memory cannot be mutated by
the sandbox itself. The breakers are external writers only:

- a region writable by both the sandbox and a peripheral/DMA engine —
  including the *software* instance of it: the async-VFS worker thread
  operating on guest buffers concurrently with guest code (see
  [note_sb_async_vfs.md](note_sb_async_vfs.md), which applies the
  copy-in-once contract to metadata and DMA semantics to data buffers);
- **the planned shared-memory region API** (another SB or the host mutating
  a validated buffer mid-syscall).

Sharpest case: `sb_check_string` scans for the terminator to bound length,
then the VFS layer re-reads the string -> classic double-fetch if a
concurrent writer changes it in between.

**Constraint for the shared-memory API and any shared/DMA-backed buffer:**
copy-in to a private privileged buffer once, validate and use the copy,
never re-dereference guest memory after the check. Bake this into the API
contract from the start.

### 2. VIO native-handle surface (escape-relevant, tracked in open_points)

Not covered by the range checks above: the VIO layer passes direct
validated native handles across the boundary (VETH model), with
validation that is intentionally structural, not ownership-aware, and
open "enforce fault instead" TODOs in `vio/sbvio_spi.c` /
`vio/sbvio_uart.c`. A forged or stale native handle is a classic escape
vector; this surface ranks with point 1 in priority. Detailed items live
under open_points "Host VIO" — recorded here so this assessment is not
read as covering a surface it does not.

### 3. Syscall number read from guest code memory

The ALT handler recovers the SVC number with a privileged
`ldrb [stacked_pc, #-2]` — a privileged read at a guest-influenced address.
Not exploitable for escape (PC was just executed, within the code region),
but it is a privileged access derived from guest state. Moving the number
to R12 (optimization #2 in the optimizations note) removes the surface and
is faster — security and performance agree. **Required with the R12 ABI:**
R12 is a 32-bit guest-controlled value, unlike the one-byte immediate, so
the handler must clamp it (`uxtb`, ~1 cycle) before any table indexing —
without the clamp, the OOB-impossible-by-construction property in the
defenses above is lost and the dispatch becomes a guest-indexed `blx`.

### 4. FPCA / lazy-stacking invariant is convention, not enforced

`__port_do_syscall_entry` / `__port_do_syscall_return` require that syscall
code does not change FPCA state, or the return uses the wrong frame shape.
A handler that touches the FPU could corrupt the return frame (supervisor
crash, not escape). Add a compile-time / debug assertion instead of relying
on reviewers. Note the invariant's exact statement depends on the
configured mode: `PORT_USE_FPU_FAST_SWITCHING >= 2` deliberately forces
FPCA on at thread start (`sbhost.c`, the `__get_FPSCR()` read).

### 5. Privileged stack overflow is guard-page-dependent

Entry skips overflow checking on the privileged frame ("created on the
privileged PSP so no need to check"). True only while the guard page backs
`SB_CFG_PRIVILEGED_STACK_SIZE`. Deep host call chains under a syscall (VFS
path resolution, lwIP) are the realistic consumers. v8-M sets PSPLIM; on
**v7-M the guard MPU region is the only backstop** — confirm it covers the
privileged stack base for every sandbox config. A privileged overflow that
misses the guard is direct supervisor-memory corruption.

### 6. VRQ injection vs. syscall-return window

The `vrq.isr` / `SB_VRQ_ISR_DISABLED` masking discipline prevents a VRQ
being injected during the return transition. `sbVRQTriggerI` fires
asynchronously from ISRs, so the masking atomicity across the entry ->
dispatch -> return sequence needed explicit re-review.

**Re-reviewed 2026-06-17 — safe by priority design, now guarded.** The
fastcall handlers in `host/sbvrq.c` perform unlocked read-modify-write on
the shared VRQ state (`vrq.isr`, `vrq.wtmask`, `vrq.flags[]`); their
atomicity against the asynchronous producers `sbVRQTriggerI()` /
`sbVRQSetFlagsI()` (both I-class) rests on a priority invariant rather than
a lock. The handlers run at SVCall priority; any IRQ permitted to call an
I-class function is at `CORTEX_MAX_KERNEL_PRIORITY` or below (numerically
higher = less urgent), which the ALT ports pin one level under SVCall
(`CORTEX_MAX_KERNEL_PRIORITY == CORTEX_PRIORITY_SVCALL + 1`,
`ARMv7-M-ALT`/`ARMv8-M-ML-ALT` `chcore.h`). A VRQ producer therefore cannot
preempt a handler mid-sequence, so the would-be lost-update windows are
closed by construction:

- `wtmask` RMW vs. a producer's `|= (1 << nvrq)` — no interleave, no dropped
  VRQ.
- `flags[n]` read-and-clear in `gcsts` vs. `sbVRQSetFlagsI`'s `|=` — no
  interleave, no lost completion flags (this is the VIO completion path).
- the `isr = 0` -> mask-check window in `vrq_enable` / `vrq_return` — no
  producer runs between the store and the check; the `asm("":::"memory")`
  barrier is only compiler-ordering belt-and-suspenders.

The fast-priority band *above* SVCall cannot produce VRQs (it cannot call OS
primitives). The S-class producer `sbVRQTriggerS()` runs under `chSysLock`,
same-core only (`owner == currcore` assert), so it cannot overlap a handler
either. Converting these handlers to the IRQ-fastcall lock contract would be
redundant overhead. Because the safety depends entirely on
`CORTEX_PRIORITY_SVCALL < CORTEX_MAX_KERNEL_PRIORITY`, a `#if … #error`
guard in `host/sbvrq.c` now fails the build if a future port or priority
reconfiguration breaks that ordering.

## Design-decision impacts (2026-06-11)

Security effects of the designs decided in
[note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md):

- **MPU region-table design (its point 3)** — ✅ implemented 2026-06-12
  (`d9e47421` + `6d2fa405`): (a) an SB -> kernel switch
  always loads the const default table, so no privileged
  thread ever runs with a dead sandbox's regions programmed — the
  "regions only grant, never restrict" AP rule becomes defense-in-depth
  instead of a correctness requirement; (b) the switch code compares
  table pointers only, so **revoking** a shared region from a running SB
  must write the live MPU registers in the same critical section as the
  table update, or the sandbox retains access until the next domain
  crossing.
- **Dedicated context-switch IRQ (its point 1)**: the switch handler
  becomes unreachable from unprivileged mode by construction (NVIC/STIR
  are in the SCS, privileged-only regardless of MPU, provided
  CCR.USERSETMPEND stays 0 — assert it), and SVC_Handler shrinks to
  guest-facing code only with no immediate fetch on any path, reducing
  the attack surface of the single guest-reachable kernel entry.
- **R12 syscall-number ABI (its point 2)**: removes the privileged read
  at a guest-influenced address (point 3 above) but requires the explicit
  clamp before table indexing, also point 3 above.
- **IRQ-like fastcalls (its point 5)**: fastcall handlers gain I-class
  capability (sound: a fastcall is always entered from unprivileged
  thread mode at BASEPRI=0, so it never preempts a kernel lock zone).
  Argument-validation discipline is unchanged, but the blast radius of a
  handler bug grows from corrupted SB state to corrupted scheduler state
  at ISR level. The prologue/lock/unlock/epilogue contract is enforced by
  the state checker in debug, in both directions (I-class without lock
  asserts; lock without prologue asserts).

## Bottom line

Architecture is fundamentally sound and the strongest property is that
guest code cannot run while a syscall is in flight, so TOCTOU is not live
today for a self-contained SB. The concentrated risks are the two
guest-facing surfaces: memory the sandbox does not exclusively own (DMA
today, the shared-memory API tomorrow — hence the copy-in-once
discipline) and the VIO native-handle model (point 2). Items 3-6 are
supervisor-integrity (crash-resistance) hardening rather than escape
vectors, with #5 the one to verify concretely per-config.
