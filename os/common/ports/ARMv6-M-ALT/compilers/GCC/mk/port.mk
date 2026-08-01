# List of the ChibiOS/RT Cortex-M0 alternate port files.

# Dependencies.
include $(CHIBIOS)/os/common/portability/GCC/ccportab.mk
include $(CHIBIOS)/os/common/ports/ARM-common/arm-common.mk

PORTSRC = $(CHIBIOS)/os/common/ports/ARMv6-M-ALT/chcore.c

PORTVECTORS = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/vectors_alt.S
PORTSTDVECTORS = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/vectors.S

PORTASM = $(CHIBIOS)/os/common/ports/ARMv6-M-ALT/compilers/GCC/chcoreasm.S \
          $(PORTVECTORS)

PORTINC = $(CHIBIOS)/os/common/ports/ARMv6-M-ALT

# This port interposes on the hardware vectors using the alternate vector
# table. Startup files are included before port files in ChibiOS projects.
STARTUPASM  := $(filter-out $(PORTSTDVECTORS),$(STARTUPASM))
ALLXASMSRC  := $(filter-out $(PORTSTDVECTORS),$(ALLXASMSRC))

# Shared variables
ALLXASMSRC += $(PORTASM)
ALLCSRC    += $(PORTSRC)
ALLINC     += $(PORTINC)
