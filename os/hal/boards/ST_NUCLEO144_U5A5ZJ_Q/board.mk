# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/ST_NUCLEO144_U5A5ZJ_Q/board.c

# Required include directories
BOARDINC = $(CHIBIOS)/os/hal/boards/ST_NUCLEO144_U5A5ZJ_Q

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
