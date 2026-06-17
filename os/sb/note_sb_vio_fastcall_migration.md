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
      (`adcStopConversionI`) moved to the fastcall handler as ISR bodies;
      `SELCFG` (`drvSetCfgX`, now X-class — see cross-cutting) moved as a
      plain fastcall case; `GCERR` already a fastcall. Only `INIT`/`DEINIT`
      stay on 228. HW-validated on G474 (`testsb/SB_VIO`, `adc stream`)
      under the full state checker — SELCFG/START/completion-VRQ/STOP with
      no `SV#` assertion.
- [x] **2. SPI** (226 -> 98) — **done 2026-06-16, HW-validated**:
      `PULSES`/`RECEIVE`/`SEND`/`EXCHANGE` (`spiStart*I`) and `STOP`
      (`spiStopTransferI`) moved to `sb_fastc_vio_spi` as ISR bodies;
      `SELECT`/`UNSELECT` (`spiSelectX`/`spiUnselectX`) and `SELCFG`
      (`drvSetCfgX`) are plain fastcall cases. The blanket `state == READY`
      gate at the top of `sb_fastc_vio_spi` was removed (it is incompatible
      with `STOP`, which must abort an *active* transfer) and replaced with
      per-case state checks, matching the ADC handler. Only `INIT`/`DEINIT`
      stay on 226. Sub-codes were renumbered into one contiguous space
      (`sbsysc.h`) because the syscall and fastcall spaces formerly overlapped
      (`PULSES`==`UNSELECT`==2) and now share the fastcall switch.
      HW-validated on G474 (`testsb/SB_VIO`, new `spi` command: two threads
      contending on `SPID1` with different configurations + a start/abort
      sequence) under the full state checker: 1205 + 2408 iterations, 0
      errors, no `SV#` assertion.
- [~] **3. I2C** (230 -> 102) — **code migrated 2026-06-16, HW validation
      pending (no I2C device yet)**: `TX`/`RX` (`i2cStartMaster{Transmit,
      Receive}I`) and `STOP` (`i2cStopTransferI`) moved to `sb_fastc_vio_i2c`
      as ISR bodies. `SELCFG` and `GCERR` (`i2cGetAndClearErrorsX`) were
      *already* fastcalls here, so no GCERR unification was needed (the
      cross-cutting note below was stale on that point). Only `INIT`/`DEINIT`
      stay on 230. **No sub-code renumber needed**: the I2C fastcall space
      (`GCERR=0`,`SELCFG=1`) and the migrated codes (`TX=2`,`RX=3`,`STOP=4`)
      do not collide. Compile-verified on both host and client (I2C
      temporarily enabled), stylecheck-clean. Not yet HW-validated: needs an
      I2C device + an `i2c` test command in SB_VIO.
- [x] **4. GPT** (229 -> 101) — **done 2026-06-16**: every op except
      `INIT`/`DEINIT` is now a fastcall. `START`/`STOP`/`CHGI` moved to
      `sb_fastc_vio_gpt` as ISR bodies, swapping the api-class
      `gptStartContinuous`/`gptStartOneShot`/`gptStopTimer`/
      `gptChangeInterval` for their `...I` forms; `SELCFG`, `SETCB` and
      `PDELAY` moved as plain fastcall cases. `SETCB` uses the X-class
      `drvSetCallbackX`. `PDELAY` was unblocked by **reclassifying
      `gptPolledDelay` as X-class and renaming it `gptPolledDelayX`** in the
      XHAL GPT driver (codegen `hal_gpt.xml`: `<api>`->`<xclass>`,
      regenerated) — it is a pure busy-wait that takes no OS lock and never
      deschedules, so it is context-safe; it was previously mis-classified
      as `@api`. (Classic-HAL `gptPolledDelay` is a separate API and is
      unchanged.) Sub-codes
      were renumbered into one contiguous space (`sbsysc.h`): `PDELAY=2`
      would otherwise collide with the old fastcall `GETFREQ=2`, so the
      `GET*` codes moved to 8/9/10. HW-validated on G474 via `adc stream`
      (drives `gptStartContinuous` + `gptStopTimer` on `GPTD1` to trigger
      the ADC) under the full state checker — clean stream, no `SV#`
      assertion. `CHGI`/`SELCFG`/`SETCB`/`PDELAY` are compile-verified but
      not exercised by that path (same pattern, low risk).
