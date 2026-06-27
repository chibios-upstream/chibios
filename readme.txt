*****************************************************************************
*** Files Organization                                                    ***
*****************************************************************************

--{root}                        - ChibiOS directory.
  +--.devcontainer/             - VS Code dev container configuration.
  +--readme.txt                 - This file.
  +--documentation.html         - Shortcut to the web documentation page.
  +--license.txt                - GPL license text.
  +--demos/                     - Demo projects, one directory per platform.
  +--doc/                       - Documentation.
  |  +--common/                 - Documentation common build resources.
  |  +--hal/                    - Builders for HAL.
  |  +--nil/                    - Builders for NIL.
  |  +--rt/                     - Builders for RT.
  +--ext/                       - External libraries, not part of ChibiOS.
  +--os/                        - ChibiOS components.
  |  +--common/                 - Shared OS modules.
  |  |  +--abstractions/        - API emulator wrappers.
  |  |  |  +--cmsis_os/         - CMSIS OS emulation layer for RT.
  |  |  |  +--nasa_osal/        - NASA Operating System Abstraction Layer.
  |  |  +--ext/                 - Vendor files used by the OS.
  |  |  +--oop/                 - Object-oriented framework.
  |  |  +--ports/               - RTOS ports usable by both RT and NIL.
  |  |  +--startup/             - Startup support.
  |  |  +--utils/               - Generic and portable C modules.
  |  +--ex/                     - EX component.
  |  |  +--dox/                 - EX documentation resources.
  |  |  +--include/             - EX header files.
  |  |  +--devices /            - EX complex drivers.
  |  +--hal/                    - HAL component.
  |  |  +--boards/              - HAL board support files.
  |  |  +--dox/                 - HAL documentation resources.
  |  |  +--include/             - HAL high level headers.
  |  |  +--lib/                 - HAL libraries.
  |  |  |  +--complex/          - HAL collection of complex drivers.
  |  |  |  |  +--mfs/           - HAL managed flash storage driver.
  |  |  |  |  +--serial_nor/    - HAL legacy SNOR stack.
  |  |  |  |  +--xsnor/         - HAL improved SNOR stack.
  |  |  |  +--fallback/         - HAL fall back software drivers.
  |  |  |  +--peripherals/      - HAL peripherals interfaces.
  |  |  |  +--streams/          - HAL streams.
  |  |  +--osal/                - HAL OSAL implementations.
  |  |  |  +--lib/              - HAL OSAL common modules.
  |  |  +--src/                 - HAL high level source.
  |  |  +--ports/               - HAL ports.
  |  |  +--templates/           - HAL driver template files.
  |  |     +--osal/             - HAL OSAL templates.
  |  +--oslib/                  - RTOS modules usable by both RT and NIL.
  |  |  +--include/             - OSLIB high level headers.
  |  |  +--src/                 - OSLIB high level source.
  |  |  +--templates/           - OSLIB configuration template files.
  |  +--nil/                    - NIL RTOS component.
  |  |  +--dox/                 - NIL documentation resources.
  |  |  +--include/             - NIL high level headers.
  |  |  +--src/                 - NIL high level source.
  |  |  +--templates/           - NIL configuration template files.
  |  +--rt/                     - RT RTOS component.
  |  |  +--dox/                 - RT documentation resources.
  |  |  +--include/             - RT high level headers.
  |  |  +--src/                 - RT high level source.
  |  |  +--templates/           - RT configuration template files.
  |  +--various/                - Various portable support files.
  +--test/                      - Kernel test suite source code.
  |  +--lib/                    - Portable test engine.
  |  +--hal/                    - HAL test suites.
  |  |  +--testbuild/           - HAL build test and MISRA check.
  |  +--nil/                    - NIL test suites.
  |  |  +--testbuild/           - NIL build test and MISRA check.
  |  +--rt/                     - RT test suites.
  |  |  +--testbuild/           - RT build test and MISRA check.
  |  |  +--coverage/            - RT code coverage project.
  +--testex/                    - EX integration test demos.
  +--testhal/                   - HAL integration test demos.

NOTE: The optional VS Code dev container setup is in .devcontainer/.
See .devcontainer/README.md for included tools and usage.

*****************************************************************************
*** Releases and Change Log                                               ***
*****************************************************************************

