# XHAL OSAL Removal Migration

## Objective

Remove the OSAL dependency from XHAL completely. XHAL modules use the
ChibiOS RT/NIL `ch` API directly because OSAL does not provide a useful RTOS
abstraction for XHAL.

The migration is complete when:

- No XHAL source, generated source, port, library, configuration, demo, or
  build recipe uses an `osal*` API, an `OSAL_*` definition, or `osal.h`.
- No XHAL build recipe includes `os/hal/osal/rt-nil/osal.mk`.
- All current XHAL build recipes compile and link without `osal.c` or the
  OSAL include directory.
- Classic HAL continues to build with its existing OSAL implementation.

## Baseline

The initial inventory on `chibios-master` found:

- 558 files under `os/xhal`.
- 273 files containing OSAL identifiers.
- 3,304 OSAL identifier matches.
- 49 tracked XHAL build recipes.
- 15 affected XHAL `xmcuconf.h.ftl` templates.
- 45 generated XHAL `xmcuconf.h` files containing OSAL hooks.
- No explicit OSAL API use or `osal.h` inclusion under `os/hal/boards`.

The affected XHAL files are distributed as follows:

| Area | Files |
| --- | ---: |
| Platform and driver ports | 202 |
| Core sources | 25 |
| Code-generator XML | 20 |
| Public headers | 18 |
| XHAL libraries | 8 |

There are another two affected XML descriptions in nested XHAL library
generators, for a total of 22 affected XML files.

## Architectural Decisions

### Direct ChibiOS API

XHAL includes `ch.h` directly. There will be no OSAL compatibility aliases or
transition layer in the final implementation. Losing transitive access to
OSAL names through `hal.h` is an accepted source compatibility break because
XHAL is new and strict compatibility with classic HAL is not required.

Most OSAL calls map directly:

- `osalDbg*` to `chDbg*`.
- `OSAL_IRQ_*` to `CH_IRQ_*`.
- `osalSys*` locking and status operations to `chSys*`.
- `osalOsRescheduleS()` to `chSchRescheduleS()`.
- Thread, queue, and reference operations to `chThd*`.
- Event operations to `chEvt*`.
- Time operations to `chVT*` and `chTime*`.
- `OSAL_ST_*` configuration to `CH_CFG_ST_*`.

`osalThreadSuspendS()` must map to
`chThdSuspendTimeoutS(trp, TIME_INFINITE)` in order to preserve NIL
compatibility. `osalInit()` has no replacement and is removed.

### Driver Mutual Exclusion

The base driver encapsulates the RT/NIL synchronization difference. Derived
drivers continue to use `drvLock()` and `drvUnlock()`.

When `HAL_USE_MUTUAL_EXCLUSION` is enabled:

- Use `mutex_t` and `chMtx*` when `CH_CFG_USE_MUTEXES` is enabled.
- Otherwise use `semaphore_t` and `chSem*` when
  `CH_CFG_USE_SEMAPHORES` is enabled.
- Issue a compile-time error if neither primitive is available.

The OSAL no-operation fallback is removed. The authoritative implementation
belongs in `os/xhal/codegen/hal_base_driver.xml` and is regenerated into the
base-driver header and source.

### Events

The OSAL event fallback is removed. An enabled module that requires events
must require `CH_CFG_USE_EVENTS == TRUE` and issue a targeted compile-time
error otherwise.

This applies to:

- ETH event support.
- Buffered SIO.
- Serial-over-USB.
- XHAL lwIP bindings.

### Tickless System Timer

The `chcore_timer.h` binding between the ChibiOS kernel port and the XHAL
system timer moves into `os/xhal/lib/complex/rt-nil_bindings`. `xhal.mk`
exports the binding include path by default. Classic HAL keeps its existing
OSAL copy.

XHAL tickless builds with `CH_CFG_ST_TIMEDELTA > 0` must find the XHAL-owned
header without adding the OSAL include directory.

### Common OOP

