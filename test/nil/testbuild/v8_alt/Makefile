# NIL compile/link smoke test for the alternate Cortex-M33 port.
# Uses the existing test main and a local, standalone configuration.

USE_OPT ?= -O2 -ggdb -fomit-frame-pointer -falign-functions=16
USE_COPT ?=
USE_CPPOPT ?= -fno-rtti
USE_LINK_GC ?= yes
USE_LDOPT ?=
USE_LTO ?= yes
USE_VERBOSE_COMPILE ?= no
USE_SMART_BUILD ?= yes
USE_PROCESS_STACKSIZE ?= 0x400
USE_EXCEPTIONS_STACKSIZE ?= 0x400
USE_FPU ?= hard
USE_FPU_OPT ?= -mfloat-abi=$(USE_FPU) -mfpu=fpv5-sp-d16

PROJECT = ch
MCU = cortex-m33
CHIBIOS = ../../../..
CONFDIR = ./cfg
BUILDDIR = ./build
DEPDIR = ./.dep

include $(CHIBIOS)/os/license/license.mk
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32h5xx.mk
include $(CHIBIOS)/os/nil/nil.mk
include $(CHIBIOS)/os/common/ports/ARMv8-M-ML-ALT/compilers/GCC/mk/port.mk

LDSCRIPT = $(STARTUPLD)/STM32H563xI.ld
CSRC = $(ALLCSRC) ../main.c
CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)
INCDIR = $(CONFDIR) $(ALLINC)
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Werror
CPPWARN = -Wall -Wextra -Wundef -Werror
UDEFS = -DSTM32H563xx
UADEFS = -DSTM32H563xx
UINCDIR =
ULIBDIR =
ULIBS =

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk
