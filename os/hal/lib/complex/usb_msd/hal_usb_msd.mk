# List of all the USB Mass Storage driver files.
USBMSDSRC := $(CHIBIOS)/os/hal/lib/complex/usb_msd/hal_usb_msd.c

# Required include directories
USBMSDINC := $(CHIBIOS)/os/hal/lib/complex/usb_msd

# Shared variables
ALLCSRC += $(USBMSDSRC)
ALLINC  += $(USBMSDINC)
