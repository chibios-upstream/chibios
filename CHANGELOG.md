# Changelog

All notable changes to ChibiOS are documented in this file. Entries reference
the GitHub pull request implementing the change where available; fixes also
applied to a maintenance branch are marked *(backported to 21.11.6)*.

## [Unreleased]

### Added

- [SB] Sandbox PIDs: each start of a sandbox gets a new positive PID, valid
  until `sbFinalize()`, returned by `sbGetPidX()` on the host so that objects
  referring to one execution can detect a restart; guests get it with
  `sbGetPid()` and POSIX `getpid()`, and the `SB-CLIENT-RT-XHAL-XSHELL` demo
  has an `sbpid` command
  ([#537](https://github.com/chibios-upstream/chibios/pull/537)).
- [SB] Application-defined sandbox fastcalls and syscalls: the host table
  entries are weak aliases of the not implemented handler and
  `SB_SVC_HANDLER()` binds a handler to a code, for the host modules and for
  the application; the codes 64 to 95 and 192 to 223 are never used by
  ChibiOS, guests invoke them with `sbFastcallN()` and `sbSyscallN()`, and
  two handlers bound to the same code are a link error
  ([#533](https://github.com/chibios-upstream/chibios/pull/533)).
- [SB] FPU support in the FreeRTOS, ThreadX and Zephyr sandbox guests: the
  context switch also preserves FPSCR, the Zephyr guest is built for the FPU
  with `CONFIG_FPU`, the self-test clients check the FPU registers and
  `make USE_FPU=hard` builds the multi-target host demos and their sandbox
  images for the FPU
  ([#525](https://github.com/chibios-upstream/chibios/pull/525)).
- [SB] Eclipse ThreadX as a sandbox guest: ThreadX bindings in
  `os/various/threadx_bindings` with the `ARMvx-M-SB` port, the ThreadX
  kernel in `ext/threadx-6.5.2.202603.7z`, the `SB-CLIENT-THREADX` self-test
  client and the `RT-SB-SWITCHED-XSHELL-THREADX-MULTI` host demo
  ([#524](https://github.com/chibios-upstream/chibios/pull/524)).
- [SB] Zephyr kernel as a sandbox guest: Zephyr module in
  `os/various/zephyr_bindings` providing the `chibios_sb` architecture, SoC,
  board and tickless system timer, plus the `SB-CLIENT-ZEPHYR` self-test
  client and the `RT-SB-SWITCHED-XSHELL-ZEPHYR-MULTI` host demo; the Zephyr
  tree is fetched into `ext/zephyr` as described in `ext/zephyr_readme.md`,
  and it requires a Zephyr external architecture patch, provided in the
  module until an equivalent change is available upstream
  ([#521](https://github.com/chibios-upstream/chibios/pull/521),
  [#522](https://github.com/chibios-upstream/chibios/pull/522),
  [#523](https://github.com/chibios-upstream/chibios/pull/523)).
- [SB] ChibiOS/NIL as a sandbox guest over the `ARMvx-M-SB` port, with XHAL
  VIO drivers: the `SB-CLIENT-NIL-XHAL` guest demo and the
  `RT-SB-SWITCHED-XSHELL-NIL-MULTI` host demo. `sbuser.h` provides the host
  interval conventions as `SB_TIME_xxx` and leaves `TIME_xxx` to a ChibiOS
  guest kernel, and the sandbox port requires a 32-bit system time
  ([#519](https://github.com/chibios-upstream/chibios/pull/519)).
- [SB] Sandbox test suite in `test/sb`, executed by the RT XSHELL sandbox
  client when `XSHELL_CMD_TEST_SB_ENABLED` is `TRUE`: sandbox environment,
  host services, VRQ delivery and boundary benchmarks. The null fastcall
  `SB_FASTC_NOP` and the null syscall `SB_SYSC_NOP` (`sbNopFastcall()`,
  `sbNopSyscall()`) are added as round-trip references
  ([#517](https://github.com/chibios-upstream/chibios/pull/517)).
- [RT] New RFCU fault `CH_RFCU_THD_MISSED_DEADLINE`, collected when
  `chThdSleepUntilWindowed()` is called outside its `[prev, next)` window;
  the function still returns immediately without sleeping
  ([#276](https://github.com/chibios-upstream/chibios/pull/276)).
- [NIL] `chMsgPoll()` and `chMsgPollS()` in NIL, matching the RT polling
  interface. `chMsgWaitTimeout(TIME_IMMEDIATE)` with no sender now returns
  immediately as documented, instead of entering a maximum-length timed wait
  ([#312](https://github.com/chibios-upstream/chibios/pull/312)).
- [HAL] Safety counter API (classic HAL and XHAL): `halSftGetCounterX()`,
  `halSftGetCounterFrequencyX()`, `halSftIsCounterWithinX()`,
  `HAL_SFT_US2CNT()`/`HAL_SFT_NS2CNT()`, `halSftDelayX()`/`halSftDelayFromX()`.
  The register waits now use it, so on STM32C0, G0 and U0 a wait crossing the
  16-bit counter wrap no longer times out right away; the STM32 OTGv1 driver and
  STM32U5 OTG HS PHY startup delays use it instead of the OS realtime counter
  ([#435](https://github.com/chibios-upstream/chibios/pull/435),
  [#438](https://github.com/chibios-upstream/chibios/pull/438),
  [#440](https://github.com/chibios-upstream/chibios/pull/440))
  *(backported to 21.11.6, except #438 and #440)*.
- [HAL] USB Mass Storage class driver (Bulk-Only Transport, SCSI) exporting a
  `BaseBlockDevice` as a single logical unit, in `os/hal/lib/complex/usb_msd`:
  an application thread loops on `msdServe()`, `msdMediumChangedI()` reports a
  medium change, and `USB_USE_EP0_THREAD == FALSE` is required
  ([#428](https://github.com/chibios-upstream/chibios/pull/428)).
- [HAL] WSPI status polling (classic and XHAL): `wspiPollStatusTimeout()`
  reads a status register until `(status & mask) == match` or a timeout
  expires, in hardware on STM32 OCTOSPIv2/v3 and through a generic fallback
  elsewhere. WSPI LLDs must now define `WSPI_LLD_SUPPORTS_STATUS_POLL`
  ([#279](https://github.com/chibios-upstream/chibios/pull/279)).
- [XHAL] STM32 XHAL platform integration of existing drivers: DAC, Ethernet,
  SDMMC and RNG on STM32H7; ADC, DAC, I2C, SPI and RNG on STM32L4+, plus RTC
  and SDMMC2 on STM32L4P5/Q5; RTC on STM32G0, U0 and U3; ADCv5, SPIv2 and
  xWDGv1 on STM32U0; RTC, OCTOSPI (two instances) and OTG USB on STM32U5. The
  STM32H7 and STM32L4+ ports now export the DWT-based safety timeout counter,
  so positive register-wait timeouts expire on them
  ([#338](https://github.com/chibios-upstream/chibios/pull/338),
  [#340](https://github.com/chibios-upstream/chibios/pull/340),
  [#401](https://github.com/chibios-upstream/chibios/pull/401),
  [#406](https://github.com/chibios-upstream/chibios/pull/406)).
- [XHAL] Crypto driver (`hal_crypto`) in XHAL: begin/update/final streams for
  hash, MAC, cipher and AEAD, single calls for signature, asymmetric
  encryption, key agreement and HKDF, caller-owned stream contexts and a key
  model that also fits HSM backends. STM32 CRYPv1 LLD for STM32H7 with
  AES-ECB/CBC, AES-GCM (96-bit nonces, not on STM32H74x/75x revision Y) and
  SHA-256 streams, and the `test/xcrypto` test suite
  ([#373](https://github.com/chibios-upstream/chibios/pull/373),
  [#374](https://github.com/chibios-upstream/chibios/pull/374)).
- [XHAL] STM32H755 dual-core demos `RT-XHAL-STM32H755ZIQ-DUAL-M7` and
  `RT-XHAL-STM32H755ZIQ-DUAL-M4` for the NUCLEO-H755ZI-Q: one RT instance per
  core in separate flash banks and RAM regions, the M7 initializing the shared
  clocks and GPIOs and signaling readiness to the M4 through HSEM 31
  ([#363](https://github.com/chibios-upstream/chibios/pull/363)).
- [XHAL] New STM32 XHAL low level drivers: SPIv4 (STM32H5/U3/U5), SPIv3
  (STM32H7), USBv2 (STM32C0/G0/H5/U0/U3), OTGv1 (STM32H7/L4+), ADCv4, FDCANv2
  and QUADSPIv2 (STM32H7), ADCv7 (STM32U3, with dual mode), CANv1 (bxCAN, CAN1
  on STM32L4+, with a three-argument `canSTM32SetFilters()` usable while the
  driver is ready) and the STM32U0 embedded flash (EFL) driver
  ([#336](https://github.com/chibios-upstream/chibios/pull/336),
  [#342](https://github.com/chibios-upstream/chibios/pull/342),
  [#343](https://github.com/chibios-upstream/chibios/pull/343),
  [#344](https://github.com/chibios-upstream/chibios/pull/344),
  [#347](https://github.com/chibios-upstream/chibios/pull/347)).
- [XHAL] XHAL port for the TI AM67/J722S MCU-domain Cortex-R5F, the first TI
  port in XHAL: `AM67R5F` startup device, ARMv7-R AM67 sub-port handling the
  VIM interrupt manager, platform in `os/xhal/ports/TI/AM67`, DMTIMERv1 ST,
  UARTv1 SIO and MCSPIv1 SPI drivers, the `T3_GEM_O1` board (T3 Gemstone O1)
  and the `RT-XHAL-T3-GEM-O1` demo
  ([#287](https://github.com/chibios-upstream/chibios/pull/287),
  [#289](https://github.com/chibios-upstream/chibios/pull/289),
  [#320](https://github.com/chibios-upstream/chibios/pull/320)).
- [XHAL] Plain non-blocking byte queue (`plain_queue_t`, `qObjectInit()`,
  `qResetI()`, `qPutI()`, `qGetI()`, `qReadI()`, `qWriteI()`) in the XHAL
  queues module, now aggregated by the input and output queues; `qSpaceI()`
  is removed
  ([#266](https://github.com/chibios-upstream/chibios/pull/266)).
- [STM32] Double-buffered bulk endpoints in the STM32 USBv1 and USBv2 USB
  drivers (classic and XHAL), enabled by `STM32_USB_USE_DOUBLE_BUFFERING`
  (default `FALSE`) and `ep_buffers = 2` on a unidirectional bulk endpoint, for
  transfers of two or more packets (12-20% more IN throughput)
  ([#431](https://github.com/chibios-upstream/chibios/pull/431)).
- [STM32] USB audio test applications `testhal/STM32/multi/USB_AUDIO` (classic
  HAL) and `testxhal/USB_AUDIO` (XHAL): a full-speed UAC1 device with a
  microphone streaming a 440 Hz tone and a 48 kHz stereo speaker on an adaptive
  isochronous OUT endpoint, played by DAC1 on PA4/PA5. Targets: NUCLEO-H723ZG
  (OTGv1), NUCLEO-G474RE (USBv1) and NUCLEO-H563ZI (USBv2); `testxhal/USB_CDC`
  also gains a NUCLEO-H723ZG target
  ([#402](https://github.com/chibios-upstream/chibios/pull/402),
  [#409](https://github.com/chibios-upstream/chibios/pull/409),
  [#411](https://github.com/chibios-upstream/chibios/pull/411)).
- [STM32] STM32U5F7 embedded flash (EFL) driver in the classic HAL, for the
  4 MiB and the 2 MiB (dual-bank mode required) devices, with bank-swap aware
  page mapping and ICACHE handling around program and erase; other STM32U5
  devices and secure builds are rejected
  ([#405](https://github.com/chibios-upstream/chibios/pull/405)).
- [STM32] STM32U0 ADC support in the classic HAL through the ADCv5 driver. The
  ADC section of the STM32U0 mcuconf template, which carried STM32U5 settings
  and IRQ priorities invalid on the Cortex-M0+, is corrected
  ([#403](https://github.com/chibios-upstream/chibios/pull/403)).
- [STM32] STM32U5 Stop-mode clock retention (classic and XHAL):
  `STM32_CFG_HSI16_STOP_ENABLE` (default `TRUE`, the previous behavior) and
  `STM32_CFG_MSIK_STOP_ENABLE` (default `FALSE`) select whether HSI16 and
  MSIK stay available to autonomous peripherals in Stop 0, 1 and 2
  ([#272](https://github.com/chibios-upstream/chibios/pull/272)).
  With `STM32_CFG_CLOCK_DYNAMIC`, the new `halClockResume()` restores the
  clock configuration after Stop, checking (not rewriting) the retained state
  ([#321](https://github.com/chibios-upstream/chibios/pull/321)).
- [STM32] STM32H533xx support (classic and XHAL): registry, mcuconf and
  xmcuconf templates, linker script and the `ST_NUCLEO64_H533RE` board. The
  STM32H5xx CMSIS device headers are updated to ST version 1.7.0
  ([#299](https://github.com/chibios-upstream/chibios/pull/299)).
- [STM32] STM32U5F7 and STM32U5F9 support (classic and XHAL): clock-tree
  variants, registry entries without the crypto blocks these devices lack,
  STM32U595 mcuconf/xmcuconf template support and the `STM32U5FxxI.ld` and
  `STM32U5FxxJ.ld` linker scripts for the 2 MiB and 4 MiB devices
  ([#267](https://github.com/chibios-upstream/chibios/pull/267)).
- [STM32] SYSTICKv3, an STM32U5 system timer on a Smart Run Domain LPTIM
  (LPTIM1, LPTIM3 or LPTIM4, chosen with `STM32_ST_USE_TIMER`) in the classic
  HAL, selected with `USE_ST_LPTIM=yes` in the makefile; SYSTICKv1 remains the
  default. It supports the tickless mode only, with a 16-bit counter
  ([#233](https://github.com/chibios-upstream/chibios/pull/233)).
- [RP] GPT driver for the RP2350 on TIMER1: `GPTD1` to `GPTD4` on the four
  alarms (`RP_GPT_USE_GPT1` to `RP_GPT_USE_GPT4`), counting at 1 MHz or an
  exact divider of it, with continuous mode kept on the period grid and
  missed periods merged into one event; the system timer stays on TIMER0
  ([#447](https://github.com/chibios-upstream/chibios/pull/447)).
- [SB] FreeRTOS as a sandbox guest: bindings in `os/various/freertos_bindings`
  with the `ARMvx-M-SB` port and a VRQ API, FreeRTOS-Kernel V11.3.1 in
  `ext/FreeRTOS-Kernel-11.3.1.7z`, the `SB-CLIENT-FREERTOS` client and the
  `RT-SB-SWITCHED-XSHELL-FREERTOS-MULTI` host demo. Guests not using the XHAL
  VIO port can use the `os/sb/user/sbuservio.h` VIO wrappers. FreeRTOS and RT
  guests recover the tick periods elapsed while not running from host time
  ([#501](https://github.com/chibios-upstream/chibios/pull/501),
  [#505](https://github.com/chibios-upstream/chibios/pull/505),
  [#508](https://github.com/chibios-upstream/chibios/pull/508),
  [#514](https://github.com/chibios-upstream/chibios/pull/514),
  [#515](https://github.com/chibios-upstream/chibios/pull/515)).
- [SB] SIO-backed POSIX TTY for sandboxes: the `tty_i` terminal interface
  (`os/xhal/include/hal_tty.h`) and its XHAL implementation over SIO in
  `os/xhal/lib/complex/posix_tty-sio` (canonical/noncanonical input, echo,
  flow control), exposed as a VFS character device, with `tcgetattr()`,
  `tcsetattr()`, `sys/termios.h` and `sys/ioctl.h` for sandboxed apps. The
  `RT-SB-DYNAMIC-RAMBOX-MULTI` demo loads `msh` from a ROMFS (`make mkfs`)
  ([#308](https://github.com/chibios-upstream/chibios/pull/308),
  [#309](https://github.com/chibios-upstream/chibios/pull/309),
  [#319](https://github.com/chibios-upstream/chibios/pull/319)).
- [ARMV7R] Cortex-R5 support in the ARMv7-R port (`ARM_CORE_CORTEX_R5`) with
  a generic `ARMCR5` ARMCRx startup device. ARMCRx devices now declare fixed
  `CORTEX_HAS_xxx` core capabilities in `crparams.h`, no longer overridable
  from the build, and `CRT0_VBAR_INIT` is rejected on devices without VBAR
  ([#273](https://github.com/chibios-upstream/chibios/pull/273),
  [#285](https://github.com/chibios-upstream/chibios/pull/285)).
- [STM32] OCTOSPIv3 WSPI driver for STM32U5, built on GPDMA (DMA3v1) rather
  than the OCTOSPIv2 MDMA path: the driver and its OCTOSPI IRQ handlers, the
  OCTOSPIM clock wiring, and the STM32U575/U595 mcuconf templates, with the
  transfer-complete ISR waiting for GPDMA idle before the portable ISR code;
  CR-register options and the U5 OCTOSPI IRQ vectors followed, and a
  duplicate OCTOSPI IRQ registration that broke the U5 build with WSPI
  enabled was removed
  ([#228](https://github.com/chibios-upstream/chibios/pull/228),
  [#253](https://github.com/chibios-upstream/chibios/pull/253),
  [#268](https://github.com/chibios-upstream/chibios/pull/268)).
- [STM32] STM32G0xx embedded flash (EFL) support for 256K and 512K devices
  (single-bank; dual-bank G0B1/G0C1 parts must be configured single-bank)
  ([#191](https://github.com/chibios-upstream/chibios/pull/191)).
- [STM32] FDCAN timestamp counter (`TSCC`) configuration, exposed and
  programmed in configuration mode across classic FDCANv1/FDCANv2 and XHAL
  FDCANv1 ([#237](https://github.com/chibios-upstream/chibios/pull/237)).
- [STM32] Configurable oscillator startup timeouts:
  `STM32_CFG_OSCILLATORS_STARTUP_TIME` is the deadline of the oscillators
  started together on STM32C0, G0, G4, H5, U3 and U5 (classic and XHAL),
  defaulting to the previous family value, so a board with a slow HSE can
  raise it ([#271](https://github.com/chibios-upstream/chibios/pull/271)),
  and STM32U5 gained a configurable LSE startup time, independent of the
  shared oscillator startup timeout
  ([#240](https://github.com/chibios-upstream/chibios/pull/240)).
- [STM32] Promoted the generated STM32G4 and STM32U3 clock-tree ports into XHAL
  (previous implementations retained as `_OLD`) and added STM32U5G support
  ([#261](https://github.com/chibios-upstream/chibios/pull/261)).
- [XHAL] XHAL port for the RP2040 and RP2350: ST/PAL, SIO, WDG and DMA services,
  SPI, PWM and ADC drivers, an EFL driver with XIP flash safety, RTC drivers
  with absolute-time alarms, a USB device driver and an I2C driver with
  asynchronous abort, plus RP2350 Hazard3 RISC-V support and a multi-target
  SMP demo console
  ([#199](https://github.com/chibios-upstream/chibios/pull/199),
  [#201](https://github.com/chibios-upstream/chibios/pull/201),
  [#207](https://github.com/chibios-upstream/chibios/pull/207),
  [#208](https://github.com/chibios-upstream/chibios/pull/208),
  [#209](https://github.com/chibios-upstream/chibios/pull/209),
  [#210](https://github.com/chibios-upstream/chibios/pull/210),
  [#211](https://github.com/chibios-upstream/chibios/pull/211),
  [#212](https://github.com/chibios-upstream/chibios/pull/212),
  [#213](https://github.com/chibios-upstream/chibios/pull/213)).
- [RP] RTCv2 driver for the RP2350, built on the POWMAN Always-On timer: a
  millisecond time base, absolute-time alarms with field masking, and a
  hardware validation target
  ([#140](https://github.com/chibios-upstream/chibios/pull/140)).
- [STM32] STM32U0xx embedded flash (EFL) driver
  ([#164](https://github.com/chibios-upstream/chibios/pull/164)).
- [STM32] STM32U5 I2C5 and I2C6 support
  ([#196](https://github.com/chibios-upstream/chibios/pull/196)).
- [RP] RP PIOv1 and DMAv1 API extensions: a state-machine configuration builder
  with pin routing and DMA glue, allocation masks and SM handle access, PIO
  IRQ flag get/clear/force, RP2350 FIFO join modes, a TX-FIFO drain helper,
  `dmaChannelGetCounterX()`, block-level interrupt/callback and GPIO routing,
  runtime state-machine setters and readback, in-place instruction patching
  with synchronized multi-SM enable, and instruction encoders
  ([#138](https://github.com/chibios-upstream/chibios/pull/138),
  [#152](https://github.com/chibios-upstream/chibios/pull/152),
  [#167](https://github.com/chibios-upstream/chibios/pull/167),
  [#168](https://github.com/chibios-upstream/chibios/pull/168),
  [#169](https://github.com/chibios-upstream/chibios/pull/169),
  [#170](https://github.com/chibios-upstream/chibios/pull/170),
  [#222](https://github.com/chibios-upstream/chibios/pull/222),
  [#223](https://github.com/chibios-upstream/chibios/pull/223),
  [#224](https://github.com/chibios-upstream/chibios/pull/224),
  [#225](https://github.com/chibios-upstream/chibios/pull/225)).
- [ARMV6M] Alternate common IRQ dispatcher port for ARMv6-M
  ([#171](https://github.com/chibios-upstream/chibios/pull/171)).
- [RP] RP2350 POWMAN register map added to the CMSIS device header
  ([#147](https://github.com/chibios-upstream/chibios/pull/147)).
- [RT] RT non-releasing thread synchronization
  ([#143](https://github.com/chibios-upstream/chibios/pull/143)) and an
  arbitrary-thread-priority helper
  ([#162](https://github.com/chibios-upstream/chibios/pull/162)).
- [SB] SB explicit sandbox lifecycle
  ([#144](https://github.com/chibios-upstream/chibios/pull/144)), an expanded
  userspace application suite
  ([#153](https://github.com/chibios-upstream/chibios/pull/153)) and shell
  ([#154](https://github.com/chibios-upstream/chibios/pull/154)), and a
  sandbox configuration schema with hardened failed starts
  ([#155](https://github.com/chibios-upstream/chibios/pull/155)).
- [VFS] VFS extended stat metadata support
  ([#151](https://github.com/chibios-upstream/chibios/pull/151)).
- [RP] RP2350 runtime clock switching (`RP_CLOCK_DYNAMIC`, default `FALSE`): real
  `halClockSwitchMode()` support with runtime-validated PLL_SYS
  configurations, flash-timing-safe reclocking with both cores kept executing
  from XIP, dynamic clock point queries and a hardware validation target.
  Optional overclocking behind a second opt-in (`RP_ALLOW_OVERCLOCK`, default
  `FALSE`) with a mandatory explicit flash divider above the rated frequency
  and POWMAN core voltage control up to 1.30 V
  ([#109](https://github.com/chibios-upstream/chibios/pull/109));
  the regulator is unlocked before each voltage change
  ([#357](https://github.com/chibios-upstream/chibios/pull/357)) and
  `RP_CLOCK_DYNAMIC` is rejected at build time on the Hazard3 cores
  ([#361](https://github.com/chibios-upstream/chibios/pull/361)).
- [RP] Hazard3 RISC-V support for RP2350: RT/NIL ports, dual-core SMP, Xh3irq
  interrupt handling, periodic and tickless MTIME support, PMP stack guards,
  startup/HAL integration, demo, and hardware validation target
  ([#90](https://github.com/chibios-upstream/chibios/pull/90)); the
  RP2 port also builds with `CH_CFG_SMP_MODE` disabled
  ([#355](https://github.com/chibios-upstream/chibios/pull/355)).
- [STM32] STM32U5 support extended: EPOD booster clock handling and a generated clock
  tree, the STM32U575ZI-Nucleo144 board and RT-STM32-MULTI demo configuration,
  and the STM32U575xx mcuconf template and updater
  ([#56](https://github.com/chibios-upstream/chibios/pull/56)).
- [STM32] SYSTICKv1 free running mode gained `STM32_ST_FREQUENCY_TOLERANCE`, the
  allowed ST tick deviation in per-mille (default 0 = exact divisor required,
  unchanged behavior), letting devices whose clock tree cannot produce an
  exact multiple (e.g. STM32U0/U3 with MSI feeding the PLL) run without a
  clock-rounding system halt
  ([#17](https://github.com/chibios-upstream/chibios/pull/17)). The STM32U0
  and STM32U3 device configurations set the tolerance to 0.5%
  ([#18](https://github.com/chibios-upstream/chibios/pull/18)).
- [STM32] STM32G4xx: added FSMC RCC macros, IRQ vector definitions and registry
  switch for the FMC-capable devices (G473/G483/G474/G484), in all four G4
  port copies ([#14](https://github.com/chibios-upstream/chibios/pull/14))
  *(backported to 21.11.6)*.
- [BUILD] `ADDITIONAL_OUTFILES` variable in the ARMCMx GCC build rules, included
  makefiles can now add extra generated files to the "all" target (forum bug
  report, [#10](https://github.com/chibios-upstream/chibios/pull/10)).
- [XHAL] Sensors subsystem in XHAL equivalent to EX for the old HAL.
- [HAL] Thread mode for EP0 handling in the USB HAL driver.
- [XHAL] ADC driver in XHAL.
- [XHAL] RTC driver in XHAL, with optional periodic wakeup APIs
  `rtcSetPeriodicWakeup()`/`rtcGetPeriodicWakeup()`, implemented by the
  STM32 RTCv2 (STM32H7 with the legacy RTC, STM32L4R/S) and RTCv3 drivers
  ([#341](https://github.com/chibios-upstream/chibios/pull/341)).
- [SIM] EFL driver for the simulator and a simulator MFS test application.
- [SIM] Simulator support for x86-64 on Posix/Linux.
- [RP] UART driver (LLD) for the RP2040 port.
- [XSHELL] `XSHELL_EXIT_HOOK` in xshell.
- [RT] Dedicated functional safety module in RT.
- [RT] Multicore memory classes modifiers in RT in order to support NUMA
  architectures and non-coherent cache architectures.
- [ARMV7M] MPU initialization settings in the ARMv7-M, ARMv7-M-ALT and ARMv8-M-ML-ALT
  ports, allowing any region to be statically initialized.
- [ARMV7M] Missing context switch hook in the ARMv7-M-ALT and ARMv8-M-ML-ALT ports.
- [VFS] Integration demos for VFS+littlefs/FatFS+XSHELL, now also available on the
  STM32U0 Nucleo-64.
- [ARMV7M] Faster context switch modes in the ARMv7-M and ARMv8-M ports, allowing to
  avoid saving the FP context for threads that do not use the FPU.
- [ARMV8M] New ARMv8-M port aligned with the features of the ARMv7-M port for
  sandboxing, also usable with NIL; the shared `mpu_v8m.h` helpers now
  disable the MPU and encode regions 8 to 15 correctly, and
  `MPU_RBAR_AP_RW_NA` replaces the misnamed `MPU_RBAR_AP_RW_RO`
  ([#325](https://github.com/chibios-upstream/chibios/pull/325)).
- [XSHELL] XShell support for prompt change, multiple commands per line, line editing,
  user definable extra fields, init and execution hooks.
- [VFS] Improved littlefs support: a file system can now be created at arbitrary
  positions in flash.
- [XSHELL] New XShell specific to the new RT, leveraging the thread dispose feature.
- [RT] New RT thread spawning API decoupling the thread stack from the `thread_t`
  structure, as required on NUMA multicore devices. The old "create" API is
  still present and supported.
- [RT] Capability to associate a "dispose" function to threads; the dynamic API
  was modified to use this mechanism.
- [RT] New API `chVTGetCurrentDelta()`; RT virtual timers can now recalculate the
  value of `CH_CFG_ST_TIMEDELTA` at runtime and continue using the
  recalculated value.
- [RT] `waend` field in the RT thread structure for debug convenience.
- [XHAL] Para-virtualized XHAL port for use in sandboxes.
- [SB] VIO subsystem in sandboxes supporting driver para-virtualization, PAL and
  SIO supported so far.
- [RT] RT port for use in virtualized sandboxes.
- [SB] Full virtualization support in sandboxes with a virtual IRQ mechanism.
- [RT] `__CH_OWNEROF()` macro in RT.
- [SB] Posix-flavored shell named "msh" (Mini Shell), able to run sub-apps inside
  the same sandbox, placed statically in flash or loaded dynamically in RAM.
- [SB] Runnable "apps" capability in sandboxes; apps available so far: msh, ls.
- [SB] Ability to load ELF files in sandboxes.
- [SB] Enhanced Posix API for sandboxes leveraging the VFS integration.
- [SB] Sandboxes and VFS integration: each sandbox can see its own VFS instance.
- [RT] `MEM_IS_VALID_FUNCTION()` macro in RT and NIL.
- [RT] `CH_CFG_HARDENING_LEVEL` option in RT, with cumulative levels: object
  integrity checks and clearing on disposal (1), link pair consistency
  checks (2) and validation of fetched pointers before use (3); list and
  virtual timer links are accessed through checked accessors
  ([#297](https://github.com/chibios-upstream/chibios/pull/297),
  [#298](https://github.com/chibios-upstream/chibios/pull/298)).
- [RT] `chXXXDispose()` functions for all objects in RT and NIL.
- [RT] `MEM_NATURAL_ALIGN` macro in RT and NIL.
- [RT] Static initializer for virtual timers in RT.
- [OSLIB] New function `chHeapIntegrityCheck()`.
- [OSLIB] Memory areas/pointers checker functions in OSLIB; the intersection
  checks are symmetric and handle whole-address-space and address-zero
  ranges, and the generic functions stay available with
  `CH_CFG_USE_MEMCHECKS` disabled
  ([#316](https://github.com/chibios-upstream/chibios/pull/316)).

### Changed

- [SB] Sandbox ABI revision, hosts and guests must be rebuilt together, the
  image header carries the ABI version (`hdr_abi`, `SB_ABI_VERSION`) and
  hosts reject images built for another version: the host keeps the VRQ
  status and masks in a 32-byte system block at the top of the sandbox data
  region, guests use it for userspace VRQ critical zones
  (`SB_USER_CFG_FAST_VRQ`, enabled by default, a lock/unlock pair from ~122
  to ~32 cycles on G474, ~24 with the enable inlined by
  `SB_USER_CFG_FAST_VRQ_INLINE`), and VRQ handlers receive the VRQ flags as
  an argument; the `SB_FASTC_VRQ_GCSTS` fastcall and its guest wrappers are
  removed, the Zephyr guest slot overlays exclude the block
  ([#529](https://github.com/chibios-upstream/chibios/pull/529)).
- [NIL] `CH_CFG_USE_MUTEXES`, `CH_CFG_SMP_MODE` and `CH_DBG_STATISTICS`,
  not available in NIL, are checked after `chconf.h` instead of being forced
  to `FALSE` before it, where the assembler modules could still see the
  `chconf.h` value: undefined means `FALSE` and enabling one is now a build
  error. `CH_CFG_SMP_MODE` is added to the NIL configuration template
  ([#519](https://github.com/chibios-upstream/chibios/pull/519)).
- [SB] The guest-side sandbox files (`os/sb/common`, `os/sb/user`, the
  ARMvx-M-SB VRQ entry code and `errcodes.h`) and the FreeRTOS ARMvx-M-SB port
  are relicensed from GPLv3 to the Apache License 2.0, so guests running on
  bare metal or on non-ChibiOS kernels such as FreeRTOS are no longer bound by
  the GPL; license notices only, no code change
  ([#520](https://github.com/chibios-upstream/chibios/pull/520)).
- [RT] The RT8 registry debugger descriptor (`ch_debug`) is redesigned after
  its eight-byte header: 16-bit sizes and offsets, port identification, SMP
  registries, saved core/FPU/MPU contexts and port extensions. Debuggers
  select the layout from the kernel version, RT7 layouts are unchanged
  ([#404](https://github.com/chibios-upstream/chibios/pull/404)).
- [RT] `CH_CFG_IRQ_PROLOGUE_HOOK()` now runs after the ISR statistics, trace
  and state-checker entry code and `CH_CFG_IRQ_EPILOGUE_HOOK()` before the
  matching exit code, so balanced ISR locking inside the hooks agrees with
  the state checker
  ([#293](https://github.com/chibios-upstream/chibios/pull/293)).
- [RT] The Runtime Faults Collection Unit (RFCU) is optional:
  `CH_CFG_USE_RFCU` (default `TRUE`) must be defined in `chconf.h`. When
  `FALSE`, the RFCU API, `CH_CFG_RUNTIME_FAULTS_HOOK` and `chrfcu.c` are left
  out and the virtual timers fall back to debug assertions
  ([#275](https://github.com/chibios-upstream/chibios/pull/275)).
- [XHAL] XHAL drivers no longer serve interrupt events after `drvStop()` has
  started or while no operation is running, so a late event can no longer
  invoke the callback during a stop or overwrite `HAL_DRV_STATE_STOPPING`
  with `READY`. SDC callbacks no longer run with the system lock held, as in
  the other XHAL drivers, and the RP SIO, I2C and USB drivers assert that
  `drvStop()` is called from the core that started them
  ([#461](https://github.com/chibios-upstream/chibios/pull/461),
  [#491](https://github.com/chibios-upstream/chibios/pull/491),
  [#492](https://github.com/chibios-upstream/chibios/pull/492),
  [#493](https://github.com/chibios-upstream/chibios/pull/493),
  [#494](https://github.com/chibios-upstream/chibios/pull/494),
  [#495](https://github.com/chibios-upstream/chibios/pull/495),
  [#496](https://github.com/chibios-upstream/chibios/pull/496),
  [#497](https://github.com/chibios-upstream/chibios/pull/497),
  [#498](https://github.com/chibios-upstream/chibios/pull/498),
  [#499](https://github.com/chibios-upstream/chibios/pull/499),
  [#500](https://github.com/chibios-upstream/chibios/pull/500),
  [#513](https://github.com/chibios-upstream/chibios/pull/513)).
- [XHAL] `drvStop()` on an ADC or SPI driver now aborts a running conversion or
  transfer and releases the waiting thread with `MSG_RESET`, which most low
  level drivers left blocked. The `stop` virtual method of `hal_base_driver`
  is now `stop(void *ip, driver_state_t prevstate)`; overriding driver
  classes must add the parameter
  ([#460](https://github.com/chibios-upstream/chibios/pull/460)).
- [XHAL] STM32 XHAL interrupt priorities are set only by the platform-owned
  `STM32_IRQ_<vector>_PRIORITY` settings; legacy per-driver ones such as
  `STM32_SPI_SPI1_IRQ_PRIORITY` are no longer accepted, so custom `xmcuconf.h`
  files must be regenerated and non-default priorities moved. ADC and WSPI DMA
  interrupts take the priority of their peripheral vector, the STM32H7 WSPI
  vectors that of `STM32_IRQ_MDMA_PRIORITY`
  ([#337](https://github.com/chibios-upstream/chibios/pull/337)).
- [STM32] The Ethernet PHY reset delay `BOARD_PHY_RESET_DELAY`, in core clock
  dependent realtime counter cycles, is replaced by `BOARD_PHY_RESET_DELAY_US`,
  in microseconds, in the classic MACv1/MACv2 and XHAL ETHv2 drivers; defining
  the old name is now a build error. The classic STM32F1, F4, F7 and H7 ports
  start the DWT cycle counter in `hal_lld_init()`, so polled delays in
  `halInit()` no longer hang without a debugger
  ([#443](https://github.com/chibios-upstream/chibios/pull/443)).
- [STM32] STM32 OTGv1 isochronous endpoint support is now compiled only when
  `STM32_USB_USE_ISOCHRONOUS` is `TRUE` (default `FALSE`), as in the USBv1/USBv2
  drivers (classic and XHAL): applications using isochronous endpoints on an OTG
  controller must enable it, otherwise such an endpoint is left inactive
  ([#425](https://github.com/chibios-upstream/chibios/pull/425)).
- [STM32] The bit rate switch field of the STM32 FDCAN `CANTxFrame` is renamed
  from `BPS` to `BRS` (classic FDCANv1 and FDCANv2, XHAL FDCANv1); the frame
  layout is unchanged, applications using `.BPS` must be updated
  ([#300](https://github.com/chibios-upstream/chibios/pull/300)).
- [SB] The sandbox ABI is defined in `os/sb/common/sbsysc.h`, usable from C
  and assembler: guests use its `SB_FASTC_xxx`/`SB_SYSC_xxx` codes and the
  host fastcall and syscall tables are built from them, replacing the
  per-number `SB_SVCnnn_HANDLER` definitions. It also exports the VIO return
  codes, VRQ flags, I2C errors and UART events. The XHAL VIO port uses the
  `sbuservio.h` wrappers, whose prefixes do not collide with the XHAL API
  ([#504](https://github.com/chibios-upstream/chibios/pull/504),
  [#510](https://github.com/chibios-upstream/chibios/pull/510)).
- [VFS] VFS file nodes no longer implement `random_stream_i`:
  `vfsGetFileStream()` and the `getstream` method are replaced by file control
  operations, `vfsControlFile()` and the `vfsFileControl()` method. Streams
  driver tables are written with the `DRV_STREAMS_ELEMENT_xxx()` initializers
  ([#308](https://github.com/chibios-upstream/chibios/pull/308)).
- [ARM] The generic ARM port (`os/common/ports/ARM`) is limited to ARM7 and
  ARM9: the Cortex-A5/A7/A8/A9 core selectors are removed, and
  `PORT_SUPPORTS_RT` is now `FALSE`, the realtime counter always read zero on
  ARM7/ARM9
  ([#280](https://github.com/chibios-upstream/chibios/pull/280)).
- [RT] Removed the optional `PORT_SYSTEM_STATE_ACQUIRE()`/`_RELEASE()` startup
  hooks (the only merged implementation was Hazard3's)
  ([#244](https://github.com/chibios-upstream/chibios/pull/244)).
- [XHAL] XHAL no longer depends on OSAL
  ([#156](https://github.com/chibios-upstream/chibios/pull/156)); a redundant
  MMC SPI module switch was removed
  ([#157](https://github.com/chibios-upstream/chibios/pull/157)).
- [STM32] STM32 RTC clock validation hardened
  ([#197](https://github.com/chibios-upstream/chibios/pull/197)).
- [RP] RP2350 EFL interrupt masking made architecture-neutral (Arm and RISC-V)
  ([#139](https://github.com/chibios-upstream/chibios/pull/139)); on RISC-V,
  `RP_EFL_XIP_SAFETY_LOCKOUT` is rejected at build time instead of linking
  no-op hooks ([#356](https://github.com/chibios-upstream/chibios/pull/356)).
- [SB] SB sandbox VFS root is now optional and explicitly externally owned.
  `sbSetFileSystem()` is renamed `sbSetRoot()` and a matching `sbGetRoot()`
  accessor is added. A `NULL` root is allowed, leaving the sandbox without a
  path namespace while operations on registered file descriptors remain
  available. Enabling the SB VFS support now requires
  `VFS_CFG_ENABLE_DRV_ROOT == TRUE`
  ([#91](https://github.com/chibios-upstream/chibios/pull/91)).
- [SB] Coding-style cleanup (whitespace, spacing and comment formatting), no
  functional change: HAL sources
  ([#20](https://github.com/chibios-upstream/chibios/pull/20)), XHAL
  hand-written sources
  ([#22](https://github.com/chibios-upstream/chibios/pull/22)), VFS sources
  ([#23](https://github.com/chibios-upstream/chibios/pull/23)), XHAL OOP
  driver code generator and regenerated sources
  ([#24](https://github.com/chibios-upstream/chibios/pull/24)), SB sources
  ([#26](https://github.com/chibios-upstream/chibios/pull/26)), os/common
  sources ([#27](https://github.com/chibios-upstream/chibios/pull/27)),
  os/various sources
  ([#29](https://github.com/chibios-upstream/chibios/pull/29)) and os/hal/lib
  sources ([#30](https://github.com/chibios-upstream/chibios/pull/30)).
- [BUILD] `tools/style/stylecheck.py` no longer reports two false positives:
  `#endif /* defined(X) */` guard comments and operators/commas inside string
  literals; strictly more permissive
  ([#28](https://github.com/chibios-upstream/chibios/pull/28)).
- [OSLIB] Memory areas functions in OSLIB addressed for portability.
- [NIL] Implemented a better `chThdSleepUntil()` in NIL using the same logic used
  in the RT implementation.
- [RT] Function `chSftIntegrityCheckI()` rewritten to be much more efficient in
  performing lists integrity checks.
- [RT] Function `chSysIntegrityCheckI()` moved into the new RT functional safety
  module and renamed to `chSftIntegrityCheckI()`.
- [ARMV7M] Improved interrupts processing in the ARMv7-M-ALT and ARMv8-M-ML-ALT ports,
  saving a few cycles on the context switch code path.
- [RT] Recursive locks in RT and NIL made optional, only enabled if the underlying
  port supports the capability.
- [OSLIB] OSLIB release methods now return the value of the reference counter.
- [SB] SB configuration option names changed to be prefixed with `SB_CFG_`.
- [RT] Function `chCoreGetStatusX()` changed to return a memory region object
  instead of a simple size.
- [OSLIB] RT and NIL upgraded to support the enhanced OSLIB.

### Fixed

- [SB] A sandbox executing an SVC with an invalid stack pointer was not
  terminated: the SVC left pending by the failed exception stacking was taken
  on the fault exit frame and resumed the sandbox with the kernel lock held,
  debug hosts halted with `SV#4`; the pending SVC is now discarded
  ([#535](https://github.com/chibios-upstream/chibios/pull/535))
  *(backported to 21.11.6)*.
- [SB] The sandbox cleanup stopped the VIO UART units while data accepted by
  the TX FIFO was still being transmitted, the last characters written by a
  sandbox before exiting were lost or truncated; the cleanup now waits for
  the end of the transmission, for at most 100 ms per unit
  ([#534](https://github.com/chibios-upstream/chibios/pull/534)).
- [RP] Test 6 of the RP2040 and RP2350 `PIO-VALIDATION` tests (the RP2350 one
  also built as `PIO-VALIDATION-RISCV`) waited for the end of its DMA stream on
  the sticky `TXSTALL` flag, already set by the stall before the DMA started,
  and stopped the state machine while the last word was still being shifted
  out (63 of 64 words on the pin) while still reporting a pass; the flag is
  now cleared once the FIFO is being fed
  ([#531](https://github.com/chibios-upstream/chibios/pull/531)).
- [RT] The tickless timer boundary test cases 3.4 and 3.5 of the RT test
  suite failed on applications with other armed virtual timers, such as the
  `RT-STM32-XSHELL` LED blinker; the cases now use the list coordinate of
  their timers and skip the head saturation step when other timers are
  armed ([#530](https://github.com/chibios-upstream/chibios/pull/530)).
- [STM32] The instruction cache was never enabled on STM32H5 (classic and
  XHAL), STM32U5 (classic and XHAL, a regression from the previous U5 port,
  which enabled it) and STM32C5: the ports did not call `icache_init()`, so
  the code executed from flash with the full wait states; the cache is now
  enabled at clock initialization, and the XHAL H5 xmcuconf templates default
  to the 2-way mode instead of the direct-mapped one
  ([#527](https://github.com/chibios-upstream/chibios/pull/527))
  *(backported to 21.11.6)*.
- [SB] The RT/NIL `ARMvx-M-SB` sandbox port did not save FPSCR in the thread
  context of FPU builds, threads switched voluntarily could see the FPU
  rounding modes and flags of other threads; FPSCR is now saved as in the
  `ARMv7-M` port
  ([#526](https://github.com/chibios-upstream/chibios/pull/526)).
- [RT] RT tracing: a halt on a secondary SMP core could write a halt record
  through the trace buffer pointer of a not yet initialized instance; trace
  readiness is now tracked per instance. `CH_DBG_TRACE_BUFFER_SIZE` must be
  in the 1..65535 range when tracing is enabled, and `chTraceWrite()` and
  `chTraceWriteI()` become empty, overridable macros when tracing is off
  ([#296](https://github.com/chibios-upstream/chibios/pull/296)).
- [RT] RT virtual timers and RFCU: with RFCU enabled, a skipped periodic
  timer deadline still triggered the "skipped deadline" debug assertion
  before the fault could be collected, the assertion is now only the
  fallback when RFCU is disabled. A tickless first-alarm retry programmed
  the alarm from a stale time base
  ([#294](https://github.com/chibios-upstream/chibios/pull/294)).
- [RT] RT and NIL events: `chEvtGetEventsX()` could return a stale value when
  polled, it now reloads the field, and `chEvtUnregister()` no longer
  accesses the event source through an incompatible listener pointer. Zero
  wait masks are now rejected in both kernels
  ([#286](https://github.com/chibios-upstream/chibios/pull/286)).
- [RT] RT time measurement: an interval shorter than the calibrated overhead
  wrapped around to a huge value, it is now recorded as zero. Calibration
  ran once before the port initialization, possibly with a stopped counter,
  it is now done per OS instance after it. `S2RTC()`, `MS2RTC()`, `US2RTC()`,
  `RTC2S()`, `RTC2MS()` and `RTC2US()` rounded before scaling and returned
  non-zero for zero cycles, they now compute over the whole value
  ([#277](https://github.com/chibios-upstream/chibios/pull/277),
  [#278](https://github.com/chibios-upstream/chibios/pull/278),
  [#283](https://github.com/chibios-upstream/chibios/pull/283),
  [#284](https://github.com/chibios-upstream/chibios/pull/284)).
- [NIL] NIL tickless mode encoded finite timeouts incorrectly after a long
  inactive interval and when a new full-range wait could not be represented
  relative to the existing time origin; existing waits are now rebased when
  needed. `chTimeAddX()` returns a wrapped `systime_t`, and non-smart builds
  (`USE_SMART_BUILD=no`) no longer stop with a "missing separator" error in
  `os/nil/nil.mk`
  ([#323](https://github.com/chibios-upstream/chibios/pull/323))
  *(backported to 21.11.6)*.
- [OSLIB] OSLIB factory: object reference counters could overflow without
  any diagnostic; `FACTORY_MAX_REFERENCES` is added and every reference
  increment asserts room for another reference
  ([#322](https://github.com/chibios-upstream/chibios/pull/322)).
- [OSLIB] `chDelegateDispatchTimeout(TIME_IMMEDIATE)` passed `TIME_IMMEDIATE`
  to `chMsgWaitTimeout()`, which the RT message API does not accept; an
  immediate dispatch now polls with `chMsgPoll()` and returns `MSG_TIMEOUT`
  on an empty queue, in RT and NIL. `chJobObjectInit()` limits the capacity
  to 1..`SEMAPHORE_MAX_COUNT`
  ([#313](https://github.com/chibios-upstream/chibios/pull/313),
  [#314](https://github.com/chibios-upstream/chibios/pull/314)).
- [OSLIB] OSLIB pipes: a reader or writer could miss the wakeup of a
  concurrent transfer between its check and suspending, and a caller queued
  on a direction lock could wait on, or refill, a pipe reset in the meantime;
  counts and the reset state are now rechecked under the system lock.
  `chPipeResume()` is now an out-of-line, thread-context function
  ([#311](https://github.com/chibios-upstream/chibios/pull/311)).
- [OSLIB] OSLIB mailboxes and objects FIFOs: `chMBPostAheadTimeoutS()` and
  `chMBPostAheadI()` formed a pointer before the mailbox buffer when wrapping
  the read pointer, and the objects FIFO receive functions stored the
  received pointer through an incompatible `msg_t` lvalue, it is now
  converted from a local `msg_t` on `MSG_OK` only
  ([#302](https://github.com/chibios-upstream/chibios/pull/302)).
- [OSLIB] OSLIB memory allocators: an oversized core memory request could
  pass the arena bounds check through address wraparound, it is now rejected
  before either arena boundary changes. `chHeapIntegrityCheck()` also
  bounds-checks free-block headers and rejects overlapping free blocks
  ([#301](https://github.com/chibios-upstream/chibios/pull/301)).
- [OOP] `roRelease()` read the object after unlocking and possibly disposing
  it, a use-after-free when the destructor frees the object; the decremented
  count is now saved under the lock and returned
  ([#327](https://github.com/chibios-upstream/chibios/pull/327)).
- [OOP] The OOP null stream (`null_stream_c`) reported reads as successful:
  `stmRead()` returned the requested size without writing the buffer and
  `stmGet()` returned 4. Reads now report end of file, `stmRead()` returns
  zero and `stmGet()` returns `STM_RESET`
  ([#308](https://github.com/chibios-upstream/chibios/pull/308)).
- [HAL] USB driver core fixes. Host GET_STATUS, SYNCH_FRAME and
  CLEAR_FEATURE/SET_FEATURE(ENDPOINT_HALT) requests for an invalid endpoint are
  now stalled instead of dereferencing NULL on RP or accessing past the STM32
  USBv1/USBv2 endpoint registers. Also fixed: a NULL dereference with
  `USB_USE_WAIT` when an endpoint callback disabled the endpoints, busy bits
  lost to higher priority handlers and empty IN control replies timing out
  ([#415](https://github.com/chibios-upstream/chibios/pull/415),
  [#423](https://github.com/chibios-upstream/chibios/pull/423),
  [#432](https://github.com/chibios-upstream/chibios/pull/432),
  [#450](https://github.com/chibios-upstream/chibios/pull/450))
  *(backported to 21.11.6)*.
- [HAL] `sdcConnect()` asked MMC/eMMC cards for their relative card address
  (RCA) with CMD3 as it does with SD cards, but MMC/eMMC cards expect the
  host to assign it; MMC cards are now assigned RCA 2, SD cards are unchanged
  ([#290](https://github.com/chibios-upstream/chibios/pull/290))
  *(backported to 21.11.6)*.
- [STM32] STM32L0xx `stm32_isr.h` redefined the DMA IRQ-sharing masks
  `STM32_DMA1_CH4_CMASK` and `STM32_DMA1_CH5_CMASK`, already defined in the
  registry, causing redefinition warnings on STM32L011 builds; the duplicate
  is removed ([#509](https://github.com/chibios-upstream/chibios/pull/509))
  *(backported to 21.11.6)*.
- [STM32] The STM32F303, L07x, L422, L432 and WB55 mcuconf templates lacked the
  USB settings `STM32_USB_USE_ISOCHRONOUS` and `STM32_USB_USE_FAST_COPY`, and
  the G4x1/G4x4 templates `STM32_USB_USE_FAST_COPY`; they are added with the
  driver defaults (`FALSE`)
  ([#490](https://github.com/chibios-upstream/chibios/pull/490)).
- [STM32] After a remote wakeup, STM32 USBv1 and USBv2 devices affected by the
  ESOF timing erratum (STM32G4, L0, U0) could report a false suspend. Suspend
  detection is now held off until the host resumes the bus, for at most the new
  `STM32_USB_HOST_RESUME_TIMEOUT` (default 500 ms) (classic and XHAL)
  ([#487](https://github.com/chibios-upstream/chibios/pull/487))
  *(backported to 21.11.6)*.
- [STM32] The STM32 OTGv1 full speed turnaround time (`GUSBCFG.TRDT`) was fixed
  at 5, below the reference manual minimum, so below 32 MHz the core answered IN
  tokens too late (an STM32L4 at 23.986 MHz did not enumerate). It now follows
  `STM32_HCLK` (classic and XHAL), based on a patch by Bob Anderson
  ([#486](https://github.com/chibios-upstream/chibios/pull/486))
  *(backported to 21.11.6)*.
- [STM32] On STM32C051, C071, C091 and C092 with `STM32_CLOCK_DYNAMIC`
  (classic and XHAL), `halClockSwitchMode()` ignored the `RCC_CR` `SYSDIV`
  divider, overstating the clock points used for baud rates, timers, timeouts,
  `SystemCoreClock` and flash wait states; the divider is now applied
  ([#481](https://github.com/chibios-upstream/chibios/pull/481))
  *(backported to 21.11.6)*.
- [STM32] The HAL timeout counter of STM32C0, G0 and U0 (TIM17 on C0 and G0,
  TIM7 on U0) was stopped by the blanket APB reset in `stm32_clock_init()`
  right after being started, so timeouts never expired and `halSftDelayX()`
  never returned, hanging drivers such as STM32 USBv2; its prescaler also
  kept the old rate across clock switches. The timer is now left out of the
  reset and its prescaler follows each clock change (classic and XHAL)
  ([#470](https://github.com/chibios-upstream/chibios/pull/470),
  [#478](https://github.com/chibios-upstream/chibios/pull/478))
  *(backported to 21.11.6)*.
- [STM32] STM32C0 clock initialization and `halClockSwitchMode()` could run
  SYSCLK above 24 MHz with zero flash wait states, the HSISYS divider taking
  effect before the flash latency was set. The latency is now raised first and
  lowered only once the target clock is active (classic and XHAL)
  ([#476](https://github.com/chibios-upstream/chibios/pull/476))
  *(backported to 21.11.6)*.
- [STM32] TIM17, the HAL timeout counter on STM32G0, was not marked as used
  (`STM32_TIM17_IS_USED`), so GPT, PWM or the ST could take it over, after
  which timeouts never expired and `halSftDelayX()` never returned; such
  configurations now fail at compile time (classic and XHAL)
  ([#474](https://github.com/chibios-upstream/chibios/pull/474))
  *(backported to 21.11.6)*.
- [STM32] STM32 SPIv1 and SPIv2 on DMAv2 devices (STM32F4, F7) could invoke
  the completion callback after `spiStopTransfer()`, from a DMA event left
  pending during the stop; DMA events are now ignored unless the driver is
  `SPI_ACTIVE` ([#463](https://github.com/chibios-upstream/chibios/pull/463))
  *(backported to 21.11.6)*.
- [STM32] STM32 USBv1 and USBv2 USB drivers (classic and XHAL):
  CLEAR_FEATURE(ENDPOINT_HALT) did not reset the bulk and interrupt endpoint
  data toggle to DATA0, so the next packet could be dropped as a duplicate, and
  after clearing a halt set during a transfer, that transfer never completed.
  Clearing a halt now resets the toggle and resumes the transfer; USBv2 endpoint
  initialization restarts the toggles from DATA0
  ([#412](https://github.com/chibios-upstream/chibios/pull/412),
  [#448](https://github.com/chibios-upstream/chibios/pull/448))
  *(backported to 21.11.6)*.
- [STM32] Isochronous fixes in the STM32 OTGv1, USBv1 and USBv2 USB drivers
  (classic and XHAL). OTGv1 gave a callback every other frame on incomplete OUT
  transfers or rewrote an enabled endpoint; it now reports the missed frame
  once, with no data. USBv1/USBv2 copied OUT packets with no transfer active
  through a stale pointer and repeated IN packets instead of sending zero-length
  packets; on USBv1 both handlers could serve an endpoint
  ([#410](https://github.com/chibios-upstream/chibios/pull/410),
  [#445](https://github.com/chibios-upstream/chibios/pull/445))
  *(backported to 21.11.6)*.
- [STM32] STM32 ADCv4 (H7), ADCv6 (H5), ADCv7 (C5, U3) (classic and XHAL) and
  XHAL ADCv3 (G4, L4+): regulator startup and calibration delays were sized from
  the configured clock, too short after a run-time switch to a faster clock.
  They now use `halSftDelayX()`, requiring the platform counter hooks
  ([#439](https://github.com/chibios-upstream/chibios/pull/439)).
- [STM32] STM32 USBv1 and USBv2 USB driver packet memory and interrupt fixes
  (classic and XHAL): a final full OUT packet overflowing the receive buffer was
  copied past its end, receive buffers above 62 bytes were undersized in packet
  memory, overwriting the next buffer, unaligned SETUP packets faulted on
  STM32F0, and stale events followed a bus reset. USBv2 now waits 800 ns before
  reading OUT data from the packet memory (STM32H5, U0 errata)
  ([#378](https://github.com/chibios-upstream/chibios/pull/378),
  [#419](https://github.com/chibios-upstream/chibios/pull/419),
  [#436](https://github.com/chibios-upstream/chibios/pull/436))
  *(backported to 21.11.6)*.
- [STM32] STM32 USBv2 (C0, G0, H5, C5, U0, U3, U5) `usbStop()` left the
  peripheral drawing about 0.9 mA and requested resume signalling while powering
  it down; `usb_lld_stop()` now follows ST's stop sequence and resets the block
  through RCC before gating its clock, which also leaves the D+ pull-up off. The
  USBv1 and USBv2 `usb_lld_start()` now waits tSTARTUP before releasing the
  transceiver reset (classic and XHAL)
  ([#427](https://github.com/chibios-upstream/chibios/pull/427))
  *(backported to 21.11.6)*.
- [STM32] `adcSTM32SetCCR()` in STM32 ADCv5 (classic and XHAL; STM32C0, G0, U0
  and WL) and classic ADCv1 (STM32L0) cleared the `ADC_CCR` prescaler set by
  `adcStart()`, e.g. when enabling VREFINT, leaving the ADC clock undivided
  (out of specification on a 56 MHz STM32U0); `PRESC` is now preserved
  ([#426](https://github.com/chibios-upstream/chibios/pull/426))
  *(backported to 21.11.6)*.
- [STM32] The STM32 USBv1 and USBv2 USB drivers (classic and XHAL) enabled the
  unserved ERR interrupt, which the floating bus of an unplugged device with the
  D+ pull-up on (self-powered) raised continuously, starving every thread. ERR
  is no longer enabled; bus errors are still recovered by retransmission
  ([#421](https://github.com/chibios-upstream/chibios/pull/421))
  *(backported to 21.11.6)*.
- [STM32] STM32 OTGv1 USB driver hardened with the XHAL fixes: `usbConnectBus()`
  and `usbDisconnectBus()` work on stepping 1 devices (STM32F105/F107, F2,
  F401/F405/F407/F411/F427/F429) without VBUS sensing or with a ULPI PHY,
  `usbGetFrameNumberX()` works, and hardware timeouts make `usbStart()` fail
  with `HAL_RET_HW_FAILURE` or disconnect the device. The driver needs the
  safety counter hooks; preempted `halRegWait*X()` no longer time out falsely
  ([#408](https://github.com/chibios-upstream/chibios/pull/408),
  [#418](https://github.com/chibios-upstream/chibios/pull/418))
  *(backported to 21.11.6)*.
- [STM32] STM32 OTGv1 USB driver EP0, FIFO and PHY timing fixes. OTG1 and OTG2
  shared their EP0 state and SETUP buffer, so traffic on one could corrupt the
  control transfers of the other. A reconfiguration disturbed EP0 and could
  overlap the EP0 FIFO, IN FIFO depths were not rounded up, and the 4 Kbyte
  STM32H7 OTG1 FIFO was limited to 1.25 Kbytes. An STM32H723 could hang at
  start, and `STM32_USB_OTGFIFO_FILL_BASEPRI` dropped a stronger BASEPRI mask
  ([#333](https://github.com/chibios-upstream/chibios/pull/333),
  [#380](https://github.com/chibios-upstream/chibios/pull/380),
  [#417](https://github.com/chibios-upstream/chibios/pull/417))
  *(backported to 21.11.6)*.
- [STM32] STM32 DACv1 driver fixes ported from XHAL (F0, F1, F3, F37x, F4, F7,
  G0, G4, H7, L0, L1, L4, L4+, WL). Stopping a CH2 conversion in single mode
  turned CH1 on and left CH2 off; a DMA error released a NULL DMA stream; a
  half-buffer callback stopping or restarting the conversion could dereference
  the released group or get the old conversion's full-buffer event. DMA events
  are now ignored with no conversion active
  ([#414](https://github.com/chibios-upstream/chibios/pull/414))
  *(backported to 21.11.6)*.
- [STM32] STM32 DACv2 driver fixes ported from XHAL (H5, U5, U3, C5). Dual mode
  moved the wrong data in 12-bit (fixed with #410) and 8-bit modes. Double DMA,
  HFSEL and the CH1 settings restored on stop were set wrongly, depth-one
  conversions reported half-buffer events, and stopping or restarting from the
  half-buffer callback could dereference the released group. Underrun interrupts
  are now always enabled during conversions
  ([#413](https://github.com/chibios-upstream/chibios/pull/413))
  *(backported to 21.11.6)*.
- [STM32] STM32 ADCv3, ADCv5 and ADCv6 error handling: error interrupts were
  only enabled for groups with an error callback, so a failed conversion
  neither stopped nor woke a waiting thread, and errors were reported
  outside an active conversion. In ADCv3/ADCv6 dual mode, slave overruns were
  not detected; both ADCs are now captured and acknowledged before one
  combined error notification, and an ADCv6 dual-mode build error is fixed
  ([#395](https://github.com/chibios-upstream/chibios/pull/395),
  [#397](https://github.com/chibios-upstream/chibios/pull/397),
  [#399](https://github.com/chibios-upstream/chibios/pull/399))
  *(backported to 21.11.6)*.
- [STM32] STM32 ADC drivers (ADCv1 to ADCv7, STM32F1, F37x and L1) handled DMA
  events with no active conversion, so a late DMA error could enter the error
  handler after the group was cleared and an inactive driver could receive
  callbacks; DMA events are now ignored unless the driver is `ADC_ACTIVE`
  ([#393](https://github.com/chibios-upstream/chibios/pull/393))
  *(backported to 21.11.6)*.
- [STM32] STM32 ADCv3 in dual mode on STM32F3 halted on a debug assertion with
  a correctly initialized slave ADC: the slave regulator check used the wrong
  mask and now matches the master check
  ([#390](https://github.com/chibios-upstream/chibios/pull/390))
  *(backported to 21.11.6)*.
- [STM32] STM32 SPIv2 I2S: valid DMA stream assignments were rejected on
  devices without DMAMUX, the SPI3 RX/TX DMAMUX request sources were not
  programmed, and a DMA allocation failure in `i2sStart()` was not handled; on
  that failure the driver is now left stopped and safe to retry
  ([#388](https://github.com/chibios-upstream/chibios/pull/388))
  *(backported to 21.11.6)*.
- [STM32] STM32 SPIv2: stopping a transfer in slave mode waited indefinitely
  for BSY to clear, which needs the external clock, and read the remaining
  frame count before RX DMA had stopped. Slave stops now halt both DMA streams
  and reset the SPI; a new transfer can start without another `spiStart()`
  ([#386](https://github.com/chibios-upstream/chibios/pull/386))
  *(backported to 21.11.6)*.
- [STM32] STM32 SPIv3 (STM32H7) and SPIv4 (STM32H5, U3, U5, C5): DMA and
  overrun interrupts were served while the driver was inactive, an RX DMA
  error also dispatched a completion, and a circular full-buffer callback
  still ran after the half callback stopped or restarted the transfer. SPIv4
  reported the remaining count on stop in bytes instead of frames; SPIv3
  `SPID6` is now placed in `.ram4_clear` for BDMA (`SPI_SPID6_MEMORY` overrides)
  ([#382](https://github.com/chibios-upstream/chibios/pull/382),
  [#384](https://github.com/chibios-upstream/chibios/pull/384))
  *(backported to 21.11.6)*.
- [STM32] STM32 FDCANv2 (STM32H7): stopping one controller or a failed start
  could reset or gate the FDCAN clock shared with a running peer, and start
  failures did not reach `canStart()`; the clock is now reference-counted and
  `canStart()` returns configuration errors and timeouts. Classic-CAN frames
  with DLC 9..15 copied more than eight bytes, the extended-filter type mask
  was wrong, and stale filters stayed active after a table shrank or was cleared
  ([#376](https://github.com/chibios-upstream/chibios/pull/376))
  *(backported to 21.11.6)*.
- [STM32] The STM32 USARTv1/v2/v3 Serial and UART drivers and the USARTv2 SIO
  driver computed the baud rate from a kernel clock captured at init, stale
  after a clock change with dynamic clock management, and the USARTv2/v3 SIO
  drivers truncated the divider instead of rounding it. The rounded divider is
  now computed at start from the current clock and the Serial drivers disable
  the USART before writing it. The `clock` field of these structures is removed
  ([#375](https://github.com/chibios-upstream/chibios/pull/375)).
- [STM32] On dual-core STM32H7 devices (H745/H747/H755/H757) with assertions
  enabled, the first `sioStart()` (USARTv3), `i2cStart()` (I2Cv3), XHAL ETH
  initialization or DMA stream free (DMAv2) halted the system on a reset
  before clock enable, or on disabling a never-enabled DMA controller clock.
  Clocks are now enabled before resets and `dmaStreamFreeI()` disables only
  the controller owning the freed stream (classic and XHAL)
  ([#370](https://github.com/chibios-upstream/chibios/pull/370))
  *(backported to 21.11.6, classic DMAv2, USARTv3 SIO and I2Cv3 fixes only)*.
- [STM32] STM32H7A3/H7B3/H7B0 in the classic HAL: builds needing BDMA (e.g.
  SPI6 or I2C4) failed to compile because the BDMA RCC helpers used the `BDMA`
  bit names, which these devices call `BDMA2`, and the OCTOSPI interrupt
  handlers and their NVIC setup were missing; both are fixed
  ([#329](https://github.com/chibios-upstream/chibios/pull/329)).
- [STM32] The DWT-based safety timeout counter of STM32U3 and STM32U5 (classic
  and XHAL), STM32C5 and the XHAL STM32H5 took its frequency from HCLK values
  that can be zero, stale or not yet reached at boot, so timeouts were scaled
  wrongly during startup and clock switches; it now uses `SystemCoreClock`
  ([#270](https://github.com/chibios-upstream/chibios/pull/270)).
- [RP] RP UARTv1 SIO driver: `sioSynchronizeRXIdle()` could not return
  `MSG_OK` and `SIO_EV_RXIDLE` was hidden from the event getters and
  callbacks; the ISR now latches the idle state and the idle event
  ([#467](https://github.com/chibios-upstream/chibios/pull/467)).
  Reconfiguring a running driver with `sioStart()` wrote the baud rate and
  line control with the UART enabled, which the PL011 does not allow; the
  UART is now disabled first, and the line must be idle when reconfiguring
  ([#469](https://github.com/chibios-upstream/chibios/pull/469)).
- [RP] RP2040 RTCv1 driver: `rtcSetTime()` could leave the date at its reset
  value, a `NULL` alarm specification was dereferenced instead of disabling
  the alarm, and a disabled alarm could still be reported or fire
  ([#113](https://github.com/chibios-upstream/chibios/pull/113)). A read
  right after a set returned the previous date/time; the set now waits for
  the new values, and the XHAL driver reports a never-set RTC as not set
  ([#455](https://github.com/chibios-upstream/chibios/pull/455))
  *(backported to 21.11.6)*.
- [RP] RP DMAv1 interrupt service (classic and XHAL): a channel restarted
  from another channel's callback could get the cancelled event and be
  disabled (debug builds halted); pending bits are now re-read per channel
  ([#452](https://github.com/chibios-upstream/chibios/pull/452)).
  Releasing a channel from the other core could make the ISR call a `NULL`
  handler ([#360](https://github.com/chibios-upstream/chibios/pull/360)), and
  stopping an SPI driver there tripped a `dmaChannelFreeI()` assertion
  ([#358](https://github.com/chibios-upstream/chibios/pull/358)).
- [RP] RP TIMERv1 system timer (classic and XHAL): an alarm stopped or
  re-armed after it fired could cause a spurious callback or a debug halt
  (classic) or be lost (XHAL); the callback now runs only when the alarm is
  pending ([#442](https://github.com/chibios-upstream/chibios/pull/442)).
  A deadline already passed by less than `RP_ST_MISSED_WINDOW` (2^16) ticks
  is re-armed 2 ticks ahead instead of waiting about 71 minutes for the wrap
  ([#444](https://github.com/chibios-upstream/chibios/pull/444)).
- [RP] RP GPIOv1 line mode programming (classic and XHAL) could briefly drive
  a line in an unrequested state, because the output was enabled before the
  IO mux write and the pad output disable (`PAL_RP_PAD_OD`); it is now enabled
  after both, and re-applying an output mode keeps a driven line driven
  ([#353](https://github.com/chibios-upstream/chibios/pull/353)).
- [RP] RP DMAv1 (classic and XHAL) set the priority of a core's shared DMA
  vector only on the core's first channel allocation, so priorities requested
  later were dropped, depending on the driver start order. The vector now
  runs at the most urgent priority requested by the core's allocations, and
  `pioSmAllocI()` does the same for the PIO vectors. On Hazard3, debug builds
  assert if an ISR-context allocation raises a live vector's priority
  ([#351](https://github.com/chibios-upstream/chibios/pull/351),
  [#352](https://github.com/chibios-upstream/chibios/pull/352)).
- [RP] RP SPIv1 (classic and XHAL) read the dummy TX word of receive-only and
  ignore operations from XIP flash in flash-linked builds, so one overlapping
  a flash erase or program could fail its TX DMA (on the RP2350 the classic
  driver ended it short, silently); the word is now placed in RAM
  ([#350](https://github.com/chibios-upstream/chibios/pull/350)).
- [SB] Sandbox FP context: `sbexc.S` did not include the port configuration,
  silently excluding the FPU and ARMv8-M stack limit handling of its fault
  exit path; with lazy FP stacking, VRQ delivery, syscalls and fault exits
  switched frames without completing a pending FP save or restoring the new
  frame's FP state. Pending saves are now completed before a frame switch and
  sandboxes always start with an extended frame when the FPU is enabled
  ([#305](https://github.com/chibios-upstream/chibios/pull/305)).
- [VFS] The overlay driver called `roRelease()` on its file systems when a
  mapping was unregistered or the overlay disposed, but file systems are not
  reference counted; the calls are removed and the caller manages their
  lifetime. New `vfsNodeGetOwner()` returns the file system owning a node
  ([#328](https://github.com/chibios-upstream/chibios/pull/328)).
- [VFS] The LittleFS driver translated `LFS_ERR_NOENT` to `CH_RET_INNER_ERROR`,
  so a missing path could not be told apart from a file system or adapter
  failure; it now returns `CH_RET_ENOENT`
  ([#282](https://github.com/chibios-upstream/chibios/pull/282)).
- [SIM] The POSIX simulator ports (SIMIA32, SIMX86_64) used `gettimeofday()`
  for the realtime counter and the system tick, so host wall-clock
  corrections could produce huge time measurements, stall the system tick or
  trigger long catch-up bursts; they now use `CLOCK_MONOTONIC`, and
  arbitrary tick frequencies no longer drift
  ([#281](https://github.com/chibios-upstream/chibios/pull/281)).
- [BUILD] `tools/updater/update_chconf_rt.sh` dropped the `PORT_xxx`
  overrides of an application RT `chconf.h` when regenerating it and did
  not preserve the spaces inside single-line values; both are now preserved
  ([#308](https://github.com/chibios-upstream/chibios/pull/308)).
- [ARMV6M] The post-IRQ `chSchDoPreemption()` call chain, which runs on the
  stack of the preempted thread, could silently overflow thread working areas
  at `-O0`. The `PORT_INT_REQUIRED_STACK` default is raised from 64 to 80 in
  the ARMv6-M, ARMv6-M-ALT, ARMv7-M and ARMv8-M-ML ports (16 more bytes per
  thread, NIL included), and the ARMv6-M and ARMv6-M-ALT
  `PORT_IDLE_THREAD_STACK_SIZE` from 16 to 64
  ([#441](https://github.com/chibios-upstream/chibios/pull/441),
  [#459](https://github.com/chibios-upstream/chibios/pull/459))
  *(backported to 21.11.6, except #441)*.
- [ARMV7M] The ARMv7-M port did not preserve FPSCR across thread context
  switches, so a thread could resume from a kernel call with a different FPU
  rounding mode; FPSCR is now part of the thread context and new threads
  start from `FPDSCR`. The RVCT guard page code imported `_port_set_region`
  instead of `__port_set_region`
  ([#331](https://github.com/chibios-upstream/chibios/pull/331)).
- [ARMV7M] ARMv7-M-ALT could leave the MPU stack guard page on the previously
  running thread when a switch through SVC resumed a preempted thread; the
  incoming guard is now set before releasing the kernel lock. The static MPU
  region initializers lacked `MPU_RBAR_VALID` and did not select their
  intended regions, `PORT_MPU_INITIALIZE` defaults to `FALSE` on devices
  without an MPU, and NIL can be built on ARMv7-M-ALT
  ([#330](https://github.com/chibios-upstream/chibios/pull/330)).
- [ARMV8M] ARMv8-M-ML: in advanced (BASEPRI) mode the thread working area did
  not reserve the second exception frame needed when a fast interrupt
  arrives during a context switch. The startup PSPLIM stayed active with
  stack checking disabled and could raise STKOF faults. FPSCR was not
  preserved across context switches, it is now saved per thread and
  initialized from `FPDSCR`
  ([#332](https://github.com/chibios-upstream/chibios/pull/332)).
- [XHAL] SPI transfer-start failures now restore the driver to `READY` instead
  of leaving it `ACTIVE` with no transfer in progress, allowing immediate
  retries and avoiding waits for a nonexistent transfer
  ([#326](https://github.com/chibios-upstream/chibios/pull/326)).
- [STM32] ADCv4 (STM32H7) boost-level selection tested the ADC clock thresholds
  in ascending order, so any clock above 6.25 MHz selected BOOST level 1 and
  levels 2 and 3 were never reached (under-boosting the analog stage above
  12.5 MHz). The ADC12 and ADC3 thresholds are now tested in descending order
  ([#260](https://github.com/chibios-upstream/chibios/pull/260))
  *(backported to 21.11.6)*.
- [STM32] STM32 SPIv3 and SPIv4 did not fully deactivate the peripheral on
  `spiStop()`; the control, configuration and interrupt-enable registers are
  now cleared ([#231](https://github.com/chibios-upstream/chibios/pull/231))
  *(backported to 21.11.6)*.
- [STM32] Dynamic clock-tree register writes on STM32U5 and STM32U3 were
  safety-verified against complete register values even where the registers
  hold read-only or hardware-controlled fields; the readback now covers only
  the writable fields
  ([#232](https://github.com/chibios-upstream/chibios/pull/232),
  [#235](https://github.com/chibios-upstream/chibios/pull/235),
  [#236](https://github.com/chibios-upstream/chibios/pull/236)).
- [STM32] TIM channel 5 and 6 mode encoding took the OC5M/OC6M extension bit
  from mode bit 2 instead of bit 3, breaking PWM mode 1 on those channels; the
  encoding is corrected (HAL and XHAL TIMv1)
  ([#258](https://github.com/chibios-upstream/chibios/pull/258))
  *(backported to 21.11.6)*.
- [RP] SMP robustness: a durable per-target panic latch so panic propagation no
  longer depends on inter-core FIFO space
  ([#251](https://github.com/chibios-upstream/chibios/pull/251)), flash-lockout
  startup-readiness serialization
  ([#250](https://github.com/chibios-upstream/chibios/pull/250)), and
  compile-time rejection of unsafe ARMv8-M spinlock overrides (RP2350-E2)
  ([#249](https://github.com/chibios-upstream/chibios/pull/249)).
- [RT] Initial SMP kernel-lock ownership is now established at startup
  ([#241](https://github.com/chibios-upstream/chibios/pull/241)), and explicit
  thread affinity is validated
  ([#243](https://github.com/chibios-upstream/chibios/pull/243)).
- [ARMV7R] Avoided an alignment-increasing cast in `port_setup_context()`
  ([#230](https://github.com/chibios-upstream/chibios/pull/230)).
- [SHELL] The legacy shell read `line[-1]` when Enter was pressed at an empty
  prompt with history enabled; the line length is now checked first
  ([#256](https://github.com/chibios-upstream/chibios/pull/256))
  *(backported to 21.11.6)*.
- [STM32] OCTOSPIv2 WSPI driver had three defects: the OCTOSPI2 initialization
  used the OCTOSPI1 DHQC option bit, a non-full-init start wrote an
  uninitialized DCR2 prescaler, and the transfer-complete ISR invoked the
  portable ISR code (whose callback may start a new transaction) before
  waiting for MDMA completion
  ([#227](https://github.com/chibios-upstream/chibios/pull/227)).
- [ARMV6M] ARMv6-M could fail to link with `-flto` (an "undefined reference" to the
  interrupt handler body) once an image grew large enough for the LTO
  partitioner to split it; the `PORT_IRQ_HANDLER` body now has external
  linkage and a reserved name so the trampoline's assembler reference
  resolves across partitions
  ([#216](https://github.com/chibios-upstream/chibios/pull/216)).
- [RT] Broad RT kernel correctness sweep: invariant and lifecycle hardening plus
  fixes across mutexes, condition variables, virtual timers, tickless
  operation, semaphores, thread lifecycle and SMP ownership — notably mutex
  priority-inheritance scheduling
  ([#158](https://github.com/chibios-upstream/chibios/pull/158)),
  mutex/condition-variable contracts
  ([#160](https://github.com/chibios-upstream/chibios/pull/160)), NASA OSAL
  mutex deletion lifecycle
  ([#161](https://github.com/chibios-upstream/chibios/pull/161)), deferred
  round-robin preemption
  ([#179](https://github.com/chibios-upstream/chibios/pull/179)), tickless
  continuous-reload safety
  ([#183](https://github.com/chibios-upstream/chibios/pull/183)),
  queue-retained messages
  ([#185](https://github.com/chibios-upstream/chibios/pull/185)) and semaphore
  reset ordering and counter bounds
  ([#194](https://github.com/chibios-upstream/chibios/pull/194),
  [#200](https://github.com/chibios-upstream/chibios/pull/200),
  [#206](https://github.com/chibios-upstream/chibios/pull/206)), with a
  supporting validation series (#159, #163, #174–#178, #182, #184, #186–#190,
  #193, #195, #202, #203).
- [RP] RP RTCv2 build failure from a wrong alarm-mask field name
  ([#146](https://github.com/chibios-upstream/chibios/pull/146)).
- [SB] SB: `sbDup2` syscall selector
  ([#149](https://github.com/chibios-upstream/chibios/pull/149)) and `r4`
  preservation in the ELF call trampoline
  ([#150](https://github.com/chibios-upstream/chibios/pull/150)).
- [XHAL] XHAL: stop-time waiter wakeups are now locked
  ([#145](https://github.com/chibios-upstream/chibios/pull/145)), invalid ETH
  handle operations are reported
  ([#148](https://github.com/chibios-upstream/chibios/pull/148)), and the
  requested alarm is checked in `stBindAlarmN()`
  ([#198](https://github.com/chibios-upstream/chibios/pull/198)).
- [STM32] STM32C5xx DMA3 request map corrected
  ([#142](https://github.com/chibios-upstream/chibios/pull/142)).
- [RP] RP USB driver fixes (classic and XHAL): buffer control is published
  before the buffer is handed to the controller, with a delay that is long
  enough on the RP2350 Cortex-M33; isochronous buffer sizes and the IN
  double-buffer offset are encoded correctly; `usb_lld_start()` keeps the
  controller registers at their reset values, preserving the RP2350 PHY
  trims and silicon fixes
  ([#214](https://github.com/chibios-upstream/chibios/pull/214),
  [#354](https://github.com/chibios-upstream/chibios/pull/354),
  [#359](https://github.com/chibios-upstream/chibios/pull/359),
  [#434](https://github.com/chibios-upstream/chibios/pull/434)).
- [HAL] Serial over USB input remained inactive after a USB suspend/wakeup cycle on
  STM32 OTG devices. The generic USB driver terminates all pending
  transactions on suspend and the OTG LLD also disables the endpoints in
  hardware, so the bulk OUT receive armed before suspend was lost. Now
  `sduWakeupHookI()` restarts the receive operation, as the XHAL driver
  already did ([#180](https://github.com/chibios-upstream/chibios/pull/180))
  *(backported to 21.11.6)*.
- [STM32] STM32 I2Cv4 did not build on devices without SMBus support (STM32U0xx),
  whose headers do not define the `I2C_ISR_PECERR`/`TIMEOUT`/`ALERT` flags
  referenced by the error mask. These flags now default to zero when
  undefined, compiling out the SMBus error paths
  ([#172](https://github.com/chibios-upstream/chibios/pull/172)).
- [STM32] STM32H7xx HAL initialization reset the SYSCFG block, clearing the overdrive
  enable (`SYSCFG_PWRCR` ODEN) set during clock initialization and dropping
  the core out of overdrive while the PLL kept running above the no-boost
  maximum. SYSCFG is no longer reset during `hal_lld_init()`
  ([#132](https://github.com/chibios-upstream/chibios/pull/132))
  *(backported to 21.11.6)*.
- [STM32] STM32 USBv1 and USBv2 packet memory could be double-allocated across a
  configuration rebuild. `usb_lld_disable_endpoints()` reset the PMA
  allocator to the descriptor-table boundary while endpoint zero remained
  active, so a subsequent SET_CONFIGURATION allocated endpoint 1 over the
  still-live EP0 buffers. The allocator now re-reserves the EP0 IN/OUT
  buffers immediately after the reset; the bus-reset path is unchanged
  ([#85](https://github.com/chibios-upstream/chibios/pull/85))
  *(backported to 21.11.6)*.
- [RT] RT thread registry reference accounting was inconsistent when dynamic
  threads are disabled (`CH_CFG_USE_DYNAMIC = FALSE`): `chRegFirstThread()`
  and `chRegNextThread()` did not count the reference they hand out while
  `chThdRelease()` still released it, risking a reference-count underflow.
  The registry lookups now reference threads unconditionally
  ([#51](https://github.com/chibios-upstream/chibios/pull/51))
  *(backported to 21.11.6)*.
- [OSAL] NASA OSAL `OS_TaskGetInfo()` computed the task working-area size before
  validating the task id, dereferencing a possibly-invalid thread pointer;
  the computation is now done after the validity check
  ([#51](https://github.com/chibios-upstream/chibios/pull/51))
  *(backported to 21.11.6)*.
- [HAL] `nvicSetSystemHandlerPriority()` programmed the wrong `SCB->SHPR` field on
  Cortex-M0, M0+ and M23: the positive ChibiOS handler index was passed to
  the CMSIS `_SHP_IDX()`/`_BIT_SHIFT()` macros, which expect the negative
  system exception number, so the priority write (e.g. SysTick from the ST
  driver) landed on the wrong register slot — SysTick was left at its reset
  priority and another handler's priority was corrupted. The handler index is
  now converted to the matching exception number (HAL and XHAL ports) (forum
  bug report, [#34](https://github.com/chibios-upstream/chibios/pull/34))
  *(backported to 21.11.6)*.
- [RP] RP2040 early (pre-XOSC) tick generator was configured with a divisor of 1
  instead of clk/1MHz, so the boot-time microsecond tick ran about six times
  too fast until clk_ref switched to the XOSC (the post-switch
  reconfiguration was already correct). The RP2350 path was already correct,
  a redundant semicolon was removed there
  ([#35](https://github.com/chibios-upstream/chibios/pull/35)).
- [STM32] STM32U3 RTC was completely non-functional — the driver hung at boot in
  `rtc_enter_init()` waiting for INITF. The RTC APB clock was never enabled:
  hal_lld guarded it on `defined(RCC_APB3ENR_RTCAPBEN)` (the STM32H5/U5
  register), but on STM32U3 the bit is `RCC_APB1ENR1_RTCAPBEN` (APB1ENR1
  bit 30), so the guard was always false and the RTC register interface was
  unclocked. Enabled via `rccEnableAPB1R1(RCC_APB1ENR1_RTCAPBEN)` on
  STM32U3xx (HAL + XHAL). HW-verified on NUCLEO-U385RG
  ([#31](https://github.com/chibios-upstream/chibios/pull/31)).
- [STM32] STM32U3 and STM32U5 RTC drivers operated on the wrong EXTI lines. The ports
  defined `STM32_RTC_GLOBAL_EXTI=17` / `STM32_RTC_TAMP_EXTI=19` (copied from
  STM32H5) and enabled/cleared them, but on U3/U5 those lines are COMP1 and
  VDDUSB (RM0487 Table 131 / RM0456 Table 187) and the RTC has no EXTI line
  at all (RTC interrupts go directly to the NVIC). The RTC EXTI enable/clear
  are now no-ops on both families (HAL and XHAL ports)
  ([#31](https://github.com/chibios-upstream/chibios/pull/31)).
- [STM32] The STM32F303 mcuconf template was missing its I2S driver settings section,
  so regenerating an F303 configuration silently dropped the I2S settings;
  the section was restored and all templated mcuconf/xmcuconf configurations
  were regenerated against their templates
  ([#19](https://github.com/chibios-upstream/chibios/pull/19)).
- [STM32] STM32L4+/L4Rxx clock point name table (`CLK_POINT_NAMES`) had a comma
  misplaced inside the "PLLSAI2R" string literal, so adjacent string literals
  were concatenated and the table held one entry fewer than
  `CLK_ARRAY_SIZE`; clock point names from PLLSAI2R onward were shifted and
  the last was `NULL`. The comma is moved outside the string (HAL and XHAL
  ports) ([#21](https://github.com/chibios-upstream/chibios/pull/21)).
- [STM32] STM32U0xx RTC alarm/tamper interrupt could halt the system on the first
  event (assertions enabled): `rtc_lld_serve_interrupt()` cleared the
  RTC/TAMP EXTI lines, which are direct event inputs on STM32U0 (no EXTI
  pending bit) and tripped the `extiClearGroup1()` fixed-lines assertion. The
  clear now masks out the direct lines (HAL and XHAL ports). Same defect as
  the STM32H5 fix in #15
  ([#16](https://github.com/chibios-upstream/chibios/pull/16)).
- [STM32] STM32H5xx RTC alarm/tamper interrupt caused a system halt on the first
  event: `rtc_lld_serve_interrupt()` cleared the RTC/TAMP EXTI lines, which
  are direct event inputs on STM32H5 (no EXTI pending bit) and tripped the
  `extiClearGroup1()` fixed-lines assertion. The clear now masks out the
  direct lines (HAL and XHAL ports) (forum bug report,
  [#15](https://github.com/chibios-upstream/chibios/pull/15)).
- [STM32] OTG1 on STM32H7 kept its ULPI clock gate at the reset-enabled state,
  preventing sleep mode entry/exit when the driver is active (forum bug
  report, [#13](https://github.com/chibios-upstream/chibios/pull/13)).
- [STM32] Missing SPI2 RCC macros and DMAMUX identifiers in the STM32C0xx HAL and
  XHAL ports, SPI2 was unusable on the devices that have it (forum bug
  report, [#12](https://github.com/chibios-upstream/chibios/pull/12)).
- [STM32] Missing `STM32_ADC_ADC2_IRQ_HOOK` invocations in the STM32 ADCv6 and ADCv7
  drivers (forum bug report,
  [#11](https://github.com/chibios-upstream/chibios/pull/11)).
- [RT] RT: `chThdCreateFromMemoryPool()` rejected valid fixed memory pools due to
  an overly strict alignment assertion
  ([#3](https://github.com/chibios-upstream/chibios/pull/3))
  *(backported to 21.11.6)*.
- [RT] RT: fixed alignment of heap-created thread working area size (bug #1307)
  *(backported to 21.11.6)*.
- [NIL] NIL: fixed wrong alignment check in `chThdCreateI()` (bug #1306)
  *(backported to 21.11.6)*.
- [HAL] DACv1 trigger mask sized for 3 or 4 bit trigger source identifiers.
