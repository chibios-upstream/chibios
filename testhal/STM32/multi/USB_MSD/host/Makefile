# Actual USB_MSD driver with mocked OSAL, USB driver and block device.
CHIBIOS ?= ../../../../../..
MSD ?= $(CHIBIOS)/os/hal/lib/complex/usb_msd
BUILDDIR ?= build
CPPFLAGS := -I. -I$(MSD) -I$(CHIBIOS)/os/hal/include
CFLAGS := -std=gnu11 -O1 -g -Wall -Wextra -Werror -pthread
ASAN := -fsanitize=address,undefined -fno-sanitize-recover=all
TESTS := normal small large opt tsan
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean negative
# ThreadSanitizer requires a non randomized address space on recent kernels.
all: $(BINARIES)
	@for test in $(BINARIES); do echo "== $$test"; \
	  setarch $$(uname -m) -R $$test || exit $$?; done

$(BUILDDIR):
	mkdir -p $@

# Buffer halves of 4, 1 and 8 blocks, optimized code and thread sanitizer.
$(BUILDDIR)/normal $(BUILDDIR)/small $(BUILDDIR)/large $(BUILDDIR)/opt: CFLAGS += $(ASAN)
$(BUILDDIR)/small: CPPFLAGS += -DUSB_MSD_CFG_BUFFER_SIZE=1024
$(BUILDDIR)/large: CPPFLAGS += -DUSB_MSD_CFG_BUFFER_SIZE=8192
$(BUILDDIR)/opt: CFLAGS += -O2
$(BUILDDIR)/tsan: CFLAGS += -fsanitize=thread

$(BINARIES): msd_host.c hal.h $(MSD)/hal_usb_msd.c $(MSD)/hal_usb_msd.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# Reverts each driver behavior in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) $(BINARIES)
