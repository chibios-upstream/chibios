# Native regression using the real OCTOSPIv3 LLD and shared IRQ handlers.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/OCTOSPIv3
SHARED := $(CHIBIOS)/os/xhal/ports/STM32/LLD/OCTOSPI
BUILDDIR := ../../build/host/octospiv3
CPPFLAGS := -I. -I$(LLD) -I$(SHARED) \
            -I$(CHIBIOS)/os/common/ext/ST/STM32U5xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
TESTS := ospi1 ospi2 dual single_device no_sync

.PHONY: all clean
all: $(addprefix $(BUILDDIR)/,$(TESTS))
	@set -e; for test in $^; do $$test; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/ospi1: CPPFLAGS += -DSTM32_WSPI_USE_OCTOSPI2=0
$(BUILDDIR)/ospi2: CPPFLAGS += -DSTM32_WSPI_USE_OCTOSPI1=0
$(BUILDDIR)/single_device: CPPFLAGS += -DSTM32_WSPI_USE_OCTOSPI2=0 -DSTM32_HAS_OCTOSPI2=0
$(BUILDDIR)/no_sync: CPPFLAGS += -DWSPI_USE_SYNCHRONIZATION=0

$(addprefix $(BUILDDIR)/,$(TESTS)): octospi_v3.c hal.h $(LLD)/hal_wspi_lld.c \
                                $(LLD)/hal_wspi_lld.h \
                                $(SHARED)/stm32_octospi1.inc \
                                $(SHARED)/stm32_octospi2.inc | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(addprefix $(BUILDDIR)/,$(TESTS))
