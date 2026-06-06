# List of all the LIS302DL chip files.
LIS302DLSRC := $(CHIBIOS)/os/xhal/lib/complex/sensors/devices/ST/lis302dl.c

# Shared sensor interfaces.
include $(CHIBIOS)/os/xhal/lib/complex/sensors/hal_sensors.mk

# Required include directories.
LIS302DLINC := $(CHIBIOS)/os/common/oop/include \
               $(CHIBIOS)/os/xhal/include \
               $(CHIBIOS)/os/xhal/lib/complex/sensors/devices/ST

# Shared variables.
ALLCSRC += $(LIS302DLSRC)
ALLINC  += $(LIS302DLINC)