*** Next ***
- NEW: STM32U5 support extended: EPOD booster clock handling and a generated
       clock tree, the STM32U575ZI-Nucleo144 board and RT-STM32-MULTI demo
       configuration, and the STM32U575xx mcuconf template and updater
       (github PR #56).
- FIX: RT thread registry reference accounting was inconsistent when dynamic
       threads are disabled (CH_CFG_USE_DYNAMIC = FALSE): chRegFirstThread()
       and chRegNextThread() did not count the reference they hand out while
       chThdRelease() still released it, risking a reference-count underflow.
       The registry lookups now reference threads unconditionally (github
       PR #51)(backported to 21.11.6).
- FIX: NASA OSAL OS_TaskGetInfo() computed the task working-area size before
       validating the task id, dereferencing a possibly-invalid thread pointer;
       the computation is now done after the validity check (github PR #51)(backported to 21.11.6).
- FIX: nvicSetSystemHandlerPriority() programmed the wrong SCB->SHPR field on
       Cortex-M0, M0+ and M23: the positive ChibiOS handler index was passed
       to the CMSIS _SHP_IDX()/_BIT_SHIFT() macros, which expect the negative
       system exception number, so the priority write (e.g. SysTick from the
       ST driver) landed on the wrong register slot - SysTick was left at its
       reset priority and another handler's priority was corrupted. The handler
       index is now converted to the matching exception number (HAL and XHAL
       ports) (forum bug report, github PR #34)(backported to 21.11.6).
- FIX: RP2040 early (pre-XOSC) tick generator was configured with a divisor of
       1 instead of clk/1MHz, so the boot-time microsecond tick ran about six
       times too fast until clk_ref switched to the XOSC (the post-switch
       reconfiguration was already correct). The RP2350 path was already
       correct, a redundant semicolon was removed there (github PR #35).
- FIX: STM32U3 RTC was completely non-functional - the driver hung at boot in
       rtc_enter_init() waiting for INITF. The RTC APB clock was never enabled:
       hal_lld guarded it on defined(RCC_APB3ENR_RTCAPBEN) (the STM32H5/U5
       register), but on STM32U3 the bit is RCC_APB1ENR1_RTCAPBEN (APB1ENR1
       bit 30), so the guard was always false and the RTC register interface
       was unclocked. Enable it via rccEnableAPB1R1(RCC_APB1ENR1_RTCAPBEN) on
       STM32U3xx (HAL + XHAL). HW-verified on NUCLEO-U385RG (github PR #31).
- FIX: STM32U3 and STM32U5 RTC drivers operated on the wrong EXTI lines. The
       ports defined STM32_RTC_GLOBAL_EXTI=17 / STM32_RTC_TAMP_EXTI=19 (copied
       from STM32H5) and enabled/cleared them, but on U3/U5 those lines are
       COMP1 and VDDUSB (RM0487 Table 131 / RM0456 Table 187) and the RTC has
       no EXTI line at all (RTC interrupts go directly to the NVIC). The RTC
       EXTI enable/clear are now no-ops on both families (HAL and XHAL ports)
       (github PR #31).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the os/hal/lib sources (streams, mfs, serial_nor), no functional
       change (github PR #30).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the os/various sources (shell, xshell, bindings glue), no functional
       change (github PR #29).
- NEW: tools/style/stylecheck.py no longer reports two false positives:
       "#endif /* defined(X) */" guard comments (the lower-case "defined"
       operator) and operators/commas inside string literals (only the first
       string per line was previously blanked). Strictly more permissive
       (github PR #28).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the os/common sources (ports, oop, abstractions), no functional change
       (github PR #27).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the SB (sandbox) sources, no functional change (github PR #26).
- NEW: Coding-style cleanup of the XHAL OOP driver code generator: a glued
       comparison operator was corrected in the hal_base_driver codegen model
       (os/xhal/codegen) and the generated source regenerated; no functional
       change (github PR #24).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the VFS sources, no functional change (github PR #23).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the XHAL hand-written sources, no functional change (github PR #22).
- NEW: Coding-style cleanup (whitespace, spacing and comment formatting) of
       the HAL sources, no functional change (github PR #20).
- FIX: The STM32F303 mcuconf template was missing its I2S driver settings
       section, so regenerating an F303 configuration silently dropped the
       I2S settings; the section was restored and all templated mcuconf/
       xmcuconf configurations were regenerated against their templates
       (github PR #19).
- FIX: STM32L4+/L4Rxx clock point name table (CLK_POINT_NAMES) had a comma
       misplaced inside the "PLLSAI2R" string literal, so adjacent string
       literals were concatenated and the table held one entry fewer than
       CLK_ARRAY_SIZE; clock point names from PLLSAI2R onward were shifted
       and the last was NULL. The comma is moved outside the string (HAL and
       XHAL ports) (github PR #21).
- NEW: SYSTICKv1 free running mode gained STM32_ST_FREQUENCY_TOLERANCE, the
       allowed ST tick deviation in per-mille (default 0 = exact divisor
       required, unchanged behavior). The prescaler is rounded to the
       nearest integer and the achieved tick is checked against the
       tolerance, letting devices whose clock tree cannot produce an
       exact multiple (e.g. STM32U0/U3 with MSI feeding the PLL) run
       without a clock-rounding system halt (github PR #17). The STM32U0 and
       STM32U3 device configurations set the tolerance to 0.5% (github PR #18).
- FIX: STM32U0xx RTC alarm/tamper interrupt could halt the system on the
       first event (assertions enabled): rtc_lld_serve_interrupt() cleared
       the RTC/TAMP EXTI lines, which are direct event inputs on STM32U0
       (no EXTI pending bit) and tripped the extiClearGroup1() fixed-lines
       assertion. The clear now masks out the direct lines (HAL and XHAL
       ports). Same defect as the STM32H5 fix in #15 (github PR #16).
- FIX: STM32H5xx RTC alarm/tamper interrupt caused a system halt on the
       first event: rtc_lld_serve_interrupt() cleared the RTC/TAMP EXTI
       lines, which are direct event inputs on STM32H5 (no EXTI pending
       bit) and tripped the extiClearGroup1() fixed-lines assertion. The
       clear now masks out the direct lines (HAL and XHAL ports)
       (forum bug report, github PR #15).
- NEW: STM32G4xx: added FSMC RCC macros, IRQ vector definitions and
       registry switch for the FMC-capable devices (G473/G483/G474/G484),
       in all four G4 port copies (github PR #14)(backported to
       21.11.6).
- FIX: OTG1 on STM32H7 kept its ULPI clock gate at the reset-enabled state,
       preventing sleep mode entry/exit when the driver is active (forum
       bug report, github PR #13).
- FIX: Missing SPI2 RCC macros and DMAMUX identifiers in the STM32C0xx
       HAL and XHAL ports, SPI2 was unusable on the devices that have it (forum
       bug report, github PR #12).
- FIX: Missing STM32_ADC_ADC2_IRQ_HOOK invocations in the STM32 ADCv6 and
       ADCv7 drivers (forum bug report, github PR #11).
- NEW: Add ADDITIONAL_OUTFILES to the ARMCMx GCC build rules (forum bug
       report, github PR #10).
- FIX: RT: Fixed chThdCreateFromMemoryPool() rejects valid fixed memory pools
       due to overly strict alignment assertion (bug github #3)
       (backported to 21.11.6).
- FIX: RT: Fixed align heap-created thread working area size (bug #1307)
       (backported to 21.11.6).
- FIX; NIL: Fixed wrong alignment check in chThdCreateI() (bug #1306)
       (backported to 21.11.6).
- FIX: DACv1 trigger mask sized for 3 or 4 bit trigger source identifier
- NEW: Added ADDITIONAL_OUTFILES variable to the ARMCMx GCC build rules,
       included makefiles can now add extra generated files to the "all"
       target (github PR #10).
- NEW: Sensors subsystem in XHAL equivalent to EX for old HAL.
- NEW: Thread mode for EP0 handling in USB HAL driver.
- NEW: ADC driver in XHAL.
- NEW: RTC driver in XHAL.
- NEW: Added EFL driver for simulator and a simulator MFS test application.
- NEW: Added simulator support for x84-64 on Posix/Linux.
- NEW: Added UART driver (LLD) for the RP2040 port.
- NEW: Added XSHELL_EXIT_HOOK to xshell.
- NEW: Memory areas functions in OSLIB addressed for portability.
- NEW: Implemented better chThdSleepUntil() in NIL using the same logic used
       in the RT implementation.
- NEW: Function chSftIntegrityCheckI() rewritten to be much more efficient in
       performing lists integrity checks.
- NEW: Added to RT a dedicated functional safety module, the function
       chSysIntegrityCheckI() has been moved in this module and renamed to
       chSftIntegrityCheckI().
- NEW: Added multicore memory classes modifiers to RT in order to support
       NUMA architectures and non-coherent cache architectures.
- NEW: Added MPU initialization settings in ARMv7-M, ARv7-M-ALT, ARMv8-ML-ALT
       ports. Settings allow to statically initialize any region.
- NEW: Improved interrupts processing in ARMv7-M-ALT and ARMv8-M-ML-ALT
       ports. Specifically, saved few cycles on the context switch code path.
- NEW: Added missing context switch hook in ARMv7-M-ALT and ARMv8-M-ML-ALT
       ports.
- NEW: Added integration demos for VFS+LittleFS/FatFS+XSHELL, now also
       available on STM32U0 Nucleo-64.
- NEW: Added faster context switch modes to ARMv7-M and ARMv8-M ports. The
       new modes allow to avoid saving FP context for threads that do not
       use the FPU.
- NEW: Added new ARMv8-M port aligned with features of then ARMv7-M port for
       sandboxing.
- NEW: Added XShell support for change prompt, multi-command per line, line 
       editing, user definable extra fields, init and execution hooks.
- NEW: Improved LFS support, now it is possible to make an LFS file system
       at arbitrary positions in flash.
- NEW: New XShell specific for the new RT, it leverages the new thread
       dispose feature.
- NEW: New RT threads spawning API decoupling the thread stack from the
       thread_t structure as required in NUMA multicore devices. Old
       "create" API is still present and supported, long term it could be
       removed.
- NEW: Added capability to associate a "dispose" functions to threads,
       dynamic API modified to use this mechanism.
- NEW: Recursive locks in RT and NIL made optional, only enabled if the
       underlying port supports the capability.
- NEW: Now RT virtual timers can recalculate the value of CH_CFG_ST_TIMEDELTA
       at runtime and continue using the recalculated value. This has two
       consequences: 1) The value is recalculated once 2) it is possible
       to use the new API chVTGetCurrentDelta() and update the static
       setting in order to avoid recalculation.
- NEW: OSLIB release methods now return the value of the reference counter.
- NEW: Added a "waend" field to the thread structure in RT for debug
       convenience.
- NEW: Added a para-virtualized XHAL port for use in sandboxes.
- NEW: Added a VIO subsystem to sandboxes supporting drivers
       para-virtualization, PAL and SIO supported so far.
- NEW: Added and RT port for use in virtualized sandboxes.
- NEW: Added full virtualization support to sandboxes with a virtual IRQ
       mechanism.
- NEW: Added __CH_OWNEROF() macro to RT.
- NEW: Added a Posix-favored shell named "msh" (Mini Shell). The shell is able
       to run sub-apps inside the same sandbox. The shell can either be placed
       statically in flash or loaded dynamically in RAM.
- NEW: Added runnable "apps" capability to SBs, apps available so far: msh, ls.
- NEW: Added ability to load ELF files to SBs.
- NEW: Enhanced Posix API for SBs leveraging the VFS integration.
- NEW: SBs and VFS integration. Each SB can see its own VFS instance.
- NEW: Added a new MEM_IS_VALID_FUNCTION() macro to RT and NIL.
- NEW: Changed SB configuration options names to be prefixed with SB_CFG_.
- NEW: Added a new CH_CFG_HARDENING_LEVEL option to RT.
- NEW: Added a chXXXDispose() function to all objects in NIL.
- NEW: Added a chXXXDispose() function to all objects in RT.
- NEW: Added MEM_NATURAL_ALIGN macro to RT and NIL.
- NEW: Added static initializer for virtual timers in RT.
- NEW: Added new function chHeapIntegrityCheck().
- NEW: Function chCoreGetStatusX() changed to return a memory region object
       instead of a simple size.
- NEW: RT and NIL upgraded to support the enhanced OSLIB.
- NEW: Memory areas/pointers checker functions added to OSLIB.
