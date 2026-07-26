##############################################################################
# Common ARMv7-M sandbox application build.
#

ifeq ($(PROJECT),)
  $(error PROJECT must be defined before including app-arm.mk)
endif

ifeq ($(CHIBIOS),)
  $(error CHIBIOS must be defined before including app-arm.mk)
endif

CONFDIR  ?= .
BUILDDIR ?= ./build/$(PROJECT)
DEPDIR   ?= ./.dep/$(PROJECT)

SBAPP_OPT                  ?= -Os -fomit-frame-pointer --specs=nano.specs
SBAPP_LDOPT                ?= -q -Wl,-zmax-page-size=512,--no-warn-rwx-segment
SBAPP_PROCESS_STACKSIZE    ?= 0x400
SBAPP_EXCEPTIONS_STACKSIZE ?= 0
SBAPP_CSRC                 ?= main.c
SBAPP_CPPSRC               ?=
SBAPP_ASMSRC               ?=
SBAPP_ASMXSRC              ?=
SBAPP_UDEFS                ?=
SBAPP_UADEFS               ?= -DCRT0_INIT_DATA=0
SBAPP_UINCDIR              ?=
SBAPP_ULIBDIR              ?=
SBAPP_ULIBS                ?=

# Build global options, all externally overridable.
ifeq ($(USE_OPT),)
  USE_OPT = $(SBAPP_OPT)
endif
ifeq ($(USE_COPT),)
  USE_COPT =
endif
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = no
endif
ifeq ($(USE_LDOPT),)
  USE_LDOPT = $(SBAPP_LDOPT)
endif
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

# Architecture options.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = $(SBAPP_PROCESS_STACKSIZE)
endif
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = $(SBAPP_EXCEPTIONS_STACKSIZE)
endif
ifeq ($(USE_FPU),)
  USE_FPU = no
endif
ifeq ($(USE_FPU_OPT),)
  USE_FPU_OPT = -mfloat-abi=$(USE_FPU) -mfpu=fpv4-sp-d16
endif

MCU = cortex-m4

# Imported modules.
include $(CHIBIOS)/os/common/startup/ARMCMx-SB/compilers/GCC/mk/startup.mk
include $(CHIBIOS)/os/common/utils/utils.mk
include $(CHIBIOS)/os/sb/user/sbuser.mk
include $(CHIBIOS)/tools/mk/autobuild.mk

SBAPP_LDSCRIPT ?= $(STARTUPLD)/ram_sandbox.ld
LDSCRIPT = $(SBAPP_LDSCRIPT)

# Project sources and paths.
CSRC    = $(ALLCSRC) $(TESTSRC) $(SBAPP_CSRC)
CPPSRC  = $(ALLCPPSRC) $(SBAPP_CPPSRC)
ASMSRC  = $(ALLASMSRC) $(SBAPP_ASMSRC)
ASMXSRC = $(ALLXASMSRC) $(SBAPP_ASMXSRC)
INCDIR  = $(CONFDIR) $(ALLINC) $(TESTINC)

CWARN   = -Wall -Wextra -Wundef -Wstrict-prototypes
CPPWARN = -Wall -Wextra -Wundef

# Application-provided settings.
UDEFS   = $(SBAPP_UDEFS)
UADEFS  = $(SBAPP_UADEFS)
UINCDIR = $(SBAPP_UINCDIR)
ULIBDIR = $(SBAPP_ULIBDIR)
ULIBS   = $(SBAPP_ULIBS)

# Common rules.
RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk

.PHONY: read

read:
	@echo "Reading elf..."
	@$(TRGT)readelf -atSlnr $(BUILDDIR)/$(PROJECT).elf > $(BUILDDIR)/$(PROJECT).read

#
##############################################################################
