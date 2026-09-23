CHIBIOS := ../../../..
LLD := $(CHIBIOS)/os/xhal/ports/STM32/LLD/FDCANv2
BUILDDIR := ../../build/host/fdcanv2
CPPFLAGS := -I. -I$(LLD) -I$(CHIBIOS)/os/xhal/include \
            -I$(CHIBIOS)/os/xhal/src \
            -I$(CHIBIOS)/os/xhal/ports/STM32/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ST/STM32H7xx \
            -I$(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -fsanitize=address,undefined
TESTS := can1 can2 can3 can12 can13 can23 can123 configurations nosync h743 h7a3
BINARIES := $(addprefix $(BUILDDIR)/,$(TESTS))
.PHONY: all clean
all: $(BINARIES)
	@for test in $(BINARIES); do $$test || exit; done

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/can1: CPPFLAGS += -DSTM32_CAN_USE_FDCAN2=0 -DSTM32_CAN_USE_FDCAN3=0
$(BUILDDIR)/can2: CPPFLAGS += -DSTM32_CAN_USE_FDCAN1=0 -DSTM32_CAN_USE_FDCAN3=0
$(BUILDDIR)/can3: CPPFLAGS += -DSTM32_CAN_USE_FDCAN1=0 -DSTM32_CAN_USE_FDCAN2=0
$(BUILDDIR)/can12: CPPFLAGS += -DSTM32_CAN_USE_FDCAN3=0
$(BUILDDIR)/can13: CPPFLAGS += -DSTM32_CAN_USE_FDCAN2=0
$(BUILDDIR)/can23: CPPFLAGS += -DSTM32_CAN_USE_FDCAN1=0
$(BUILDDIR)/configurations: CPPFLAGS += -DCAN_USE_CONFIGURATIONS=1
$(BUILDDIR)/nosync: CPPFLAGS += -DCAN_USE_SYNCHRONIZATION=0
$(BUILDDIR)/h743: CPPFLAGS += -DTEST_H743
$(BUILDDIR)/h7a3: CPPFLAGS += -DTEST_H7A3

$(BINARIES): fdcanv2.c hal.h $(wildcard $(LLD)/*) \
             $(CHIBIOS)/os/xhal/include/hal_can.h \
             $(CHIBIOS)/os/xhal/include/hal_cb_driver.h \
             $(CHIBIOS)/os/xhal/src/hal_can.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(BINARIES)