`os/xhal/xhal.mk` selects `OOP_USE_CHIBIOS` so common OOP code includes
`ch.h` and uses the direct ChibiOS backend.

Current XHAL recipes do not select the synchronized OOP class. If it is used
later, its RT-only mutex dependency must be addressed separately or declared
as an RT-only feature.

### XHAL-Owned MFS

MFS is not shared with classic HAL. A new implementation exists under:

```text
os/xhal/lib/complex/mfs/
```

It may initially preserve the existing API and on-flash format, but it uses
XHAL flash APIs and the `ch` API directly. The classic implementation under
`os/hal/lib/complex/mfs` remains unchanged.

The four current XHAL MFS build recipes are redirected to the XHAL version.

### XHAL-Owned lwIP and HTTPD Bindings

The complete lwIP adaptation layer is split, not only its Ethernet low-level
adapter. The XHAL versions live under:

```text
os/xhal/lib/complex/lwip_bindings/
os/xhal/lib/complex/httpd_vfs_bindings/
os/xhal/lib/complex/httpd_posix_bindings/
```

The new copies use ChibiOS and XHAL APIs directly. Classic bindings under
`os/various` remain unchanged.

The current lwIP binding already requires RT facilities such as mailboxes,
heaps or memory pools, dynamic threads, semaphores, and events. The XHAL
version declares those requirements explicitly and does not claim NIL
support.

### Sandbox Runtime

The sandbox user runtime pure-virtual handler uses `chSysHalt()` directly
instead of `osalSysHalt()`.

### Boards

Boards stay under `os/hal/boards` during this migration:

- They contain no OSAL API use.
- Moving them would break or churn classic HAL builds without helping OSAL
  removal.
- They are hardware descriptions rather than shared HAL/XHAL software
  modules.

Boards can move upward, most naturally under `os/common`, when classic HAL is
retired. That is a separate repository-wide migration.

### XHAL-Owned STM32 Infrastructure

XHAL does not select classic HAL low-level infrastructure. The STM32 EXTI and
RCC make fragments select their XHAL-local implementations and include
directories. The XHAL copies were synchronized with the current classic
implementations at the split point, then converted where required to use the
direct ChibiOS API. They evolve independently after this migration.

### NIL Validation

Dedicated NIL-XHAL build coverage is deferred until after the OSAL migration.
The migration must preserve known NIL-compatible API mappings, but adding and
validating a NIL-XHAL target is not a cutover gate.

## Migration Strategy

The tree must remain buildable at each milestone. Keep the current OSAL
include and build dependency temporarily while consumers are converted.
Remove them only at the final cutover after the XHAL lexical OSAL count reaches
zero.

Each milestone should be independently reviewable and should not begin until
the preceding milestone's exit criteria pass.

## Milestone 0: Baseline and Build Manifest

Create a fixed manifest of the XHAL build recipes:

```sh
rg -l 'os/xhal/xhal\.mk' \
  --glob 'Makefile' --glob '*.make' | sort
```

Store the result in `os/xhal/OSAL_REMOVAL_BUILDS.txt`. Subsequent milestones
use this fixed list even after recipe contents change and OSAL includes
disappear.

Build representative targets before making changes:

- RT-XHAL STM32G474.
- Tickless RT-XHAL STM32H563.
- VIO sandbox host and client.
- ADC-GPT and USB CDC.
- EFL-MFS and WSPI-XSNOR-MFS.
- XHAL lwIP and sandbox lwIP/HTTP.

### Exit Criteria

- The manifest contains the expected 49 recipes.
- All representative baseline builds pass.
- The starting OSAL inventory is reproducible.
- The worktree contains no unexpected changes.

### Verified Result: 2026-07-27

Milestone 0 passed on branch `chibios-vfs-dev` using Arm GNU Toolchain
14.3.Rel1.

The frozen manifest contains 49 recipes and exactly matches a fresh recipe
discovery. Excluding the migration document and manifest, the inventory
reproduced the expected baseline:

