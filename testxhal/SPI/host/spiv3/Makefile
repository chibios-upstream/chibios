# Actual SPIv3 LLD with host-modeled DMA/BDMA and SPI registers.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/SPIv3
BUILDDIR := ../../build/host/spiv3
CPPFLAGS := -I. -I$(LLD) -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O0 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
TESTS := mixed dma bdma configurations pad
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/dma: CPPFLAGS += -DTEST_BDMA=0
$(BUILDDIR)/bdma: CPPFLAGS += -DTEST_DMA=0
$(BUILDDIR)/configurations: CPPFLAGS += -DSPI_USE_CONFIGURATIONS=1
$(BUILDDIR)/pad: CPPFLAGS += -DSTM32_SPI_SELECT_MODE=1

$(BINARIES): spi_v3.c hal.h $(LLD)/hal_spi_lld.c $(LLD)/hal_spi_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
