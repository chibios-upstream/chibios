# SB Open Points

Authoritative sandbox backlog, consolidated 2026-07-25. This is the only
document that tracks implementation status and remaining work in `os/sb/`.
The following notes retain detailed analysis and design rationale but are
not separate backlogs:

- [Isolation and escape resistance](note_sb_isolation_security.md)
- [Async VFS](note_sb_async_vfs.md)
- [Lifecycle and restart protocol](note_sb_lifecycle.md)
- [SVC and MPU optimizations](note_svc_mpu_optimizations.md)
- [TTY integration and signal delivery](note_sb_tty.md)

## Completed baseline

The following work is complete and must not be reintroduced as an open item:

- VFS roots are optional and externally owned (PR #91).
- MPU switched regions use shared table pointers; the default privileged
  table is constant and no region state is stored back on a switch.
- VRQ masking and pending-state read-modify-write sequences were re-reviewed
  and guarded by the required SVCall/kernel priority relationship.
- Kernel-object IRQ-like fastcalls are implemented for flag broadcast and
  VRQ alarm set/reset. Message reply deliberately remains a syscall.
- ADC, SPI, I2C, GPT, and ETH data-plane operations have been migrated to
  fastcalls. Only driver lifecycle operations remain syscalls. ADC, SPI, and
  GPT were hardware-validated on G474; ETH was validated on H563. I2C is
  compile-validated but still needs hardware validation.
- The explicit `UNINIT`/`STOPPED`/`STARTING`/`RUNNING`/`STOPPING` lifecycle,
  `sbSync()`, producer quiescence, and `sbFinalize()` protocol is implemented
  (PR #144). VRQs are accepted only in `RUNNING`.
- Invalid guest ranges detected by host services return `CH_RET_EFAULT`;
  actual processor protection faults terminate the sandbox.
- The null stream uses the VFS FIFO stream mapping (`S_IFIFO`), not the TTY
  interface. `/dev/null` therefore reports false from `isatty()` and rejects
  terminal controls. This classification is intentional, not pending work.
  Reads return zero bytes, `stmGet()` returns `STM_RESET`, and writes succeed
  while discarding data.
- HAL and XHAL define identical operation status values in their respective
  `hal.h` files, preserving the legacy HAL values and adding invalid driver
  state at -22. The VFS TTY adapter maps configuration errors to `EINVAL`
  and retains `EIO` for other failures. XHAL hosts and VIO guests must be
  rebuilt together after this status-ABI alignment; see
  [note_sb_tty.md](note_sb_tty.md).
- The SIO-backed TTY supports all four noncanonical `VMIN`/`VTIME` read
  modes. `chedit` configures and restores terminal attributes in both native
  and sandbox builds. Deterministic input tests cover timing, partial reads,
  resets and canonical EOF; native PTY tests cover editing and normal/error
  restoration. Nucleo testing confirmed interactive editing, fragmented
  escape sequences, standalone Escape, read-only save errors, canonical
  input after editor exit, and multi-byte/inter-byte timing.
- Shared terminal headers live under `os/common/posix/include` with an
  Apache-2.0 license, retaining the existing host/guest ABI. The XHAL TTY
  no longer depends on the sandbox header directory. Concurrent drains use
  a thread queue, including reset/stop wakeups and delayed-caller handling.

## Priority 1: security and isolation

### VIO handles and invalid guest memory

This is the highest-priority live security surface.

- Make native-handle validation ownership-aware, especially for VETH.
  Structural validation alone cannot reject a forged handle that names a
  valid object owned by another sandbox or a previous execution.
- Add malformed-request tests covering invalid ranges, stop-result buffers,
  callback behavior, and completion VRQs.
- Exercise stale VETH handles across sandbox finalization and restart.

### Copy-in-once contract

- Define helpers and a host-service contract for metadata in memory that can
  be modified by DMA, a worker, the host, or another sandbox.
- Copy metadata into privileged private storage once, validate that copy,
  and never re-read the guest metadata after validation.
- Use this contract before enabling async VFS or shared-memory regions.
  Bulk data buffers may retain explicit DMA-like concurrent-access semantics.

### Privileged stack and FP state

- Verify that every v7-M sandbox configuration maps the MPU guard over the
  base of `SB_CFG_PRIVILEGED_STACK_SIZE`. v8-M has PSPLIM; v7-M depends on
  the guard region.
- Add a compile-time or debug assertion for the FPCA/lazy-stacking invariant
  required by syscall entry and return, including the
  `PORT_USE_FPU_FAST_SWITCHING >= 2` case.

## Priority 2: functional features

### Sandbox TTY integration

Review recorded 2026-09-12 for the SIO-backed TTY and
`RT-SB-DYNAMIC-RAMBOX-MULTI` demo. Design context and the proposed signal
transport are in [note_sb_tty.md](note_sb_tty.md).

#### 3. TTY signals through a VRQ

- Reserve one signal-delivery VRQ and define its sandbox-visible pending
  signal bits. The proposed transport reuses `sbVRQSetFlagsI()`,
  `sbVRQTriggerI()` and `__sb_vrq_gcsts()`; no additional event queue is
  required. A name such as `SB_VRQ_SIGNALS` is provisional.
- Connect the TTY callback to this transport, mapping `INTR`, `QUIT` and
  `SUSP` into the common signal ABI. Preserve callback context, VRQ masking
  and lifecycle rules described in the design note.
- Define interruptible syscall behavior. A VRQ alone does not release an
  arbitrary host-side wait. A signal that interrupts a read before any
  bytes are transferred must have a distinct outcome mapped to `EINTR`,
  not the current queue-reset result that appears as EOF. Define partial
  transfer, ignored/blocked signal, restart and `NOFLSH` behavior too.
- Include software-flow-control restart in the signal policy. Currently a
  signal character clears `output_stopped` only when `NOFLSH` is clear;
  resuming output independently of flushing must be an explicit decision.
- Provide a guest dispatcher and active-application handling across nested
  `sbRunElf()` calls. Define default actions and shell prompt behavior;
  suspend/resume and full job control are not implied by the transport.
- Test signals during input waits and application execution, repeated and
  combined bits, masked delivery, nested commands, and sandbox restart.

### Async VFS

The design is in [note_sb_async_vfs.md](note_sb_async_vfs.md): one worker per
sandbox, a submit/complete ABI, VRQ-flags completion, and synchronous guest
semantics.

- Define the submission-slot layout, ownership rules, status/error codes,
  and cancellation states.
- Choose the first asynchronous operations. Block-backed `read` and `write`
  are the primary stall sources; `open` may also block during path lookup.
- Decide whether to allow only one in-flight operation per descriptor.
  Rejecting overlap is the preferred simple initial contract.
- Define operation-specific cancellation and drain behavior. After
  `sbSync()`, all operations must be cancelled and joined, or detached
  without retaining sandbox references, before `sbFinalize()`.
- Integrate completion VRQs with guest green-thread wait/wake behavior and
  define the `vrq_wait` idle policy.

### Shared-memory regions

- Define the host and guest API for granting, updating, and revoking shared
  regions.
- Require the copy-in-once contract for shared metadata.
- Update both the sandbox master MPU table and, when that table is active,
  the live MPU registers in the same critical section. Revocation must take
  effect immediately because context switching compares table pointers and
  will not reload a modified active table.

### VETH completeness

- Decide and document the final VETH ABI: directly validated native handles
  or host-generated slot tokens.
- Add an explicit receive-size query or equivalent contract so copy-mode
  clients do not infer packet length from an MTU-sized buffer.
- Improve ownership and range validation in `vio/sbvio_eth.c`.
- Align the VIO ETH handle-validity queries with the generic API contract.
  The VIO client currently treats any nonzero opaque RX/TX handle as locally
  valid. Handle operations remain safe because the host validates each use
  through the native driver and returns an error, but
  `ethIsRXHandleValidX()` and `ethIsTXHandleValidX()` cannot reliably
  identify forged or stale handles. Decide whether to add host validity-query
  operations or narrow and document the VIO-local query semantics.
- Add optional zero-copy only as an explicit capability using host-mapped
  packet buffers; do not share native descriptor rings.
- Add a concise VETH protocol document covering handle lifetime, restart,
  copy-mode behavior, and the zero-copy extension boundary.

## Priority 3: SVC and MPU architecture

Detailed designs are in
[note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md).

- Move the context-switch operation from SVC to an opt-in, software-pended
  unused NVIC IRQ. Keep the shared-SVC `movs r0, #0` convention only as a
  fallback for platforms without a spare IRQ.
- Assert that `CCR.USERSETMPEND` is clear when the dedicated switch IRQ is
  used, preserving its guest-unreachable property.
- Pass the syscall/fastcall number in R12 instead of reading the SVC
  immediate from guest code. Clamp the 32-bit guest value with `uxtb` before
  table indexing and batch this with an SB ABI revision.
- Precompute the deliverable-VRQ word used on syscall return.
- If SVCall is later moved into the kernel priority band, audit every handler
  for the required locking contract first.

## Priority 4: validation and API completion

### Hardware and integration tests

- Add an I2C command to the SB VIO test client and validate the migrated I2C
  fastcall path with a real device under the state checker.
- Add host-side multi-image bring-up validation that distinguishes incorrect
  flashing order from sandbox startup defects.
- Preserve the working host + SB1 + SB2 VETH/lwIP configuration as an
  integration regression test or maintained demo.
- Stress UART asynchronous read/write, event delivery, and restart behavior.
- Add a minimal networking-oriented sandbox sample after the VETH contract
  is stable.

### API and ABI review

- Review `common/sbsysc.h` after the VETH additions and keep the
  fastcall/syscall split semantically strict.
- Keep ETH, SPI, UART, and GPIO VIO layouts aligned and documented across
  host and client ports.
- Re-review user syscall wrappers in `user/sbuser.h`, especially calling
  context and early-startup assumptions.
- Decide whether GPIO read/write/set-mode permissions need finer granularity.
- Verify that copying SIO configuration structures remains sufficient if the
  native configuration type evolves.
- Expand user helpers only after the corresponding host contracts stabilize.

### POSIX and examples

- Complete the missing `stat` metadata in `host/sbposix.c`
  (`st_blocks`, `st_blksize`, `st_ino`, and timestamps).
- Replace the placeholder directory-entry inode value when the VFS can
  provide one.
- Add concise host and user documentation for the required flashing and
  debugging order of multi-image systems.
- Classify applications under `apps/` as maintained regressions/demos or
  historical examples.

## Maintenance rule

Add new actionable sandbox work only to this file. Design notes may explain
an item in detail, but must link here instead of maintaining their own status
or open-item lists. Remove completed items from the priority sections and,
when the result is an important architectural baseline, summarize it under
Completed baseline.