- 558 XHAL implementation files.
- 273 files containing OSAL identifiers.
- 3,304 OSAL identifier matches.
- 22 affected XML descriptions.
- 15 affected XHAL `xmcuconf.h.ftl` templates.
- 45 generated XHAL `xmcuconf.h` files containing OSAL hooks.
- All 49 recipes include `os/hal/osal/rt-nil/osal.mk`.
- No OSAL identifiers or includes exist under `os/hal/boards`.

The following targets were cleaned, compiled, and linked successfully:

| Coverage | Target |
| --- | --- |
| Standalone RT | RT-XHAL STM32G474 |
| Tickless RT | RT-XHAL STM32H563 |
| Driver synchronization | ADC-GPT STM32G474 |
| USB | USB CDC STM32G474 |
| VIO host | SB VIO STM32G474 host |
| VIO client | SB VIO XSHELL client |
| Internal flash storage | EFL-MFS STM32G474 |
| External flash storage | WSPI-XSNOR-MFS STM32H735 |
| Native networking | XHAL lwIP STM32H563 |
| Sandbox networking | SB XHAL lwIP/HTTP client |

lwIP is an external, untracked dependency expected under `ext/lwip`. The two
networking builds used the locally available lwIP 2.1.2 source tree through a
command-line `LWIPDIR` override, without adding unversioned files to the
worktree.

## Milestone 1: Split Shared Adaptation Modules

Create XHAL-owned MFS, lwIP, and HTTPD modules and redirect only XHAL build
recipes to them.

Preserve classic HAL implementations and recipes unchanged.

### Exit Criteria

- All four XHAL MFS recipes build using only the XHAL MFS path.
- Both XHAL lwIP recipes build using only XHAL lwIP and HTTPD paths.
- A classic HAL MFS target still builds.
- The classic STM32H563 lwIP target still builds.
- No OSAL identifier exists in the new XHAL modules.
- No XHAL recipe references:

```text
os/hal/lib/complex/mfs
os/various/lwip_bindings
os/various/httpd_vfs_bindings
os/various/httpd_posix_bindings
```

When hardware is available:

- Run the EFL and XSNOR MFS suites through mount, write, read, garbage
  collection, and remount.
- Start lwIP through DHCP or a static address, ping the target, and complete
  an HTTP request.

### Verified Result: 2026-07-27

Milestone 1 passed on branch `chibios-vfs-dev`.

Independent XHAL versions now exist under:

```text
os/xhal/lib/complex/mfs/
os/xhal/lib/complex/lwip_bindings/
os/xhal/lib/complex/httpd_vfs_bindings/
os/xhal/lib/complex/httpd_posix_bindings/
```

The new modules contain no OSAL identifiers or includes. The XHAL MFS public
header is identical to the classic header, and its implementation differs
only in the direct-Ch diagnostics, preserving the API and storage format.
The XHAL lwIP binding has no classic MAC backend or backend-selection branch
and declares its required ChibiOS features explicitly.

All six XHAL consumer recipes use XHAL-local module paths. None of the 49
recipes in `OSAL_REMOVAL_BUILDS.txt` references the classic MFS, lwIP, or
HTTPD binding paths.

The following builds passed:

- XHAL EFL-MFS on STM32G474 and STM32WL55.
- XHAL WSPI-XSNOR-MFS on STM32H735 and STM32L4R9.
- Native XHAL lwIP/HTTP on STM32H563.
- Sandbox XHAL lwIP/HTTP client.
- Classic HAL EFL-MFS on STM32WL55.
- Classic HAL lwIP/HTTP on STM32H563.

Networking builds used the same external lwIP 2.1.2 source recorded for
Milestone 0. Hardware runtime tests remain part of Milestone 7.

## Milestone 2: Establish Direct-Ch Infrastructure

- Make `xhal.mk` select `OOP_USE_CHIBIOS`.
- Add the XHAL-owned `chcore_timer.h`.
- Change the sandbox pure-virtual handler to `chSysHalt()`.
- Leave classic OSAL infrastructure and boards unchanged.

