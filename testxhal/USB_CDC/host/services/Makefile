# Generated XHAL USB binder registration and CDC service functions.
CHIBIOS ?= ../../../..
BUILDDIR := build
CPPFLAGS := -I. -I$(BUILDDIR)
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -fsanitize=address,undefined -fno-sanitize-recover=undefined
TESTS := services services_max15
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/services_max15: CPPFLAGS += -DUSB_MAX_ENDPOINTS=15U

$(BUILDDIR)/generated.inc: $(CHIBIOS)/os/xhal/src/hal_usb.c $(CHIBIOS)/os/xhal/src/hal_usb_cdc.c extract.awk | $(BUILDDIR)
	awk -f extract.awk $(filter %.c,$^) > $@

$(BUILDDIR)/cdc_constants.inc: $(CHIBIOS)/os/xhal/include/hal_usb_cdc.h | $(BUILDDIR)
	awk '/^#define (CDC_DEFAULT_|LC_)/ {print}' $< > $@

$(BINARIES): services.c hal.h $(BUILDDIR)/generated.inc $(BUILDDIR)/cdc_constants.inc | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) -r $(BUILDDIR)
