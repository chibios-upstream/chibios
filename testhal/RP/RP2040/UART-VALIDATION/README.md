# RP2040 UART-VALIDATION

This test application validates RP2040 UART driver integration using the HAL UART API.

## Purpose

- Enable and build the RP UART low level driver path.
- Configure UART0 on GPIO0/GPIO1.
- Exercise UART transmission periodically from `UARTD0`.

## Build

From repository root:

```sh
cd testhal/RP/RP2040/UART-VALIDATION
make clean
make -j$(nproc)
```

Produced artifacts are in the local `build/` directory (`ch.elf`, `ch.hex`, `ch.bin`).

## Configuration Notes

- HAL UART is enabled in [cfg/halconf.h](cfg/halconf.h).
- HAL SIO is disabled in [cfg/halconf.h](cfg/halconf.h) to avoid UART ownership overlap.
- UART0 is assigned to UART driver in [cfg/mcuconf.h](cfg/mcuconf.h).

## Runtime Notes

- Default UART settings: 115200 8N1.
- TX pin: GPIO0, RX pin: GPIO1.
- Board LED toggles on successful periodic transmission.
