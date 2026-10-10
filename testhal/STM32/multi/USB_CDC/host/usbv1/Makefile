# Actual HAL USB frontend/LLD with host-backed registers and packet memory.
CHIBIOS ?= ../../../../../..
LLD ?= $(CHIBIOS)/os/hal/ports/STM32/LLD/USBv1
BUILDDIR ?= build
CPPFLAGS := -I. -I$(LLD) -I$(CHIBIOS)/os/hal/include -I$(CHIBIOS)/os/hal/src \
            -I$(CHIBIOS)/os/common/ext/ST/STM32G4xx \
            -I$(CHIBIOS)/os/common/ext/ST/STM32F3xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=all
DENSE := normal fast iso fast_iso wait ep0_thread release dbl dbl_iso
SPARSE := sparse sparse_fast sparse_iso
TESTS := $(DENSE) $(SPARSE)
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all probe clean negative
all: $(BINARIES)
	@for test in $(BINARIES); do echo "== $$test"; $$test || exit $$?; done

probe: $(BUILDDIR)/iso
	@ulimit -c 0; $< $(PROBE)

$(BUILDDIR):
	mkdir -p $@

# Dense PMA (STM32G474), sparse PMA (STM32F303).
$(addprefix $(BUILDDIR)/,$(DENSE)): CPPFLAGS += -DTEST_DEVICE_HEADER='"stm32g474xx.h"'
$(addprefix $(BUILDDIR)/,$(SPARSE)): CPPFLAGS += -DTEST_DEVICE_HEADER='"stm32f303xc.h"' -DTEST_SPARSE_PMA
$(BUILDDIR)/fast $(BUILDDIR)/sparse_fast: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1
$(BUILDDIR)/iso $(BUILDDIR)/sparse_iso: CPPFLAGS += -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/fast_iso: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1 -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/wait: CPPFLAGS += -DUSB_USE_WAIT=1
$(BUILDDIR)/ep0_thread: CPPFLAGS += -DUSB_USE_WAIT=1 -DUSB_USE_EP0_THREAD=1
$(BUILDDIR)/release: CPPFLAGS += -DTEST_NO_DEBUG -DSTM32_USB_USE_ISOCHRONOUS=1
# Double buffering compiled in, the endpoints stay single-buffered.
$(BUILDDIR)/dbl: CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1
$(BUILDDIR)/dbl_iso: CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1 -DSTM32_USB_USE_ISOCHRONOUS=1

$(BINARIES): usbv1.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h $(LLD)/stm32_usb.h \
             $(CHIBIOS)/os/hal/include/hal_usb.h $(CHIBIOS)/os/hal/src/hal_usb.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# Reverts each driver fix in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) $(BINARIES)
