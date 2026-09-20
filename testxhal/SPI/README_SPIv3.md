# XHAL STM32 SPIv3 tests

The SPIv3 port covers STM32H7 with DMA1/2 for SPI1..5 and BDMA for SPI6.
Both H7 platform makefiles include the driver; shared SPI interrupts use the
same `.inc` dispatchers as the other STM32 XHAL ports.

## Build and host regression

From `testxhal/SPI`, with the ARM GNU toolchain on `PATH`:

```sh
make -f make/stm32h743zi_nucleo144.make -j4 USE_COPT=-Werror
make -f make/stm32h735ig_discovery.make -j4 USE_COPT=-Werror
make -f make/stm32h7a3ziq_nucleo144.make -j4 USE_COPT=-Werror
make -C host/spiv3
```

Repeat each command with `clean` after testing. Clean before changing build
flags; `USE_SMART_BUILD=no` also exercises unconditional driver inclusion.

All six SPI instances are enabled for compile/link coverage. The board
fixtures run the existing test application on SPI1 only, using PA5 for SCK,
PA6 for MISO, PB5 for MOSI (all AF5), and PD14 as GPIO chip select. These are
the classic HAL H743 test pin choices; check board routing before wiring
other boards. A MOSI-to-MISO jumper enables loopback observation. No second
SPI instance is wired as a slave. Hardware validation remains pending.

The host suite compiles the real LLD against modeled SPI/DMA registers.
It covers DMA-only, BDMA-only and mixed builds, user configuration tables,
both chip-select representations, allocation rollback, live configuration,
frame widths/counts, remaining frames, circular cancellation, combined
error/completion interrupts, overrun, and stop/restart cleanup. It does not
simulate real DMA traffic, interrupt timing or cache behavior.

## Configuration and memory

Wire widths are 4..32 bits on SPI1..3 and 4..16 bits on SPI4..6, as described
in the ST [H743 reference manual](https://www.st.com/resource/en/reference_manual/dm00314099.pdf)
and [H7A3 datasheet](https://www.st.com/resource/en/datasheet/stm32h7a3vi.pdf).
`SPI_MODE_FSIZE` must match the rounded-up memory element size: 8, 16 or
32 bits. Transfer counts are in frames, from 1 to 65535, and must be even
in circular mode.

DMA buffers must be reachable by the selected DMA engine and managed for
cache coherency by the application. SPI6 buffers must be in SRAM accessible
to BDMA (SRAM4 on these targets), not ordinary AXI SRAM or DTCM. The driver
places SPID6, including its internal sink/filler words, in `.ram4_clear`
by default. Custom linker scripts or `SPI_SPID6_MEMORY` overrides must
preserve BDMA accessibility. The internal TX filler is flushed at start;
application buffers still require their own cache maintenance.

Interrupt priorities now use `STM32_IRQ_SPIx_PRIORITY`. The global
`xmcuconf` updaters migrate old `STM32_SPI_SPIx_IRQ_PRIORITY` values,
with an explicitly supplied new setting taking precedence.

The default DMA error hook halts, as in the other STM32 SPI ports. A returning
`STM32_SPI_DMA_ERROR_HOOK` enables XHAL error notification; the caller must
then stop the failed transfer before starting another one.
