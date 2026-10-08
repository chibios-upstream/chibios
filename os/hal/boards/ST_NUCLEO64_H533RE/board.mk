# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/ST_NUCLEO64_H533RE/board.c

# Required include directories
BOARDINC = $(CHIBIOS)/os/hal/boards/ST_NUCLEO64_H533RE

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
