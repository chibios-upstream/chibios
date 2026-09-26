# Actual XHAL DMA sources and CMSIS registers, with host-mapped MMIO.
CHIBIOS := ../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD
BUILDDIR := ../build/host
CPPFLAGS := -D_DEFAULT_SOURCE -I. \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := dmav1 dmav1_shared dmav2 bdma dma3_h5 dma3_u5 mdma
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/dmav1: CPPFLAGS += -DTEST_DMAV1 -DSTM32G474xx \
  -I$(CHIBIOS)/os/common/ext/ST/STM32G4xx \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32G4xx -I$(LLD)/DMAv1
$(BUILDDIR)/dmav1_shared: CPPFLAGS += -DTEST_DMAV1 -DTEST_SHARED -DSTM32G0B1xx \
  -I$(CHIBIOS)/os/common/ext/ST/STM32G0xx \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32G0xx -I$(LLD)/DMAv1
$(BUILDDIR)/dmav2: CPPFLAGS += -DTEST_DMAV2 -I$(LLD)/DMAv2
$(BUILDDIR)/bdma: CPPFLAGS += -DTEST_BDMA -I$(LLD)/BDMAv1
$(BUILDDIR)/mdma: CPPFLAGS += -DTEST_MDMA -I$(LLD)/MDMAv1
$(BUILDDIR)/dmav2 $(BUILDDIR)/bdma $(BUILDDIR)/mdma: CPPFLAGS += \
  -DSTM32H743xx -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32H7xx
$(BUILDDIR)/dma3_h5: CPPFLAGS += -DTEST_DMA3 -DSTM32H563xx \
  -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32H5xx -I$(LLD)/DMA3v1
$(BUILDDIR)/dma3_u5: CPPFLAGS += -DTEST_DMA3 -DSTM32U575xx \
  -I$(CHIBIOS)/os/common/ext/ST/STM32U5xx \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx -I$(LLD)/DMA3v1

SOURCES := $(foreach backend,DMAv1 DMAv2 BDMAv1 DMA3v1 MDMAv1,\
             $(wildcard $(LLD)/$(backend)/*.[ch] $(LLD)/$(backend)/*.inc))
$(BINARIES): dma_flags.c hal.h $(SOURCES) | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
