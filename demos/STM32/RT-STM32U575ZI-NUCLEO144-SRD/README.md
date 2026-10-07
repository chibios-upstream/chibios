# STM32U575ZI NUCLEO144 Smart Run Domain demo

This demo exercises the SYSTICKv3 LPTIM backend on a NUCLEO-U575ZI-Q. It uses
LPTIM3, clocked from LSE through the divide-by-4 prescaler, as an 8192 Hz,
16-bit ChibiOS system timer.

`CH_CFG_ST_TIMEDELTA` remains eight ST ticks (approximately 0.977 ms).
This retains the SYSTICKv3 default tick margin; target-specific interrupt
latency and register synchronization still require validation.

Compare-write acknowledgement (`CMP1OK`) depends on `lptim_ker_ck / prescaler`.
Custom-board measurements of the remaining acknowledgement wait were about
2.81 ms at LSE/32 (1024 Hz), 1.40-1.42 ms at LSE/16 (2048 Hz), and 0.30 ms at
LSE/4 (8192 Hz). These are observations, not an exact universal three-cycle
guarantee. Faster counter clocking reduces time blocked by compare updates.
Tickless 8192 Hz does not mean 8192 periodic IRQs per second. With a 16-bit
counter it does shorten wrap time to eight seconds and half-range timestamp
maintenance to about four seconds. The application-owned CCR2 maintenance
below must therefore remain enabled.

The 8192 Hz/delta-8 configuration has custom-board application evidence, but
this exact updated NUCLEO demo and its autonomous PA1 probe still require
hardware validation. Earlier 1024 Hz demo results do not qualify this build.

The user button starts a two-second STOP2 interval. The LPTIM3 CCR1 compare wakes
the core, and the common ChibiOS IRQ-prologue hook restores the default RUN
clock tree before the SYSTICKv3 driver dispatches the virtual-timer callback.
The callback turns on the green LED and wakes the application thread. Results
are reported over SD1 at the ChibiOS demo default of 38400 baud.

The common hook runs after ChibiOS has entered its statistics, trace and debug
state, but before any normal IRQ driver body. It therefore covers system-timer
and other kernel-aware wake sources without a driver-specific hook. The demo
also owns LPTIM3 CCR2 in this application hook. A CCR2-only timestamp-
maintenance interrupt remains on the lightweight STOP wake clock; simultaneous
enabled CCR1 and CCR2 flags restore the RUN clocks normally.

The clock-restore hook masks all maskable interrupts with PRIMASK. A ChibiOS
kernel lock uses BASEPRI and does not mask priority-zero autonomous interrupts;
allowing one of those handlers to run during RCC and Flash reconfiguration
would expose a partially restored clock tree. Fast priority-zero handlers do
not enter the ChibiOS IRQ prologue and remain outside this wake policy.

## Time measurement

Application elapsed time is measured with `systimestamp_t`,
`chVTGetTimeStamp()`, and `chTimeStampDiffX()`. The wrapping 16-bit system-time
counter is not used as an application clock.

The reduced SYSTICKv3 driver owns only CCR1. The application independently
enables CCR2 at `TIME_MAX_SYSTIME / 2` and services its enabled match from
`CH_CFG_IRQ_PROLOGUE_HOOK()`. The hook calls `chVTGetTimeStampI()` under a
balanced ISR lock before the 16-bit timer can wrap. Each next match is derived
from the previous compare to remain phase-locked; a late or invalid compare is
recovered relative to the current counter and reported in the diagnostics.

This split also demonstrates why SYSTICKv3 alarm operations preserve non-CCR1
`DIER` bits and why its ISR qualifies `CC1IF` with `CC1IE`: the application can
use CCR2 without the system-timer driver enabling, disabling or dispatching it.

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
   restore failure, and approximately 16384 elapsed timestamp ticks.

For a meaningful STOP2 current measurement, low-power debug must not be held
active by the debugger. The demo clears `DBG_STOP` before arming STOP2.
