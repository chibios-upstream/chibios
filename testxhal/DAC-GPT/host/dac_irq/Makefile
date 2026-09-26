# DAC IRQ routing tests, with mocked services and NVIC.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
BUILDDIR := ../../build/host/dac_irq
CPPFLAGS := -I$(STM32) -I$(STM32)/LLD
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wundef -Werror
TESTS := g0_none g0_dac g0_ch2 g0_tim g0_both g0_st g0_st_dac \
         g0_hook g0_all g0_unassigned wl_none wl_dac wl_hook wl_unassigned
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(filter $(BUILDDIR)/wl_%,$(BINARIES)): CPPFLAGS += -DTEST_WL=1
$(filter %_dac %_both %_all %_ch2 %_unassigned,$(BINARIES)): CPPFLAGS += -DTEST_DAC=1
$(filter %_tim %_both %_all,$(BINARIES)): CPPFLAGS += -DTEST_TIM=1
$(filter %_st %_st_dac,$(BINARIES)): CPPFLAGS += -DTEST_ST=1
$(filter %_hook %_all,$(BINARIES)): CPPFLAGS += -DTEST_HOOK=1
$(BUILDDIR)/wl_hook: CPPFLAGS += -DTEST_DAC=1
$(BUILDDIR)/g0_ch2: CPPFLAGS += -DTEST_CHANNEL=2
$(filter %_unassigned,$(BINARIES)): CPPFLAGS += -DTEST_CHANNEL=0

$(BINARIES): dac_irq.c $(STM32)/LLD/DAC/stm32_dac1.inc \
             $(STM32)/LLD/TIMv1/stm32_tim6_dac_lptim1.inc \
             $(STM32)/STM32G0xx/stm32_isr.h \
             $(STM32)/STM32WLxx/stm32_isr.h \
             $(STM32)/STM32WLxx/stm32_isr_m4.inc | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