### Exit Criteria

- XHAL OOP objects depend on `ch.h`, not `osal.h`.
- A target with `CH_CFG_ST_TIMEDELTA > 0` builds using XHAL's
  `chcore_timer.h`.
- A target with `CH_CFG_ST_TIMEDELTA == 0` also builds.
- VIO sandbox host and client builds pass.
- Representative classic HAL tickless and sandbox builds still pass.

### Verified Result: 2026-07-27

Milestone 2 passed on branch `chibios-vfs-dev`.

`xhal.mk` now selects the direct-Ch common OOP backend with
`OOP_USE_CHIBIOS`. The XHAL-owned tickless binding is
`os/xhal/lib/complex/rt-nil_bindings/chcore_timer.h`; `xhal.mk` exports its
include path by default. The sandbox pure-virtual handler calls `chSysHalt()`
directly. Classic OSAL infrastructure and board files are unchanged.

The tickless XHAL STM32H563 build was also run with a temporary `OSALINC`
override that exposed `osal.h` but not the classic `chcore_timer.h`. This
disambiguated the two headers during the transition:

- The tickless dependency set selected
  `os/xhal/lib/complex/rt-nil_bindings/chcore_timer.h`.
- No dependency file selected `os/hal/osal/rt-nil/chcore_timer.h`.
- The common OOP base-object dependency selected `ch.h`, not `osal.h`.

The following builds passed:

- Tickless XHAL STM32H563.
- Tickless XHAL VIO host on STM32G474.
- Periodic XHAL VIO XSHELL client; no `chcore_timer.h` dependency was
  produced.
- Tickless classic HAL STM32L452; its dependencies continued to select the
  classic OSAL timer binding.
- Bare-metal sandbox client, including the changed pure-virtual source.

All generated build and dependency directories were removed after
inspection.

## Milestone 3: Convert XHAL Core and Generated Interfaces

Convert OSAL use under:

```text
os/xhal/include/
os/xhal/src/
os/xhal/codegen/
os/xhal/lib/
```

Implement base-driver mutual exclusion and the event requirements described
above. Keep the transitional OSAL inclusion only where it is still required
by unconverted ports.

Update every changed XML description and its checked-in generated output
together.

Validate each changed codegen XML:

```sh
xmllint --noout --noent \
  --schema tools/ftl/schema/ccode/modules.xsd \
  path/to/changed.xml
```

Regenerate each affected generator root:

```sh
cd path/to/codegen
fmpp -C config.fmpp
```

Relevant roots include the main XHAL generator and the nested serial-USB,
XSNOR, and sensor generator roots when their outputs are changed.

### Exit Criteria

- All changed XML files validate against the schema.
- Running each affected generator a second time produces no further changes.
- No OSAL identifier remains in XHAL `include`, `src`, `codegen`, or `lib`,
  except an explicitly documented transitional include required by
  unconverted ports.
- Core representative builds pass while `osal.mk` is still temporarily
  present.
- Event-dependent modules fail with targeted configuration errors when Ch
  events are disabled.

### Verified Result: 2026-07-27

Milestone 3 passed on branch `chibios-vfs-dev`.

XHAL core, generated interfaces, and XHAL-owned libraries now use direct
ChibiOS APIs. Indefinite OSAL suspension calls were converted to
`chThdSuspendTimeoutS(..., TIME_INFINITE)` to preserve NIL-compatible
semantics. The system timer uses `CH_CFG_ST_TIMEDELTA` directly and is always
initialized; its alarm API is present only in tickless mode.

The base driver now owns the RT/NIL mutual-exclusion choice:

- `mutex_t` and `chMtx*` are selected when `CH_CFG_USE_MUTEXES == TRUE`.
- `semaphore_t` and `chSem*` are selected otherwise when
  `CH_CFG_USE_SEMAPHORES == TRUE`.
- A targeted configuration error is issued when neither primitive is
  available.

