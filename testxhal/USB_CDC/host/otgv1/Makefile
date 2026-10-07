# Actual OTGv1 LLD with shared-memory registers and a reset/flush helper.
CHIBIOS ?= ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/OTGv1
BUILDDIR := build
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(BUILDDIR) -I$(LLD) \
            -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32 \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=undefined
TESTS := dual fs hs ulpi_fs ulpi_hs configurations basepri
TESTS += stepping1 stepping1_novbus stepping1_ulpi_hs
U5_FS := u575 u585
U5_HS := u595 u599 u5a5 u5a9 u5f7 u5f9 u5g7 u5g9
TESTS += $(U5_FS) $(U5_HS) u5a5_fs
TESTS += no_sync
TESTS += novbus ulpi_fs_novbus ulpi_hs_novbus stepping1_ulpi_novbus
TESTS += u575_novbus u5a5_novbus u5a5_fs_novbus
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
PHY_BINARIES := $(addprefix $(BUILDDIR)/phy_,$(U5_HS))
.PHONY: all clean registry
all: $(BINARIES) $(PHY_BINARIES) registry
	@for test in $(BINARIES) $(PHY_BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/fs: CPPFLAGS += -DTEST_OTG2=0
$(BUILDDIR)/hs: CPPFLAGS += -DTEST_OTG1=0
$(BUILDDIR)/ulpi_fs $(BUILDDIR)/ulpi_fs_novbus: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=0
$(BUILDDIR)/ulpi_hs $(BUILDDIR)/ulpi_hs_novbus: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=1
$(BUILDDIR)/configurations: CPPFLAGS += -DUSB_USE_CONFIGURATIONS=1
$(BUILDDIR)/no_sync: CPPFLAGS += -DUSB_USE_SYNCHRONIZATION=0
$(BUILDDIR)/basepri: CPPFLAGS += -DSTM32_USB_OTGFIFO_FILL_BASEPRI=6 -DSTM32_OTG_SEQUENCE_WORKAROUND
$(BUILDDIR)/stepping1 $(BUILDDIR)/stepping1_novbus $(BUILDDIR)/stepping1_ulpi_hs $(BUILDDIR)/stepping1_ulpi_novbus: CPPFLAGS += -DSTM32_OTG_STEPPING=1
$(BUILDDIR)/novbus $(filter %_novbus,$(BINARIES)): CPPFLAGS += -DBOARD_OTG_NOVBUSSENS
$(BUILDDIR)/stepping1_ulpi_hs $(BUILDDIR)/stepping1_ulpi_novbus: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=1

$(addprefix $(BUILDDIR)/,$(U5_FS) $(U5_HS) u5a5_fs u575_novbus u5a5_novbus u5a5_fs_novbus): CPPFLAGS += -DTEST_U5 \
  -I$(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx \
  -I$(CHIBIOS)/os/common/ext/ST/STM32U5xx
$(addprefix $(BUILDDIR)/,$(U5_FS) u575_novbus): CPPFLAGS += -DTEST_OTG2=0
$(addprefix $(BUILDDIR)/,$(U5_HS) u5a5_fs u5a5_novbus u5a5_fs_novbus): CPPFLAGS += -DTEST_OTG1=0
$(BUILDDIR)/u5a5_fs $(BUILDDIR)/u5a5_fs_novbus: CPPFLAGS += -DSTM32_USE_USB_OTG2_HS=0
$(BUILDDIR)/u575 $(BUILDDIR)/u575_novbus: CPPFLAGS += -DSTM32U575xx -DTEST_DEVICE_HEADER=\"stm32u575xx.h\"
$(BUILDDIR)/u585: CPPFLAGS += -DSTM32U585xx -DTEST_DEVICE_HEADER=\"stm32u585xx.h\"
$(BUILDDIR)/u595: CPPFLAGS += -DSTM32U595xx -DTEST_DEVICE_HEADER=\"stm32u595xx.h\"
$(BUILDDIR)/u599: CPPFLAGS += -DSTM32U599xx -DTEST_DEVICE_HEADER=\"stm32u599xx.h\"
$(BUILDDIR)/u5a5 $(BUILDDIR)/u5a5_fs $(BUILDDIR)/u5a5_novbus $(BUILDDIR)/u5a5_fs_novbus: CPPFLAGS += -DSTM32U5A5xx -DTEST_DEVICE_HEADER=\"stm32u5a5xx.h\"
$(BUILDDIR)/u5a9: CPPFLAGS += -DSTM32U5A9xx -DTEST_DEVICE_HEADER=\"stm32u5a9xx.h\"
$(BUILDDIR)/u5f7: CPPFLAGS += -DSTM32U5F7xx -DTEST_DEVICE_HEADER=\"stm32u5f7xx.h\"
$(BUILDDIR)/u5f9: CPPFLAGS += -DSTM32U5F9xx -DTEST_DEVICE_HEADER=\"stm32u5f9xx.h\"
$(BUILDDIR)/u5g7: CPPFLAGS += -DSTM32U5G7xx -DTEST_DEVICE_HEADER=\"stm32u5g7xx.h\"
$(BUILDDIR)/u5g9: CPPFLAGS += -DSTM32U5G9xx -DTEST_DEVICE_HEADER=\"stm32u5g9xx.h\"

$(BUILDDIR)/hld_fault.inc: $(CHIBIOS)/os/xhal/src/hal_usb.c $(CHIBIOS)/os/xhal/src/hal_base_driver.c extract_hld.awk | $(BUILDDIR)
	awk -f extract_hld.awk $(filter %.c,$^) > $@

$(BUILDDIR)/usb_constants.inc: $(CHIBIOS)/os/xhal/include/hal_usb.h | $(BUILDDIR)
	awk '/^#define USB_(RTYPE|REQ_|FEATURE_|LATE_SET_ADDRESS|FLAGS_)/ {print "#ifndef " $$2; print; print "#endif"}' $< > $@

$(BINARIES): otg_v1.c hal.h in_disable.inc out_disable.inc reviewer.inc $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h $(LLD)/stm32_otg.h $(BUILDDIR)/usb_constants.inc $(BUILDDIR)/hld_fault.inc | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BINARIES): $(CHIBIOS)/os/xhal/ports/STM32/STM32H7xx/stm32_registry.h \
             $(CHIBIOS)/os/xhal/src/hal_safety.c

$(addprefix $(BUILDDIR)/,$(U5_FS) $(U5_HS) u5a5_fs u575_novbus u5a5_novbus u5a5_fs_novbus): \
  $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/stm32_registry.h \
  $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/stm32_isr.h \
  $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/stm32_clock_usage.h \
  $(LLD)/stm32_otg1.inc $(LLD)/stm32_otg2.inc

registry: $(BUILDDIR)/usb_constants.inc
	CHIBIOS="$(abspath $(CHIBIOS))" python3 registry_checks.py

$(BUILDDIR)/phy_code.inc: $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/hal_lld.c phy/extract.awk | $(BUILDDIR)
	awk -f phy/extract.awk $< > $@

$(BUILDDIR)/phy_constants.inc: $(CHIBIOS)/os/xhal/ports/STM32/STM32U5xx/hal_lld.h | $(BUILDDIR)
	awk '/^#define STM32_USB_BOOSTER_STARTUP_TIME / {print; found++} END {if (found != 1) exit 1}' $< > $@

$(PHY_BINARIES): $(BUILDDIR)/phy_%: phy/u5_phy.c phy/hal.h $(BUILDDIR)/phy_code.inc $(BUILDDIR)/phy_constants.inc $(CHIBIOS)/os/xhal/src/hal_safety.c
	$(CC) -Iphy $(CPPFLAGS) -I$(CHIBIOS)/os/common/ext/ST/STM32U5xx \
	  -DTEST_DEVICE_HEADER='"stm32$*xx.h"' $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES) $(PHY_BINARIES) $(BUILDDIR)/hld_fault.inc $(BUILDDIR)/usb_constants.inc $(BUILDDIR)/phy_code.inc $(BUILDDIR)/phy_constants.inc
