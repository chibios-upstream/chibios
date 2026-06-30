# List of the ChibiOS/RT ARMv7-R simulator port files.

# Generic ARMv7-R port.
include $(CHIBIOS)/os/common/ports/ARMv7-R/compilers/GCC/mk/port.mk

# Simulator interrupt controller support.
include $(CHIBIOS)/os/common/ports/ARM-common/GICv2/gicv2.mk

DDEFS += -DARMCR8_HAS_GIC=1                                   \
         -DPORT_HAS_PLATFORM=TRUE                             \
         -DGICV2_INTERFACE_BASE=0xAE000100U                    \
         -DGICV2_DISTRIBUTOR_BASE=0xAE001000U

PORTSIMSRC = $(CHIBIOS)/os/common/ports/ARMv7-R/platforms/simulator/port_platform.c

PORTSIMINC = $(CHIBIOS)/os/common/ports/ARMv7-R/platforms/simulator

# Shared variables.
ALLCSRC += $(PORTSIMSRC)
ALLINC  += $(PORTSIMINC)
