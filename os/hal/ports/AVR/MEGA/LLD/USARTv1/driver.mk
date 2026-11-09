ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_SERIAL TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AVR/MEGA/LLD/USARTv1/hal_serial_lld.c
endif
ifneq ($(findstring HAL_USE_SIO TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AVR/MEGA/LLD/USARTv1/hal_sio_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AVR/MEGA/LLD/USARTv1/hal_serial_lld.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AVR/MEGA/LLD/USARTv1/hal_sio_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/hal/ports/AVR/MEGA/LLD/USARTv1
