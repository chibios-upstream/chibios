ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_I2C TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/RP/LLD/I2Cv1/hal_i2c_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/RP/LLD/I2Cv1/hal_i2c_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/RP/LLD/I2Cv1
