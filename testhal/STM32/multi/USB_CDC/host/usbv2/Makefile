# Actual HAL USB frontend/LLD with host-backed registers and packet memory.
CHIBIOS := ../../../../../..
LLD ?= $(CHIBIOS)/os/hal/ports/STM32/LLD/USBv2
BUILDDIR ?= ../../build/host/usbv2
CPPFLAGS := -I. -I$(LLD) -I$(CHIBIOS)/os/hal/include -I$(CHIBIOS)/os/hal/src \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=all
TESTS := normal fast iso fast_iso wait ep0_thread release dbl dbl_iso
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all probe clean negative
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit $$?; done

probe: $(BUILDDIR)/iso
	@ulimit -c 0; $< $(PROBE)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/fast: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1
$(BUILDDIR)/iso: CPPFLAGS += -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/fast_iso: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1 -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/wait: CPPFLAGS += -DUSB_USE_WAIT=1
$(BUILDDIR)/ep0_thread: CPPFLAGS += -DUSB_USE_WAIT=1 -DUSB_USE_EP0_THREAD=1
$(BUILDDIR)/release: CPPFLAGS += -DTEST_NO_DEBUG -DSTM32_USB_USE_ISOCHRONOUS=1
# Double buffering compiled in, the endpoints stay single-buffered.
$(BUILDDIR)/dbl: CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1
$(BUILDDIR)/dbl_iso: CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1 -DSTM32_USB_USE_ISOCHRONOUS=1

$(BINARIES): usbv2.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h \
             $(CHIBIOS)/os/hal/include/hal_usb.h $(CHIBIOS)/os/hal/src/hal_usb.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# Reverts each driver fix in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) $(BINARIES)
