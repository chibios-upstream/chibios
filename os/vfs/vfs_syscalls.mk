# List of all the ChibiOS/VFS syscall files.
VFSSYSSRC := $(CHIBIOS)/os/various/newlib_bindings/syscalls.c \

# Required include directories
VFSSYSINC := $(CHIBIOS)/os/various/newlib_bindings

# Shared variables
ALLCSRC += $(VFSSYSSRC)
ALLINC  += $(VFSSYSINC)
