# Actual USBv2 LLD with host-backed register and PMA storage.
CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/USBv2
BUILDDIR := ../../build/host/usbv2
CPPFLAGS := -I. -I$(LLD) -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=undefined,address
TESTS := normal fast iso fast_iso configurations
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/fast: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1
$(BUILDDIR)/iso: CPPFLAGS += -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/fast_iso: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1 -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/configurations: CPPFLAGS += -DUSB_USE_CONFIGURATIONS=1

$(BINARIES): usb_v2.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
