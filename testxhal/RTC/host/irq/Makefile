# RTCv3 IRQ ownership tests with real platform registries and mocked services.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
BUILDDIR := ../../build/host/irq
CPPFLAGS := -I$(STM32)/LLD/RTC -I$(STM32)/LLD/RTCv3
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wundef -Werror
PLATFORMS := g031 g071 g0b1 u083 u385 u575 l4p5 g474 h563 h723 h743 h7a3 l4r5
TESTS := $(PLATFORMS) $(addsuffix _disabled,$(PLATFORMS))
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all check-ownership clean
all: check-ownership $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

# A driver stop must not disable the vectors owned by irqInit()/irqDeinit().
check-ownership:
	@if grep -E -q 'nvic(Enable|Disable)Vector' $(STM32)/LLD/RTCv2/hal_rtc_lld.c; then \
	  echo "RTCv2 must leave NVIC ownership to the platform IRQ layer"; exit 1; \
	fi
	@if grep -E -q 'nvic(Enable|Disable)Vector' $(STM32)/LLD/RTCv3/hal_rtc_lld.c; then \
	  echo "RTCv3 must leave NVIC ownership to the platform IRQ layer"; exit 1; \
	fi

$(BUILDDIR):
	mkdir -p $@

$(addprefix $(BUILDDIR)/,$(PLATFORMS)): CPPFLAGS += -DTEST_ENABLED=1
$(filter %_disabled,$(BINARIES)): CPPFLAGS += -DTEST_ENABLED=0
$(filter $(BUILDDIR)/g0%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32G0xx -DTEST_LAYOUT=1
$(filter $(BUILDDIR)/g031%,$(BINARIES)): CPPFLAGS += -DSTM32G031xx
$(filter $(BUILDDIR)/g071%,$(BINARIES)): CPPFLAGS += -DSTM32G071xx
$(filter $(BUILDDIR)/g0b1%,$(BINARIES)): CPPFLAGS += -DSTM32G0B1xx
$(filter $(BUILDDIR)/u083%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32U0xx -DSTM32U083xx -DTEST_LAYOUT=1
$(filter $(BUILDDIR)/u385%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32U3xx -DSTM32U385xx -DTEST_LAYOUT=2
$(filter $(BUILDDIR)/u575%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32U5xx -DSTM32U575xx -DTEST_LAYOUT=2
$(filter $(BUILDDIR)/l4p5%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32L4xx+ -DSTM32L4P5xx -DTEST_LAYOUT=3
$(filter $(BUILDDIR)/g474%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32G4xx -DSTM32G474xx -DTEST_LAYOUT=3
$(filter $(BUILDDIR)/h563%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32H5xx -DSTM32H563xx -DTEST_LAYOUT=2
$(filter $(BUILDDIR)/h7%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32H7xx -DTEST_LAYOUT=3
$(filter $(BUILDDIR)/h723%,$(BINARIES)): CPPFLAGS += -DSTM32H723xx
$(filter $(BUILDDIR)/h743%,$(BINARIES)): CPPFLAGS += -DSTM32H743xx
$(filter $(BUILDDIR)/h7a3%,$(BINARIES)): CPPFLAGS += -DSTM32H7A3xxQ
$(filter $(BUILDDIR)/l4r5%,$(BINARIES)): CPPFLAGS += -I$(STM32)/STM32L4xx+ -DSTM32L4R5xx -DTEST_LAYOUT=3

$(BINARIES): rtc_irq.c $(wildcard $(STM32)/LLD/RTCv3/*.inc) \
             $(wildcard $(STM32)/LLD/RTC/*.inc) \
             $(wildcard $(STM32)/STM32*/stm32_isr.h) \
             $(wildcard $(STM32)/STM32*/stm32_registry.h) | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
