# Repository Guidelines

Paths and commands below are relative to the directory containing this file, the Git repository root.

## Project Structure & Module Organization

- Operating system modules are under `./os`.
- Core STM32 HAL sources live under `os/hal/ports/STM32`.
- XHAL sources live under `os/xhal`; generated driver interfaces and common code come from XML descriptions under `os/xhal/codegen`.
- Shared STM32 infrastructure (RCC helpers, registry, limits) sits alongside in the same tree, while board examples and demos are under `demos/`.
- Tests and validation harnesses reside in the `test*` directories; use them to verify changes before submitting.
- Configuration templates and regeneration helpers are under `tools/ftl` and `tools/updater`.

## Coding Style & Naming Conventions

- Follow existing style patterns: 2-space indentation for C sources, and `STM32_*` macro naming for register constants (see `hal_lld.h`/`stm32_rcc.h`).
- Keep file-local helpers `static`; expose functions only where external linkage is required, such as public APIs, cross-file internal interfaces, and interrupt handlers.
- Match the surrounding formatting by hand; do not introduce clang-format changes.
- General rule: line endings must be LF, except for externally provided files (non-ChibiOS copyright).
- In C functions, keep automatic variable declarations grouped at the start of the block and separate the declaration block from executable statements with an empty line.

## API Context & Interrupt Handling

- API context contracts must drive design decisions. Unless documented otherwise, no suffix means normal, unlocked thread context; `X` permits any context; `S` requires locked thread context (S-Locked); `I` requires I-Locked or S-Locked context. Being in an ISR alone does not satisfy the `I`-class contract. See `doc/build/common/src/concepts.dox` for the context definitions.
- Only place calls in fastcall/ISR paths when their documented context and locking requirements are satisfied.
- Driver callbacks must follow their context contract. Generic XHAL driver callbacks are invoked from ISR context and out of system locks; do not call them from thread-context APIs, and do not invoke them while holding `osalSysLock*()`/`chSysLock*()`. If an ISR helper must wake waiters, lock only around the `I`-class wakeup/VRQ operations, unlock, then invoke the callback.
- While debugging new code, prefer enabling assertions, parameter checks, and the state checker in `chconf.h`. In ChibiOS, contract violations often halt early with those options enabled, which is desirable for catching context and state misuse quickly.
- STM32 shared IRQ handlers follow the `.inc` pattern used by SPI/USART/RTC: platform `stm32_isr.c` includes the right `.inc`, vector names and numbers belong in `stm32_isr.h`, and the corresponding `driver.mk` must export the include directory that contains the shared `.inc` files.
- For STM32 vectors shared by multiple peripherals, choose one peripheral as the primary owner and treat the others as secondaries. The shared `.inc` filename and the IRQ priority macro must name all participating peripherals, while the primary peripheral defines the overall handler structure and inclusion point.
- For primary/secondary shared vectors, keep the ISR body high level. The `.inc` should dispatch to driver service entry points, while peripheral-specific flag decoding and per-channel/per-instance detail should live inside the driver-side service functions unless the hardware has a dedicated vector and the established pattern already keeps that logic in the `.inc`.

## XHAL Architecture Notes

- XHAL stateful drivers derive from `hal_base_driver`; lifecycle, configuration selection, mutual exclusion, and registry integration should be exposed through the base-driver API when the semantics are common.
- `drvStart()` and `drvStop()` are the XHAL lifecycle entry points, both called from thread context. Driver-specific `xxxStart()`/`xxxStop()` wrappers should not be reintroduced unless there is a deliberate compatibility layer outside the generated XHAL driver API.
- `drvStart(ip, config)` performs initial hardware enable and configuration from `HAL_DRV_STATE_STOP`; `drvStart(ip, NULL)` uses configuration zero. While already `HAL_DRV_STATE_READY`, passing `NULL` or the current configuration pointer is a no-op; a different non-`NULL` configuration requests live reconfiguration through the configuration hook. See `os/xhal/codegen/hal_base_driver.xml` for the lifecycle contract.
- Live configuration APIs such as `drvSetCfgX()` and `drvSelectCfgX()` are only valid in `HAL_DRV_STATE_READY`. Use the `config` parameter of `drvStart()` for initial configuration because hardware may be reset or clock-gated before start.
- Initial start enables resources and configures hardware through the start hook; the base driver enters `HAL_DRV_STATE_STARTING` before invoking it. Live reconfiguration uses the configuration hook while READY and changes settings without tearing down resources.
- Synchronization is driver-specific in XHAL. Do not add generic synchronization methods to `hal_base_driver`; expose waits at driver level with names or parameters that identify the wait condition, such as SIO RX/TX waits or ADC state synchronization.
- XHAL synchronization feature switches use the `XXX_USE_SYNCHRONIZATION` naming convention. Do not introduce new `XXX_USE_WAIT` settings for driver synchronization; keep `XXX_USE_WAIT` only for legacy or non-driver-event semantics such as PAL wait support.
- Some waits are intentionally not generic driver synchronization points. Examples include SIO directional waits (`RX`, `RX idle`, `TX space`, `TX end`), USB endpoint-zero setup waits, ETH/CAN descriptor or mailbox waits, ICU capture waits, and flash erase polling.
- SB VIO host code must use public XHAL APIs and must not call XHAL low-level driver entry points directly. Fastcall handlers are restricted to `X`-class APIs and lock-free state queries; operations requiring thread context, blocking behavior, or non-`X` APIs belong in the syscall path.
- SB VIO sandbox LLDs should avoid requiring sandbox-visible hardware configuration structures. Prefer opaque configuration indices and VIO query operations for runtime state needed by sandbox code.

