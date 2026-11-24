*****************************************************************************
** ChibiOS/NIL port for Atmel AVR ATmega2560.                              **
*****************************************************************************

** TARGET **

The demo runs on an Arduino Mega board.

** The Demo **

One thread toggles the LED on PB7 (pin 13 on Arduino IDE). A push-button
between PE4 (pin 2 on Arduino IDE) and GND cycles the blink half-period
through 100, 250, 500 and 1000ms. The internal pull-up is enabled on PE4.

The button is serviced by INT4 using the PAL synchronous API: a second
thread blocks in palWaitLineTimeout() on the falling edge, waits 50ms and
accepts the press only if the button is still held. The new period is
printed on the serial console, which is available on the board USB
connector, 115200 baud 8N1.

** Build Procedure **

The demo was built using the GCC AVR toolchain.
