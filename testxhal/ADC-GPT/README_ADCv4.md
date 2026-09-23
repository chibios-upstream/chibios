# STM32H7 ADC coverage

The ADCv4 XHAL port supports ADC1 (`ADCD1`), regular simultaneous ADC1/ADC2
operation, and independent ADC3 (`ADCD3`) with DMA or BDMA where the device
registry exposes the compatible ADC3 peripheral. It is included by both STM32H7
platform makefiles. The different ADC3 IP on H723/H735 is not covered by this
driver; those devices use ADC1/ADC2 only.

## Configuration and buffers

- Use `drvStart()` with a `hal_adc_config_t` containing an indexed group table.
  A NULL configuration selects configuration zero. Without configuration-table
  support, the default configuration enables single-ended inputs but contains
  no conversion groups.
- Differential selection and calibration apply at startup. Changing them
  requires stop/start; a live configuration change can replace the group table
  but cannot change these hardware settings.
- Single-ADC samples can be 8, 16 or 32 bits wide. ADC1/ADC2 dual mode requires
  `STM32_ADC_SAMPLES_SIZE=32`: each buffer word packs one ADC1 result in bits
  15:0 and one ADC2 result in bits 31:16. `num_channels` counts both ADCs, so the
  DMA transfers `num_channels / 2 * depth` words. Results wider than 16 bits
  are not supported in dual mode. ADC3 remains independent and uses the full
  selected sample width even when ADC1/ADC2 dual mode is enabled.
- Each ADC sequence has 1..16 channels. Depth is one or a positive even number;
  the transfer must fit the 65535-item DMA counter. Buffers must be naturally
  aligned and accessible to the chosen DMA controller.
- The group `ccr` field accepts only `VREFEN`, `TSEN` and `VBATEN`. These sensor
  enables accumulate until explicitly disabled or the driver is restarted.
  Clock mode, packing and regular simultaneous mode are established at startup.
  Injected and interleaved modes are not implemented.
- IRQ priorities use `STM32_IRQ_ADC12_PRIORITY` and
  `STM32_IRQ_ADC3_PRIORITY`. The configuration updaters migrate the legacy
  `STM32_ADC_ADC12_IRQ_PRIORITY` and `STM32_ADC_ADC3_IRQ_PRIORITY` names, giving
  precedence to an explicitly supplied new name. ADC12 defaults to disabled.

## Firmware fixtures

Run from `testxhal/ADC-GPT`:

```sh
make -j6 -f make/stm32h743zi_nucleo144.make USE_COPT=-Werror
make -j6 -f make/stm32h723zg_nucleo144.make USE_COPT=-Werror
make -j6 -f make/stm32h7a3ziq_nucleo144.make USE_COPT=-Werror
```

Each fixture has local configuration headers, enabled debug checks, ADC1
channels IN0/IN5, and TIM4 TRGO triggering. Sample buffers are aligned to 32 bytes
and placed in `.ram4`, covered by a non-cacheable MPU region at `0x38000000`.
These fixtures were built, not flashed or tested for analog accuracy.

Additional H743 compile coverage used non-smart, non-LTO builds with these
settings overridden after including the fixture's `xmcuconf.h`:

| Variant | ADC12 | ADC3 | Dual mode | Sample width | ADC3 route |
| --- | --- | --- | --- | --- | --- |
| Dual | enabled | disabled | enabled | 32 | — |
| ADC3 DMA | disabled | enabled | disabled | 8 | DMA |
| ADC3 BDMA | disabled | enabled | disabled | 16 | BDMA |
| Both | enabled | enabled | enabled | 32 | BDMA |

The ADC3-only variants select `ADCD3` in `portab.h` for compile coverage; their
pin and trigger routing has not been validated on a board.

Regression builds also passed for the existing G474, U575 and WL55 ADC-GPT
fixtures. The ADC-disabled H735 multi-demo passed a clean non-smart, non-LTO
build with `USE_COPT=-Werror USE_OPT='-Og -ggdb'`. Its `-O2 -Werror` build is
blocked by an existing unrelated `interval` maybe-uninitialized warning in
`test/rt/source/test/rt_test_sequence_003.c`; that code was not changed.

Clean each firmware target with the same makefile and `clean`, retaining any
custom `BUILDDIR` and `DEPDIR` arguments used for its build.

## Host regression tests

```sh
make -j6 -C host/adcv4
make -C host/adcv4 clean
```

The ten variants build the real XHAL ADC frontend and ADCv4 LLD with STM32H743
register definitions and real DMA/BDMA register helpers. They use AddressSanitizer,
UndefinedBehaviorSanitizer and `-Werror`. Variants cover ADC12-only, 8/16/32-bit
samples, dual mode, ADC3-only DMA/BDMA, both controllers, configuration tables,
and synchronization disabled.

Checks include allocation failure and restart, configuration selection,
conversion validation, DMA widths and counts, clock/common-register setup,
linear/circular callbacks, simultaneous half/full events, stop/restart from a
half callback, combined master/slave errors, callback-free error wakeups,
late interrupts, and stop wakeups. Callbacks are checked for unlocked ISR
context; allocation/free operations are checked for thread context.

The hardware shim models self-clearing control bits but not ADC data conversion,
DMA data movement or write-one-to-clear register semantics. These tests do not
replace hardware validation.

All three affected H7 configuration updaters were run globally and a second
pass produced identical files. Migration probes verified preservation of
legacy priorities, precedence of new priority names, and the disabled default.
