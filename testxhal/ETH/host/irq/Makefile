# ETH IRQ ownership tests with mocked service and NVIC.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
BUILDDIR := ../../build/host/irq
CPPFLAGS := -I$(STM32)/LLD/ETH
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wundef -Werror
TESTS := h503 h533 h562 h563 h573 h563_disabled \
         h743 h723 h735 h7a3 h743_disabled
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(filter $(BUILDDIR)/h5%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32H5xx -DTEST_VECTOR=106
$(filter $(BUILDDIR)/h7%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32H7xx -DTEST_VECTOR=61

$(BUILDDIR)/h743: CPPFLAGS += -DSTM32H743xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h723: CPPFLAGS += -DSTM32H723xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h735: CPPFLAGS += -DSTM32H735xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h7a3: CPPFLAGS += -DSTM32H7A3xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h743_disabled: CPPFLAGS += -DSTM32H743xx -DTEST_HAS_ETH=1 -DTEST_DISABLED

$(BUILDDIR)/h503: CPPFLAGS += -DSTM32H503xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h533: CPPFLAGS += -DSTM32H533xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h562: CPPFLAGS += -DSTM32H562xx -DTEST_HAS_ETH=0
$(BUILDDIR)/h563: CPPFLAGS += -DSTM32H563xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h573: CPPFLAGS += -DSTM32H573xx -DTEST_HAS_ETH=1
$(BUILDDIR)/h563_disabled: CPPFLAGS += -DSTM32H563xx -DTEST_HAS_ETH=1 -DTEST_DISABLED

$(BINARIES): eth_irq.c $(STM32)/LLD/ETH/stm32_eth1.inc \
             $(STM32)/STM32H5xx/stm32_isr.h \
             $(STM32)/STM32H5xx/stm32_registry.h \
             $(STM32)/STM32H7xx/stm32_isr.h \
             $(STM32)/STM32H7xx/stm32_registry.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
