# QUADSPIv2 native regression

`make` builds the real XHAL WSPI frontend, QUADSPIv2 LLD and shared IRQ
include against bundled H743 CMSIS/MDMA definitions, with ASan and UBSan.
Variants cover synchronization disabled, the command-only workaround
disabled and half-cycle sample shifting disabled. A negative preprocessing
test checks that differing QUADSPI/MDMA IRQ priorities are rejected.
Header-only cases verify that H7's `stm32_isr.h` enforces this before
any LLD is included, while disabled WSPI, disabled QUADSPI and devices
without QUADSPI are not constrained.

Coverage:

- Initial/default/live configuration, reserved-bit rejection, allocation
  failure/retry, clock lifecycle and shared vector setup/teardown.
- Command-only and addressed commands; address, alternate and dummy fields.
- Byte-sized MDMA transfers at lengths 1, 3, 32, 33 and 131071.
- Both IRQ completion orders for send and receive, no early/double callback,
  ignored buffer/block IRQ sources, and size-limit assertions.
- Peripheral and MDMA errors, simultaneous error/completion flags,
  cleanup before notification and callback lock/state contracts.
- Stop during receive, memory-map/unmap/stop, abort completion cleanup,
  immediate subsequent transfer, and resource reuse.
- Synchronous completion/error/stop wakeups and software status-poll
  match, mismatch timeout and error return.

Hardware and RTOS scheduling are mocked. MDMA programming uses the actual
helper macros; allocation and disable handshakes are modeled. QUADSPI abort
progress is emulated by a POSIX timer with a watchdog. These tests do not
simulate a FIFO, bus/cache behavior or real peripheral/DMA timing and are
not a substitute for board validation.
