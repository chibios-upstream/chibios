# Actual HAL USB frontend/USBv2 LLD, CHEPR and ISTR accesses rewritten into
# calls to a peripheral model of the double-buffered bulk endpoints. The
# regression and the access rewriting are shared with usbv1_dbl.
CHIBIOS ?= ../../../../../..
LLD ?= $(CHIBIOS)/os/hal/ports/STM32/LLD/USBv2
BUILDDIR ?= build
SRC := $(BUILDDIR)/src
SHARED := ../usbv1_dbl
CPPFLAGS := -DTEST_USBV2 -I$(SRC) -I. -I$(CHIBIOS)/os/hal/include \
            -I$(CHIBIOS)/os/hal/src -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=all
TESTS := dbl dbl_fast dbl_iso dbl_release single
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
SEEDS ?= 1 300
.PHONY: all clean negative
all: $(BINARIES)
	@for test in $(BINARIES); do echo "== $$test"; $$test $(SEEDS) || exit $$?; done

$(BUILDDIR) $(SRC):
	mkdir -p $@

$(SRC)/hal_usb_lld.c: $(LLD)/hal_usb_lld.c $(SHARED)/epr_access.py | $(SRC)
	python3 $(SHARED)/epr_access.py $< $@
$(SRC)/hal_usb_lld.h: $(LLD)/hal_usb_lld.h | $(SRC)
	cp $< $@

$(addprefix $(BUILDDIR)/,dbl dbl_fast dbl_iso dbl_release): CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1
$(BUILDDIR)/dbl_fast: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1
$(BUILDDIR)/dbl_iso: CPPFLAGS += -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/dbl_release: CPPFLAGS += -DTEST_NO_DEBUG

$(BINARIES): $(SHARED)/dblbuf.c hal.h $(SRC)/hal_usb_lld.c $(SRC)/hal_usb_lld.h \
             $(CHIBIOS)/os/hal/include/hal_usb.h $(CHIBIOS)/os/hal/src/hal_usb.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# Reverts each driver behavior in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) -r $(BUILDDIR)
