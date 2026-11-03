# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/ST_STM32H5F5J_DK/board.c

# Required include directories
BOARDINC = $(CHIBIOS)/os/hal/boards/ST_STM32H5F5J_DK

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
