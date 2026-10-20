# List of the ChibiOS sandbox VSRV files.
SBVSRVSRC =

SBVSRVASM =

SBVSRVINC = $(CHIBIOS)/os/sb/vsrv

# Shared variables
ALLXASMSRC += $(SBVSRVASM)
ALLCSRC    += $(SBVSRVSRC)
ALLINC     += $(SBVSRVINC)
