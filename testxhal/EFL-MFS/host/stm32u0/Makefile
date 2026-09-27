# Actual EFL/flash/base HLDs and U0 LLD, with a register/RTOS model.
CHIBIOS := ../../../..
XHAL := $(CHIBIOS)/os/xhal
OOP := $(CHIBIOS)/os/common/oop
LLD := $(XHAL)/ports/STM32/STM32U0xx
BUILDDIR := ../../build/host/stm32u0
CPPFLAGS := -I. -I$(LLD) -I$(XHAL)/include -I$(OOP)/include \
            -I$(CHIBIOS)/os/common/portability/GCC \
            -I$(CHIBIOS)/os/common/ext/ST/STM32U0xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include \
            -DOOP_USE_NOTHING -include ccportab.h
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -finstrument-functions -fsanitize=address,undefined
SOURCES := efl_u0.c $(XHAL)/src/hal_efl.c $(XHAL)/src/hal_flash_base.c \
           $(XHAL)/src/hal_base_driver.c $(OOP)/src/oop_base_object.c
HEADERS := hal.h $(LLD)/hal_efl_lld.c $(LLD)/hal_efl_lld.h \
           $(XHAL)/include/hal_efl.h $(XHAL)/include/hal_flash_base.h \
           $(XHAL)/include/hal_flash_interface.h $(XHAL)/include/hal_base_driver.h
BINARIES := $(addprefix $(BUILDDIR)/,u031 u073 u083)

.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/u031: CPPFLAGS += -DSTM32U031xx
$(BUILDDIR)/u073: CPPFLAGS += -DSTM32U073xx
$(BUILDDIR)/u083: CPPFLAGS += -DSTM32U083xx

$(BINARIES): $(SOURCES) $(HEADERS) | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

clean:
	$(RM) $(BINARIES)
