# OCTOSPIv3 instance regression

Run `make -C testxhal/WSPI-XSNOR-MFS/host/octospiv3` from the repository root;
run the same command with `clean` afterward.

The harness compiles the real XHAL OCTOSPIv3 source/header and shared OCTOSPI
IRQ handlers against STM32U575 CMSIS register layouts with mocked DMA, RCC,
NVIC, and driver callbacks. Five configurations cover OCTOSPI1 only, OCTOSPI2
only, both, a device without OCTOSPI2, and synchronization disabled.

Checks include independent prescalers, sampling settings, DMA requests and
priorities; DMA allocation failure/retry; matching peripheral/DMA IRQ priorities;
IRQ dispatch; send/receive register setup; per-instance polling intervals and
memory windows; stop/restart and shared I/O-manager preservation.

This is not a hardware test. Pin routing, flash commands, bus timing and
self-clearing ABORT behavior are not emulated.
