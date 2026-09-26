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
U5_FS := u575 u585
U5_HS := u595 u599 u5a5 u5a9 u5f7 u5f9 u5g7 u5g9
TESTS += $(U5_FS) $(U5_HS) u5a5_fs
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

$(addprefix $(BUILDDIR)/,$(U5_FS) $(U5_HS) u5a5_fs): CPPFLAGS += -DTEST_U5 \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx \
  -I$(CHIBIOS)/os/common/ext/ST/STM32U5xx
$(addprefix $(BUILDDIR)/,$(U5_FS)): CPPFLAGS += -DTEST_OTG2=0
$(addprefix $(BUILDDIR)/,$(U5_HS) u5a5_fs): CPPFLAGS += -DTEST_OTG1=0
$(BUILDDIR)/u5a5_fs: CPPFLAGS += -DSTM32_USE_USB_OTG2_HS=0
$(BUILDDIR)/u575: CPPFLAGS += -DSTM32U575xx -DTEST_DEVICE_HEADER=\"stm32u575xx.h\"
$(BUILDDIR)/u585: CPPFLAGS += -DSTM32U585xx -DTEST_DEVICE_HEADER=\"stm32u585xx.h\"
$(BUILDDIR)/u595: CPPFLAGS += -DSTM32U595xx -DTEST_DEVICE_HEADER=\"stm32u595xx.h\"
$(BUILDDIR)/u599: CPPFLAGS += -DSTM32U599xx -DTEST_DEVICE_HEADER=\"stm32u599xx.h\"
$(BUILDDIR)/u5a5 $(BUILDDIR)/u5a5_fs: CPPFLAGS += -DSTM32U5A5xx -DTEST_DEVICE_HEADER=\"stm32u5a5xx.h\"
$(BUILDDIR)/u5a9: CPPFLAGS += -DSTM32U5A9xx -DTEST_DEVICE_HEADER=\"stm32u5a9xx.h\"
$(BUILDDIR)/u5f7: CPPFLAGS += -DSTM32U5F7xx -DTEST_DEVICE_HEADER=\"stm32u5f7xx.h\"
$(BUILDDIR)/u5f9: CPPFLAGS += -DSTM32U5F9xx -DTEST_DEVICE_HEADER=\"stm32u5f9xx.h\"
$(BUILDDIR)/u5g7: CPPFLAGS += -DSTM32U5G7xx -DTEST_DEVICE_HEADER=\"stm32u5g7xx.h\"
$(BUILDDIR)/u5g9: CPPFLAGS += -DSTM32U5G9xx -DTEST_DEVICE_HEADER=\"stm32u5g9xx.h\"

$(BINARIES): otg_v1.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h $(LLD)/stm32_otg.h | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(addprefix $(BUILDDIR)/,$(U5_FS) $(U5_HS) u5a5_fs): \
  $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/stm32_registry.h \
  $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/stm32_isr.h \
  $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/stm32_clock_usage.h \
  $(LLD)/stm32_otg1.inc $(LLD)/stm32_otg2.inc

clean:
	$(RM) $(BINARIES)
