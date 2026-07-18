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

CSRC = $(ALLCSRC) $(TESTSRC) main.c
CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)
INCDIR = $(CONFDIR) $(ALLINC) $(TESTINC)

UDEFS = -DSIMULATOR -DTEST_CFG_DELAY_BETWEEN_TESTS=0 \
        -DTEST_CFG_SIZE_REPORT=FALSE
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

.PHONY: test

test: all
	./$(BUILDDIR)/$(PROJECT)

##############################################################################
