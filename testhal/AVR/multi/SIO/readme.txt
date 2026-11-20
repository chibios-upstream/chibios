*****************************************************************************
** ChibiOS/RT SIO test on Atmel AVR MEGA devices.                          **
*****************************************************************************

** TARGETS **

The test is built for several targets, each one has its configuration in
cfg/<target> and its make file in make/Makefile-<target>.make, the
Makefile builds all of them:
- arduino_uno: Arduino Uno (ATmega328P), USART0.
- atmega8_minimal: minimal ATmega8 board, the USART. The UBRRH and UCSRC
  registers share an address (URSEL).
- atmega162_minimal: minimal ATmega162 board, USART0 and USART1, both with
  the shared UBRRH and UCSRC registers (URSEL).
- arduino_mega: Arduino Mega (ATmega2560), USART0 to USART3.

The ports and their pins are listed in cfg/<target>/portab.h.

** The Demo **

The demo uses the SIO driver on every port of the target at 115200 baud
8N1. On each port a banner with the port number is sent at start and the
end of its transmission is awaited with sioSynchronizeTXEnd(), then a
thread per port echoes back every received byte. The board LED toggles for
each block of echoed bytes.

The USARTs are allocated to the SIO driver in mcuconf.h using the
AVR_SIO_USE_USARTx switches, the same USART cannot be also assigned to the
Serial driver. The ATmega8 and ATmega162 targets disable the kernel
features the test does not use in chconf.h, for their 8 KiB of flash and
1 KiB of SRAM respectively.

The echo can be checked with any terminal program or by sending a stream
of random bytes and comparing the received data. On the Arduino Uno the
echo keeps up with a continuous stream at 115200 baud with the default
configuration. With the kernel debug options enabled in chconf.h each
received byte takes longer than a frame time to be echoed, the 2 frames RX
FIFO overruns under continuous traffic and bytes are lost.

** Build Procedure **

The demo was built using the GCC AVR toolchain. "make" builds all the
targets, "make -f make/Makefile-<target>.make" a single one.
