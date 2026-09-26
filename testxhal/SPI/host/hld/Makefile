# Real generated SPI/base/callback HLDs, with a minimal LLD and kernel model.
CHIBIOS := ../../../..
XHAL := $(CHIBIOS)/os/xhal
OOP := $(CHIBIOS)/os/common/oop
BUILDDIR := ../../build/host/hld
CPPFLAGS := -I. -I$(XHAL)/include -I$(OOP)/include \
            -DOOP_USE_NOTHING -DCC_FORCE_INLINE=
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -fno-strict-aliasing
SOURCES := spi_hld.c $(XHAL)/src/hal_spi.c $(XHAL)/src/hal_cb_driver.c \
           $(XHAL)/src/hal_base_driver.c $(OOP)/src/oop_base_object.c
HEADERS := hal.h hal_spi_lld.h $(XHAL)/include/hal_spi.h \
           $(XHAL)/include/hal_cb_driver.h $(XHAL)/include/hal_base_driver.h
TESTS := circular_sync circular_async linear_sync linear_async
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/circular_sync: CPPFLAGS += -DTEST_CIRCULAR=1 -DSPI_USE_SYNCHRONIZATION=1
$(BUILDDIR)/circular_async: CPPFLAGS += -DTEST_CIRCULAR=1 -DSPI_USE_SYNCHRONIZATION=0
$(BUILDDIR)/linear_sync: CPPFLAGS += -DTEST_CIRCULAR=0 -DSPI_USE_SYNCHRONIZATION=1
$(BUILDDIR)/linear_async: CPPFLAGS += -DTEST_CIRCULAR=0 -DSPI_USE_SYNCHRONIZATION=0

$(BINARIES): $(SOURCES) $(HEADERS) | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

clean:
	$(RM) $(BINARIES)
