# List of files for HTTPD-to-VFS bindings.
HTTPDVFSSRC = $(CHIBIOS)/os/xhal/lib/complex/httpd_vfs_bindings/httpd_vfs.c

HTTPDVFSINC = $(CHIBIOS)/os/xhal/lib/complex/httpd_vfs_bindings

# Shared variables
ALLCSRC += $(HTTPDVFSSRC)
ALLINC  += $(HTTPDVFSINC)
