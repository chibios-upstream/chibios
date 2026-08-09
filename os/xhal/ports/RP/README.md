# RP2040 / RP2350 XHAL port

XHAL port for the Raspberry Pi RP2040 (Cortex-M0+) and RP2350 (Cortex-M33,
Hazard3 RISC-V planned as a separate milestone). The port is a twin of the
classic HAL port at `os/hal/ports/RP`, adapted to the XHAL driver contract
(no OSAL, `drvStart()`/`drvStop()` lifecycle, `setcfg`/`selcfg` configuration
model, callback-driver semantics).

## Fork provenance and drift policy

Files below were forked from `os/hal/ports/RP` and converted; the classic
tree remains the reference for hardware behavior. When a fix lands in a
classic-RP file listed here, mirror it (re-converted) or record the
intentional divergence in this table.

| File group | Forked from (commit) | Notes |
|---|---|---|
| `rp_bootrom.c/.h` | `0db31951d5` | debug macros converted |
| `LLD/GPIOv1` (PAL) | `0db31951d5` | event macros align with XHAL `hal_pal.h` |
| `LLD/TIMERv1` (ST) | `0db31951d5` | `CH_CFG_ST_TIMEDELTA` replaces the OSAL ST mode selection |
| `RP2040/`, `RP2350/` family files | `0db31951d5` | UART shared-vector includes deferred to the SIO stage |

Intentional divergences from the classic tree:

- `hal_lld_init()` calls `stBind()` unconditionally (classic gates it to
  free running mode, leaving the periodic SysTick path configured by
  `st_lld_bind()` but never started).
- The ST and PAL headers validate their IRQ priority knobs with
  `CH_IRQ_IS_VALID_KERNEL_PRIORITY` (classic performs no checks; these
  handlers take kernel locks, so architectural-only validity would be
  insufficient).

Shared with the classic tree (referenced in place, not forked): startup
files and kernel ports (`os/common/startup/…`, `os/common/ports/…`), board
files (`os/hal/boards/RP_PICO_RP2040`, `RP_PICO2_RP2350`), and
`os/hal/ports/RP/rp_uf2_image.mk`.
