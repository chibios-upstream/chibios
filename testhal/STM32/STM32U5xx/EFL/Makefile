# Standalone HAL EFL qualification on a custom STM32U5F7 (4 MiB) board.
# No UART, external oscillator or application-board GPIO configuration.
PROJECT = efl_u5
CHIBIOS := ../../../..
CONFDIR := ./cfg
BUILDDIR ?= ./build
DEPDIR ?= ./.dep
MCU = cortex-m33
USE_OPT ?= -O2 -ggdb -fomit-frame-pointer -falign-functions=16
USE_COPT ?=
USE_CPPOPT ?= -fno-rtti
USE_LINK_GC ?= yes
USE_LTO ?= yes
USE_SMART_BUILD ?= yes
USE_VERBOSE_COMPILE ?= no
USE_PROCESS_STACKSIZE ?= 0x800
USE_EXCEPTIONS_STACKSIZE ?= 0x800
USE_FPU = no
include $(CHIBIOS)/os/license/license.mk
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32u5xx.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32U5xx/platform_type2.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv8-M-ML-ALT/compilers/GCC/mk/port.mk
LDSCRIPT = $(CONFDIR)/efl_test.ld
CSRC = $(ALLCSRC) $(CONFDIR)/board.c main.c
CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)
INCDIR = $(CONFDIR) $(ALLINC)
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Wcast-align=strict
CPPWARN = -Wall -Wextra -Wundef
UDEFS = -DSTM32U5F7xx
UADEFS = -DSTM32U5F7xx
RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk
