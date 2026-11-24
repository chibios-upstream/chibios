# List of all the GPIO quadrature encoder files.
GPIOQENCODERSRC := $(CHIBIOS)/os/hal/lib/complex/gpio_qencoder/hal_gpio_qencoder.c

# Required include directories
GPIOQENCODERINC := $(CHIBIOS)/os/hal/lib/complex/gpio_qencoder

# Shared variables
ALLCSRC += $(GPIOQENCODERSRC)
ALLINC  += $(GPIOQENCODERINC)
