# XHAL STM32 SPIv4 tests

The SPIv4 port uses GPDMA on STM32H5, STM32U3 and STM32U5. These targets
exercise the existing SPI test application with SPI1; other available SPI
instances are enabled for compile/link coverage only. There is no second SPI
instance wired as a slave in these fixtures.

From `testxhal/SPI`, build with the ARM GNU toolchain on `PATH`:

```sh
make -f make/stm32h563zi_nucleo144.make -j4
make -f make/stm32u385rg_nucleo64.make -j4
make -f make/stm32u575zi_nucleo144.make -j4
```

Repeat each command with `clean` after testing. To check non-smart-build
integration, clean first and add `USE_SMART_BUILD=no USE_COPT=-Werror`.

## Hardware setup

The fixtures configure these pins. SCK, MISO and MOSI use alternate function 5;
CS is a GPIO output, initially high.

| Target | SCK | MISO | MOSI | CS |
| --- | --- | --- | --- | --- |
| H563ZI Nucleo-144 | PA5 | PG9 | PB5 | PD14 |
| U385RG Nucleo-64 | PA5 | PA6 | PA7 | PB6 |
| U575ZI Nucleo-144 | PA5 | PA6 | PA7 | PB6 |

Use a MOSI-to-MISO jumper for loopback observation and a logic analyzer to
check clocks and CS. The user button advances the existing test application
through transfer-size, circular-transfer and bus-contention phases. Board
routing, signal integrity and real slave operation still require hardware
validation; a successful build is not a hardware test.

## Host regression

```sh
make -C host
make -C host clean
```

This compiles the actual LLD against a small SPI/GPDMA model, using the in-tree
STM32H563 register definitions. Variants cover built-in defaults, a user
configuration table, both chip-select representations, and 16/32-bit defaults.
Tests cover allocation failure rollback, resource-preserving reconfiguration,
4..32-bit wire widths, transfer-count validation, remaining frame counts,
stop/restart cleanup, circular callback cancellation, RX/TX DMA errors,
overrun handling and polled-transfer DMA-setting restoration.

The model does not simulate bus traffic, interrupt timing or GPDMA hardware.

## Configuration constraints

`SPI_MODE_FSIZE` must match the storage element required by `CFG1.DSIZE`:
8 bits for 4..8-bit wire frames, 16 bits for 9..16, and 32 bits for 17..32.
Reduced-feature SPI instances accept only 8- or 16-bit wire frames. Packed or
independently sized memory/wire transfers and 64-bit elements are unsupported.
Transfer lengths are in frames, must fit the GPDMA 65535-byte block limit,
and must be even in circular mode.

The default DMA error hook halts the system, as in the other STM32 SPI ports.
Override `STM32_SPI_DMA_ERROR_HOOK` with a returning hook to use the SPI error
callback and synchronization error path; the host tests use such a hook.
