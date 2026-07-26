##############################################################################
# Common native 32-bit POSIX command build.
#

ifeq ($(PROJECT),)
  $(error PROJECT must be defined before including app-posix-x86.mk)
endif

ifeq ($(CHIBIOS),)
  $(error CHIBIOS must be defined before including app-posix-x86.mk)
endif

CONFDIR  ?= .
BUILDDIR ?= ./build/$(PROJECT)-posix-x86
DEPDIR   ?= ./.dep/$(PROJECT)-posix-x86

SBAPP_OPT      ?= -Og -ggdb -m32
SBAPP_CSRC     ?= main.c
SBAPP_CPPSRC   ?=
SBAPP_ASMSRC   ?=
SBAPP_ASMXSRC  ?=
SBAPP_UDEFS    ?=
SBAPP_UADEFS   ?=
SBAPP_UINCDIR  ?=
SBAPP_ULIBDIR  ?=
SBAPP_ULIBS    ?=
SBAPP_TESTS    ?=

# Build global options, all externally overridable.
ifeq ($(USE_OPT),)
  USE_OPT = $(SBAPP_OPT)
endif
ifeq ($(USE_COPT),)
  USE_COPT =
endif
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif
ifeq ($(USE_LDOPT),)
  USE_LDOPT = --defsym=__main_thread_stack_base__=0,--defsym=__main_thread_stack_end__=0
endif
ifeq ($(USE_LTO),)
  USE_LTO = no
endif
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

include $(CHIBIOS)/tools/mk/autobuild.mk

# Project sources and paths.
CSRC    = $(ALLCSRC) $(TESTSRC) $(SBAPP_CSRC)
CPPSRC  = $(ALLCPPSRC) $(SBAPP_CPPSRC)
ASMSRC  = $(ALLASMSRC) $(SBAPP_ASMSRC)
ASMXSRC = $(ALLXASMSRC) $(SBAPP_ASMXSRC)
INCDIR  = $(CONFDIR) $(ALLINC) $(TESTINC)

UDEFS   = $(SBAPP_UDEFS)
UADEFS  = $(SBAPP_UADEFS)
UINCDIR = $(SBAPP_UINCDIR)
ULIBDIR = $(SBAPP_ULIBDIR)
ULIBS   = $(SBAPP_ULIBS)

# Compiler settings.
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

CWARN   = -Wall -Wextra -Wundef -Wstrict-prototypes
CPPWARN = -Wall -Wextra -Wundef

RULESPATH = $(CHIBIOS)/os/common/startup/SIMIA32/compilers/GCC
include $(RULESPATH)/rules.mk

.PHONY: check

check: all
	@for t in $(SBAPP_TESTS); do \
		sh $$t "$(BUILDDIR)/$(PROJECT)" || exit $$?; \
	done

#
##############################################################################