## Generated Code & Configuration

- Modify XML-generated sources and headers only by regeneration: update the XML and/or generator templates, regenerate, and include the affected checked-in outputs in the same change. Verify that a second regeneration produces no additional changes.
- XML driver descriptions must include the documentation elements expected by code generation; otherwise generated Doxygen comments disappear and regeneration becomes lossy.
- After editing codegen or board XML (`os/xhal/codegen`, `os/vfs/codegen`, `tools/ftl/xml`, etc.), validate it before regenerating or committing. Map the `.../schema/...` path in `xsi:noNamespaceSchemaLocation` to the local schema under `tools/ftl/schema/` and run `xmllint --noout --noent --schema <local.xsd> <doc.xml>`.
- If validation fails, check both the XML and the schema. Do not loosen the schema blindly; when correcting board schemas, take device subtypes from the ST CMSIS headers and GPIO port sets from the in-tree `stm32_registry.h`. Re-validate after each fix because further errors may become visible.
- For updater-managed configuration headers such as `mcuconf.h`, `xmcuconf.h`, `halconf.h`, and `xhalconf.h`, edit existing demo/test-specific values in the local headers. These changes do not require changing global template defaults.
- When adding, renaming, or removing configuration options, or changing shared defaults, update the corresponding `.ftl` templates and any necessary updater logic, then regenerate the affected configuration headers. For XHAL settings, use `tools/ftl/processors/conf/xhalconf/xhalconf.h.ftl` and `tools/updater/update_xhalconf.sh`; for MCU settings, use the matching MCU template and updater.
- Run `tools/updater` scripts from that directory. They share temporary files, so regenerate sequentially. Updaters read existing configuration values; review the outputs to ensure migrations preserve intended local settings.
- Each demo or test must contain its own configuration files in its local `cfg/` directory. Do not use cross-demo or cross-test include shims for `chconf.h`, `halconf.h`, `xhalconf.h`, `mcuconf.h`, `xmcuconf.h`, or similar configuration headers.

XHAL validation and regeneration example, starting from the repository root (requires `xmllint` and `fmpp`):

```sh
(
  cd os/xhal/codegen || exit 1
  xmllint --noout --noent --schema ../../../tools/ftl/schema/ccode/modules.xsd modules.xml || exit 1
  fmpp -C config.fmpp
)
```

Configuration migration example for one SPI test, starting from the repository root:

```sh
(
  cd tools/updater || exit 1
  bash update_xhalconf.sh ../../testxhal/SPI/cfg/stm32g474re_nucleo64/xhalconf.h
)
```

## Build & Test Hygiene

- Choose host regressions and target demo/test builds relevant to the changed code. Report which checks ran and any hardware validation that remains unperformed.
- Clean task-generated build outputs after testing unless otherwise specified; preserve pre-existing user-owned artifacts.
- Eclipse metadata policy: keep `.project` and `.cproject` when useful for multi-target projects, but do not add generated `debug/*.launch` files unless there is a specific request to version them.

Example SPI HLD host regression, run from the repository root; no board is required:

```sh
make -C testxhal/SPI/host/hld
make -C testxhal/SPI/host/hld clean
```

## Repository

- Repository is git, hosted at github.com/chibios-upstream/chibios. Use git commands; there is no `.svn` directory.
- The repository is self-contained; `tools/ftl` is regular tracked content, not a submodule. Do not use `git -C tools/ftl ...` as a separate repository workflow.
- Branch model: `master` plus `stable-*` maintenance branches; releases are `ver_*` tags. Target PRs at `master`; maintenance branches receive maintainer-selected backports. Do not force-push or move tags.
- Keep notes and Markdown (`.md`) files on development branches by default. Exclude their additions and edits from commits or PRs targeting `master` or `stable-*` unless explicitly specified otherwise.
- Preserve unrelated working-tree changes. Ask before overwriting or deleting existing user-owned untracked files unless already authorized. Creating files needed for the requested task and generating or cleaning task-owned build outputs do not require additional confirmation.
- Generated `build/` and `.dep/` outputs are git-ignored; avoid a blanket `git add -A` in freshly built demo/test trees and stage only intended files.
