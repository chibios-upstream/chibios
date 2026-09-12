/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
 * SMP panic notification kernel-spinlock regression.
 *
 * Build from either RP2040/EFL-SMP-LOCKOUT or RP2350/EFL-SMP-LOCKOUT with
 * PANIC_SPINLOCK=yes. Core 0 halts while holding the kernel spinlock and core
 * 1 attempts to acquire it with interrupts masked. A debugger must observe
 * both cores halted and ch1.dbg.panic_msg pointing to "remote panic".
 *
 * Without polling the durable latch in port_spinlock_take(), core 1 remains
 * wedged on the abandoned lock and never records the remote panic.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

volatile uint32_t panic_spinlock_ready;
volatile uint32_t panic_spinlock_go;
volatile uint32_t panic_spinlock_entering;
volatile uint32_t panic_spinlock_heartbeat;

static BaseSequentialStream *chp = (BaseSequentialStream *)&SIOD0;

int main(void) {
  uint32_t start;

  halInit();
  chSysInit();

  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);

  chprintf(chp, "\r\n*** SMP panic kernel-spinlock regression\r\n");

  start = TIMER0->TIMERAWL;
  while (panic_spinlock_ready == 0U) {
    if ((TIMER0->TIMERAWL - start) > 5000000U) {
      chprintf(chp, "[FAIL] core 1 did not become ready\r\n");
      while (true) {
      }
    }
  }

  chprintf(chp, "Halting core 0 while core 1 waits on the kernel lock\r\n");
  (void)sioSynchronizeTXEnd(&SIOD0, TIME_INFINITE);

  chSysLock();
  panic_spinlock_go = 1U;
  __DMB();

  start = TIMER0->TIMERAWL;
  while (panic_spinlock_entering == 0U) {
    if ((TIMER0->TIMERAWL - start) > 5000000U) {
      chSysUnlock();
      chprintf(chp, "[FAIL] core 1 did not attempt the kernel lock\r\n");
      while (true) {
      }
    }
  }

  chSysHalt("panic while holding kernel spinlock");
}
