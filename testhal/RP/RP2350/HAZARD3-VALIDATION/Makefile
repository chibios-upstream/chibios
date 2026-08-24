# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=4
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
  USE_LTO = no
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

# Stack size to be allocated to the RISC-V process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x800
endif

# Stack size to the allocated to the RISC-V main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x800
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, target, sources and paths
#

# Define project name here
PROJECT = hazard3_validation

# Target settings.
# Hazard3 is RV32IMAC
MCU  = rv32imac_zba_zbb_zbs_zbkb_zcb_zcmp

# Imported source files and paths.
CHIBIOS  := ../../../..
CONFDIR  := ./cfg
BUILDDIR := ./build
DEPDIR   := ./.dep

# Licensing files.
include $(CHIBIOS)/os/license/license.mk
# Startup files.
include $(CHIBIOS)/os/common/startup/RISCV-HAZARD3/compilers/GCC/mk/startup_rp2350_riscv.mk
# HAL-OSAL files (optional).
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/ports/RP/RP2350/platform_riscv.mk
include $(CHIBIOS)/os/hal/boards/RP_PICO2_RP2350/board.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
include $(CHIBIOS)/os/hal/lib/streams/streams.mk
include $(CHIBIOS)/os/hal/ports/RP/rp_uf2_image.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/RISCV-HAZARD3/compilers/GCC/mk/port_rp2.mk
# Define linker script file here
LDSCRIPT= $(STARTUPLD)/RP2350_RISCV_FLASH.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
ifeq ($(PANIC_SATURATION),yes)
  # The regression applications are minimal; smaller stacks leave room in
  # the scratch banks for the core-private OS instances.
  USE_PROCESS_STACKSIZE = 0x5C0
  USE_EXCEPTIONS_STACKSIZE = 0x5C0
  CSRC = $(ALLCSRC) \
         ../../multi/SMP-PANIC/panic_main.c \
         ../../multi/SMP-PANIC/panic_c1_main.c
else ifeq ($(PANIC_SPINLOCK),yes)
  USE_PROCESS_STACKSIZE = 0x5C0
  USE_EXCEPTIONS_STACKSIZE = 0x5C0
  CSRC = $(ALLCSRC) \
         ../../multi/SMP-PANIC/panic_spinlock_main.c \
         ../../multi/SMP-PANIC/panic_spinlock_c1_main.c
else
  CSRC = $(ALLCSRC) \
         main.c c1_main.c
endif

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = $(ALLCPPSRC)

# List ASM source files here.
ASMSRC = $(ALLASMSRC)

# List ASM with preprocessor source files here.
ASMXSRC = $(ALLXASMSRC) validation_pmp.S

# Inclusion directories.
INCDIR = $(CONFDIR) $(ALLINC)

# Define C warning options here.
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes

# Define C++ warning options here.
CPPWARN = -Wall -Wextra -Wundef

#
# Project, target, sources and paths
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = -DCRT0_EXTRA_CORES_NUMBER=1 \
        -DPORT_ENABLE_GUARD_PAGES=TRUE \
        -DPORT_USE_GUARD_PMP_REGION=0

# Define ASM defines here
UADEFS = -DCRT0_EXTRA_CORES_NUMBER=1 \
         -DCH_DBG_ENABLE_STACK_CHECK=TRUE \
         -DPORT_ENABLE_GUARD_PAGES=TRUE \
         -DPORT_USE_GUARD_PMP_REGION=0

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

RULESPATH = $(CHIBIOS)/os/common/startup/RISCV-HAZARD3/compilers/GCC/mk
include $(RULESPATH)/riscv-none-elf.mk
include $(RULESPATH)/rules.mk

#
# Common rules
##############################################################################

##############################################################################
# Custom rules
#

# Firmware uploading via the bootloader, requires the .uf2 image built via
# rp_uf2_image.mk and therefore an installed picotool.
upload: $(BUILDDIR)/$(PROJECT).uf2
	$(PICOTOOL) load -v -x $(BUILDDIR)/$(PROJECT).uf2

#
# Custom rules
##############################################################################
