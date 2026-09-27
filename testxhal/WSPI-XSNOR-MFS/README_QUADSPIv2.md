# STM32H7 QUADSPIv2

This XHAL port covers the MDMA-backed QUADSPI peripheral selected by
`STM32H7xx/platform.mk`. H7 variants with OCTOSPI retain their existing LLD.
Classic HAL is unchanged.

Enable `HAL_USE_WSPI` and `STM32_WSPI_USE_QUADSPI1`. Use the common
`drvStart/Stop` lifecycle and `hal_wspi_config_t.dcr`. A NULL initial
configuration uses `STM32_WSPI_DEFAULT_DCR` (zero by default); real flash
devices normally need an explicit FSIZE and CSHT. Reserved DCR bits are
rejected; live configuration is only allowed while idle.

The shared QUADSPI include owns the vector. QUADSPI and MDMA must use the
same interrupt priority; the existing H743 configuration template aliases
`STM32_IRQ_QUADSPI1_PRIORITY` to `STM32_IRQ_MDMA_PRIORITY`. H7's
`stm32_isr.h` rejects mismatches when QUADSPI is enabled, keeping this
MDMA-specific constraint out of the shared include. MDMA arbitration
priority remains independently
selectable with `STM32_WSPI_QUADSPI1_MDMA_PRIORITY`.

The port provides commands, indirect reads/writes and memory mapping.
Status polling uses the existing HLD software fallback, not hardware polling.
Both peripheral TCF and MDMA CTCIF must arrive before a data transfer completes,
in either order. Errors and stop quiesce hardware; cleanup precedes notification.
Callbacks run in ISR context outside system locks. The default MDMA error
hook halts, as in the other MDMA-backed WSPI driver; a returning hook allows
the normal error callback/waiter notification.

Register behavior was checked against RM0433 Rev 7: MDMA sections
14.3.13–14.3.15 and 14.5.2, and QUADSPI sections 23.3.5, 23.3.11,
23.3.15 and 23.5.1. In particular, H7 QUADSPI CR bit 2 is reserved despite
the compatibility `QUADSPI_CR_DMAEN` definition in CMSIS. The port leaves
it clear. The existing command-only alternate-byte erratum workaround is
retained.

Limits:

- One MDMA block per transfer: 1–131071 bytes, enforced by a debug assertion.
- Buffers must be MDMA-accessible through the AXI bus, not TCM. The application
  must maintain cache coherency or use a properly configured noncacheable area.
- Half-cycle receive sampling defaults on and is configurable using the
  existing `STM32_WSPI_SET_CR_SSHIFT` setting (SDR only).
- Abort/MDMA-disable handshakes retain unbounded hardware waits; software
  polling timeouts do not individually bound an indirect read.

## Native tests

From this directory:

```sh
make -C host/quadspiv2 -j4
make -C host/quadspiv2 clean
```

See [the harness notes](host/quadspiv2/README.md) for coverage and limitations.

## NUCLEO-H743ZI read-only fixture

```sh
make -f make/stm32h743zi_nucleo144.make -j8
make -f make/stm32h743zi_nucleo144.make clean
```

This target runs `quadspi_v2.c`, **not** the MFS erase/program test. It requires
an external 3.3 V SPI NOR supporting 9F (JEDEC ID), 05 (status, WIP bit 0) and
03 (three-byte-address read), initially in ordinary SPI mode. It does not
change flash contents. Wire CLK to PF10, CS to PG6, IO0/MOSI to PD11 and
IO1/MISO to PF9, plus common ground and the appropriate supply. Hold the
flash's WP/HOLD inputs inactive as required by its datasheet. The Nucleo has
no onboard NOR; do not run the fixture without appropriate hardware.

The serial clock is 25 MHz with this configuration. The fixture checks JEDEC
ID, software status polling, a 64-byte indirect read, matching memory-mapped
data, unmap/read recovery, and stop/restart. The DMA buffers are placed at
0x30040000 in SRAM3; the target's noncacheable MPU window matches the linker
section. Assertions and the RTOS state checker are enabled.

Debugger observables:

- `wspi_test_result == 0x13579BDF`: passed.
- `wspi_test_result == 0x2468ACE0`: failed; inspect `wspi_test_failure`.
- `wspi_test_stage`: progress (8 at successful completion).

Eclipse project `XHAL-WSPI-XSNOR-MFS` includes this target. It is build-verified,
**not board-tested**. Flash programming, DDR timing, electrical behavior and
real MDMA request timing remain hardware-validation work.
