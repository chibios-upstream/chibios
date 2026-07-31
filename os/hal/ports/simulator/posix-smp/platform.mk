# List of all the POSIX SMP simulator platform files.
#
# Most POSIX low-level drivers are reused initially. The local HAL source uses
# the real SMP kernel lock for post-interrupt rescheduling; remaining per-core
# state is split during the POSIX-SMP HAL phase.
PLATFORMSRC = ${CHIBIOS}/os/hal/ports/simulator/posix-smp/hal_lld.c \
              ${CHIBIOS}/os/hal/ports/simulator/posix/hal_serial_lld.c \
              ${CHIBIOS}/os/hal/ports/simulator/console.c \
              ${CHIBIOS}/os/hal/ports/simulator/hal_pal_lld.c \
              ${CHIBIOS}/os/hal/ports/simulator/hal_efl_lld.c \
              ${CHIBIOS}/os/hal/ports/simulator/hal_st_lld.c

# Required include directories.
PLATFORMINC = ${CHIBIOS}/os/hal/ports/simulator/posix-smp \
              ${CHIBIOS}/os/hal/ports/simulator/posix \
              ${CHIBIOS}/os/hal/ports/simulator

# Shared variables.
ALLCSRC += $(PLATFORMSRC)
ALLINC  += $(PLATFORMINC)
