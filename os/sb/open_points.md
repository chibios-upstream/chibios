# SB Open Points

This file tracks the current known follow-up work in `os/sb/`.
It is not meant to be a strict roadmap; it is a grouped backlog of the
remaining technical points across the SB subsystems.

A prioritized, tiered list of the 2026-06 proposed SB improvements is
maintained in
[note_sb_improvement_priorities.md](note_sb_improvement_priorities.md).

## Common

- Review the SB syscall namespace in `common/sbsysc.h` after the recent VETH addition and keep the fastcall/syscall split semantically strict.
- Keep the ETH, SPI, UART, GPIO VIO call layout aligned and documented so client and host ports evolve together.
- Add a short protocol note for VETH in `os/sb/` describing the current direct-handle model, its assumptions, and the intended future zero-copy extension.

## Host

- Re-review syscall return and VRQ paths in `host/sbsyscall.c` and `host/sbvrq.c`, especially around restart/teardown windows and late producers. **Masking-atomicity part resolved 2026-06-17:** the `vrq.isr`/`SB_VRQ_ISR_DISABLED` masking RMW sequences are atomic by priority design — SVCall out-prioritizes every kernel-preempting (I-class-capable) IRQ in the ALT ports (`CORTEX_PRIORITY_SVCALL < CORTEX_MAX_KERNEL_PRIORITY`), so no VRQ producer can preempt a handler mid-sequence. Documented in `note_sb_isolation_security.md` margin 6 and guarded by a `#if … #error` in `host/sbvrq.c`. Restart/teardown/late-producer review (the `chThdTerminatedX` guards) still open.
- Decide whether sandbox memory-range violations in host services should keep returning `CH_RET_EFAULT` or escalate to a sandbox fault/termination policy.
- Add more host-side validation tests for multi-image bring-up, because incorrect flashing order can produce misleading startup failures during debug.
- Evaluate whether a host-side integration demo/test should be added for the working host + SB1 + SB2 configuration used during VETH/lwIP bring-up.
- Evaluate the SVC/MPU context switch optimizations and the planned shared-memory region API analyzed in [note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md). MPU region-table design (point 3) is **implemented** on `chibios-sandboxes-dev` (2026-06-12, commits `d9e47421` + `6d2fa405`): thread context holds a pointer to a region table (per-SB shared table, const default table honoring the static init settings for ordinary threads), switch-in compares table pointers only and burst-loads via the RBAR/RASR alias registers, no store-back. Still open from that note: the SVC role split (point 1), R12 syscall-number ABI (point 2), and the VRQ deliverable-word precompute (point 4). When the shared-memory API is added, writers of a live table must update the live registers in the same critical section (revocation especially) — the switch code compares pointers only.
- Async VFS for sandboxes, designed in [note_sb_async_vfs.md](note_sb_async_vfs.md) (2026-06-11): submit/complete host ABI with a per-SB worker thread and VRQ-flags completion, fixing the whole-SB stall on blocking POSIX calls under the guest sub-scheduler threading model (host-side multi-threading per SB assessed and rejected, reasoning in the note). Open items listed in the note: slot ABI, one-in-flight-per-FD, restart/cancellation semantics, guest runtime integration.
- IRQ-like fastcalls decided 2026-06-11 ([note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md) point 5): fastcall handlers may opt into RTOS interaction by being written as ISR bodies (CH_IRQ_PROLOGUE / lock / I-class / unlock / CH_IRQ_EPILOGUE), dispatcher untouched, trivial fastcalls zero-cost; contract enforced by the debug state checker. Start with SVC above the kernel mask (existing handlers keep implicit atomicity); kernel-band placement is a later refinement requiring a handler locking audit. Prologue/epilogue behavior is port-defined and the portable RTOS is priority-agnostic, so SVC priority is not a design concern (any adaptation would be local to the port). Enables single-exception submit/status/cancel for async VFS. A syscall/fastcall re-distribution analysis in light of this (which current syscalls should become fastcalls) is in that note's point 5 addendum (2026-06-16): kernel-object side — `broadcast_flags` and the VRQ alarm set/reset are clean moves, `reply_message` needs a new `chMsgReleaseI` primitive first. The larger surface is VIO: the data-plane sub-codes of the SPI/I2C/ADC/GPT/ETH syscall handlers (225-230) are non-blocking async I-class kicks (already calling `spiStart*I`/`i2cStartMaster*I`/`adcStartConversion*`/etc.) and should migrate per sub-code, leaving only `INIT`/`DEINIT`/`SELCFG` lifecycle and GPT `PDELAY` as syscalls — UART (225) already has this shape. A guest ABI change; see the addendum for the per-peripheral list and the two-number-split consequence. Driver-by-driver tracking checklist in [note_sb_vio_fastcall_migration.md](note_sb_vio_fastcall_migration.md) (order: ADC, SPI, I2C, GPT, ETH; gated on point 5 being implemented first).
- Preferred SVC design decided 2026-06-11 (same note, point 1): move the context switch off SVC to a software-pended unused NVIC IRQ (opt-in per platform; trigger is `str` to STIR + 2-byte spin loop, handler skips 2 bytes on the stacked PC — no DSB/ISB). SVC becomes SB-only; the `svc 1` syscall return loses the discrimination fetch on every syscall, kernel-side only. Fallback for IRQ-constrained platforms: shared SVC with the `movs r0, #0` register convention. Complemented by the syscall-number-in-R12 ABI change for the entry side.

