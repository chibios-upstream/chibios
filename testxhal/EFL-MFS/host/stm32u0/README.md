# STM32U0 EFL regression

From the repository root:

```sh
make -C testxhal/EFL-MFS/host/stm32u0
make -C testxhal/EFL-MFS/host/stm32u0 clean
```

The tests compile the actual XHAL EFL, flash-base and base-driver HLDs and the
U0 LLD with each of the U031/U073/U083 CMSIS headers. GCC, AddressSanitizer and
UndefinedBehaviorSanitizer are required.

Coverage includes geometry, the device-specific size-register address,
start/stop/restart, reads, every initial programming alignment, partial and
multiple double words, first/last pages, erase verification, both busy flags,
error returns, operation cleanup, and debug bounds checks.

The LLD is included unchanged. GCC function instrumentation supplies the
simulated write-one-to-clear status and completion effects at its static helper
boundaries. RAM stands in for flash. This checks the driver/interface logic,
not actual flash programming, ECC behavior, instruction-cache reset pulses,
unlock-key bus ordering, or hardware timing. Cache tests check that completion
requests synchronization and preserves the ACR configuration.

The existing flash HLD retains FLASH_ERASE after an erase error. The tests check
the error result and recovery through drvStop()/drvStart(); this port does not
change that common HLD policy.

## U083 board build

```sh
cd testxhal/EFL-MFS
make -f make/stm32u083rc_nucleo64.make
```

An Eclipse build configuration is also provided. Unlike the other EFL-MFS
targets, this target runs `efl_u0.c`, a read-only smoke test, not the destructive
MFS suite. Inspect `efl_u0_result` with the debugger: 0 means not complete,
1 passed, and -1 failed. It checks geometry, mapped reads and stop/restart.

No hardware test is run by these build commands. Before adding or running
erase/program tests on a board, reserve an explicit flash area in its linker
script and confirm that it contains neither firmware nor persistent user data.

## Port notes

The driver uses the HAL U0 programming model, adapted to XHAL's embedded
descriptor, typed LLD entry points and HLD-owned operation states. Classic HAL
and the common XHAL HLDs are unchanged.

U0-specific handling was checked against local RM0503 Rev 4, sections 3.3.6,
3.3.7 and 3.7.1/4/5. The new port waits for both BSY1 and CFGBSY, acknowledges
only program/erase status, reports all program/erase error flags, and resets the
instruction cache after modifications. Stop waits for any pending operation
before locking the flash. Full-device erase remains unimplemented, as in HAL.

Partial double words are padded with erased bits. ECC prevents programming
separate nonzero portions of one double word in multiple calls without an
intervening erase. Uncorrectable ECC reads raise NMI; U0 has no RDERR flag.
