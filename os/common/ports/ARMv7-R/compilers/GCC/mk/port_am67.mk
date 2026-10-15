# List of the ChibiOS/RT ARMv7-R TI AM67 port files.

# Generic ARMv7-R port.
include $(CHIBIOS)/os/common/ports/ARMv7-R/compilers/GCC/mk/port.mk

DDEFS += -DPORT_HAS_PLATFORM=TRUE

PORTAM67SRC = $(CHIBIOS)/os/common/ports/ARMv7-R/platforms/am67/port_platform.c

PORTAM67INC = $(CHIBIOS)/os/common/ports/ARMv7-R/platforms/am67

# Shared variables.
ALLCSRC += $(PORTAM67SRC)
ALLINC  += $(PORTAM67INC)
