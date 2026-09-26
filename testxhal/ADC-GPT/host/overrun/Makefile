# Focused tests of the actual ADCv3/v6 IRQ service functions.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
HLD := $(CHIBIOS)/os/xhal/include
BUILDDIR := ../../build/host/overrun
CPPFLAGS := -I$(BUILDDIR) \
            -I$(CHIBIOS)/os/common/ext/ST/STM32G4xx \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
DRIVERS := adcv3 adcv6
BINARIES := $(foreach driver,$(DRIVERS),\
              $(addprefix $(BUILDDIR)/$(driver)_,single dual))
FRAGMENTS := $(addprefix $(BUILDDIR)/,$(addsuffix .inc,$(DRIVERS)))
DEFINITIONS := $(addprefix $(BUILDDIR)/,$(addsuffix _defs.h,$(DRIVERS)))
STATE_HEADERS := $(HLD)/hal_base_driver.h $(HLD)/hal_cb_driver.h $(HLD)/hal_adc.h

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/adcv3.inc: $(STM32)/LLD/ADCv3/hal_adc_lld.c | $(BUILDDIR)
	sed -n '/^void adc_lld_serve_interrupt(/,/^}/p' $< > $@
	test -s $@

$(BUILDDIR)/adcv6.inc: $(STM32)/LLD/ADCv6/hal_adc_lld.c | $(BUILDDIR)
	sed -n '/^void adc_lld_serve_interrupt(/,/^}/p' $< > $@
	test -s $@

$(BUILDDIR)/adcv3_defs.h: $(STATE_HEADERS) $(STM32)/LLD/ADCv3/hal_adc_lld.h | $(BUILDDIR)
	awk '/^#define HAL_DRV_STATE_/ || /^#define ADC_ACTIVE_/ || /^#define ADC_ERR_/ {print}' $^ > $@

$(BUILDDIR)/adcv6_defs.h: $(STATE_HEADERS) $(STM32)/LLD/ADCv6/hal_adc_lld.h | $(BUILDDIR)
	awk '/^#define HAL_DRV_STATE_/ || /^#define ADC_ACTIVE_/ || /^#define ADC_ERR_/ {print}' $^ > $@

$(filter %_single,$(BINARIES)): CPPFLAGS += -DSTM32_ADC_DUAL_MODE=0
$(filter %_dual,$(BINARIES)): CPPFLAGS += -DSTM32_ADC_DUAL_MODE=1

$(BUILDDIR)/adcv3_single $(BUILDDIR)/adcv3_dual: overrun.c $(BUILDDIR)/adcv3.inc $(BUILDDIR)/adcv3_defs.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTEST_DEVICE=\"stm32g474xx.h\" \
	      -DTEST_IRQ=\"adcv3.inc\" -include $(BUILDDIR)/adcv3_defs.h $< -o $@

$(BUILDDIR)/adcv6_single $(BUILDDIR)/adcv6_dual: overrun.c $(BUILDDIR)/adcv6.inc $(BUILDDIR)/adcv6_defs.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTEST_DEVICE=\"stm32h563xx.h\" \
	      -DTEST_IRQ=\"adcv6.inc\" -include $(BUILDDIR)/adcv6_defs.h $< -o $@

clean:
	$(RM) $(BINARIES) $(FRAGMENTS) $(DEFINITIONS)
