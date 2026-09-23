# Actual OTGv1 LLD with shared-memory registers and a reset/flush helper.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/OTGv1
BUILDDIR := ../../build/host/otgv1
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(LLD) \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := dual fs hs ulpi_fs ulpi_hs configurations basepri
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/fs: CPPFLAGS += -DTEST_OTG2=0
$(BUILDDIR)/hs: CPPFLAGS += -DTEST_OTG1=0
$(BUILDDIR)/ulpi_fs: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=0
$(BUILDDIR)/ulpi_hs: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=1
$(BUILDDIR)/configurations: CPPFLAGS += -DUSB_USE_CONFIGURATIONS=1
$(BUILDDIR)/basepri: CPPFLAGS += -DSTM32_USB_OTGFIFO_FILL_BASEPRI=6 -DSTM32_OTG_SEQUENCE_WORKAROUND

$(BINARIES): otg_v1.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h $(LLD)/stm32_otg.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
