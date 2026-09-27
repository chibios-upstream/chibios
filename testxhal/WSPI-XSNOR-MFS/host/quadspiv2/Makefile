CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/QUADSPIv2
BUILDDIR := ../../build/host/quadspiv2
CPPFLAGS := -I. -I$(LLD) -I$(LLD)/../QUADSPI -I$(LLD)/../MDMAv1 \
            -I$(CHIBIOS)/os/xhal/include -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := normal nosync no_workaround no_sshift
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean priority_check
all: $(BINARIES) priority_check
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/nosync: CPPFLAGS += -DWSPI_USE_SYNCHRONIZATION=0
$(BUILDDIR)/no_workaround: CPPFLAGS += -DSTM32_USE_STM32_D1_WORKAROUND=0
$(BUILDDIR)/no_sshift: CPPFLAGS += -DSTM32_WSPI_SET_CR_SSHIFT=0

$(BINARIES): quadspi_v2.c hal.h $(wildcard $(LLD)/*) \
             $(LLD)/../QUADSPI/stm32_quadspi1.inc \
             $(LLD)/../MDMAv1/stm32_mdma.h \
             $(CHIBIOS)/os/xhal/ports/STM32/STM32H7xx/stm32_isr.h \
             $(CHIBIOS)/os/xhal/include/hal_wspi.h \
             $(CHIBIOS)/os/xhal/include/hal_cb_driver.h \
             $(CHIBIOS)/os/xhal/src/hal_wspi.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

priority_check: | $(BUILDDIR)
	@if $(CC) $(CPPFLAGS) -DSTM32_IRQ_QUADSPI1_PRIORITY=9 -E quadspi_v2.c \
	    > /dev/null 2> $(BUILDDIR)/priority.log; then \
	  echo "Mismatched priorities unexpectedly accepted"; exit 1; fi
	@grep -q -F "QUADSPI1 and MDMA IRQ priorities must match" $(BUILDDIR)/priority.log
	@$(CC) $(CPPFLAGS) -Wundef -Werror -E priority_check.c > /dev/null
	@if $(CC) $(CPPFLAGS) -DSTM32_IRQ_QUADSPI1_PRIORITY=9 -E priority_check.c \
	    > /dev/null 2> $(BUILDDIR)/priority.log; then \
	  echo "Header-only priority check unexpectedly accepted mismatch"; exit 1; fi
	@grep -q -F "QUADSPI1 and MDMA IRQ priorities must match" $(BUILDDIR)/priority.log
	@$(CC) $(CPPFLAGS) -Wundef -Werror -DSTM32_IRQ_QUADSPI1_PRIORITY=9 \
	    -DHAL_USE_WSPI=0 -E priority_check.c > /dev/null
	@$(CC) $(CPPFLAGS) -Wundef -Werror -DSTM32_IRQ_QUADSPI1_PRIORITY=9 \
	    -DSTM32_WSPI_USE_QUADSPI1=0 -E priority_check.c > /dev/null
	@$(CC) $(CPPFLAGS) -Wundef -Werror -DSTM32_IRQ_QUADSPI1_PRIORITY=9 \
	    -DSTM32_HAS_QUADSPI1=0 -E priority_check.c > /dev/null

clean:
	$(RM) $(BINARIES) $(BUILDDIR)/priority.log
