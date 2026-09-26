# Preprocessor/compile checks only: no generated objects or executables.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
CFLAGS := -std=c11 -Wall -Wextra -Wundef -Werror -fsyntax-only
U5_FS := u575 u585
U5_HS := u595 u599 u5a5 u5a9 u5f7 u5f9 u5g7 u5g9
TESTS := $(U5_FS) $(U5_HS) u5_drd_model u5_absent g474 u385 g4_absent u3_absent

.PHONY: all $(TESTS) clean
all: $(TESTS)

$(U5_FS) $(U5_HS) u5_drd_model u5_absent: CPPFLAGS += -I$(STM32)/STM32U5xx
$(U5_FS): CPPFLAGS += -DTEST_CONTROLLER=1
$(U5_HS): CPPFLAGS += -DTEST_CONTROLLER=2
u575: CPPFLAGS += -DSTM32U575xx
u585: CPPFLAGS += -DSTM32U585xx
u595: CPPFLAGS += -DSTM32U595xx
u599: CPPFLAGS += -DSTM32U599xx
u5a5: CPPFLAGS += -DSTM32U5A5xx
u5a9: CPPFLAGS += -DSTM32U5A9xx
u5f7: CPPFLAGS += -DSTM32U5F7xx
u5f9: CPPFLAGS += -DSTM32U5F9xx
u5g7: CPPFLAGS += -DSTM32U5G7xx
u5g9: CPPFLAGS += -DSTM32U5G9xx
u5_drd_model: CPPFLAGS += -DTEST_U5_DRD_MODEL -DTEST_CONTROLLER=0
u5_absent: CPPFLAGS += -DSTM32U575xx
g474 g4_absent: CPPFLAGS += -I$(STM32)/STM32G4xx -DSTM32G474xx
u385 u3_absent: CPPFLAGS += -I$(STM32)/STM32U3xx -DSTM32U385xx
g474 u385: CPPFLAGS += -DTEST_CONTROLLER=0
u5_absent g4_absent u3_absent: CPPFLAGS += -DTEST_NO_USB -DTEST_CONTROLLER=-1

$(TESTS): usb_clock_usage.c
	@for hal in 0 1 2; do \
	  for selection in 0 1 2 3 4 5 6 7 8; do \
	    $(CC) $(CPPFLAGS) $(CFLAGS) -DTEST_HAL=$$hal \
	      -DTEST_SELECTION=$$selection $< || exit; \
	  done; \
	done
	@echo "$@: 27 USB clock-demand checks passed"

clean:
	@:
