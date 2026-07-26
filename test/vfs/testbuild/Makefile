##############################################################################
# VFS simulator tests.

SIM_BITS ?= 64
ifeq ($(SIM_BITS),32)
  SIM_PORT = SIMIA32
else
  SIM_PORT = SIMX86_64
endif

USE_OPT              ?= -O2 -ggdb -m$(SIM_BITS)
USE_COPT             ?=
USE_CPPOPT           ?= -fno-rtti
USE_LINK_GC          ?= yes
USE_LDOPT            ?= --defsym=__main_thread_stack_base__=0,--defsym=__main_thread_stack_end__=0
USE_LTO              ?= no
USE_VERBOSE_COMPILE  ?= no
USE_SMART_BUILD      ?= yes

PROJECT  = ch
CHIBIOS  = ../../..
CONFDIR  = ./cfg
BUILDDIR = ./build
DEPDIR   = ./.dep

OOPSELECT := base referenced

include $(CHIBIOS)/os/license/license.mk
include $(CHIBIOS)/os/common/oop/oop.mk
include $(CHIBIOS)/os/common/utils/utils.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/boards/simulator/board.mk
include $(CHIBIOS)/os/hal/ports/simulator/posix/platform.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/$(SIM_PORT)/compilers/GCC/port.mk
include $(CHIBIOS)/os/vfs/vfs.mk
include $(CHIBIOS)/os/test/test.mk
include $(CHIBIOS)/test/vfs/vfs_test.mk
ifeq ($(VFS_TEST_FATFS),yes)
FATFS_TEST_DIR ?= $(CHIBIOS)/ext/fatfs/source
ALLCSRC += $(FATFS_TEST_DIR)/ff.c
ALLINC  += $(FATFS_TEST_DIR)
endif
ifeq ($(VFS_TEST_LITTLEFS),yes)
include $(CHIBIOS)/os/various/littlefs_bindings/littlefs.mk
endif

CSRC = $(ALLCSRC) $(TESTSRC) main.c
CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)
INCDIR = $(CONFDIR) $(ALLINC) $(TESTINC)

UDEFS = -DSIMULATOR -DTEST_CFG_DELAY_BETWEEN_TESTS=0 \
        -DTEST_CFG_SIZE_REPORT=FALSE \
        -DDRV_CFG_ROM_ENABLE_COMPRESSION=TRUE
ifeq ($(VFS_TEST_FATFS),yes)
UDEFS += -DVFS_CFG_ENABLE_DRV_FATFS=TRUE
endif
ifeq ($(VFS_TEST_LITTLEFS),yes)
UDEFS += -DHAL_USE_EFL=TRUE -DVFS_CFG_ENABLE_DRV_LITTLEFS=TRUE \
         -DVFS_CFG_PATHLEN_MAX=255 -DDRV_CFG_LITTLEFS_DIR_NODES_NUM=2
endif
UADEFS =
UINCDIR =
ULIBDIR =
ULIBS =

TRGT =
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
LD   = $(TRGT)gcc
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary
COV  = gcov

CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes
CPPWARN = -Wall -Wextra -Wundef

RULESPATH = $(CHIBIOS)/os/common/startup/$(SIM_PORT)/compilers/GCC
include $(RULESPATH)/rules.mk

.PHONY: test test-fatfs clean-fatfs test-littlefs clean-littlefs

test: all
	./$(BUILDDIR)/$(PROJECT)

test-fatfs:
	$(MAKE) VFS_TEST_FATFS=yes USE_SMART_BUILD=no BUILDDIR=./build/fatfs \
	        DEPDIR=./.dep-fatfs test

clean-fatfs:
	$(MAKE) VFS_TEST_FATFS=yes USE_SMART_BUILD=no BUILDDIR=./build/fatfs \
	        DEPDIR=./.dep-fatfs clean

test-littlefs:
	$(MAKE) VFS_TEST_LITTLEFS=yes USE_SMART_BUILD=no BUILDDIR=./build/littlefs \
	        DEPDIR=./.dep-littlefs test

clean-littlefs:
	$(MAKE) VFS_TEST_LITTLEFS=yes USE_SMART_BUILD=no BUILDDIR=./build/littlefs \
	        DEPDIR=./.dep-littlefs clean

##############################################################################