An RT build with mutexes and condition variables disabled compiled and linked
using the semaphore path. A configuration with both mutexes and semaphores
disabled stopped at the intended base-driver error.

The OSAL event fallback has been removed. Targeted event-disabled builds
stopped at the intended errors for:

- `ETH_USE_EVENTS`.
- `SIO_USE_BUFFERING`.
- Serial-over-USB.

The XHAL lwIP binding retains the explicit event requirement established in
Milestone 1.

All 44 XML inputs in the main, serial-USB, XSNOR, and sensor generator roots
validate against `tools/ftl/schema/ccode/modules.xsd`. All four roots were
regenerated sequentially. A complete second pass produced no changes.

The following positive builds passed:

- Periodic standalone XHAL STM32G474.
- Tickless standalone XHAL STM32H563.
- XHAL ADC-GPT on STM32G474.
- XHAL USB CDC and serial-USB on STM32G474.
- XHAL MMC-SPI on STM32G474; this recipe also compiled all generated core
  drivers.
- XHAL WSPI-XSNOR-MFS on STM32H735.
- XHAL VIO host and client.
- Native XHAL lwIP on STM32H563 and the sandbox XHAL lwIP client.
- A temporary STM32G474 consumer of the editable XHAL LIS302DL body.

Exactly two OSAL cutover anchors remain in the Milestone 3 paths:

```text
os/xhal/include/hal.h: #include "osal.h"
os/xhal/src/hal.c:     osalInit()
```

They retain the temporary OSAL dependency required by current recipes and
unconverted ports and are explicitly removed in Milestone 6. Classic HAL and
board files are unchanged. All generated build and dependency directories
were removed after inspection.

## Milestone 4: Convert XHAL Ports

Convert the affected platform and driver-port files:

- IRQ handler, prologue, epilogue, and priority macros.
- Interrupt and system locking.
- ISR wakeups and scheduler operations.
- Timer mode, resolution, and frequency checks.
- Debug checks and halt hooks.

Preserve ChibiOS API context-class semantics exactly. `I`, `S`, `X`, and
unsuffixed APIs are not interchangeable.

### Exit Criteria

The following command finds no matches:

```sh
rg -n '\b(osal[A-Za-z0-9_]*|OSAL_[A-Za-z0-9_]*)\b' os/xhal/ports
```

Build a platform matrix covering:

- ARMv6-M: STM32C0, STM32G0, and STM32U0.
- ARMv7-M: STM32G4, STM32WL, STM32L4, and STM32H7.
- ARMv8-M: STM32H5, STM32U3, and STM32U5.
- VIO sandbox host and client.
- SYSTICKv1 and SYSTICKv2.
- DMA-heavy ADC, SPI, I2S, USB, and WSPI configurations.

Enable assertions and the state checker where supported.

### Verified Result: 2026-07-27

Milestone 4 passed on branch `chibios-vfs-dev`.

All 202 XHAL port files that referenced OSAL now use direct ChibiOS APIs.
IRQ declarations and framing use `CH_IRQ_*`; debug, locking, timer, thread,
and event operations use their corresponding `ch*` APIs. The conversion
preserves every original API context class, including `I`, `S`, `X`, and
unsuffixed calls.

Port timer checks now use `CH_CFG_ST_RESOLUTION`,
`CH_CFG_ST_FREQUENCY`, and `CH_CFG_ST_TIMEDELTA` directly. SYSTICKv1
supports periodic and tickless configurations. SYSTICKv2 explicitly requires
tickless mode, and VIO explicitly rejects tickless mode because its virtual
timer is periodic-only.

The port lexical gate passes with no OSAL identifiers or `osal.h` includes.
`git diff --check` also passes. Classic HAL and board files are unchanged.

The build matrix passed with the following coverage:

- ARMv6-M: STM32C0, STM32G0, and STM32U0.
- ARMv7-M: STM32G4, STM32WL, STM32L4, and STM32H7.
- ARMv8-M: STM32H5, STM32U3, and STM32U5.
- SYSTICKv1 in both periodic and tickless modes.
- SYSTICKv2 on STM32WL with a temporary valid 1024 Hz RTC-derived system
  tick.
