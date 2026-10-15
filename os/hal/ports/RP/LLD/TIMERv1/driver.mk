ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_GPT TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/RP/LLD/TIMERv1/hal_gpt_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/RP/LLD/TIMERv1/hal_gpt_lld.c
endif

PLATFORMSRC += $(CHIBIOS)/os/hal/ports/RP/LLD/TIMERv1/hal_st_lld.c


PLATFORMINC += $(CHIBIOS)/os/hal/ports/RP/LLD/TIMERv1
