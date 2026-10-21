# List of the ChibiOS sandbox VSRV files.
SBVSRVSRC = $(CHIBIOS)/os/sb/vsrv/sbvsrv.c

SBVSRVASM =

SBVSRVINC = $(CHIBIOS)/os/sb/vsrv

# Shared variables
ALLXASMSRC += $(SBVSRVASM)
ALLCSRC    += $(SBVSRVSRC)
ALLINC     += $(SBVSRVINC)
