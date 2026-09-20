# Host regression: real LLD, modeled SPI/GPDMA registers, no attached board.
CHIBIOS := ../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/SPIv4
BUILDDIR := ../build/host
CPPFLAGS := -I. -I$(LLD) \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O0 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast

.PHONY: all clean
all: $(BUILDDIR)/spi_v4 $(BUILDDIR)/spi_v4_configurations $(BUILDDIR)/spi_v4_pad \
     $(BUILDDIR)/spi_v4_default16 $(BUILDDIR)/spi_v4_default32
	$(BUILDDIR)/spi_v4
	$(BUILDDIR)/spi_v4_configurations
	$(BUILDDIR)/spi_v4_pad
	$(BUILDDIR)/spi_v4_default16
	$(BUILDDIR)/spi_v4_default32

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/spi_v4: spi_v4.c hal.h $(LLD)/hal_spi_lld.c $(LLD)/hal_spi_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILDDIR)/spi_v4_configurations: spi_v4.c hal.h $(LLD)/hal_spi_lld.c $(LLD)/hal_spi_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSPI_USE_CONFIGURATIONS=1 $< -o $@

$(BUILDDIR)/spi_v4_pad: spi_v4.c hal.h $(LLD)/hal_spi_lld.c $(LLD)/hal_spi_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSTM32_SPI_SELECT_MODE=1 $< -o $@

$(BUILDDIR)/spi_v4_default16: spi_v4.c hal.h $(LLD)/hal_spi_lld.c $(LLD)/hal_spi_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSTM32_SPI_DEFAULT_CFG1=SPI_CFG1_DSIZE_16BITS $< -o $@

$(BUILDDIR)/spi_v4_default32: spi_v4.c hal.h $(LLD)/hal_spi_lld.c $(LLD)/hal_spi_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSTM32_SPI_DEFAULT_CFG1=SPI_CFG1_DSIZE_32BITS $< -o $@

clean:
	$(RM) $(BUILDDIR)/spi_v4 $(BUILDDIR)/spi_v4_configurations $(BUILDDIR)/spi_v4_pad \
	      $(BUILDDIR)/spi_v4_default16 $(BUILDDIR)/spi_v4_default32
