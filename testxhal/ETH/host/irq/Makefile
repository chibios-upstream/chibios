# ETH IRQ ownership tests with mocked service and NVIC.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
BUILDDIR := ../../build/host/irq
CPPFLAGS := -I$(STM32)/STM32H5xx -I$(STM32)/LLD/ETH
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wundef -Werror
TESTS := h503 h533 h562 h563 h573 h563_disabled
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/h503: CPPFLAGS += -DSTM32H503xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h533: CPPFLAGS += -DSTM32H533xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h562: CPPFLAGS += -DSTM32H562xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h563: CPPFLAGS += -DSTM32H563xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h573: CPPFLAGS += -DSTM32H573xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h563_disabled: CPPFLAGS += -DSTM32H563xx -DTEST_HAS_ETH=1 -DTEST_DISABLED

$(BINARIES): eth_irq.c $(STM32)/LLD/ETH/stm32_eth1.inc \
             $(STM32)/STM32H5xx/stm32_isr.h \
             $(STM32)/STM32H5xx/stm32_registry.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
