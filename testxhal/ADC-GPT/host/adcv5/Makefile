# Real ADC frontend/ADCv5 LLD, CMSIS and DMA register helpers, mocked RTOS/ADC.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/ADCv5
BUILDDIR := ../../build/host/adcv5
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(LLD) \
            -I$(CHIBIOS)/os/xhal/include -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32/STM32U0xx \
            -I$(CHIBIOS)/os/xhal/ports/STM32/LLD/DMAv1 \
            -I$(CHIBIOS)/os/common/ext/ST/STM32U0xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := u031 u073 u083 configurations nosync
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/u031: CPPFLAGS += -DSTM32U031xx
$(BUILDDIR)/u073: CPPFLAGS += -DSTM32U073xx
$(BUILDDIR)/u083 $(BUILDDIR)/configurations $(BUILDDIR)/nosync: CPPFLAGS += -DSTM32U083xx
$(BUILDDIR)/configurations: CPPFLAGS += -DADC_USE_CONFIGURATIONS=1
$(BUILDDIR)/nosync: CPPFLAGS += -DADC_USE_SYNCHRONIZATION=0

$(BINARIES): adcv5.c hal.h $(LLD)/hal_adc_lld.c $(LLD)/hal_adc_lld.h \
             $(CHIBIOS)/os/xhal/include/hal_adc.h $(CHIBIOS)/os/xhal/src/hal_adc.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
