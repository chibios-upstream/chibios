# List of the ChibiOS TI AM67/J722S Cortex-R5F startup and CMSIS files.
STARTUPSRC = $(CHIBIOS)/os/common/startup/ARMCRx/compilers/GCC/crt1.c

STARTUPASM = $(CHIBIOS)/os/common/startup/ARMCRx/compilers/GCC/crt0_v7r.S \
             $(CHIBIOS)/os/common/startup/ARMCRx/compilers/GCC/vectors.S

STARTUPINC = $(CHIBIOS)/os/common/portability/GCC \
             $(CHIBIOS)/os/common/startup/ARMCRx/compilers/GCC \
             $(CHIBIOS)/os/common/startup/ARMCRx/devices/AM67R5F \
             $(CHIBIOS)/os/common/ext/ARM/CMSIS6/Include

STARTUPLD  = $(CHIBIOS)/os/common/startup/ARMCRx/compilers/GCC/ld

# Shared variables
ALLXASMSRC += $(STARTUPASM)
ALLCSRC    += $(STARTUPSRC)
ALLINC     += $(STARTUPINC)
