# LittleFS files.
LITTLEFSSRC = $(CHIBIOS)/os/various/littlefs_bindings/lfs_hal.c \
              $(CHIBIOS)/ext/littlefs/lfs.c \
              $(CHIBIOS)/ext/littlefs/lfs_util.c

LITTLEFSINC = $(CHIBIOS)/os/various/littlefs_bindings \
              $(CHIBIOS)/ext/littlefs

# VFS supplies optional per-instance serialization. Define LFS_THREADSAFE in
# the application only when native lock hooks are also needed.
DDEFS      += -DLFS_NO_DEBUG=0 -DLFS_CONFIG=lfs_config.h

# Shared variables
ALLCSRC += $(LITTLEFSSRC)
ALLINC  += $(LITTLEFSINC)
