# Changelog

All notable changes to ChibiOS 21.11.x are documented in this file. Entries
reference the GitHub pull request implementing the change where available,
for a fix backported from `master` the `master` pull request; older entries
reference the bugs of the former SourceForge tracker.

## [Unreleased]

### Added

- [HAL] Safety counter API: `halSftGetCounterX()`,
  `halSftGetCounterFrequencyX()`, `halSftIsCounterWithinX()`,
  `HAL_SFT_US2CNT()`/`HAL_SFT_NS2CNT()`,
  `halSftDelayX()`/`halSftDelayFromX()`. The register waits now use it, so on
  STM32C0, G0 and U0 a wait crossing the 16-bit counter wrap no longer times
  out right away; the STM32 OTGv1 driver uses it instead of the OS realtime
  counter ([#435](https://github.com/chibios-upstream/chibios/pull/435)).
- [STM32] SYSTICKv1 free running mode gained `STM32_ST_FREQUENCY_TOLERANCE`,
  the allowed ST tick deviation in per-mille (default 0 = exact divisor
  required, unchanged behavior), letting devices whose clock tree cannot
  produce an exact multiple (e.g. STM32U0/U3 with MSI feeding the PLL) run
  without a clock-rounding system halt
  ([#17](https://github.com/chibios-upstream/chibios/pull/17)). The STM32U0
  and STM32U3 device configurations set the tolerance to 0.5%
  ([#18](https://github.com/chibios-upstream/chibios/pull/18)).
- [STM32] STM32G4xx: added FSMC RCC macros, IRQ vector definitions and
  registry switch for the FMC-capable devices (G473/G483/G474/G484)
  ([#14](https://github.com/chibios-upstream/chibios/pull/14)).
- [BUILD] Python port of the source-code style checker,
  `tools/style/stylecheck.py` with per-subsystem `py_style_*.sh` wrappers,
  functionally matching `stylecheck.pl` without two of its false positives:
  `#endif /* defined(X) */` guard comments and operators or commas inside
  string literals
  ([#28](https://github.com/chibios-upstream/chibios/pull/28)).

### Changed

- [EXT] FatFs R0.14b gets the fixes for CVE-2026-6682, CVE-2026-6683 and
  CVE-2026-6687 from ChaN's R0.16 patch-2: a FAT size of 0x200000 sectors or
  more and exFAT volumes with fewer than 256 clusters are rejected at mount,
  `f_getlabel()` copies at most 11 characters of an exFAT volume label.
  `ffconf.h` and the API are unchanged
  ([#697](https://github.com/chibios-upstream/chibios/pull/697),
  [#706](https://github.com/chibios-upstream/chibios/pull/706)).
- [VFS] The VFS subsystem was updated to the current generation: file nodes
  expose a random-access stream (`random_stream_i`) rather than a plain
  sequential stream, the driver code-generation models were refreshed, and a
  read-only ROMFS driver (optionally packbits-compressed) was added, disabled
  by default (`VFS_CFG_ENABLE_DRV_ROMFS`). The OOP framework gained the
  `oop_random_stream` base class and os/common/utils the packbits helper
  ([#32](https://github.com/chibios-upstream/chibios/pull/32)).
- [VFS] LittleFS driver: removing, renaming or replacing by a rename an open
  file, and removing or replacing an open directory, return `EBUSY` instead of
  detaching the open handles and losing their writes
  ([#660](https://github.com/chibios-upstream/chibios/pull/660)).
- [HAL] Coding-style cleanup (whitespace, spacing and comment formatting), no
  functional change: HAL sources
  ([#20](https://github.com/chibios-upstream/chibios/pull/20)), os/common
  sources ([#27](https://github.com/chibios-upstream/chibios/pull/27)),
  os/various sources
  ([#29](https://github.com/chibios-upstream/chibios/pull/29)) and os/hal/lib
  sources ([#30](https://github.com/chibios-upstream/chibios/pull/30)).
- [BUILD] The FTL code generation libraries use the current code-first syntax
  of FreeMarker Codegen (a single generic `end` closer, a scope keyword on
  assignments) and parse with its current version
  ([#292](https://github.com/chibios-upstream/chibios/pull/292)).

### Fixed

- [HAL] STM32 MACv2 and FDCANv2: the units are no longer reset before their
  clocks are enabled; on the dual-core STM32H7 devices the system halted in
  `halInit()` with "peripherals not allocated" when the debug asserts were
  enabled ([#701](https://github.com/chibios-upstream/chibios/pull/701)).
- [STM32] STM32H5: the TIM1 update vector number is named
  `STM32_TIM1_UP_NUMBER`, as expected by the TIM drivers, so PWM, ICU and GPT
  on TIM1 build
  ([#664](https://github.com/chibios-upstream/chibios/pull/664)).
- [VFS] LittleFS driver: independent writable opens of the same file were
  accepted and the data of one writer was lost; an open conflicting with a
  writer, or a writable open of an open file, now returns `EBUSY`
  ([#648](https://github.com/chibios-upstream/chibios/pull/648)).
- [VFS] Root: renaming the current directory or one of its ancestors, or
  removing the current directory, left a stale current directory; through the
  root these operations now return `EBUSY`
  ([#646](https://github.com/chibios-upstream/chibios/pull/646)).
- [VFS] FatFS driver: a write on a full volume returned 0 instead of `ENOSPC`,
  `mkdir()` on a full volume and `rmdir()` of a non-empty directory returned
  `EACCES` instead of `ENOSPC` and `ENOTEMPTY`
  ([#643](https://github.com/chibios-upstream/chibios/pull/643)).
- [HAL] STM32 USARTv1, USARTv2 and USARTv3 serial drivers: data written from
  an interrupt with a higher priority than the USART's while the USART
  interrupt was being served could stay in the output queue until a later
  write, the `CR1` bits set by its notification were overwritten
  ([#626](https://github.com/chibios-upstream/chibios/pull/626)).
- [HAL] STM32 USARTv2 and USARTv3 serial drivers: when a byte was received in
  the same interrupt in which the transmission completed, the end of
  transmission was missed, `CHN_TRANSMISSION_END` was not raised and `TCIE`
  stayed set with no further interrupt to clear it
  ([#622](https://github.com/chibios-upstream/chibios/pull/622)).
- [VFS] With a `VFS_CFG_PATHLEN_MAX` value giving buffers that are not a
  multiple of the pointer size, 128 for example, the shared path buffers were
  misaligned: with checks enabled the system halted in `vfsInit()`, without
  checks the pool used misaligned pointers
  ([#609](https://github.com/chibios-upstream/chibios/pull/609)).
- [HAL] AVR ATmega SYSTICKv1: the driver did not build on the ATmega8, 16, 32,
  64, 162 and 8515, periodic mode now uses Timer 2 on the ATmega8. The
  ATmega64 Timer 0 ticked 2, 4 or 8 times too fast at CLK/64, CLK/256 and
  CLK/1024, and on the devices sharing `TIMSK` between the timers, such as the
  ATmega128, both tick modes cleared the interrupt enables of the other timers
  ([#615](https://github.com/chibios-upstream/chibios/pull/615)).
- [AVR] RT and NIL did not assemble on the devices without `CALL` (avr2, avr25
  and avr4 cores, such as the ATmega8, ATmega88 and ATmega8515): the thread
  start now calls `chThdExit()` with `RCALL` there
  ([#614](https://github.com/chibios-upstream/chibios/pull/614)).
- [STM32] STM32H7A3/H7B3/H7B0: the APB3 and APB4 clocks were named
  `STM32_CDPCLK` and `STM32_SRDPCLK` while the clock tree used `STM32_PCLK4`
  for LPTIM2-5, LPUART1, SPI6, I2C4 and DFSDM2, so builds using one of those
  peripherals on its default bus clock failed to compile, the `RT-STM32-MULTI`
  H7A3 demo among them. The clocks are now `STM32_PCLK3` and `STM32_PCLK4`, as
  on the other STM32H7 devices, and the APB3 frequency check is enabled
  ([#553](https://github.com/chibios-upstream/chibios/pull/553)).
- [STM32] STM32H7: with LSE selected as the USART2-8 clock source, UART5 had
  no clock defined and its baud rate was computed from PCLK1
  ([#553](https://github.com/chibios-upstream/chibios/pull/553)).
- [STM32] STM32U3: the I3C2 kernel clock selection 0 is PCLK2, the port named
  and computed it as PCLK3.
- [SB] A sandbox executing an SVC with an invalid stack pointer was not
  terminated: the SVC left pending by the failed exception stacking was taken
  on the fault exit frame and resumed the sandbox with the kernel lock held,
  debug hosts halted with `SV#4`; the pending SVC is now discarded
  ([#535](https://github.com/chibios-upstream/chibios/pull/535)).
- [STM32] The instruction cache was never enabled on STM32H5: the port did not
  call `icache_init()`, so the code executed from flash with the full wait
  states; the cache is now enabled at clock initialization
  ([#527](https://github.com/chibios-upstream/chibios/pull/527)).
- [NIL] NIL tickless mode encoded finite timeouts incorrectly after a long
  inactive interval and when a new full-range wait could not be represented
  relative to the existing time origin; existing waits are now rebased when
  needed. `chTimeAddX()` returns a wrapped `systime_t`, and non-smart builds
  (`USE_SMART_BUILD=no`) no longer stop with a "missing separator" error in
  `os/nil/nil.mk`
  ([#323](https://github.com/chibios-upstream/chibios/pull/323)).
- [HAL] USB driver core fixes. Host GET_STATUS, SYNCH_FRAME and
  CLEAR_FEATURE/SET_FEATURE(ENDPOINT_HALT) requests for an invalid endpoint
  are now stalled instead of dereferencing NULL on RP or accessing past the
  STM32 USBv1/USBv2 endpoint registers. Also fixed: a NULL dereference with
  `USB_USE_WAIT` when an endpoint callback disabled the endpoints, busy bits
  lost to higher priority handlers and empty IN control replies timing out
  ([#415](https://github.com/chibios-upstream/chibios/pull/415),
  [#423](https://github.com/chibios-upstream/chibios/pull/423),
  [#432](https://github.com/chibios-upstream/chibios/pull/432),
  [#450](https://github.com/chibios-upstream/chibios/pull/450)).
- [HAL] `sdcConnect()` asked MMC/eMMC cards for their relative card address
  (RCA) with CMD3 as it does with SD cards, but MMC/eMMC cards expect the host
  to assign it; MMC cards are now assigned RCA 2, SD cards are unchanged
  ([#290](https://github.com/chibios-upstream/chibios/pull/290)).
- [STM32] STM32L0xx `stm32_isr.h` redefined the DMA IRQ-sharing masks
  `STM32_DMA1_CH4_CMASK` and `STM32_DMA1_CH5_CMASK`, already defined in the
  registry, causing redefinition warnings on STM32L011 builds; the duplicate
  is removed ([#509](https://github.com/chibios-upstream/chibios/pull/509)).
- [STM32] After a remote wakeup, STM32 USBv1 and USBv2 devices affected by the
  ESOF timing erratum (STM32G4, L0, U0) could report a false suspend. Suspend
  detection is now held off until the host resumes the bus, for at most the
  new `STM32_USB_HOST_RESUME_TIMEOUT` (default 500 ms)
  ([#487](https://github.com/chibios-upstream/chibios/pull/487)).
- [STM32] The STM32 OTGv1 full speed turnaround time (`GUSBCFG.TRDT`) was
  fixed at 5, below the reference manual minimum, so below 32 MHz the core
  answered IN tokens too late (an STM32L4 at 23.986 MHz did not enumerate). It
  now follows `STM32_HCLK`, based on a patch by Bob Anderson
  ([#486](https://github.com/chibios-upstream/chibios/pull/486)).
- [STM32] On STM32C051, C071, C091 and C092 with `STM32_CLOCK_DYNAMIC`,
  `halClockSwitchMode()` ignored the `RCC_CR` `SYSDIV` divider, overstating
  the clock points used for baud rates, timers, timeouts, `SystemCoreClock`
  and flash wait states; the divider is now applied
  ([#481](https://github.com/chibios-upstream/chibios/pull/481)).
- [STM32] The HAL timeout counter of STM32C0, G0 and U0 (TIM17 on C0 and G0,
  TIM7 on U0) was stopped by the blanket APB reset in `stm32_clock_init()`
  right after being started, so timeouts never expired and `halSftDelayX()`
  never returned, hanging drivers such as STM32 USBv2; its prescaler also kept
  the old rate across clock switches. The timer is now left out of the reset
  and its prescaler follows each clock change
  ([#470](https://github.com/chibios-upstream/chibios/pull/470),
  [#478](https://github.com/chibios-upstream/chibios/pull/478)).
- [STM32] STM32C0 clock initialization and `halClockSwitchMode()` could run
  SYSCLK above 24 MHz with zero flash wait states, the HSISYS divider taking
  effect before the flash latency was set. The latency is now raised first and
  lowered only once the target clock is active
  ([#476](https://github.com/chibios-upstream/chibios/pull/476)).
- [STM32] TIM17, the HAL timeout counter on STM32G0, was not marked as used
  (`STM32_TIM17_IS_USED`), so GPT, PWM or the ST could take it over, after
  which timeouts never expired and `halSftDelayX()` never returned; such
  configurations now fail at compile time
  ([#474](https://github.com/chibios-upstream/chibios/pull/474)).
- [STM32] STM32 SPIv1 and SPIv2 on DMAv2 devices (STM32F4, F7) could invoke
  the completion callback after `spiStopTransfer()`, from a DMA event left
  pending during the stop; DMA events are now ignored unless the driver is
  `SPI_ACTIVE` ([#463](https://github.com/chibios-upstream/chibios/pull/463)).
- [STM32] STM32 USBv1 and USBv2 USB drivers: CLEAR_FEATURE(ENDPOINT_HALT) did
  not reset the bulk and interrupt endpoint data toggle to DATA0, so the next
  packet could be dropped as a duplicate, and after clearing a halt set during
  a transfer, that transfer never completed. Clearing a halt now resets the
  toggle and resumes the transfer; USBv2 endpoint initialization restarts the
  toggles from DATA0
  ([#412](https://github.com/chibios-upstream/chibios/pull/412),
  [#448](https://github.com/chibios-upstream/chibios/pull/448)).
- [STM32] Isochronous fixes in the STM32 OTGv1, USBv1 and USBv2 USB drivers.
  OTGv1 gave a callback every other frame on incomplete OUT transfers or
  rewrote an enabled endpoint; it now reports the missed frame once, with no
  data. USBv1/USBv2 copied OUT packets with no transfer active through a stale
  pointer and repeated IN packets instead of sending zero-length packets; on
  USBv1 both handlers could serve an endpoint
  ([#410](https://github.com/chibios-upstream/chibios/pull/410),
  [#445](https://github.com/chibios-upstream/chibios/pull/445)).
- [STM32] STM32 USBv1 and USBv2 USB driver packet memory and interrupt fixes:
  a final full OUT packet overflowing the receive buffer was copied past its
  end, receive buffers above 62 bytes were undersized in packet memory,
  overwriting the next buffer, unaligned SETUP packets faulted on STM32F0, and
  stale events followed a bus reset. USBv2 now waits 800 ns before reading OUT
  data from the packet memory (STM32H5, U0 errata)
  ([#378](https://github.com/chibios-upstream/chibios/pull/378),
  [#419](https://github.com/chibios-upstream/chibios/pull/419),
  [#436](https://github.com/chibios-upstream/chibios/pull/436)).
- [STM32] STM32 USBv2 (C0, G0, H5, U0, U3, U5) `usbStop()` left the peripheral
  drawing about 0.9 mA and requested resume signalling while powering it down;
  `usb_lld_stop()` now follows ST's stop sequence and resets the block through
  RCC before gating its clock, which also leaves the D+ pull-up off. The USBv1
  and USBv2 `usb_lld_start()` now waits tSTARTUP before releasing the
  transceiver reset
  ([#427](https://github.com/chibios-upstream/chibios/pull/427)).
- [STM32] `adcSTM32SetCCR()` in STM32 ADCv5 (STM32C0, G0, U0 and WL) and ADCv1
  (STM32L0) cleared the `ADC_CCR` prescaler set by `adcStart()`, e.g. when
  enabling VREFINT, leaving the ADC clock undivided (out of specification on a
  56 MHz STM32U0); `PRESC` is now preserved
  ([#426](https://github.com/chibios-upstream/chibios/pull/426)).
- [STM32] The STM32 USBv1 and USBv2 USB drivers enabled the unserved ERR
  interrupt, which the floating bus of an unplugged device with the D+ pull-up
  on (self-powered) raised continuously, starving every thread. ERR is no
  longer enabled; bus errors are still recovered by retransmission
  ([#421](https://github.com/chibios-upstream/chibios/pull/421)).
- [STM32] STM32 OTGv1 USB driver hardened with the XHAL fixes:
  `usbConnectBus()` and `usbDisconnectBus()` work on stepping 1 devices
  (STM32F105/F107, F2, F401/F405/F407/F411/F427/F429) without VBUS sensing or
  with a ULPI PHY, `usbGetFrameNumberX()` works, and hardware timeouts make
  `usbStart()` fail with `HAL_RET_HW_FAILURE` or disconnect the device. The
  driver needs the safety counter hooks; preempted `halRegWait*X()` no longer
  time out falsely
  ([#408](https://github.com/chibios-upstream/chibios/pull/408),
  [#418](https://github.com/chibios-upstream/chibios/pull/418)).
- [STM32] STM32 OTGv1 USB driver EP0, FIFO and PHY timing fixes. OTG1 and OTG2
  shared their EP0 state and SETUP buffer, so traffic on one could corrupt the
  control transfers of the other. A reconfiguration disturbed EP0 and could
  overlap the EP0 FIFO, IN FIFO depths were not rounded up, and the 4 Kbyte
  STM32H7 OTG1 FIFO was limited to 1.25 Kbytes. An STM32H723 could hang at
  start, and `STM32_USB_OTGFIFO_FILL_BASEPRI` dropped a stronger BASEPRI mask
  ([#333](https://github.com/chibios-upstream/chibios/pull/333),
  [#380](https://github.com/chibios-upstream/chibios/pull/380),
  [#417](https://github.com/chibios-upstream/chibios/pull/417)).
- [STM32] STM32 DACv1 driver fixes ported from XHAL (F0, F1, F3, F37x, F4, F7,
  G0, G4, H7, L0, L1, L4, L4+, WL). Stopping a CH2 conversion in single mode
  turned CH1 on and left CH2 off; a DMA error released a NULL DMA stream; a
  half-buffer callback stopping or restarting the conversion could dereference
  the released group or get the old conversion's full-buffer event. DMA events
  are now ignored with no conversion active
  ([#414](https://github.com/chibios-upstream/chibios/pull/414)).
- [STM32] STM32 DACv2 driver fixes ported from XHAL (H5, U5, U3). Dual mode
  moved the wrong data in 12-bit and 8-bit modes. Double DMA, HFSEL and the
  CH1 settings restored on stop were set wrongly, depth-one conversions
  reported half-buffer events, and stopping or restarting from the half-buffer
  callback could dereference the released group. Underrun interrupts are now
  always enabled during conversions
  ([#413](https://github.com/chibios-upstream/chibios/pull/413)).
- [STM32] STM32 DACv1 trigger mask sized for 3 or 4 bit trigger source
  identifiers.
- [STM32] STM32 ADCv3, ADCv5 and ADCv6 error handling: error interrupts were
  only enabled for groups with an error callback, so a failed conversion
  neither stopped nor woke a waiting thread, and errors were reported outside
  an active conversion. In ADCv3/ADCv6 dual mode, slave overruns were not
  detected; both ADCs are now captured and acknowledged before one combined
  error notification, and an ADCv6 dual-mode build error is fixed
  ([#395](https://github.com/chibios-upstream/chibios/pull/395),
  [#397](https://github.com/chibios-upstream/chibios/pull/397),
  [#399](https://github.com/chibios-upstream/chibios/pull/399)).
- [STM32] STM32 ADC drivers (ADCv1 to ADCv7, STM32F1, F37x and L1) handled DMA
  events with no active conversion, so a late DMA error could enter the error
  handler after the group was cleared and an inactive driver could receive
  callbacks; DMA events are now ignored unless the driver is `ADC_ACTIVE`
  ([#393](https://github.com/chibios-upstream/chibios/pull/393)).
- [STM32] STM32 ADCv3 in dual mode on STM32F3 halted on a debug assertion with
  a correctly initialized slave ADC: the slave regulator check used the wrong
  mask and now matches the master check
  ([#390](https://github.com/chibios-upstream/chibios/pull/390)).
- [STM32] STM32 SPIv2 I2S: valid DMA stream assignments were rejected on
  devices without DMAMUX, the SPI3 RX/TX DMAMUX request sources were not
  programmed, and a DMA allocation failure in `i2sStart()` was not handled; on
  that failure the driver is now left stopped and safe to retry
  ([#388](https://github.com/chibios-upstream/chibios/pull/388)).
- [STM32] STM32 SPIv2: stopping a transfer in slave mode waited indefinitely
  for BSY to clear, which needs the external clock, and read the remaining
  frame count before RX DMA had stopped. Slave stops now halt both DMA streams
  and reset the SPI; a new transfer can start without another `spiStart()`
  ([#386](https://github.com/chibios-upstream/chibios/pull/386)).
- [STM32] STM32 SPIv3 (STM32H7) and SPIv4 (STM32H5, U3, U5): DMA and overrun
  interrupts were served while the driver was inactive, an RX DMA error also
  dispatched a completion, and a circular full-buffer callback still ran after
  the half callback stopped or restarted the transfer. SPIv4 reported the
  remaining count on stop in bytes instead of frames; SPIv3 `SPID6` is now
  placed in `.ram4_clear` for BDMA (`SPI_SPID6_MEMORY` overrides)
  ([#382](https://github.com/chibios-upstream/chibios/pull/382),
  [#384](https://github.com/chibios-upstream/chibios/pull/384)).
- [STM32] STM32 FDCANv2 (STM32H7): stopping one controller or a failed start
  could reset or gate the FDCAN clock shared with a running peer, and start
  failures did not reach `canStart()`; the clock is now reference-counted and
  `canStart()` returns configuration errors and timeouts. Classic-CAN frames
  with DLC 9..15 copied more than eight bytes, the extended-filter type mask
  was wrong, and stale filters stayed active after a table shrank or was
  cleared ([#376](https://github.com/chibios-upstream/chibios/pull/376)).
- [STM32] On dual-core STM32H7 devices (H745/H747/H755/H757) with assertions
  enabled, the first `sioStart()` (USARTv3), `i2cStart()` (I2Cv3) or DMA
  stream free (DMAv2) halted the system on a reset before clock enable, or on
  disabling a never-enabled DMA controller clock. Clocks are now enabled
  before resets and `dmaStreamFreeI()` disables only the controller owning the
  freed stream ([#370](https://github.com/chibios-upstream/chibios/pull/370)).
- [RP] RP2040 RTCv1 driver: `rtcSetTime()` could leave the date at its reset
  value, a `NULL` alarm specification was dereferenced instead of disabling
  the alarm, and a disabled alarm could still be reported or fire
  ([#113](https://github.com/chibios-upstream/chibios/pull/113)). A read right
  after a set returned the previous date/time; the set now waits for the new
  values ([#455](https://github.com/chibios-upstream/chibios/pull/455)).
- [ARMV6M] The post-IRQ `chSchDoPreemption()` call chain, which runs on the
  stack of the preempted thread, could silently overflow thread working areas
  at `-O0`. The `PORT_INT_REQUIRED_STACK` default is raised from 64 to 80 in
  the ARMv6-M, ARMv6-M-RP2, ARMv7-M and ARMv8-M-ML ports (16 more bytes per
  thread, NIL included)
  ([#459](https://github.com/chibios-upstream/chibios/pull/459)).
- [STM32] ADCv4 (STM32H7) boost-level selection tested the ADC clock
  thresholds in ascending order, so any clock above 6.25 MHz selected BOOST
  level 1 and levels 2 and 3 were never reached (under-boosting the analog
  stage above 12.5 MHz). The ADC12 and ADC3 thresholds are now tested in
  descending order
  ([#260](https://github.com/chibios-upstream/chibios/pull/260)).
- [STM32] STM32 SPIv3 and SPIv4 did not fully deactivate the peripheral on
  `spiStop()`; the control, configuration and interrupt-enable registers are
  now cleared ([#231](https://github.com/chibios-upstream/chibios/pull/231)).
- [STM32] TIM channel 5 and 6 mode encoding took the OC5M/OC6M extension bit
  from mode bit 2 instead of bit 3, breaking PWM mode 1 on those channels; the
  encoding is corrected (TIMv1)
  ([#258](https://github.com/chibios-upstream/chibios/pull/258)).
- [SHELL] The legacy shell read `line[-1]` when Enter was pressed at an empty
  prompt with history enabled; the line length is now checked first
  ([#256](https://github.com/chibios-upstream/chibios/pull/256)).
- [HAL] Serial over USB input remained inactive after a USB suspend/wakeup
  cycle on STM32 OTG devices. The generic USB driver terminates all pending
  transactions on suspend and the OTG LLD also disables the endpoints in
  hardware, so the bulk OUT receive armed before suspend was lost. Now
  `sduWakeupHookI()` restarts the receive operation
  ([#180](https://github.com/chibios-upstream/chibios/pull/180)).
- [STM32] STM32H7xx HAL initialization reset the SYSCFG block, clearing the
  overdrive enable (`SYSCFG_PWRCR` ODEN) set during clock initialization and
  dropping the core out of overdrive while the PLL kept running above the
  no-boost maximum. SYSCFG is no longer reset during `hal_lld_init()`
  ([#132](https://github.com/chibios-upstream/chibios/pull/132)).
- [STM32] STM32 USBv1 packet memory could be double-allocated across a
  configuration rebuild. `usb_lld_disable_endpoints()` reset the PMA allocator
  to the descriptor-table boundary while endpoint zero remained active, so a
  subsequent SET_CONFIGURATION allocated endpoint 1 over the still-live EP0
  buffers. The allocator now re-reserves the EP0 IN/OUT buffers immediately
  after the reset; the bus-reset path is unchanged
  ([#85](https://github.com/chibios-upstream/chibios/pull/85)).
- [RT] RT thread registry reference accounting was inconsistent when dynamic
  threads are disabled (`CH_CFG_USE_DYNAMIC = FALSE`): `chRegFirstThread()`
  and `chRegNextThread()` did not count the reference they hand out while
  `chThdRelease()` still released it, risking a reference-count underflow. The
  registry lookups now reference threads unconditionally
  ([#51](https://github.com/chibios-upstream/chibios/pull/51)).
- [OSAL] NASA OSAL `OS_TaskGetInfo()` computed the task working-area size
  before validating the task id, dereferencing a possibly-invalid thread
  pointer; the computation is now done after the validity check
  ([#51](https://github.com/chibios-upstream/chibios/pull/51)).
- [HAL] `nvicSetSystemHandlerPriority()` programmed the wrong `SCB->SHPR`
  field on Cortex-M0, M0+ and M23: the positive ChibiOS handler index was
  passed to the CMSIS `_SHP_IDX()`/`_BIT_SHIFT()` macros, which expect the
  negative system exception number, so the priority write (e.g. SysTick from
  the ST driver) landed on the wrong register slot: SysTick was left at its
  reset priority and another handler's priority was corrupted. The handler
  index is now converted to the matching exception number (forum bug report,
  [#34](https://github.com/chibios-upstream/chibios/pull/34)).
- [STM32] STM32U3 RTC was completely non-functional, the driver hung at boot
  in `rtc_enter_init()` waiting for INITF. The RTC APB clock was never
  enabled: hal_lld guarded it on `defined(RCC_APB3ENR_RTCAPBEN)` (the
  STM32H5/U5 register), but on STM32U3 the bit is `RCC_APB1ENR1_RTCAPBEN`
  (APB1ENR1 bit 30), so the guard was always false and the RTC register
  interface was unclocked. Enabled via
  `rccEnableAPB1R1(RCC_APB1ENR1_RTCAPBEN)` on STM32U3xx
  ([#31](https://github.com/chibios-upstream/chibios/pull/31)).
- [STM32] STM32U3 RTC driver operated on the wrong EXTI lines. The port
  defined `STM32_RTC_GLOBAL_EXTI=17` / `STM32_RTC_TAMP_EXTI=19` (copied from
  STM32H5) and enabled/cleared them, but on U3 those lines are COMP1 and
  VDDUSB (RM0487 Table 131) and the RTC has no EXTI line at all (RTC
  interrupts go directly to the NVIC). The RTC EXTI enable/clear are now
  no-ops ([#31](https://github.com/chibios-upstream/chibios/pull/31)).
- [STM32] The STM32F303 mcuconf template was missing its I2S driver settings
  section, so regenerating an F303 configuration silently dropped the I2S
  settings; the section was restored, the STM32C071 template gained its SPI2
  settings and all templated mcuconf configurations were regenerated against
  their templates
  ([#19](https://github.com/chibios-upstream/chibios/pull/19)).
- [STM32] STM32L4+/L4Rxx clock point name table (`CLK_POINT_NAMES`) had a
  comma misplaced inside the "PLLSAI2R" string literal, so adjacent string
  literals were concatenated and the table held one entry fewer than
  `CLK_ARRAY_SIZE`; clock point names from PLLSAI2R onward were shifted and
  the last was `NULL`. The comma is moved outside the string
  ([#21](https://github.com/chibios-upstream/chibios/pull/21)).
- [STM32] STM32U0xx RTC alarm/tamper interrupt could halt the system on the
  first event (assertions enabled): `rtc_lld_serve_interrupt()` cleared the
  RTC/TAMP EXTI lines, which are direct event inputs on STM32U0 (no EXTI
  pending bit) and tripped the `extiClearGroup1()` fixed-lines assertion. The
  clear now masks out the direct lines
  ([#16](https://github.com/chibios-upstream/chibios/pull/16)).
- [STM32] STM32H5xx RTC alarm/tamper interrupt caused a system halt on the
  first event: `rtc_lld_serve_interrupt()` cleared the RTC/TAMP EXTI lines,
  which are direct event inputs on STM32H5 (no EXTI pending bit) and tripped
  the `extiClearGroup1()` fixed-lines assertion. The clear now masks out the
  direct lines (forum bug report,
  [#15](https://github.com/chibios-upstream/chibios/pull/15)).
- [STM32] Missing SPI2 RCC macros and DMAMUX identifiers in the STM32C0xx HAL
  port, SPI2 was unusable on the devices that have it (forum bug report,
  [#12](https://github.com/chibios-upstream/chibios/pull/12)).
- [STM32] Missing `STM32_ADC_ADC2_IRQ_HOOK` invocations in the STM32 ADCv6 and
  ADCv7 drivers (forum bug report,
  [#11](https://github.com/chibios-upstream/chibios/pull/11)).
- [HAL] MFS: write verification when the source aliases the shared
  non-cacheable buffer, `MFS_CFG_STRONG_CHECKING` honoured during mount scans,
  and duplicate transaction erases no longer corrupt the used space accounting
  ([#58](https://github.com/chibios-upstream/chibios/pull/58)).
- [VFS] ROMFS: chunked reads are guarded at the end of file, backends making
  no progress or reporting more than requested are rejected, and
  `SEEK_CUR`/`SEEK_END` bounds are checked before the offset addition
  ([#59](https://github.com/chibios-upstream/chibios/pull/59)).
- [RT] C++ wrappers: `Scheduler::rescheduleS()` calls `chSchRescheduleS()`,
  message polling context and registry reference handoff, wrappers around
  embedded kernel objects are no longer copyable, typed mailbox fetch and peek
  for small types, `ObjectsPool` storage stride and `ThreadsPool` memory pool
  access ([#61](https://github.com/chibios-upstream/chibios/pull/61)).
- [RT] `chThdCreateFromMemoryPool()` rejected valid fixed memory pools due to
  an overly strict alignment assertion
  ([#3](https://github.com/chibios-upstream/chibios/pull/3)).
- [RT] The trace halt event could dereference the uninitialized trace buffer
  before the kernel startup completed (bug #1309).
- [RT] Alignment of the heap-created thread working area size (bug #1307).
- [NIL] Wrong alignment check in `chThdCreateI()` (bug #1306).
- [TEST] 23 STM32 test applications ran a 1 kHz system tick from a timer clock
  above 65.5 MHz: the free-running timer prescaler did not fit in 16 bits and
  they halted in `st_lld_init()` with assertions enabled. The tick is now 10
  kHz, an exact divisor of every affected timer clock
  ([#485](https://github.com/chibios-upstream/chibios/pull/485)).
- [AVR] Arduino Leonardo, Pro Micro and Teensy 2 demos: the serial USB CDC
  endpoints were enabled with each other's configurations and out of order;
  neither endpoint worked and the driver dereferenced a NULL pointer on
  configuration. The endpoints are now enabled with their own configurations
  in ascending order
  ([#453](https://github.com/chibios-upstream/chibios/pull/453)).

## [21.11.5] - 2026-04-04

### Added

- [STM32] STM32U0xx support.
- [STM32] STM32U3xx support.

### Fixed

- [OSLIB] Fixed OSLIB Jobs Queues: return JOB_NULL descriptors to the free
  pool (bug #1304).
- [OSLIB] Fixed serialize ref duplication and harden dynamic object size
  calculations (bug #1303).
- [OSLIB] Fixed harden memcore/heap boundary checks against arithmetic
  overflow (bug #1302).
- [RT] Fixed priority inheritance through prioritized message queues
  (bug #1301).
- [RT] Fixed chMsgSend missing rdymsg assignment causes spurious NULL return
  from chMsgWaitTimeoutS (bug #1300).
- [NIL] Fixed problem with queues counter in NIL (bug #1299).
- [NIL] Fixed problem with CH_PORT_SUPPORTS_RECURSIVE_LOCKS in NIL
  (bug #1298).

## [21.11.4] - 2025-09-13

### Added

- [STM32] STM32C0xx support.
- [STM32] STM32G0B0 support.
- [STM32] STM32H5xx support.
- [STM32] FDCAN support for STM32G4xx, STM32H5xx and STM32H7xx.
- [HAL] XSNOR serial flash memories framework.
- [HAL] I2C slave support in HAL and in ST low level drivers.
- [EXT] Support for LittleFS flash file system.
- [VFS] VFS subsystem.
- [OOP] OOP framework.
- [ARMV7M] Added missing context switch hook in ARMv7-M-ALT port.
- [STM32] FDCANv1 and FDCANv2 filter enabled.
- [RT] Recursive locks in RT and NIL made optional, only enabled if the
  underlying port supports the capability.
- [STM32] Added STM32 FDCANv2 for STM32H7xx
- [STM32] Improved DAC driver, updated STM32 DACv1.
- [STM32] Removed oscillator stabilization delay on STM32 LSE and HSE when
  bypass mode is enabled.
- [STM32] STM32 TIM1 and TIM8 support added to SYSTICKv1 driver.
- [STM32] STM32 RTCv2 and RTCv3 modified to not use shadow registers.
- [STM32] Enhanced STM32F7xx MPU configuration in mcuconf.h.
- [HAL] I2C slave support in HAL high level driver.
- [STM32] Added settings for STM32 OCTOSPIv1 and OCTOSPIv2 TCR bits SSHIFT and
  DHQC.
- [STM32] Reworked STM32 SDMMCv1 and SDMMCv2 drivers, better timeout and clock
  handling, improved speed for aligned buffers.

### Fixed

- [STM32] Set DAC_HAS_MCR FALSE in STM32F1xx registry.
- [STM32] Fixed ADCv4 common registers reset at start (bug 1296).
- [STM32] Fixed DMA2 not firing on STM32G431 (bug 1295).
- [STM32] Fixed wrong STM32 ADCv2 stop method (bug 1294).
- [STM32] Fixed STM32 OTGv1 driver does not re-enables endpoints on wakeup
  (bug 1293).
- [OSLIB] Fixed missing assertion in OSLIB factory module (bug #1292).
- [STM32] Fixed problem in FDCANv1 driver for G4 (bug #1291).
- [RT] Fixed problem in recursive locks functions (bug #1288).
- [ARMV8M] Fixed ARMv8-M-ML port compile fail when FPU is enabled (bug #1281).
- [STM32] Fixed interrupts not enabled for STM32H735 TIM15, TIM16 and TIM17
  (bug #1280).
- [STM32] Fixed wrong STM32 LSI activation check (bug #1279).
- [STM32] Fixed STM32 HAL UART ISR flaw (bug #1278).
- [OSLIB] Fixed race condition caused by chGuardedPoolAllocI() (bug #1277).
- [HAL] Fixed avoid shadowing with build-in pow10 function in chprintf.c
  (bug #1274).
- [STM32] Fixed enabling PWM on TIM1, 3, 4 causes compile errors in
  RT-STM32G0B1RE-NUCLEO64 (bug #1273).
- [HAL] Fixed unnecessary code in SNOR device drivers (bug #1265).
- [RP] Fixed RP2040 HAL GPIO failed to compile (bug #1264).
- [STM32] Fixed channel 0 corruption on STM32 BDMAv1 (bug #1263).
- [RT] Fixed wrong statistics in RT7 (bug #1262).
- [ARMV7M] Fixed missing cache management during Cortex-M RAM initializations
  (bug #1261).
- [STM32] Fixed RTC & TAMP interrupts not functional (bug #1260).
- [STM32] Fixed syntax errors in STM32H7xx/hal_lld_type2.h (bug #1259).
- [STM32] Fixed unwanted reset of cache on STM32H7xx (bug #1258).
- [STM32] Fixed invalid HSIDIV in STM32Ggxx clocks initialization (bug #1257).
- [STM32] Fixed incorrect RTC initialization on STM32G4/L4/L4+ (bug #1256).
- [RP] Fixed syntax error in RP2040 GPIO driver (bug #1255).
- [STM32] Fixed undefined STM32_SDMMC_MAXCLK value for STM32H7 type 1 and 2
  (bug #1254).
- [STM32] Fixed invalid checks on PLLP/R/Q dividers on STM32H7 (bug #1253).
- [STM32] Fixed remote wakeup failure in STM32 OTGv1 driver (bug #1252).
- [RT] Fixed wrong use of hooks in RT/NIL (bug #1251).
- [HAL] Fixed SPI_MMC driver broken in 21.11.3 (bug #1249).

## [21.11.3] - 2023-01-03

### Added

- [STM32] STM32 DMA drivers now export an STM32_DMA_MAX_TRANSFER definition.
- [RP] PAL events for RP2040 added.
- [ARMV7M] Removed obsolete sandbox code from ARMv7-M port. Now ARMv7-M-ALT is
  the official port for use with sandboxes.
- [HAL] Reworked HAL MAC driver, now with callback support.
- [STM32] Fixed setting of SYSCLK when derived from divided HSI16
- [BUILD] Mass change: Source code convention changed from CRLF to just CR
  (Unix).
- [STM32] Fixed some corner cases in ADC5, added ADC reset on start().
- [HAL] Added a "BufferedSIODriver" class that implements the behavior of the
  legacy Serial driver on top of a SIO implementation (buffering, events and
  all).
- [HAL] Now hal.h includes cc_portab.h by default making it mandatory.
- [HAL] Moved HAL serial error flags into asynchronous channels interface
  definitions.
- [HAL] Reworked HAL SIO driver.
- [BUILD] Non-proprietary LLVM build support.
- [EXT] Added integration of LittleFS on top of our flash infrastructure.
- [HAL] Added a centralized errors handler under /os/common/utils. It will
  replace those in HAL and SB and will be shared among multiple subsystems.
- [OOP] Added a new OOP model under /os/common/utils. It will replace the one
  in HAL and will be shared among multiple subsystems.
- [STM32] Added EFL driver support for STM32F401/411.

### Fixed

- [STM32] Fixed broken support for STM32 UART9 and USART10 (bug #1248).
- [STM32] Fixed wrong initialization in STM32L1xx ADC driver (bug #1247).
- [STM32] Fixed wrong HSI48 support on STM32L0xx (bug #1246).
- [STM32] Fixed wrong DMA definitions for STM32L0xx I2C3 peripheral
  (bug #1245).
- [STM32] Fixed wrong path in STM32L053 ADC demo makefile (bug #1244).
- [STM32] Fixed missing semicolon in STM32 OTGv1 driver (bug #1243).
- [STM32] Fixed HSI48 not enabled for STM32L496/​4A6 (bug #1242).
- [STM32] Fixed problem in STM32 gpt_lld_polled_delay() implementation
  (bug #1241).
- [STM32] Fixed invalid delay loop in STM32G0/WL ADCv5 driver (bug #1240).
- [STM32] Fixed STM32_MCOSEL setting problem (bug #1239).
- [STM32] Fixed problems with cache in STM32 SDMMC drivers (bug #1238).
- [STM32] Fixed missing clock enables for some GPIOS on some STM32L4s
  (bug #1237).
- [HAL] Fixed old bugs in serial driver header (bug #1236).
- [RT] Fixed virtual timers lockup under rare conditions (bug #1235).
- [STM32] Fixed STM32 RTCv2 locks for a second (bug #1234).

## [21.11.2] - 2022-06-11

### Added

- [HAL] Added dubby cycles support for SNOR using the normal SPI driver.
- [STM32] Disabled priority check on STM32 EXTI interrupts when the default
  ISR is disabled. This allows for fast interrupts.
- [STM32] Added support for UART9 and USART10 on STM32H7.
- [HAL] Improved MFS to use explicitly non-cacheable buffers for potentially
  DMA-accessible I/O areas.
- [EXT] FatFS now functional on STM32H7xx, added a target to the VFS demo.
- [STM32] Improved cache settings in STM32H7xx mcuconf.h.
- [STM32] Modified SDMMCv2 to allow for uncached buffers, tested on STM32H7xx.
- [STM32] Added OCTOSPIv2 driver using MDMA for STM32H7xx.
- [STM32] Added demos for STM32H723ZG Nucleo144 and STM32H735ZI Discovery
  boards.
- [STM32] Added support for STM32H723/25/33/35/A3/B3/A3Q/B3Q.
- [STM32] Updated ST Cube headers for STM32H7xx.
- [HAL] Improved HAL flash interface with mutual exclusion methods, improved
  EFL and SNOR drivers to use it.
- [STM32] Added EFL driver implementation for STM32G4xx.
- [STM32] STM32G0B1 USBv2 driver.
- [STM32] USBv1 driver optimization and removal of a potential race condition
  (not demonstrated).
- [HAL] Added elfAcquireBus()/eflReleaseBus() functions to EFL driver.
- [ARMV7M] Added option to copy vectors in RAM on GCC startup for ARMv6-M,
  ARMv7-M and ARMv8-M-ML.
- [STM32] On STM32WBxx added a check on STM32_LSI_ENABLE required by IWDG.
- [STM32] Added SPIv2 support also to STM32WB and STM32WL.

### Fixed

- [EX] Fixed uninitialized return message in EX subsystem (bug #1267).
- [HAL] Re-opened and fixed bug #1100.
- [STM32] Fixed wrong buffers toggling in STM32 USBv1 isochronous mode
  (bug #1232).
- [STM32] Fixed STM32 RTCv2 registers synchronization errata (bug #1231).
- [STM32] Fixed STM32 ADCv1 and ADCv5 do not allow prescaler divide value of 1
  (bug #1230).
- [STM32] Fixed missing check on STM32 SPIv2 DMA settings for SPI1
  (bug #1229).
- [ARMV6M] Fixed ARMv6-M port Keil compiler fail (bug #1228).
- [EXT] Fixed invalid handling of lwIP NETIF_FLAG_LINK_UP flag (bug #1227).
- [STM32] Fixed missing TIM16/17 errata handling for STM32G0xx (bug #1226).
- [STM32] Fixed missing ADC errata handling for STM32G0xx (bug #1225).
- [HAL] Fixed problem in the HAL I2C fallback driver (bug #1224).
- [STM32] Fixed GPIOH clock not enabled on STM32L432 (bug #1223).
- [RT] Fixed invalid cumulative time stat in RT (bug #1222).
- [RT] Fixed incorrect type cast in TIME_I2US() (bug #1221).
- [STM32] Fixed missing clock disable for STM32 OCTOSPI2 (bug #1220).
- [STM32] Fixed wrong condition in STM32 sio_lld_read() function (bug #1219).
- [STM32] Fixed STM32 Ethernet driver causes system hang after 2^31 packets
  sent/received (bug #1218).
- [STM32] Fixed clock re-initialization problem in STM32 USARTv2 and USARTv3
  drivers (bug #1217).
- [STM32] Fixed assertion on initialization of STM32H7xx (bug #1216).
- [RT] Fixed Virtual Timers failure in a specific condition (bug #1215).
- [STM32] Fixed invalid STM32_OTG_STEPPING for STM32F40_41xxx (bug #1214).
- [STM32] Fixed SPIv2 driver compile fails when SPI_USE_SYNCHRONIZATION is
  FALSE (bug #1213).
- [HAL] Fixed invalid state transition in SNOR flash driver (bug #1212).
- [HAL] Fixed missing exit condition in sioSynchronizeRX() and
  sioSynchronizeTX() (bug #1211).
- [BUILD] Some MISRA-related fixes.
- [OSLIB] Fixed missing check in chobjcaches.h (bug #1210).
- [RT] Fixed misspelled chTraceSuspendI() function name (bug #1209).
- [TEST] Fixed RT testbuild application broken (bug #1208).

## [21.11.1] - 2021-12-08

### Added

- [STM32] Added EFL driver implementation for STM32G4xx.
- [STM32] STM32G0B1 USBv2 driver.
- [STM32] USBv1 driver optimization and removal of a potential race condition
  (not demonstrated).
- [HAL] Added elfAcquireBus()/eflReleaseBus() functions to EFL driver.
- [STM32] Added STM32L073RZ-Nucleo64 to USB-CDC "multi" demo. Removed old
  demo.
- [STM32] Added an STM32 WDG "multi" demo. Removed all old WDG demos.
- [ARMV7M] Added option to copy vectors in RAM on GCC startup for ARMv6-M,
  ARMv7-M and ARMv8-M-ML.
- [STM32] On STM32WBxx added a check on STM32_LSI_ENABLE required by IWDG.
- [STM32] Added SPIv2 support also to STM32WB and STM32WL.

### Fixed

- [STM32] Fixed PWR_CR2_USV not set in STM32L4+ mcuconf.h file (bug #1207).
- [STM32] Fixed USB not enabled on STM32F103x6 (bug #1206).
- [TEST] Fixed RT test suite build fails when CH_CFG_USE_TIMESTAMP is FALSE
  (bug #1205).
- [STM32] Fixed wrong number of CAN filters for STM32L496/9A6 (bug #1204).
- [STM32] Fixed DMA stream not disabled in STM32 QUADSPIv1 driver (bug #1203).
- [STM32] Fixed I2C4 DMA streams for STM32L496 (bug #1202).
- [STM32] Fixed STM32_SDMMC2_NUMBER on STM32H7 (bug #1201).
- [STM32] Fixed STM32G0B1 demo application hangs debuggers (bug #1200).

## [21.11.0]

### Added

- [STM32] STM32 ADCv2 now supports return code on start function.
- [EXT] Integrated FatFS with lwIP HTTPD, now it is possible to serve files
  using HTTP from a storage device.
- [EXT] Updated FatFS to version 0.14b.
- [STM32] SPIv2 driver has been implemented on: STM32F0, STM32F1, STM32F3,
  STM32F4, STM32F7, STM32G0, STM32G4, STM32L0, STM32L1, STM32L4, STM32L4+,
  STM32H7.
- [STM32] New SPIv2 driver model, it is compatible with the previous SPI
  driver and introduces: better runtime errors handling, slave mode, data
  synchronization function, various other improvements.
- [ARMV7M] Added an alternate port for ARMv7-M, it uses less RAM and it is
  faster at interrupt processing, it is slightly slower at thread-to-thread
  context switch so it is not a full replacement.
- [HAL] Now all xxxStart() functions in HAL are able to report a driver
  activation error.
- [STM32] Support for STM32G031, STM32G041, STM32G0B1, STM32G0C1.
- [STM32] Made STM32H7 non-cacheable memory option also shareable.
- [STM32] EFL driver and demo for STM32F3xx.
- [TEST] New unit test subsystem under /os/test. Now it is officially
  ChibiOS/TEST.
