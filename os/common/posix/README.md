# Shared POSIX definitions

The Apache-2.0 headers in `include/` define the terminal ABI shared by XHAL
and sandbox hosts and applications. They retain the existing structure
layouts, flag values and request numbers; moving them here does not change
the sandbox ABI. The headers provide declarations, not implementations of
the POSIX library functions.

This is an opt-in include directory for embedded builds. The sandbox and
POSIX TTY make fragments add it explicitly. Do not add it to native Linux
application builds: those must use their system's termios and ioctl headers.
