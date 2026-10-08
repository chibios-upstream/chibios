ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_RTC TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/RTCv3/hal_rtc_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/RTCv3/hal_rtc_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/RTCv3

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/RTC