- [x] **5. ETH** (227 -> 99) — **done 2026-06-16, HW-validated on H563**:
      `SELCFG`, `LINK`, `RXREAD`/`TXWRITE`, `RXREL`/`TXREL` moved
      from `sb_sysc_vio_eth` to `sb_fastc_vio_eth`, joining the already-
      fastcall `RXGET`/`TXGET`. Only `INIT`/`DEINIT` stay on 227. **Pure
      relocation — no ISR bodies needed**: unlike ADC/SPI/I2C/GPT, the ETH
      data-plane ops never took `chSysLock` (they are lock-free, non-
      blocking forwarders to `eth_lld_*`: handle fetch/validate, byte
      copy in/out, descriptor release, MDIO link poll), so they go in as
      plain fastcall cases like `RXGET`/`TXGET`. No sub-code renumber: ETH
      already used one contiguous space (`RXGET=8`,`TXGET=9` vs migrated
      `2`-`7`). The five lock-free data-plane ops
      (`ethReadReceiveHandle`/`ethWriteTransmitHandle`/
      `ethReleaseReceiveHandle`/`ethReleaseTransmitHandle`/`ethPollLinkStatus`)
      were mis-annotated `@api` though they are lock-free and context-safe
      (like `gptPolledDelay` was) — **reclassified `@xclass` and renamed
      `...X`** in the XHAL ETH codegen (`hal_eth.xml`, regenerated); callers
      updated (the SB port and the `lwip_bindings/lwipthread_xhal.c`
      binding). HW-validated on an STM32H563ZI Nucleo144 with the
      `RT-STM32H563ZI-NUCLEO144-SB_HOST_SWITCHED` demo (SB2 = the
      `SB-CLIENT-RT-XHAL-LWIP-HTTP` sandbox, an lwIP HTTP server on the VIO
      ETH at 192.168.1.10): a GDB breakpoint on `sb_fastc_vio_eth` caught the
      RX path repeating cleanly (sub-codes 8 `RXGET` -> 4 `RXREAD` -> 6
      `RXREL`), `ping` succeeded (RX request + TX reply), and `curl
      http://192.168.1.10/` returned the full HTTP 200 page — so the whole
      RX+TX data plane runs through the migrated fastcalls with no
      fault/halt. (Functional end-to-end validation rather than under the
      state checker: the ETH fastcall cases are plain X-class with no
      prologue/lock, and the demo ships with debug off.)

## Cross-cutting

- **`SELCFG` moves to fastcall for every peripheral** (all use the base
  `drvSetCfgX`). This was unblocked by fixing `drvSetCfgX`/`drvSelectCfgX`
  in the base driver: they were named X-class but took `osalSysLock`,
  which is illegal. **X-class contract:** the function must be safe to
  call from *any* context (thread, ISR, locked), so it must be atomic by
  construction — any TOCTOU is handled *inside* the implementation (here:
  atomic single-word state read and config-pointer write, the in-between
  race being benign), never by taking the OS lock and never delegated to
  the caller's context. Fixed at the codegen source
  (`hal_base_driver.xml`: `<api>`->`<xclass>`, lock removed) and
  regenerated. (`setcfg`/`selcfg` were already invoked under the lock, so
  they were already non-blocking — dropping the lock removes the illegal
  part without weakening them.) Now `SELCFG` is a plain fastcall case (no
  prologue/lock, like `GCERR`); only `INIT`/`DEINIT` (`drvStart`/
  `drvStop`, genuinely mutex/blocking) stay syscalls.
- **`GCERR` is a fastcall for every peripheral** (it is X-class). Confirmed
  already true for both ADC and I2C (2026-06-16) — no unification was
  needed.
- **One ABI revision, not five.** Host handlers may move one driver at a
  time during development, but the guest-facing change (stub renumbering)
  should be released as a single batch behind the SB ABI version bump.
- Once a peripheral's data plane is all fastcall, its syscall number holds
  only lifecycle ops — consider whether the two-number per-peripheral
  split is still worth keeping (see addendum).

## Status

**All VIO drivers migrated (2026-06-16).** Code-complete for ADC, SPI, I2C,
GPT and ETH; only `INIT`/`DEINIT` remain on the syscall path anywhere.

- ADC, SPI, GPT — HW-validated on G474 under the full state checker
  (SB_VIO `adc`/`spi` commands; GPT via `adc stream`).
- ETH — HW-validated on H563 (`RT-STM32H563ZI-NUCLEO144-SB_HOST_SWITCHED`
  lwIP HTTP sandbox): RX path traced via GDB, `ping` + `curl` end-to-end
  over the migrated fastcalls.
- I2C — compile-verified both sides; HW validation pending an I2C device +
  an `i2c` SB_VIO command (the only remaining HW validation).

## Test harness (per-driver SB_VIO commands)

HW validation needs the SB_VIO client to actually drive each migrated
driver under the state checker. Plan: one shell command per driver in
`testsb/SB_VIO-XSHELL-CLIENT`, plus the matching host-side VIO enable +
unit/config wiring (`cfg/vioconf.h`, host `main.c`).

- ADC, GPT — covered.
- **SPI** — done: `spi` command modeled on `testhal/STM32/multi/SPI`, two
  threads contending for `SPID1` under different selected configurations,
  plus a start/abort sequence. Exercises `SELCFG` + `SELECT`/`UNSELECT` +
  `EXCHANGE`/`SEND`/`RECEIVE`/`IGNORE` + `STOP` under the full state
  checker — the contract proof for the migrated fastcalls. Host backs the
  virtual SPI with `SPID2` (SPI2 on PB12-15), `VIO_CFG_ENABLE_SPI=TRUE`,
  `vrqn=6` matching the guest `VIO_VSPI1_IRQ`. No external device: a
  contract-proof test (MISO floats, rx data not checked).
- **I2C** — pending: needs an I2C device on the host bus (unlike SPI, I2C
  cannot self-test with a floating bus — a master TX with no slave NAKs/
  errors immediately, so the state checker would still see the fastcall
  path but the transfer reports an error rather than completing). Add an
  `i2c` command once a device is available, backed by a host `I2CDx`
  instance + `VIO_CFG_ENABLE_I2C=TRUE`.
- ETH — add as its migration lands.

Mechanism confirmed: IRQ-like fastcalls work with the existing OSAL IRQ
macros (no port change needed). Remaining migration: ETH (I2C code done,
HW validation pending a device; GPT done + HW-validated via `adc stream`).
