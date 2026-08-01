# List of files for HTTPD-to-POSIX bindings.
HTTPDPOSIXSRC = $(CHIBIOS)/os/xhal/lib/complex/httpd_posix_bindings/httpd_posix.c

HTTPDPOSIXINC = $(CHIBIOS)/os/xhal/lib/complex/httpd_posix_bindings

# Shared variables
ALLCSRC += $(HTTPDPOSIXSRC)
ALLINC  += $(HTTPDPOSIXINC)
