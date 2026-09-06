# STM32U575ZI NUCLEO144 Smart Run Domain demo

This demo exercises the SYSTICKv3 LPTIM backend on a NUCLEO-U575ZI-Q. It uses
LPTIM4, clocked from LSE through the divide-by-32 prescaler, as a 1024 Hz,
16-bit ChibiOS system timer.

`CH_CFG_ST_TIMEDELTA` is eight ST ticks. This satisfies the SYSTICKv3 default
minimum margin for asynchronous LPTIM register updates and interrupt latency.

The user button starts a two-second STOP2 interval. The LPTIM4 compare wakes
the core, the SYSTICKv3 wake hook restores the default RUN clock tree, and only
then does the ChibiOS virtual-timer callback execute. The callback turns on the
green LED and wakes the application thread. Results are reported over SD1 at
the ChibiOS demo default of 38400 baud.

The clock-restore hook masks all maskable interrupts with PRIMASK. A ChibiOS
kernel lock uses BASEPRI and does not mask priority-zero autonomous interrupts;
allowing one of those handlers to run during RCC and Flash reconfiguration
would expose a partially restored clock tree.

## Time measurement

Application elapsed time is measured with `systimestamp_t`,
`chVTGetTimeStamp()`, and `chTimeStampDiffX()`. The wrapping 16-bit system-time
counter is not used as an application clock.

The SYSTICKv3 registry capability check reports that the one-channel LPTIM4
requires application timestamp maintenance. The demo therefore runs a
mandatory continuous virtual timer at `TIME_MAX_SYSTIME / 2`. Its I-class
callback calls `chVTGetTimeStampI()` before the 16-bit system timer can wrap,
preserving the monotonic timestamp extension even when the application does
not request a timestamp for a long period.

## Autonomous activity probe

LPTIM1 paces a circular LPDMA1 list which alternately sets and clears
LPGPIO1_P0. PA1 is configured as AF11 and exposes a 1 Hz square wave which
continues while the core is in STOP2. The LPDMA descriptor and source words are
placed in SRAM4.

None of the three onboard LED pins has an LPGPIO alternate function:

- blue: PB7
- green: PC7
- red: PG2

The autonomous waveform must therefore be observed on PA1 with a scope or
logic analyzer. The onboard green LED is deliberately reserved for post-wake
confirmation.

Set `DEMO_USE_AUTONOMOUS_PROBE` to `FALSE` in `main.c` if PA1 is needed by an
expansion board.

## Operation

1. Build and program the demo.
2. Open the ST-LINK virtual COM port at 38400 baud, 8-N-1.
3. Optionally observe PA1; it should toggle once every 500 ms.
4. Press and release the blue user button.
5. Observe the two-second output pause and the green LED at the timer wake.
6. Check that the report shows one timer wake, a confirmed STOP flag, no clock
   restore failure, and approximately 2048 elapsed timestamp ticks.

For a meaningful STOP2 current measurement, low-power debug must not be held
active by the debugger. The demo clears `DBG_STOP` before arming STOP2.
