# Required platform files.
PLATFORMSRC := $(CHIBIOS)/os/xhal/ports/common/ARMCMx/nvic.c \
               $(CHIBIOS)/os/xhal/ports/RP/rp_bootrom.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_clocks.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_isr.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_pll.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_xosc.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/hal_lld.c

# Required include directories.
PLATFORMINC := $(CHIBIOS)/os/xhal/ports/common/ARMCMx \
               $(CHIBIOS)/os/xhal/ports/RP \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350

# Optional platform files.
ifeq ($(USE_SMART_BUILD),yes)

# Configuration files directory
ifeq ($(HALCONFDIR),)
  ifeq ($(CONFDIR),)
    HALCONFDIR = .
  else
    HALCONFDIR := $(CONFDIR)
  endif
endif

HALCONF := $(strip $(shell cat $(HALCONFDIR)/xhalconf.h | grep -E "\#define"))

else
endif

# Drivers compatible with the platform.
include $(CHIBIOS)/os/xhal/ports/RP/LLD/DMAv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/GPIOv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/SPIv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/TIMERv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/UARTv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/WDGv1/driver.mk

# Shared variables
ALLCSRC += $(PLATFORMSRC)
ALLINC  += $(PLATFORMINC)
