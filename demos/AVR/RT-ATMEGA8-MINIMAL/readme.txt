*****************************************************************************
** ChibiOS/RT port for Atmel AVR ATmega8.                                  **
*****************************************************************************

** TARGET **

The demo runs on a minimal ATmega8 board at 16 MHz (ATMEGA8_MINIMAL). The
ATmega8 is pin compatible with the ATmega328P, the board has the Arduino Uno
pinout.

** The Demo **

The main thread prints a message every two seconds on the USART, PD0 (RXD)
and PD1 (TXD), at 115200 baud 8N1 using the SIO driver. Another thread
toggles the LED on PB5 (pin 13 on an Arduino board) every second.

The ATmega8 has 8 KiB of flash and 1 KiB of SRAM, the kernel features the
demo does not use are disabled in chconf.h.

** Build Procedure **

The demo was built using the GCC AVR toolchain.

** Notes **

The demo runs natively, the fuses must select the external 16 MHz crystal
(CKSEL, SUT and CKOPT, see the ATmega8 datasheet). The programmer settings
are in the Makefile.
