ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_CAN TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/CANv1/hal_can_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/CANv1/hal_can_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/CAN \
               $(CHIBIOS)/os/xhal/ports/STM32/LLD/CANv1