- DMAv1, DMA3v1, BDMAv1, DMAv2, and MDMAv1.
- ADC, SPI, I2S, USB, WSPI/XSNOR/MFS, SIO, and native Ethernet/lwIP.
- VIO sandbox host and client.

The ADC, I2S, SPI, USB, and WSPI test configurations exercised the converted
driver interrupt and debug paths with checks, assertions, and the system
state checker enabled where supported. All generated build and dependency
directories were removed after the matrix completed.

## Milestone 5: Convert Configurations and Applications

- Change the 15 XHAL `xmcuconf.h.ftl` templates from `osalSysHalt()` to
  `chSysHalt()`.
- Regenerate the 45 affected XHAL `xmcuconf.h` files.
- Run updater scripts sequentially because they share temporary files.
- Convert direct OSAL calls in XHAL demos and tests.
- Update XHAL documentation and examples.
- Do not change classic `mcuconf` templates.

### Exit Criteria

- A second updater run produces no changes.
- No OSAL identifier remains in XHAL `xmcuconf` templates or generated
  configurations.
- No XHAL demo or test source uses OSAL.
- The RT-XHAL MULTI and XSHELL aggregate builds pass.

### Verified Result: 2026-07-28

Milestone 5 passed on branch `chibios-vfs-dev`.

The DMA error-hook defaults in all 15 XHAL `xmcuconf.h.ftl` templates now
call `chSysHalt()`. All 45 affected checked-in XHAL `xmcuconf.h`
configurations use the same direct ChibiOS hook:

- 39 configurations were regenerated by the 15 family updater scripts.
- Six STM32WL configurations were updated directly. The existing
  `update_xmcuconf_stm32wlxx.sh` points to an XHAL template that is not
  present in the tree, while the available STM32WL template is the classic
  `mcuconf.h.ftl` and is intentionally unchanged.

The updater scripts were run sequentially because they share their temporary
files. A complete second pass left all 48 checked-in XHAL configurations
byte-for-byte unchanged. No updater temporary files remain.

The three direct application OSAL call sites were converted:

- The MULTI and XSHELL buffered-SIO assertions use `chDbgAssert()`.
- The SPI test interrupt callback uses `chSysLockFromISR()` and
  `chSysUnlockFromISR()`.

No other OSAL references were found in XHAL demo or test sources and no
documentation or example required another conversion. The XHAL template,
generated-configuration, and application-source lexical gates pass. Classic
`mcuconf` templates are unchanged.

All eight RT-XHAL MULTI recipes and all eight RT-XHAL XSHELL recipes compiled
and linked. The STM32G474 SPI test also passed, covering the converted ISR
lock calls. All generated build and dependency directories were removed
after the matrix completed.

## Milestone 6: Final OSAL Cutover

Only after milestones 1 through 5 pass:

- Replace the `osal.h` inclusion in XHAL `hal.h` with `ch.h`.
- Remove `osalInit()` from `halInit()`.
- Remove `os/hal/osal/rt-nil/osal.mk` from all XHAL build recipes.
- Remove remaining XHAL assumptions about the OSAL include directory.
- Keep classic `os/hal/osal` intact.

Run the final XHAL lexical gate:

```sh
! rg -n \
  '\b(osal[A-Za-z0-9_]*|OSAL_[A-Za-z0-9_]*)\b|[<"]osal\.h[>"]' \
  os/xhal --glob '!OSAL_REMOVAL_*'
```

Verify the build recipes:

```sh
manifest=$(mktemp)
rg -l 'os/xhal/xhal\.mk' \
  --glob 'Makefile' --glob '*.make' > "$manifest"

! xargs -r rg -n 'os/hal/osal/rt-nil/osal\.mk' < "$manifest"
```

### Exit Criteria

