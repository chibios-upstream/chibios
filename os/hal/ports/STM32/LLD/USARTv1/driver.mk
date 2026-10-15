ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_SERIAL TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/USARTv1/hal_serial_lld.c
endif
ifneq ($(findstring HAL_USE_UART TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/USARTv1/hal_uart_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/USARTv1/hal_serial_lld.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/USARTv1/hal_uart_lld.c
endif

PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/USART/stm32_usart_common.c

PLATFORMINC += $(CHIBIOS)/os/hal/ports/STM32/LLD/USART \
               $(CHIBIOS)/os/hal/ports/STM32/LLD/USARTv1
