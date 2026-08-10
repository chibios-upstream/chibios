ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_EFL TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/RP/LLD/EFLv1/rp_efl_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/RP/LLD/EFLv1/rp_efl_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/RP/LLD/EFLv1
