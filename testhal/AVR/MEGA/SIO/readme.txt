*****************************************************************************
** ChibiOS/RT SIO test on Atmel AVR ATmega328p.                            **
*****************************************************************************

** TARGET **

The test demo runs on an Arduino Uno board.

** The Demo **

The demo uses the SIO driver on USART0, available on the board USB
connector, at 115200 baud 8N1. A banner is sent at start and the end of its
transmission is awaited with sioSynchronizeTXEnd(), then every received
byte is echoed back. The LED on PB5 (pin 13 on Arduino IDE) toggles for each
block of echoed bytes.

The USART is allocated to the SIO driver in mcuconf.h using the
AVR_SIO_USE_USARTx switches, the same USART cannot be also assigned to the
Serial driver.

The echo can be checked with any terminal program or by sending a stream
of random bytes and comparing the received data. The echo keeps up with a
continuous stream at 115200 baud with the default configuration. With the
kernel debug options enabled in chconf.h each received byte takes longer
than a frame time to be echoed, the 2 frames RX FIFO overruns under
continuous traffic and bytes are lost.

** Build Procedure **

The demo was built using the GCC AVR toolchain.
