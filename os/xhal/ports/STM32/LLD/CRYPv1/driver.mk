ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_CRY TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/CRYPv1/hal_crypto_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/CRYPv1/hal_crypto_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/STM32/LLD/CRYPv1
