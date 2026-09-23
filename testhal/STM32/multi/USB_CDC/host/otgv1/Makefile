# Host regressions for EP0 ownership and FIFO preservation in HAL OTGv1.
CHIBIOS := ../../../../../..
LLD ?= $(CHIBIOS)/os/hal/ports/STM32/LLD/OTGv1
BUILDDIR ?= ../../build/host/otgv1
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(LLD) \
            -I$(CHIBIOS)/os/hal/include \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := dual otg1 otg2 wait ep0_thread
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit $$?; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/otg1: CPPFLAGS += -DSTM32_USB_USE_OTG2=0
$(BUILDDIR)/otg2: CPPFLAGS += -DSTM32_USB_USE_OTG1=0
$(BUILDDIR)/wait: CPPFLAGS += -DUSB_USE_WAIT=1
$(BUILDDIR)/ep0_thread: CPPFLAGS += -DUSB_USE_EP0_THREAD=1 -DUSB_USE_WAIT=1

$(BINARIES): ep0.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h \
             $(LLD)/stm32_otg.h $(CHIBIOS)/os/hal/include/hal_usb.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
