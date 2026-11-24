*****************************************************************************
** ChibiOS/HAL - Encoder interface test for Atmel AVR ATmega328p.          **
*****************************************************************************

** TARGET **

The test runs on an Arduino Uno board.

** The Test **

The test exercises the BaseEncoder interface implemented by the GPIO
quadrature encoder driver. The encoder lines are driven as outputs by the
test itself: the AVR external and pin change interrupts trigger on output
pins too, so no wiring is required, the pins must be left unconnected.

Default pins, A and B on external interrupts:
- A on PD2 (pin 2 on Arduino IDE), INT0.
- B on PD3 (pin 3 on Arduino IDE), INT1.
Building with -DENCODER_USE_PCINT (UDEFS) moves A and B to pin change
interrupts:
- A on PC0 (pin A0 on Arduino IDE), PCINT8.
- B on PC1 (pin A1 on Arduino IDE), PCINT9.
The index is on PB0 (pin 8 on Arduino IDE), PCINT0, in both cases.

The checks cover forward and backward counting, direction jitter, position
setting, the decoding error on a change of both lines, index latching,
index reset, no index line, reversed counting, restart and the edges while
the driver is stopped. Each check prints a PASS or FAIL line with the
encoder state on Serial0, available on the board USB connector, 115200
baud 8N1. The test ends with "ALL PASSED", lighting the LED on PB5 (pin 13
on Arduino IDE), or with "FAILED <count>".

The kernel state checker, parameter checks and assertions are enabled.

** Build Procedure **

The test was built using the GCC AVR toolchain.
