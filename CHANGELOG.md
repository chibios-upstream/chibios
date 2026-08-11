# Changelog

All notable changes to ChibiOS are documented in this file. Entries reference
the GitHub pull request implementing the change where available; fixes also
applied to a maintenance branch are marked *(backported to 21.11.6)*.

## [Unreleased]

### Added

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
  and `dmaChannelGetCounterX()`
  ([#138](https://github.com/chibios-upstream/chibios/pull/138),
  [#152](https://github.com/chibios-upstream/chibios/pull/152),
  [#167](https://github.com/chibios-upstream/chibios/pull/167),
  [#168](https://github.com/chibios-upstream/chibios/pull/168),
  [#169](https://github.com/chibios-upstream/chibios/pull/169),
  [#170](https://github.com/chibios-upstream/chibios/pull/170)).
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
  ([#109](https://github.com/chibios-upstream/chibios/pull/109)).
- [RP] Hazard3 RISC-V support for RP2350: RT/NIL ports, dual-core SMP, Xh3irq
  interrupt handling, periodic and tickless MTIME support, PMP stack guards,
  startup/HAL integration, demo, and hardware validation target
  ([#90](https://github.com/chibios-upstream/chibios/pull/90)).
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
- [XHAL] RTC driver in XHAL.
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
  sandboxing.
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
- [RT] `CH_CFG_HARDENING_LEVEL` option in RT.
- [RT] `chXXXDispose()` functions for all objects in RT and NIL.
- [RT] `MEM_NATURAL_ALIGN` macro in RT and NIL.
- [RT] Static initializer for virtual timers in RT.
- [OSLIB] New function `chHeapIntegrityCheck()`.
- [OSLIB] Memory areas/pointers checker functions in OSLIB.

### Changed

- [XHAL] XHAL no longer depends on OSAL
  ([#156](https://github.com/chibios-upstream/chibios/pull/156)); a redundant
  MMC SPI module switch was removed
  ([#157](https://github.com/chibios-upstream/chibios/pull/157)).
- [STM32] STM32 RTC clock validation hardened
  ([#197](https://github.com/chibios-upstream/chibios/pull/197)).
- [RP] RP2350 EFL interrupt masking made architecture-neutral (Arm and RISC-V)
  ([#139](https://github.com/chibios-upstream/chibios/pull/139)).
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
- [RP] RP USB buffer control is now published before the buffer is handed to the
  controller ([#214](https://github.com/chibios-upstream/chibios/pull/214)).
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
