CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/CANv1
BUILDDIR := ../../build/host/canv1
CPPFLAGS := -I. -I$(LLD) -I$(LLD)/../CAN -I$(CHIBIOS)/os/xhal/include \
            -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32/STM32L4xx+ \
            -I$(CHIBIOS)/os/common/ext/ST/STM32L4xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := l4r5 nosync nosleep nosync_nosleep configurations all_errors l4p5 l4s5
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/configurations: CPPFLAGS += -DCAN_USE_CONFIGURATIONS=1
$(BUILDDIR)/nosync: CPPFLAGS += -DCAN_USE_SYNCHRONIZATION=0
$(BUILDDIR)/nosleep: CPPFLAGS += -DCAN_USE_SLEEP_MODE=0
$(BUILDDIR)/nosync_nosleep: CPPFLAGS += -DCAN_USE_SLEEP_MODE=0 -DCAN_USE_SYNCHRONIZATION=0
$(BUILDDIR)/all_errors: CPPFLAGS += -DSTM32_CAN_REPORT_ALL_ERRORS=1
$(BUILDDIR)/l4p5: CPPFLAGS += -DTEST_L4P5
$(BUILDDIR)/l4s5: CPPFLAGS += -DTEST_L4S5

$(BINARIES): canv1.c hal.h $(wildcard $(LLD)/*) $(LLD)/../CAN/stm32_can1.inc \
             $(CHIBIOS)/os/xhal/include/hal_can.h \
             $(CHIBIOS)/os/xhal/include/hal_cb_driver.h \
             $(CHIBIOS)/os/xhal/src/hal_can.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
