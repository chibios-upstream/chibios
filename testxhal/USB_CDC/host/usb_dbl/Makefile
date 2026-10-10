# XHAL USBv1 and USBv2 LLDs, endpoint register and ISTR accesses rewritten
# into calls to a peripheral model of the double-buffered bulk endpoints.
# The regression and the access rewriting are those of the HAL drivers.
CHIBIOS ?= ../../../..
XLLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD
BUILDDIR ?= build
SHARED := ../../../../testhal/STM32/multi/USB_CDC/host/usbv1_dbl
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=all
V1 := v1_dbl v1_dbl_fast v1_dbl_iso v1_dbl_shared v1_dbl_release v1_single \
      v1_single_shared
V2 := v2_dbl v2_dbl_fast v2_dbl_iso v2_dbl_release v2_single
TESTS := $(V1) $(V2)
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
SEEDS ?= 1 300
.PHONY: all clean negative
all: $(BINARIES)
	@for test in $(BINARIES); do echo "== $$test"; $$test $(SEEDS) || exit $$?; done

$(BUILDDIR)/v1 $(BUILDDIR)/v2:
	mkdir -p $@

$(BUILDDIR)/v1/hal_usb_lld.c: $(XLLD)/USBv1/hal_usb_lld.c $(SHARED)/epr_access.py | $(BUILDDIR)/v1
	python3 $(SHARED)/epr_access.py $< $@
$(BUILDDIR)/v1/stm32_usb.h: $(XLLD)/USBv1/stm32_usb.h $(SHARED)/epr_access.py | $(BUILDDIR)/v1
	python3 $(SHARED)/epr_access.py $< $@
$(BUILDDIR)/v1/hal_usb_lld.h: $(XLLD)/USBv1/hal_usb_lld.h | $(BUILDDIR)/v1
	cp $< $@
$(BUILDDIR)/v2/hal_usb_lld.c: $(XLLD)/USBv2/hal_usb_lld.c $(SHARED)/epr_access.py | $(BUILDDIR)/v2
	python3 $(SHARED)/epr_access.py $< $@
$(BUILDDIR)/v2/hal_usb_lld.h: $(XLLD)/USBv2/hal_usb_lld.h | $(BUILDDIR)/v2
	cp $< $@

V1_SRC := $(addprefix $(BUILDDIR)/v1/,hal_usb_lld.c hal_usb_lld.h stm32_usb.h)
V2_SRC := $(addprefix $(BUILDDIR)/v2/,hal_usb_lld.c hal_usb_lld.h)

# USBv1 as on STM32G474, USBv2 as on STM32H563.
$(addprefix $(BUILDDIR)/,$(V1)): CPPFLAGS := -DTEST_XHAL -I$(BUILDDIR)/v1 -I. \
    -I$(CHIBIOS)/os/common/ext/ST/STM32G4xx \
    -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
$(addprefix $(BUILDDIR)/,$(V2)): CPPFLAGS := -DTEST_XHAL -DTEST_USBV2 -I$(BUILDDIR)/v2 -I. \
    -I$(CHIBIOS)/os/common/ext/ST/STM32H5xx \
    -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
$(addprefix $(BUILDDIR)/,v1_dbl v1_dbl_fast v1_dbl_iso v1_dbl_shared v1_dbl_release \
    v2_dbl v2_dbl_fast v2_dbl_iso v2_dbl_release): CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1
$(BUILDDIR)/v1_dbl_fast $(BUILDDIR)/v2_dbl_fast: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1
$(BUILDDIR)/v1_dbl_iso $(BUILDDIR)/v2_dbl_iso: CPPFLAGS += -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/v1_dbl_shared $(BUILDDIR)/v1_single_shared: CPPFLAGS += -DTEST_SHARED_VECTOR
$(BUILDDIR)/v1_dbl_release $(BUILDDIR)/v2_dbl_release: CPPFLAGS += -DTEST_NO_DEBUG

$(addprefix $(BUILDDIR)/,$(V1)): $(SHARED)/dblbuf.c hal.h hal_usb.c $(V1_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@
$(addprefix $(BUILDDIR)/,$(V2)): $(SHARED)/dblbuf.c hal.h hal_usb.c $(V2_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# Reverts each driver behavior in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) -r $(BUILDDIR)