- All 49 XHAL recipes compile and link.
- No XHAL dependency file references `osal.h`.
- No XHAL build produces or links `osal.o`.
- Tickless and periodic targets pass.
- Representative classic HAL MFS, lwIP, RT, and NIL targets still build.
- Boards are unchanged.

### Verified Result: 2026-07-28

Milestone 6 passed on branch `chibios-vfs-dev`.

The final cutover is complete:

- XHAL `hal.h` includes `ch.h` directly.
- `halInit()` no longer calls `osalInit()`.
- All 49 XHAL recipes omit `os/hal/osal/rt-nil/osal.mk`.
- The final XHAL lexical gate contains no OSAL identifier or `osal.h`
  inclusion outside the migration records.
- A fresh recipe discovery exactly matches the frozen 49-recipe manifest and
  none of those recipes contains an OSAL reference.

Removing the transitive OSAL include exposed three dependencies at the XHAL
boundary. They were resolved without restoring compatibility aliases:

- The XHAL STM32 EXTI and RCC make fragments now select their XHAL-local
  copies instead of classic HAL paths. The copies were brought to the current
  split point, including newer STM32 family guards and RCC helpers.
- The common test engine now uses direct `ch` debug, time, and sleep APIs.
- The four sandbox VIO host ISR paths now use `CH_IRQ_*` and `chSysLock*()`
  directly.

All 49 manifest recipes were cleaned, compiled, and linked successfully.
Coverage includes periodic and tickless system timers, RT drivers, XHAL MFS,
native and sandbox lwIP, XSHELL, VIO hosts and clients, and the complete
multi-platform and driver-test matrices.

Before cleanup, 3,244 generated dependency files and 3,689 build artifacts
were inspected. None referenced `osal.h` or `os/hal/osal`, produced
`osal.o`, or contained an `osalInit`, `osal.c`, or `osal.o` link/listing
reference.

The following representative classic HAL builds also passed and continued to
compile their unchanged `osal.c`:

| Coverage | Target |
| --- | --- |
| MFS | EFL-MFS STM32WL55 |
| Networking | lwIP/HTTP STM32H563 |
| RT | RT STM32WL55 |
| NIL | NIL STM32G474 |

Classic `os/hal/osal` and `os/hal/boards` contain no changed paths. All
generated build and dependency directories were removed after validation.

## Milestone 7: Runtime Regression

Run representative hardware tests:

- STM32G474 ADC-GPT, DAC-GPT, SPI, I2S, and WDG.
- STM32H563 timer, TRNG, and tickless scheduling.
- USB CDC enumeration and sustained traffic.
- EFL-MFS and WSPI-XSNOR-MFS complete suites.
- Ethernet RX/TX, link events, DHCP or static setup, and HTTP.
- VIO sandbox host/client communication.
- Base-driver `drvLock()` and `drvUnlock()` behavior using RT mutexes.

### Exit Criteria

- No assertion, state-checker, or context-check failure occurs.
- Interrupt-driven waits and callbacks behave unchanged.
- MFS data survives remount and garbage collection.
- lwIP survives sustained RX/TX and repeated link transitions.
- The final worktree contains no generated build artifacts.

## Final Acceptance Checklist

- [x] XHAL implementation files contain no OSAL identifiers or includes;
      migration records are excluded from the lexical gate.
- [x] XHAL recipes do not include `osal.mk`.
- [x] XHAL MFS, lwIP, and HTTPD bindings are independently owned.
- [x] The base driver encapsulates RT mutex and NIL semaphore selection.
- [x] Event-dependent modules require ChibiOS events.
- [x] XHAL owns its tickless system-timer binding.
- [x] XHAL selects the direct-Ch common OOP backend.
- [x] The sandbox pure-virtual handler uses `chSysHalt()`.
- [x] All XHAL generator inputs validate and regeneration is idempotent.
- [x] All 49 XHAL recipes compile and link.
- [ ] Representative runtime tests pass.
- [x] Representative classic HAL builds still pass.
- [x] Boards remain in place and unchanged.
- [x] NIL-XHAL build coverage is tracked as post-migration work.
