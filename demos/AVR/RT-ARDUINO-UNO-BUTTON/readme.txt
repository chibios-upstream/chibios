*****************************************************************************
** ChibiOS/RT port for Atmel AVR ATmega328p.                               **
*****************************************************************************

** TARGET **

The demo runs on an Arduino Uno board.

** The Demo **

The demo blinks the LED on PB5 (pin 13 on Arduino IDE) using a virtual
timer. A push-button between PD2 (pin 2 on Arduino IDE) and GND cycles the
blink half-period through 100, 250, 500 and 1000ms. The internal pull-up is
enabled on PD2.

The button is serviced by INT0 using the PAL callback API:
- The PAL callback runs in ISR context outside the kernel lock, it restarts
  a 50ms debounce virtual timer under chSysLockFromISR().
- The debounce timer callback runs in ISR context outside the kernel lock,
  if the button is still pressed it selects the next rate and, under
  chSysLockFromISR(), re-arms the blink timer and broadcasts an event.
- The main thread waits for the event and prints the new period on Serial0,
  which is available on the board USB connector, 115200 baud 8N1.

** Build Procedure **

The demo was built using the GCC AVR toolchain.
