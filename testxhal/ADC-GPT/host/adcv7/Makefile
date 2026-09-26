# Real ADC frontend/LLD and DMA register helpers, with mocked hardware/RTOS.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/ADCv7
BUILDDIR := ../../build/host/adcv7
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(LLD) \
            -I$(CHIBIOS)/os/xhal/include -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32/STM32U3xx \
            -I$(CHIBIOS)/os/xhal/ports/STM32/LLD/ADC \
            -I$(CHIBIOS)/os/xhal/ports/STM32/LLD/DMA3v1 \
            -I$(CHIBIOS)/os/common/ext/ST/STM32U3xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := adc1 adc2 both compact dual16 dual8 configurations nosync
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/adc2: CPPFLAGS += -DSTM32_ADC_USE_ADC1=0 -DSTM32_ADC_USE_ADC2=1
$(BUILDDIR)/both: CPPFLAGS += -DSTM32_ADC_USE_ADC2=1
$(BUILDDIR)/compact: CPPFLAGS += -DSTM32_ADC_COMPACT_SAMPLES=1
$(BUILDDIR)/dual16: CPPFLAGS += -DSTM32_ADC_DUAL_MODE=1
$(BUILDDIR)/dual8: CPPFLAGS += -DSTM32_ADC_DUAL_MODE=1 -DSTM32_ADC_COMPACT_SAMPLES=1
$(BUILDDIR)/configurations: CPPFLAGS += -DADC_USE_CONFIGURATIONS=1
$(BUILDDIR)/nosync: CPPFLAGS += -DADC_USE_SYNCHRONIZATION=0

$(BINARIES): adcv7.c hal.h $(LLD)/hal_adc_lld.c $(LLD)/hal_adc_lld.h \
             $(LLD)/../ADC/stm32_adc1.inc $(LLD)/../ADC/stm32_adc2.inc \
             $(LLD)/../DMA3v1/stm32_dma3.h \
             $(CHIBIOS)/os/xhal/include/hal_adc.h $(CHIBIOS)/os/xhal/src/hal_adc.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
