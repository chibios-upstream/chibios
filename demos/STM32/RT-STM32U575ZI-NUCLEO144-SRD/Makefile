##############################################################################
# Build global options
#

USE_OPT = -O2 -ggdb -fomit-frame-pointer --specs=nano.specs \
          -falign-functions=16
USE_LINK_GC = yes
USE_LTO = yes
USE_SMART_BUILD = yes

##############################################################################
# Architecture or project specific options
#

USE_PROCESS_STACKSIZE = 0x400
USE_EXCEPTIONS_STACKSIZE = 0x400
USE_FPU = no
USE_FPU_OPT = -mfloat-abi=$(USE_FPU) -mfpu=fpv5-sp-d16

##############################################################################
# Project, target, sources and paths
#

PROJECT = ch_srd
MCU = cortex-m33

CHIBIOS := ../../..
CONFDIR := ./cfg
BUILDDIR := ./build
DEPDIR := ./.dep

include $(CHIBIOS)/os/license/license.mk
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32u5xx.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32U5xx/platform.mk
include $(CHIBIOS)/os/hal/boards/ST_NUCLEO144_U575ZI_Q/board.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv8-M-ML-ALT/compilers/GCC/mk/port.mk
include $(CHIBIOS)/os/hal/lib/streams/streams.mk

LDSCRIPT = $(STARTUPLD)/STM32U575xI.ld

CSRC = $(ALLCSRC) \
       $(CONFDIR)/portab.c \
       main.c
CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)
INCDIR = $(CONFDIR) $(ALLINC)

CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Wcast-align=strict
CPPWARN = -Wall -Wextra -Wundef

UDEFS = -DSTM32U575xx
UADEFS = -DSTM32U575xx
UINCDIR =
ULIBDIR =
ULIBS =

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk

##############################################################################
