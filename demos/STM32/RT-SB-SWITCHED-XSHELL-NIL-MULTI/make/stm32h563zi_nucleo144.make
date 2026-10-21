.DEFAULT_GOAL := all

##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -Og -ggdb -fomit-frame-pointer -falign-functions=16 --specs=nano.specs
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT =
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data.
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif

# Enable this if you want link time optimizations (LTO).
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x800
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

# Enables the use of FPU (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = no
endif

# FPU-related options.
ifeq ($(USE_FPU_OPT),)
  USE_FPU_OPT = -mfloat-abi=$(USE_FPU) -mfpu=fpv5-sp-d16
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, target, sources and paths
#

# Define project name here
PROJECT = ch

# Target settings.
MCU  = cortex-m33

# Imported source files and paths.
CHIBIOS  := ../../..
CONFDIR  := ./cfg/stm32h563zi_nucleo144
BUILDDIR := ./build/stm32h563zi_nucleo144
DEPDIR   := ./.dep/stm32h563zi_nucleo144

# Required modules.
OOPSELECT := base referenced chprintf nullstreams
UTILSSELECT :=

# Licensing files.
include $(CHIBIOS)/os/license/license.mk
# Startup files.
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32h5xx.mk
# Common files.
include $(CHIBIOS)/os/common/oop/oop.mk
include $(CHIBIOS)/os/common/utils/utils.mk
# XHAL files.
include $(CHIBIOS)/os/xhal/xhal.mk
include $(CHIBIOS)/os/xhal/ports/STM32/STM32H5xx/platform.mk
include $(CHIBIOS)/os/hal/boards/ST_NUCLEO144_H563ZI/board.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv8-M-ML-ALT/compilers/GCC/mk/port.mk
# SB files (optional).
include $(CHIBIOS)/os/sb/host/compilers/GCC/sbhost.mk
include $(CHIBIOS)/os/sb/vio/sbvio.mk
include $(CHIBIOS)/os/sb/vsrv/sbvsrv.mk
# VFS files (optional).
include $(CHIBIOS)/os/vfs/vfs.mk
# Auto-build files in ./source recursively.
include $(CHIBIOS)/tools/mk/autobuild.mk

# Define linker script file here
LDSCRIPT = ./ld/stm32h563zi_nucleo144.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(ALLCSRC) \
       $(CONFDIR)/portab.c \
       main.c

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = $(ALLCPPSRC)

# List ASM source files here.
ASMSRC = $(ALLASMSRC)

# List ASM with preprocessor source files here.
ASMXSRC = $(ALLXASMSRC)

# Inclusion directories.
INCDIR = $(CONFDIR) $(ALLINC)

# Define C warning options here.
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Wcast-align=strict

# Define C++ warning options here.
CPPWARN = -Wall -Wextra -Wundef

#
# Project, target, sources and paths
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = -DOOP_USE_LEGACY

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user section
##############################################################################

##############################################################################
# Common rules
#

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk

#
# Common rules
##############################################################################

##############################################################################
# Custom rules
#

# Sandbox images matching the flash and RAM slots of this target, they are
# built by their own projects as part of the default goal.
SB1_PROJECT := $(CHIBIOS)/demos/various/SB-CLIENT-RT-XHAL-XSHELL
SB1_MAKE    := make/128k-081C0000-16k-20000000.make
SB2_PROJECT := $(CHIBIOS)/demos/various/SB-CLIENT-NIL-XHAL
SB2_MAKE    := make/128k-081E0000-16k-20004000.make

# The sandbox projects use their own directories, also when the host ones
# are overridden on the command line.
SB_DIRS = CONFDIR=./cfg BUILDDIR=./build/$(basename $(notdir $(1)))        \
          DEPDIR=./.dep/$(basename $(notdir $(1)))

.PHONY: sandboxes
sandboxes:
	+$(MAKE) --no-print-directory -C $(SB1_PROJECT) -f $(SB1_MAKE)         \
	         $(call SB_DIRS,$(SB1_MAKE)) all
	+$(MAKE) --no-print-directory -C $(SB2_PROJECT) -f $(SB2_MAKE)         \
	         $(call SB_DIRS,$(SB2_MAKE)) all

all: sandboxes

#
# Custom rules
##############################################################################
