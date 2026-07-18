# Note: SB Improvements — Priority List

Consolidated, prioritized list of all SB improvements proposed in the
2026-06-11/12 design sessions. Sources:
[note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md),
[note_sb_isolation_security.md](note_sb_isolation_security.md),
[note_sb_async_vfs.md](note_sb_async_vfs.md),
[note_sb_lifecycle.md](note_sb_lifecycle.md),
[open_points.md](open_points.md).

Priority logic: live security gaps first, then the features gated on
them (the SB stall fix being the declared main functional pain), then
performance/architecture, then minor hardening. Date: 2026-06-12.

**Implementation status (updated 2026-06-16):** item 7 (MPU
region-table pointer / read-only context switch, no store-back) is
**implemented** on `chibios-sandboxes-dev` — commits `d9e47421` ("MPU
switched regions become a shared table pointer") and `6d2fa405`
("default MPU table honors the static initialization settings"),
2026-06-12. Item numbers below are kept stable for cross-references;
done items are marked inline.

## Tier 1 — Security, live today (small efforts, do first)

1. **VIO native-handle validation** — make it ownership-aware and
   resolve the `TODO enforce fault instead.` sites in `vio/sbvio_spi.c`
   / `vio/sbvio_uart.c`, plus the EFAULT-vs-fault policy decision. The
   only *escape-relevant* gap active today; everything else
   security-wise is crash-resistance. (isolation note, margin 2)
2. **VRQ masking atomicity re-review** — ✅ **DONE (2026-06-17)**:
   the `vrq.isr` / `SB_VRQ_ISR_DISABLED` (and `wtmask`/`flags[]`) RMW
   sequences are atomic by priority design — SVCall out-prioritizes every
   kernel-preempting IRQ in the ALT ports, so no VRQ producer can preempt a
   handler mid-sequence. No locking needed. Documented in the isolation
   note (margin 6) and guarded by a `#if … #error`
   (`CORTEX_PRIORITY_SVCALL < CORTEX_MAX_KERNEL_PRIORITY`) in
   `host/sbvrq.c`. (margin 6)
3. **v7-M guard-region coverage verification** — confirm the guard MPU
   region backs the privileged stack base
   (`SB_CFG_PRIVILEGED_STACK_SIZE`) for every sandbox config (no PSPLIM
   on v7-M); a miss is direct supervisor-memory corruption. (margin 5)
4. **Copy-in-once contract** — define it and provide the helpers
   (validate -> private privileged copy -> never re-dereference guest
   memory). Cheap to do now, and it *gates* both the async-VFS metadata
   path and the shared-memory API. (margin 1)
4b. **Explicit sandbox lifecycle/finalization** — add independent
   `STOPPED`/`STARTING`/`RUNNING`/`STOPPING` state, gate VRQs on `RUNNING`,
   leave terminated sandboxes in `STOPPING`, and require the host to quiesce
   external producers before `sbFinalize()` permits restart. This closes the
   late-worker/late-IRQ restart contract without making every producer
   generation-aware. Audit `sbWait()` and dynamic-memory release ordering.
   (note_sb_lifecycle.md; isolation note, margin 7)

## Tier 2 — Features (the functional payoff)

5. **Async VFS** — submit/complete ABI, per-SB worker thread, VRQ-flags
   completion. Fixes the whole-SB stall on blocking POSIX calls, the
   main limitation of the guest sub-scheduler threading model. Depends
   on items 4 and 4b; submission wants item 6. (note_sb_async_vfs.md)
6. **IRQ-like fastcalls** — per-handler
   `CH_IRQ_PROLOGUE`/lock/I-class/unlock/`CH_IRQ_EPILOGUE`, dispatcher
   untouched. The *cheapest* item on the whole list (macros, docs,
   state-checker integration — no ABI, no dispatcher, no port surgery)
   and it unlocks single-exception submit/status/cancel for item 5.
   Do immediately before or alongside item 5. The *mechanism* is cheap
   and ABI-free, but it enables a large, broad payoff: most VIO ops
   (SPI/I2C/ADC/GPT/ETH transfers) are async-by-design and can drop from
   syscalls to fastcalls — the syscall path collapses to driver
   lifecycle + blocking primitives. That re-distribution is itself a
   guest ABI change; see the point-5 addendum in the optimizations note
   for the per-peripheral list. (optimizations note, point 5)
6b. **Kernel-object fastcall moves** — ✅ **DONE (2026-06-17)** for the two
   unblocked candidates: `broadcast_flags` (137 -> fastcall slot 3) and
   the VRQ alarm `set`/`reset` (253/254 -> slots 117/118), all rewritten as
   IRQ-like fastcall bodies. `reply_message` (133) **stays a syscall by
   decision (2026-06-17)** — not a hot path, not worth a new public
   `chMsgReleaseI` RT primitive. Kernel-object re-distribution closed.
   Detail in the optimizations-note point-5 addendum.

7. **MPU region-table design** — ✅ **IMPLEMENTED** (2026-06-12,
   `d9e47421` + `6d2fa405`). Pointer in thread context, per-SB shared
   table, const default table honoring the static init settings,
   pointer-equality switch-in with alias-register burst load, no
   store-back. Perf win on every context switch and the prerequisite
   for a clean shared-memory API. (point 3)
8. **Shared-memory region API** — the feature itself. Item 7 (its MPU
   prerequisite) is now in place, so this depends only on item 4
   (copy-in-once). The note's point 3 imposes the writer contract this
   API must honor: mutate the SB's master table *and*, iff the running
   thread points at that table, write the live RBAR/RASR in the same
   critical section — the revocation direction especially, since the
   switch code compares pointers only and will not pick up the change.
   (point 3 + isolation note)

## Tier 3 — Performance / architecture

9. **SVC split: dedicated context-switch IRQ** — STIR + spin-loop
   trigger, +2 stacked-PC fixup. Removes the discrimination fetch from
   every syscall return, makes the switch handler guest-unreachable,
   flattens worst-case timing. Opt-in per platform; also upgrades item
   6 (kernel-band SVC placement becomes available, with the handler
   locking audit). (point 1)
10. **R12 syscall-number ABI** — removes the guest-flash immediate
    fetch on syscall *and fastcall* entry. Guest ABI break: batch with
    an SB ABI version bump; **must** ship with the `uxtb` clamp.
    (point 2)
11. **VRQ deliverable-word precompute** — one load + branch on syscall
    return instead of three loads + masking. Minor. (point 4)
12. **Shared-SVC register convention** (`movs r0, #0`) — only for
    platforms that cannot spare an IRQ for item 9; subsumed otherwise.
    (point 1, fallback)

## Tier 4 — Small hardening

13. **FPCA invariant assertion** — debug assertion instead of review
    convention; mind the `PORT_USE_FPU_FAST_SWITCHING >= 2`
    interaction. (margin 4)
14. **`CCR.USERSETMPEND == 0` debug assertion** — guards the
    guest-unreachability of the switch IRQ; trivially small, lands
    naturally with item 9.

## Suggested first milestone

Tier 1 plus item 6 (IRQ-like fastcalls): all small, no ABI impact, and
they clear the runway for async VFS as the first big feature.
Development branch: `chibios-sandboxes-dev`.
