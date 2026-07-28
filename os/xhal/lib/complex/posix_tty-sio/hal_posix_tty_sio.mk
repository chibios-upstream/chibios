# List of all the POSIX TTY over SIO subsystem files.
POSIXTTYSIOSRC := $(CHIBIOS)/os/xhal/lib/complex/posix_tty-sio/hal_posix_tty_sio.c

# Required include directories.
POSIXTTYSIOINC := $(CHIBIOS)/os/xhal/lib/complex/posix_tty-sio \
                  $(CHIBIOS)/os/sb/common

# Shared variables.
ALLCSRC += $(POSIXTTYSIOSRC)
ALLINC  += $(POSIXTTYSIOINC)
