# VIO syscall -> fastcall migration (tracking)

Working checklist for moving VIO data-plane operations from the syscall
path to IRQ-like fastcalls, one driver at a time. Rationale and full
analysis: [note_svc_mpu_optimizations.md](note_svc_mpu_optimizations.md)
point 5 and its re-distribution addendum.

**Prerequisite:** the IRQ-like fastcall mechanism (point 5) must be in
place before any of these can land — the migrated handlers are written as
ISR bodies (`CH_IRQ_PROLOGUE`/lock-from-ISR/.../`CH_IRQ_EPILOGUE`).

## Principle

XHAL drivers are asynchronous by default: `start`/`stop` (driver
lifecycle) are the exceptions; every transfer/conversion/transaction is a
non-blocking kick whose completion arrives out-of-band (callback -> VRQ).
So for VIO the fastcall path is the rule and the syscall path collapses to
driver lifecycle plus the few genuinely-blocking primitives.

Already in the target shape (reference, no work):

- **UART** — data path already in fastcall 97; only `INIT`/`DEINIT` are
  syscalls.
- **GPIO** — no syscall number; all ops already in fastcall 96.

## Per-driver recipe

1. Move the migrating `case`s from `sb_sysc_vio_X` to `sb_fastc_vio_X`.
2. `chSysLock`/`chSysUnlock` -> `chSysLockFromISR`/`chSysUnlockFromISR`;
   handler is an ISR body. Bodies otherwise unchanged (they already call
   the `...I`/`...X` XHAL variants).
3. Give each migrated op a fastcall sub-code that does not collide with
   that peripheral's existing fastcall sub-codes; update the guest stubs.
4. Guest ABI change — see "Cross-cutting" for batching.
5. Validate on HW with `testsb/SB_VIO-STM32G474RE-NUCLEO64-HOST` (the
   G474 `SB_HOST_SWITCHED` demo only exercises GPIO + UART VIO). The test
   apps run with the state checker on (it asserts any prologue/lock/
   unlock/epilogue contract violation), so a clean run is the contract
   proof.

   **Build gotcha:** enable `CH_DBG_SYSTEM_STATE_CHECK` (and the other
   debug options) in `chconf.h`, *not* via a command-line `UDEFS=-D...`.
   `UDEFS` reaches only the C compiler (`DEFS`); the assembler uses
   `ADEFS`/`UADEFS`. Setting it only in `UDEFS` leaves `chcoreasm.S`
   seeing it FALSE, which compiles out `__port_thread_start`'s
   `bl __dbg_check_unlock` -> spawned threads run with `lock_cnt=1` -> a
   spurious `SV#4` on the first locking call. (Cost us a half-day red
   herring on 2026-06-16.)

## Order and checklist

- [x] **1. ADC** (228 -> 100) — **done 2026-06-16**: `START_LINEAR`,
      `START_CIRCULAR` (`adcStartConversion*I`), `STOP`
      (`adcStopConversionI`) moved to the fastcall handler; `INIT`/
      `DEINIT`/`SELCFG` kept on 228; `GCERR` already a fastcall.
      HW-validated on G474 (`testsb/SB_VIO`, `adc stream`) under the full
      state checker — START/completion-VRQ/STOP with no `SV#` assertion.
- [ ] **2. SPI** (226 -> 98): migrate `PULSES`/`RECEIVE`/`SEND`/`EXCHANGE`
      (`spiStart*I`), `STOP` (`spiStopTransferI`), `SELECT`/`UNSELECT`
      (`spiSelectX`/`spiUnselectX`). Keep `INIT`/`DEINIT`/`SELCFG`.
      Template case: `validate -> lock -> spiStartReceiveI -> unlock`.
- [ ] **3. I2C** (230 -> 102): migrate `TX`/`RX` (`i2cStartMaster*I`),
      `STOP` (`i2cStopTransferI`), `GCERR` (`i2cGetAndClearErrorsX`). Keep
      `INIT`/`DEINIT`/`SELCFG`.
- [ ] **4. GPT** (229 -> 101): migrate `START`/`STOP`/`CHGI` (swap
      `gptStartContinuous`/`gptStartOneShot`/`gptStopTimer`/
      `gptChangeInterval` for their `...I` forms). Keep
      `INIT`/`DEINIT`/`SELCFG`/`SETCB` and **`PDELAY` (`gptPolledDelay`
      busy-waits — must stay a syscall)**.
- [ ] **5. ETH** (227 -> 99): migrate `LINK`, `RXREAD`/`TXWRITE`,
      `RXREL`/`TXREL`, `RXGET`/`TXGET` (handle fetch + copy, all X-class).
      Keep `INIT`/`DEINIT`/`SELCFG`. Do last: VETH ABI/ownership is still
      in flux (open_points "Host VIO / ETH").

## Cross-cutting

- **`GCERR` should be a fastcall for every peripheral** (it is X-class);
  today ADC has it as a fastcall, I2C as a syscall — unify while here.
- **One ABI revision, not five.** Host handlers may move one driver at a
  time during development, but the guest-facing change (stub renumbering)
  should be released as a single batch behind the SB ABI version bump.
- Once a peripheral's data plane is all fastcall, its syscall number holds
  only lifecycle ops — consider whether the two-number per-peripheral
  split is still worth keeping (see addendum).

## Status

ADC done and HW-validated (2026-06-16). Mechanism confirmed: IRQ-like
fastcalls work with the existing OSAL IRQ macros (no port change needed).
Next: SPI. Remaining: SPI, I2C, GPT, ETH.
