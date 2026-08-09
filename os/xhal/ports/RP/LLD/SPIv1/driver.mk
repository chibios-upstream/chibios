ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_SPI TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/RP/LLD/SPIv1/hal_spi_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/xhal/ports/RP/LLD/SPIv1/hal_spi_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/xhal/ports/RP/LLD/SPIv1
