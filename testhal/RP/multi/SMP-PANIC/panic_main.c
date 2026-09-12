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
 * SMP panic notification saturation regression.
 *
 * Build from either RP2040/EFL-SMP-LOCKOUT or RP2350/EFL-SMP-LOCKOUT with
 * PANIC_SATURATION=yes. Core 1 masks its FIFO IRQ while core 0 fills the
 * inter-core FIFO, then core 0 halts. Core 1 re-enables its FIFO IRQ after
 * the panic notification has been published. A debugger must observe both
 * cores halted and ch1.dbg.panic_msg pointing to "remote panic".
 *
 * Without the durable panic latch, the full FIFO discards the panic token
 * and core 1 continues incrementing panic_heartbeat after enabling the IRQ.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

volatile uint32_t panic_ready;
volatile uint32_t panic_go;
volatile uint32_t panic_heartbeat;

static BaseSequentialStream *chp = (BaseSequentialStream *)&SIOD0;

int main(void) {
  uint32_t fill_count;
  uint32_t start;

  halInit();
  chSysInit();

  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);

  chprintf(chp, "\r\n*** SMP panic FIFO-saturation regression\r\n");

  start = TIMER0->TIMERAWL;
  while (panic_ready == 0U) {
    if ((TIMER0->TIMERAWL - start) > 5000000U) {
      chprintf(chp, "[FAIL] core 1 did not become ready\r\n");
      while (true) {
      }
    }
  }

  fill_count = 0U;
  while ((SIO->FIFO_ST & SIO_FIFO_ST_RDY) != 0U) {
    SIO->FIFO_WR = PORT_FIFO_RESCHEDULE_MESSAGE;
    fill_count++;
  }

  chprintf(chp, "[PASS] outbound FIFO saturated with %u messages\r\n",
           fill_count);
  chprintf(chp, "Halting core 0; debugger should find both cores halted\r\n");
  (void)sioSynchronizeTXEnd(&SIOD0, TIME_INFINITE);

  panic_go = 1U;
  __DMB();
  chSysHalt("panic FIFO saturation test");
}
