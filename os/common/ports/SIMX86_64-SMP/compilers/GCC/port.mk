# List of the ChibiOS/RT SIMX86_64-SMP port files.

# Dependencies.
include $(CHIBIOS)/os/common/portability/GCC/ccportab.mk

PORTSRC = ${CHIBIOS}/os/common/ports/SIMX86_64-SMP/chcore.c

PORTASM =

PORTINC = ${CHIBIOS}/os/common/ports/SIMX86_64-SMP/compilers/GCC \
          ${CHIBIOS}/os/common/ports/SIMX86_64-SMP

# The pthread option is required both while compiling and while linking.
USE_OPT += -pthread

# Host pthread stacks are managed by POSIX. These symbols let chsys.c
# instantiate the core 1 main-thread descriptor, like the core 0 placeholders.
ifeq ($(USE_LDOPT),)
  USE_LDOPT = --defsym=__c1_main_thread_stack_base__=0,--defsym=__c1_main_thread_stack_end__=0
else
  USE_LDOPT := $(USE_LDOPT),--defsym=__c1_main_thread_stack_base__=0,--defsym=__c1_main_thread_stack_end__=0
endif

# Shared variables
ALLXASMSRC += $(PORTASM)
ALLCSRC    += $(PORTSRC)
ALLINC     += $(PORTINC)
