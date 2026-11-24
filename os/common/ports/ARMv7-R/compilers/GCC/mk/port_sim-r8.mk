# List of the ChibiOS/RT ARMv7-R simulator port files.

# Generic ARMv7-R port.
include $(CHIBIOS)/os/common/ports/ARMv7-R/compilers/GCC/mk/port.mk

# Simulator interrupt controller support.
include $(CHIBIOS)/os/common/ports/ARM-common/GICv2/gicv2.mk

DDEFS += -DPORT_HAS_PLATFORM=TRUE                             \
         -DGICV2_INTERFACE_BASE=0xAE000100U                   \
         -DGICV2_DISTRIBUTOR_BASE=0xAE001000U

PORTSIMR8SRC = $(CHIBIOS)/os/common/ports/ARMv7-R/platforms/sim-r8/port_platform.c

PORTSIMR8INC = $(CHIBIOS)/os/common/ports/ARMv7-R/platforms/sim-r8

# Shared variables.
ALLCSRC += $(PORTSIMR8SRC)
ALLINC  += $(PORTSIMR8INC)
