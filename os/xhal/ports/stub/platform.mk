# Stub platform, no drivers.

# Required platform files.
PLATFORMSRC := $(CHIBIOS)/os/xhal/ports/stub/hal_lld.c \
               $(CHIBIOS)/os/xhal/ports/stub/hal_st_lld.c

# Required include directories.
PLATFORMINC := $(CHIBIOS)/os/xhal/ports/stub

# Shared variables
ALLCSRC += $(PLATFORMSRC)
ALLINC  += $(PLATFORMINC)
