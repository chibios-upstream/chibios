# ADCv5 U0 integration regression

Run from the repository root:

```sh
make -C testxhal/ADC-GPT/host/adcv5
make -C testxhal/ADC-GPT/host/adcv5_irq
make -C testxhal/ADC-GPT/host/irq_aliases
```

The ADCv5 harness compiles the real XHAL ADC frontend, LLD, CMSIS device
headers and DMA register helpers. The base-driver lifecycle and RTOS are
mocked, and a child process acknowledges ADC hardware commands through shared
memory. AddressSanitizer and UndefinedBehaviorSanitizer are enabled; a sandbox
that disallows LeakSanitizer process inspection may require an unsandboxed run.

Coverage includes U031/U073/U083 register layouts, optional configuration
tables, synchronization disabled, STARTING-state resource allocation, allocation
failure and retry, ADC prescaler and clock-mode programming, DMA request and
peripheral address, channel/sample/threshold setup, linear completion and
stop/restart. Overrun tests cover both linear and circular conversions, with
and without callbacks, error wakeups, and suppression of late/spurious overruns.
The companion suites cover the ADC/COMP shared vector and U0 peripheral-priority
aliases.

This is not a hardware timing, W1C, analog-accuracy or full circular-streaming
test. Injected ADC status flags are held by the model until explicitly released.
Register compatibility was checked against RM0503 Rev 4, section 14,
and U083 clock/startup limits against DS14463 Rev 2, table 70, in `_vendor_docs`.
No CMSIS files were modified.

Integration build checks also passed with GCC 14.3.1 and `-Werror`: U083 with
ADC1, SPI1/2/3 and IWDG enabled (smart build both on and off), ADC-enabled C031
and G071 builds, and the existing WL55 ADC-GPT target. No board testing was
performed for this integration.

## Overrun regression

The overrun handler previously tested only `HAL_DRV_STATE_ACTIVE`, which is
the alias for `ADC_ACTIVE_LINEAR` but not `ADC_ACTIVE_CIRCULAR`. It now accepts
both active states, preserving the guard against overruns after completion.
The new test fails on missing circular overrun reporting before this fix.

Clean generated binaries with the same make commands using the `clean` target.
