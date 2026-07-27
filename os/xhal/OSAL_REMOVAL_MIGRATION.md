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
system timer moves into XHAL. Classic HAL keeps its existing OSAL copy.

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
  os/xhal
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

- [ ] XHAL contains no OSAL identifiers or includes.
- [ ] XHAL recipes do not include `osal.mk`.
- [ ] XHAL MFS, lwIP, and HTTPD bindings are independently owned.
- [ ] The base driver encapsulates RT mutex and NIL semaphore selection.
- [ ] Event-dependent modules require ChibiOS events.
- [ ] XHAL owns its tickless system-timer binding.
- [ ] XHAL selects the direct-Ch common OOP backend.
- [ ] The sandbox pure-virtual handler uses `chSysHalt()`.
- [ ] All XHAL generator inputs validate and regeneration is idempotent.
- [ ] All 49 XHAL recipes compile and link.
- [ ] Representative runtime tests pass.
- [ ] Representative classic HAL builds still pass.
- [ ] Boards remain in place and unchanged.
- [ ] NIL-XHAL build coverage is tracked as post-migration work.
