# List of all the board related files.
BOARDSRC = ${CHIBIOS}/os/hal/boards/ATMEGA8_MINIMAL/board.c

# Required include directories
BOARDINC = ${CHIBIOS}/os/hal/boards/ATMEGA8_MINIMAL

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
