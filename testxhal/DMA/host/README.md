# XHAL STM32 DMA callback filtering

Run `make -C testxhal/DMA/host` on Linux, then `make -C testxhal/DMA/host clean`.

The harness compiles the actual DMA implementations and their headers against
in-tree CMSIS device definitions. Peripheral register pages are mapped into the
host process; RTOS locks, IRQ entry/exit, NVIC and clocks are mocked. No board is
accessed. AddressSanitizer and UndefinedBehaviorSanitizer are enabled.

Coverage includes every event/enable combination on every channel/stream for
DMAv1 (G4 dedicated and G0 shared vectors), DMAv2, BDMA, DMA3 (H5 and U5),
and MDMA. It checks:

- Only enabled pending events reach callbacks, independently of channel EN.
- Empty, disabled-only and status-only snapshots do not cause callbacks.
- DMAv2 FIFO errors use FCR.FEIE independently of CR interrupt enables.
- DMA3 IDLEF/FIFOL and MDMA CRQA/CESR retain their existing encoding.
- All captured raw flags are acknowledged before callbacks, including flags
  filtered out of the callback argument.
- Shared DMAv1 handlers do not clear flags belonging to polling neighbors.
- NULL callbacks remain safe and callback-side changes are not overwritten.

W1C side effects and hardware IRQ generation are not emulated: tests inspect
writes to the actual clear-register addresses and invoke actual IRQ handlers.
They do not validate bus timing or replace on-board testing.
