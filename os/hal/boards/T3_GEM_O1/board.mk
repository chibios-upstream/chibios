# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/T3_GEM_O1/board.c

# Required include directories.
BOARDINC = $(CHIBIOS)/os/hal/boards/T3_GEM_O1

# Shared variables.
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
