# Actual HAL USB frontend/LLD, EPR and ISTR accesses rewritten into calls to
# a peripheral model of the double-buffered bulk endpoints.
CHIBIOS ?= ../../../../../..
LLD ?= $(CHIBIOS)/os/hal/ports/STM32/LLD/USBv1
BUILDDIR ?= build
SRC := $(BUILDDIR)/src
CPPFLAGS := -I$(SRC) -I. -I$(CHIBIOS)/os/hal/include -I$(CHIBIOS)/os/hal/src \
            -I$(CHIBIOS)/os/common/ext/ST/STM32G4xx \
            -I$(CHIBIOS)/os/common/ext/ST/STM32F3xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=all
DENSE := dbl dbl_fast dbl_iso dbl_shared dbl_release single single_shared
SPARSE := dbl_sparse single_sparse
TESTS := $(DENSE) $(SPARSE)
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
SEEDS ?= 1 300
.PHONY: all clean negative
all: $(BINARIES)
	@for test in $(BINARIES); do echo "== $$test"; $$test $(SEEDS) || exit $$?; done

$(BUILDDIR) $(SRC):
	mkdir -p $@

$(SRC)/hal_usb_lld.c: $(LLD)/hal_usb_lld.c epr_access.py | $(SRC)
	python3 epr_access.py $< $@
$(SRC)/stm32_usb.h: $(LLD)/stm32_usb.h epr_access.py | $(SRC)
	python3 epr_access.py $< $@
$(SRC)/hal_usb_lld.h: $(LLD)/hal_usb_lld.h | $(SRC)
	cp $< $@

# Dense PMA (STM32G474), sparse PMA (STM32F303).
$(addprefix $(BUILDDIR)/,$(DENSE)): CPPFLAGS += -DTEST_DEVICE_HEADER='"stm32g474xx.h"'
$(addprefix $(BUILDDIR)/,$(SPARSE)): CPPFLAGS += -DTEST_DEVICE_HEADER='"stm32f303xc.h"' -DTEST_SPARSE_PMA
$(addprefix $(BUILDDIR)/,dbl dbl_fast dbl_iso dbl_shared dbl_release dbl_sparse): CPPFLAGS += -DSTM32_USB_USE_DOUBLE_BUFFERING=1
$(BUILDDIR)/dbl_fast: CPPFLAGS += -DSTM32_USB_USE_FAST_COPY=1
$(BUILDDIR)/dbl_iso: CPPFLAGS += -DSTM32_USB_USE_ISOCHRONOUS=1
$(BUILDDIR)/dbl_shared $(BUILDDIR)/single_shared: CPPFLAGS += -DTEST_SHARED_VECTOR
$(BUILDDIR)/dbl_release: CPPFLAGS += -DTEST_NO_DEBUG

$(BINARIES): dblbuf.c hal.h $(SRC)/hal_usb_lld.c $(SRC)/stm32_usb.h $(SRC)/hal_usb_lld.h \
             $(CHIBIOS)/os/hal/include/hal_usb.h $(CHIBIOS)/os/hal/src/hal_usb.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# Reverts each driver behavior in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) -r $(BUILDDIR)
