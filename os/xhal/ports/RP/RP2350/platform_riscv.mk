# Required platform files.
PLATFORMSRC := $(CHIBIOS)/os/xhal/ports/common/RISCV-HAZARD3/nvic.c \
               $(CHIBIOS)/os/xhal/ports/common/RISCV-HAZARD3/hal_st_lld.c \
               $(CHIBIOS)/os/xhal/ports/RP/rp_bootrom.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_clocks.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_isr.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_pll.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/rp_xosc.c \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350/hal_lld.c

# Required include directories.
# Note, include resolution relies on the ARM common directory being absent
# from this recipe, the build system sorts include paths so ordering alone
# would not shadow nvic.h/cache.h.
PLATFORMINC := $(CHIBIOS)/os/xhal/ports/common/RISCV-HAZARD3 \
               $(CHIBIOS)/os/xhal/ports/RP \
               $(CHIBIOS)/os/xhal/ports/RP/RP2350 \
               $(CHIBIOS)/os/common/ports/RISCV-HAZARD3

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
include $(CHIBIOS)/os/xhal/ports/RP/LLD/ADCv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/DMAv1/driver.mk
# EFLv1 is absent, the RP2350 XIP safety hooks park the other core using
# the SMP port lockout services which the Hazard3 port does not provide.
include $(CHIBIOS)/os/xhal/ports/RP/LLD/GPIOv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/PWMv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/SPIv1/driver.mk
# The Hazard3 port owns its core-local MTIME/MTIMECMP system timer.
include $(CHIBIOS)/os/xhal/ports/RP/LLD/UARTv1/driver.mk
include $(CHIBIOS)/os/xhal/ports/RP/LLD/WDGv1/driver.mk

# Shared variables
ALLCSRC += $(PLATFORMSRC)
ALLINC  += $(PLATFORMINC)