## Host isolation / escape resistance

Full assessment in [note_sb_isolation_security.md](note_sb_isolation_security.md).

- Define a copy-in-once contract for any syscall buffer backed by shared or DMA-capable memory (validate and use a private privileged copy, never re-dereference guest memory after the check). This is a prerequisite for the planned shared-memory region API, which is what activates the TOCTOU/double-fetch risk.
- Move the syscall number out of guest code memory (R12 instead of the `ldrb` of the SVC immediate); removes a privileged read at a guest-influenced address and is also a perf win. The R12 value is 32-bit guest-controlled (the immediate was one byte), so the handler must clamp it (`uxtb`) before table indexing.
- Add a compile-time/debug assertion enforcing that syscall handlers do not change FPCA state, instead of relying on review (corrupt FPCA -> wrong return frame shape -> supervisor crash).
- Confirm the guard MPU region covers the privileged stack base (`SB_CFG_PRIVILEGED_STACK_SIZE`) for every sandbox config on v7-M (no PSPLIM there); deep host call chains under a syscall are the realistic overflow consumers.

## Host VIO

### ETH

- Decide and document the final VETH ABI shape.
  Current model: direct validated native handles across the boundary.
  Alternative model kept as a backup: host-generated slot tokens.
- Add a real receive-size query or equivalent contract for copy-mode VETH so the client does not need to infer RX length from MTU-sized buffers.
- Add optional zero-copy support later, only as an explicit capability:
  host-mapped packet buffers, no descriptor sharing.
- Improve host-side VETH validation in `vio/sbvio_eth.c`.
  Current validation is intentionally structural, not ownership-aware.
- Review restart behavior and stale-handle behavior once sandbox restart and VETH reuse are exercised more aggressively.

### SPI

- Resolve the in-tree `TODO enforce fault instead.` sites in `vio/sbvio_spi.c` for invalid client buffers and invalid stop-result buffers.
- Decide whether SPI should continue returning `CH_RET_EFAULT` or should actively fault the sandbox on invalid memory access attempts.
- Add explicit tests for invalid-range handling and callback/VRQ behavior under malformed client requests.

### UART

- Resolve the in-tree `TODO enforce fault instead.` sites in `vio/sbvio_uart.c`.
- Review async read/write semantics and event signaling under heavy traffic and restart conditions.
- Verify whether the exported configuration copy remains sufficient if SIO configuration structures evolve.

### GPIO

- Review whether the current permission model in `vio/sbvio_gpio.c` is enough for future use cases or if read/write/setmode permissions need to be made more granular.

## User

- Re-review the SB user syscall wrappers in `user/sbuser.h` with emphasis on calling-context assumptions and early startup behavior.
- Add more user-side notes about the required flashing/debugging order for host + multiple sandbox images.
- Expand user-side helper coverage only after the host-side contracts are considered stable, especially for VETH.

## Apps

- Revisit the sample applications in `apps/` and decide which ones should become maintained regression/demo apps versus historical examples.
- Add a minimal networking-oriented SB sample once the current host + VETH + lwIP path is considered stable enough to preserve as a regression baseline.

## Cleanup

- Remove or archive the stale backup files `vio/sbvio_eth (copy).c` and `vio/sbvio_eth (copy).h` if they are no longer needed.
- Periodically sync this file with resolved items so it stays a useful working backlog rather than a historical dump.
