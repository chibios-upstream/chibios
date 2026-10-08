# Actual classic HAL USB driver, OTGv1 LLD and safety module on a host.
CHIBIOS ?= ../../../../../..
LLD := $(CHIBIOS)/os/hal/ports/STM32/LLD/OTGv1
BUILDDIR ?= build
CPPFLAGS := -D_DEFAULT_SOURCE -I. -I$(LLD) \
            -I$(CHIBIOS)/os/hal/include -I$(CHIBIOS)/os/hal/src \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined -fno-sanitize-recover=undefined
TESTS := dual otg1 otg2 wait ep0_thread stepping1 stepping3 novbus
TESTS += stepping1_novbus ulpi_fs ulpi_hs basepri
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
U5_HS := u595 u599 u5a5 u5a9 u5f7 u5f9 u5g7 u5g9
PHY_BINARIES := $(addprefix $(BUILDDIR)/phy_,$(U5_HS))
U5 := $(CHIBIOS)/os/hal/ports/STM32/STM32U5xx

.PHONY: all clean negative
all: $(BINARIES) $(PHY_BINARIES)
	@for test in $(BINARIES) $(PHY_BINARIES); do echo "== $$test"; $$test || exit $$?; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/otg1: CPPFLAGS += -DSTM32_USB_USE_OTG2=0
$(BUILDDIR)/otg2: CPPFLAGS += -DSTM32_USB_USE_OTG1=0
$(BUILDDIR)/wait: CPPFLAGS += -DUSB_USE_WAIT=1
$(BUILDDIR)/ep0_thread: CPPFLAGS += -DUSB_USE_EP0_THREAD=1 -DUSB_USE_WAIT=1
$(BUILDDIR)/stepping1 $(BUILDDIR)/stepping1_novbus: CPPFLAGS += -DSTM32_OTG_STEPPING=1
$(BUILDDIR)/stepping3: CPPFLAGS += -DSTM32_OTG_STEPPING=3
$(BUILDDIR)/novbus $(BUILDDIR)/stepping1_novbus: CPPFLAGS += -DBOARD_OTG_NOVBUSSENS
$(BUILDDIR)/ulpi_fs: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=0
$(BUILDDIR)/ulpi_hs: CPPFLAGS += -DSTM32_USB_OTG2_PHY=2 -DSTM32_USE_USB_OTG2_HS=1 -DUSB_USE_WAIT=1
$(BUILDDIR)/basepri: CPPFLAGS += -DSTM32_USB_OTGFIFO_FILL_BASEPRI=6 -DSTM32_OTG_SEQUENCE_WORKAROUND

$(BINARIES): otg_hld.c hal.h $(LLD)/hal_usb_lld.c $(LLD)/hal_usb_lld.h \
             $(LLD)/stm32_otg.h $(CHIBIOS)/os/hal/include/hal_usb.h \
             $(CHIBIOS)/os/hal/src/hal_usb.c $(CHIBIOS)/os/hal/src/hal_safety.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

# The U5 HS PHY helpers are compiled verbatim from the platform source.
$(BUILDDIR)/phy_code.inc: $(U5)/hal_lld.c phy/extract.awk | $(BUILDDIR)
	awk -f phy/extract.awk $< > $@

$(BUILDDIR)/phy_constants.inc: $(U5)/hal_lld.h | $(BUILDDIR)
	awk '/^#define STM32_USB_BOOSTER_STARTUP_TIME / {print; found++} END {if (found != 1) exit 1}' $< > $@

$(PHY_BINARIES): $(BUILDDIR)/phy_%: phy/u5_phy.c phy/hal.h $(BUILDDIR)/phy_code.inc \
                 $(BUILDDIR)/phy_constants.inc $(CHIBIOS)/os/hal/src/hal_safety.c
	$(CC) -Iphy -I$(BUILDDIR) -I$(CHIBIOS)/os/hal/include -I$(CHIBIOS)/os/hal/src \
	  -I$(CHIBIOS)/os/common/ext/ST/STM32U5xx \
	  -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include \
	  -DTEST_DEVICE_HEADER='"stm32$*xx.h"' $(CFLAGS) $< -o $@

# Reverts each driver fix in a temporary copy, the regression must fail.
negative:
	python3 negative_controls.py $(CHIBIOS)

clean:
	$(RM) $(BINARIES) $(PHY_BINARIES)
	$(RM) $(BUILDDIR)/phy_code.inc $(BUILDDIR)/phy_constants.inc
