# XHAL STM32 ADCv7 regression tests

Run `make -C testxhal/ADC-GPT/host/adcv7` from the repository root.
The harness compiles the real XHAL ADC frontend, ADCv7 LLD, ADC1/ADC2
interrupt includes, U385 CMSIS definitions, and DMA3 register helpers.
RTOS services, DMA allocation/disable, and ADC handshakes are simulated.
Each executable has a 20-second watchdog and uses AddressSanitizer and UBSan.

The eight variants cover ADC1, ADC2-only, both independent units, compact
samples, dual 16-bit/8-bit samples, configuration tables, and synchronization
disabled. Checks include:

- allocation failure and cleanup, shared-clock lifetime, and conflicting CCRs;
- linear/circular setup, DMA byte counts/widths/request selection, circular
  destination reload, sequence lengths, and buffer-size rejection;
- simultaneous half/full events and stop/restart from the half callback;
- DMA errors, master/slave overruns and watchdogs, including the slave vector;
- errors without an application callback, waiter wakeups, and active shutdown.

The harness does not emulate analog conversion, ADC write protection, W1C
register semantics, or GPDMA linked-list execution. Those require board tests.

## U385 board target

`make -C testxhal/ADC-GPT -f make/stm32u385rg_nucleo64.make USE_COPT=-Werror`
builds the NUCLEO-U385RG-Q target. Its local configuration uses ADC1 inputs
PC0/IN1 and PC1/IN2, and TIM4 TRGO. The shared test exercises linear conversion,
circular streaming, and timer-triggered streaming. Observe
`adc_gpt_test_result`/`adc_gpt_test_failure` as described by `main.c`.
The target also has an Eclipse build configuration.

## Port-specific configuration

Unlike the classic HAL ADCv7 donor, common CCR settings belong to
`hal_adc_config_t.ccr`, not to individual conversion groups. RM0487 Rev 1,
sections 22.8.2 (pp. 871-872), requires disabled ADCs when changing dual mode,
packing, sampling delay, VREFEN, or TSEN. The driver applies these at startup.
Use the configuration's sensor-enable bits instead of post-start register
helpers. Both independent drivers must request identical common settings;
stop both before changing them.

Dual mode requires `STM32_ADC_USE_ADC1=TRUE` and
`STM32_ADC_USE_ADC2=FALSE`. Select `ADC_CCR_DUAL_REGULAR` or
`ADC_CCR_DUAL_INTERLEAVED` in the startup CCR; the driver supplies DAMDF.
Group channel counts include both ADCs, with equal sequence lengths.
Sample buffers must be aligned to a packed pair (four bytes for 16-bit
samples, two for compact samples).

DMA IRQs use the corresponding `STM32_IRQ_ADCx_PRIORITY`. In dual mode both
peripheral vectors and DMA use `STM32_IRQ_ADC1_PRIORITY`, and the ADC2 vector
dispatches to ADCD1. NVIC setup belongs to the shared interrupt includes.

Circular mode uses the existing linker-managed DMA3 area. Its one-word node
reloads CDAR; hardware restores BNDT when UB1 is clear and another update bit
is set (RM0487 section 15.8, GPDMA_CxBR1).
