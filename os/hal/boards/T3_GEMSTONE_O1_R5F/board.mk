# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/T3_GEMSTONE_O1_R5F/board.c

# Required include directories.
BOARDINC = $(CHIBIOS)/os/hal/boards/T3_GEMSTONE_O1_R5F

# Shared variables.
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
