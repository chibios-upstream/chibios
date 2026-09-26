# ADCv5 IRQ ownership and dispatch, with mocked driver services/NVIC.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
BUILDDIR := ../../build/host/adcv5_irq
CPPFLAGS := -I$(STM32) -I$(STM32)/LLD/ADC -I$(STM32)/STM32G0xx \
            -I$(STM32)/STM32WLxx
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wundef -Werror
TESTS := g0_none g0_adc g0_hook g0_adc_hook g0_dac_only g0_unassigned \
         wl_none wl_adc wl_dac wl_hook wl_all wl_unassigned
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
DEDICATED_PLATFORMS := c0 g031 g0b0 wl_m4 h5
DEDICATED_TESTS := $(foreach platform,$(DEDICATED_PLATFORMS),\
                    $(addprefix $(platform)_,none adc hook adc_hook unassigned))
DEDICATED_BINARIES := $(addprefix $(BUILDDIR)/,$(DEDICATED_TESTS))
FRAGMENTS := $(STM32)/LLD/ADC/stm32_adc1_comp.inc \
             $(STM32)/LLD/ADC/stm32_adc1_comp_dac1.inc
DEDICATED_HEADERS := $(STM32)/LLD/ADC/stm32_adc1.inc \
                     $(STM32)/STM32C0xx/stm32_isr.h \
                     $(STM32)/STM32G0xx/stm32_isr.h \
                     $(STM32)/STM32WLxx/stm32_isr_m4.inc \
                     $(STM32)/STM32H5xx/stm32_isr.h

.PHONY: all clean
all: $(BINARIES) $(DEDICATED_BINARIES)
	@for test in $(BINARIES) $(DEDICATED_BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/g0_adc: CPPFLAGS += -DTEST_ADC=1
$(BUILDDIR)/g0_hook: CPPFLAGS += -DTEST_HOOK=1
$(BUILDDIR)/g0_adc_hook: CPPFLAGS += -DTEST_ADC=1 -DTEST_HOOK=1
$(BUILDDIR)/g0_dac_only: CPPFLAGS += -DTEST_DAC=1
$(BUILDDIR)/g0_unassigned: CPPFLAGS += -DTEST_ADC=1 -DTEST_ADC_UNIT=0
$(filter $(BUILDDIR)/wl_%,$(BINARIES)): CPPFLAGS += -DTEST_WL=1
$(BUILDDIR)/wl_adc: CPPFLAGS += -DTEST_ADC=1
$(BUILDDIR)/wl_dac: CPPFLAGS += -DTEST_DAC=1
$(BUILDDIR)/wl_hook: CPPFLAGS += -DTEST_HOOK=1
$(BUILDDIR)/wl_all: CPPFLAGS += -DTEST_ADC=1 -DTEST_DAC=1 -DTEST_HOOK=1
$(BUILDDIR)/wl_unassigned: CPPFLAGS += -DTEST_ADC=1 -DTEST_DAC=1 \
                                    -DTEST_ADC_UNIT=0 -DTEST_DAC_UNIT=0

$(filter $(BUILDDIR)/c0_%,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_C0=1
$(filter $(BUILDDIR)/g031_%,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_G031=1
$(filter $(BUILDDIR)/g0b0_%,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_G0B0=1
$(filter $(BUILDDIR)/wl_m4_%,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_WL_M4=1
$(filter $(BUILDDIR)/h5_%,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_H5=1
$(filter %_adc %_adc_hook,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_ADC=1
$(filter %_hook,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_HOOK=1
$(filter %_unassigned,$(DEDICATED_BINARIES)): CPPFLAGS += -DTEST_ADC=1 \
                                                     -DTEST_ADC_UNIT=0

$(BINARIES): adc_irq.c $(FRAGMENTS) $(STM32)/STM32G0xx/stm32_isr.h \
             $(STM32)/STM32WLxx/stm32_isr_m0.inc | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(DEDICATED_BINARIES): adc1_irq.c $(DEDICATED_HEADERS) | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES) $(DEDICATED_BINARIES)
