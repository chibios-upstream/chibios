# Real ADC frontend/LLD and DMA register helpers, with mocked hardware/RTOS.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/ADCv4
BUILDDIR := ../../build/host/adcv4
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(LLD) \
            -I$(CHIBIOS)/os/xhal/include -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32/STM32H7xx \
            -I$(CHIBIOS)/os/xhal/ports/STM32/LLD/ADC \
            -I$(CHIBIOS)/os/xhal/ports/STM32/LLD/DMAv2 \
            -I$(CHIBIOS)/os/xhal/ports/STM32/LLD/BDMAv1 \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := adc12 single8 single16 single32 dual32 adc3_dma adc3_bdma both_bdma configurations nosync
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/adc12: CPPFLAGS += -DSTM32_ADC_USE_ADC3=0
$(BUILDDIR)/single8: CPPFLAGS += -DSTM32_ADC_SAMPLES_SIZE=8
$(BUILDDIR)/single32: CPPFLAGS += -DSTM32_ADC_SAMPLES_SIZE=32
$(BUILDDIR)/dual32: CPPFLAGS += -DSTM32_ADC_SAMPLES_SIZE=32 -DSTM32_ADC_DUAL_MODE=1
$(BUILDDIR)/adc3_dma: CPPFLAGS += -DSTM32_ADC_USE_ADC12=0 -DSTM32_ADC_SAMPLES_SIZE=8
$(BUILDDIR)/adc3_bdma: CPPFLAGS += -DSTM32_ADC_USE_ADC12=0 -DSTM32_ADC_ADC3_USE_BDMA=1
$(BUILDDIR)/both_bdma: CPPFLAGS += -DSTM32_ADC_ADC3_USE_BDMA=1 -DSTM32_ADC_SAMPLES_SIZE=32 -DSTM32_ADC_DUAL_MODE=1
$(BUILDDIR)/configurations: CPPFLAGS += -DADC_USE_CONFIGURATIONS=1
$(BUILDDIR)/nosync: CPPFLAGS += -DADC_USE_SYNCHRONIZATION=0

$(BINARIES): adcv4.c hal.h $(LLD)/hal_adc_lld.c $(LLD)/hal_adc_lld.h \
             $(CHIBIOS)/os/xhal/include/hal_adc.h $(CHIBIOS)/os/xhal/src/hal_adc.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
