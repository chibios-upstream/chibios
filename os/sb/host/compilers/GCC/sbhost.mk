# The sandbox ELF loader is a wrapper over the generic ELF32 loader.
include $(CHIBIOS)/os/elf/elf32/elf32.mk

# List of the ChibiOS ARMv7-M sandbox host files.
SBHOSTSRC = $(CHIBIOS)/os/sb/host/sbhost.c \
			$(CHIBIOS)/os/sb/host/sbregions.c \
			$(CHIBIOS)/os/sb/host/sbsyscall.c \
			$(CHIBIOS)/os/sb/host/sbapi.c \
			$(CHIBIOS)/os/sb/host/sbvrq.c \
			$(CHIBIOS)/os/sb/host/sbelf.c \
			$(CHIBIOS)/os/sb/host/sbposix.c
          
SBHOSTASM = $(CHIBIOS)/os/sb/host/compilers/GCC/sbexc.S

SBHOSTINC = $(CHIBIOS)/os/common/posix/include \
            $(CHIBIOS)/os/sb/common \
            $(CHIBIOS)/os/sb/host

# Shared variables
ALLXASMSRC += $(SBHOSTASM)
ALLCSRC    += $(SBHOSTSRC)
ALLINC     += $(SBHOSTINC)
