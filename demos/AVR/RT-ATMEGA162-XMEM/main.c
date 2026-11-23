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

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

/*
 * External SRAM region, it is the core memory, see the Makefile.
 */
extern uint8_t __heap_base__[];
extern uint8_t __heap_end__[];

/*
 * Number of worker threads created from the heap at each round.
 */
#define WORKERS                 3U

/*
 * Size of the buffer each worker fills on its stack.
 */
#define WORKER_BUFFER_SIZE      64U

/*
 * SIO configuration, 115200 baud 8N1.
 */
static const SIOConfig sio_config = {
  .baud   = 115200U,
  .format = AVR_SIO_8N1
};

static BaseSequentialStream *const chp = (BaseSequentialStream *)&SIOD0;

/*
 * External memory interface setup for a 32 KiB SRAM: AD7:0 on PA7:0, latched
 * on ALE (PE1), A14:8 on PC6:0, WR on PD6 and RD on PD7. The external memory
 * starts after the internal SRAM, at 0x0500.
 */
static void xmem_init(void) {
  uint8_t mcucsr;

  /* The JTAG interface takes PC7:4, so A14:12, while the JTAGEN fuse is
     programmed, as shipped, and JTD is clear. JTD is set, it only changes
     when written twice within four cycles, hence the two back to back
     writes. The reset flags are written as read, writing ones keeps them.*/
  mcucsr = (uint8_t)(MCUCSR | _BV(JTD));
  asm volatile ("out %0, %1\n\t"
                "out %0, %1"
                :
                : "I" (_SFR_IO_ADDR(MCUCSR)), "r" (mcucsr));

  /* A single sector (SRL2:0 = 000), its wait states are SRW11:SRW10 = 01,
     one cycle during the read and write strobes.*/
  EMCUCR &= (uint8_t)~(_BV(SRL2) | _BV(SRL1) | _BV(SRL0) | _BV(SRW11));

  /* 7 high address bits, PC7 is released (XMM2:0 = 001).*/
  SFIOR = (uint8_t)((SFIOR & ~(_BV(XMM2) | _BV(XMM1) | _BV(XMM0))) |
                    _BV(XMM0));

  /* Enables the interface, it overrides the port directions.*/
  MCUCR |= _BV(SRE) | _BV(SRW10);
}

/*
 * Test pattern of an address, different for the low and high address bytes
 * so that stuck or shorted address lines are detected.
 */
static uint8_t xmem_pattern(volatile uint8_t *p) {
  uintptr_t a = (uintptr_t)p;

  return (uint8_t)(a ^ (a >> 8) ^ 0x55U);
}

/*
 * Destructive memory test, the pattern then its complement are written to
 * the whole region and verified. Returns the first failing address or NULL.
 */
static uint8_t *xmem_test(uint8_t *base, uint8_t *end) {
  static const uint8_t inv[2] = {0x00U, 0xFFU};
  volatile uint8_t *p;
  unsigned i;

  for (i = 0U; i < 2U; i++) {
    for (p = base; p < end; p++) {
      *p = (uint8_t)(xmem_pattern(p) ^ inv[i]);
    }
    for (p = base; p < end; p++) {
      if (*p != (uint8_t)(xmem_pattern(p) ^ inv[i])) {
        return (uint8_t *)p;
      }
    }
  }
  return NULL;
}

/*
 * LED blinker thread, static in the internal SRAM.
 */
static THD_WORKING_AREA(waBlinker, 32);
static THD_FUNCTION(Blinker, arg) {

  (void)arg;
  while (true) {
    palTogglePad(IOPORT2, PORTB_LED1);
    chThdSleepMilliseconds(500);
  }
}

/*
 * Worker thread, created from the heap so its stack is in the external SRAM.
 * It fills a buffer on its stack and exits with the buffer checksum.
 */
static THD_FUNCTION(Worker, arg) {
  unsigned n = (unsigned)(uintptr_t)arg;
  uint8_t buf[WORKER_BUFFER_SIZE];
  unsigned i, sum;

  for (i = 0U; i < WORKER_BUFFER_SIZE; i++) {
    buf[i] = (uint8_t)(i * (n + 1U));
  }
  chThdSleepMilliseconds(10 * (n + 1U));
  sum = 0U;
  for (i = 0U; i < WORKER_BUFFER_SIZE; i++) {
    sum += buf[i];
  }
  chThdExit((msg_t)sum);
}

/*
 * Application entry point.
 */
int main(void) {
  uint8_t *fail;

  /*
   * The external memory is enabled first, nothing uses it before the heap.
   */
  xmem_init();

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  (void) sioStart(&SIOD0, &sio_config);
  chprintf(chp, "\r\nATmega162 external memory demo\r\n");

  /*
   * The whole external region is tested before the heap allocates from it.
   */
  chprintf(chp, "Testing 0x%04x-0x%04x: ",
           (unsigned)(uintptr_t)__heap_base__,
           (unsigned)(uintptr_t)__heap_end__ - 1U);
  fail = xmem_test(__heap_base__, __heap_end__);
  if (fail != NULL) {
    chprintf(chp, "failed at 0x%04x\r\n", (unsigned)(uintptr_t)fail);
    while (true) {
      palTogglePad(IOPORT2, PORTB_LED1);
      chThdSleepMilliseconds(100);
    }
  }
  chprintf(chp, "OK\r\n");

  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, Blinker, NULL);

  /*
   * Worker threads are created from the heap, waited and released at each
   * round, the heap memory is reused.
   */
  while (true) {
    thread_t *tp[WORKERS];
    memory_area_t core;
    size_t total, largest, fragments;
    unsigned n;

    for (n = 0U; n < WORKERS; n++) {
      tp[n] = chThdCreateFromHeap(NULL,
                                  THD_WORKING_AREA_SIZE(WORKER_BUFFER_SIZE + 64U),
                                  "worker", NORMALPRIO - 1, Worker,
                                  (void *)(uintptr_t)n);
    }
    for (n = 0U; n < WORKERS; n++) {
      if (tp[n] == NULL) {
        chprintf(chp, "worker %u: no memory\r\n", n);
        continue;
      }
      chprintf(chp, "worker %u at 0x%04x: checksum %u\r\n", n,
               (unsigned)(uintptr_t)tp[n], (unsigned)chThdWait(tp[n]));
    }

    fragments = chHeapStatus(NULL, &total, &largest);
    chCoreGetStatusX(&core);
    chprintf(chp, "heap: %u bytes free in %u fragments, core: %u bytes\r\n",
             (unsigned)total, (unsigned)fragments, (unsigned)core.size);
    chThdSleepMilliseconds(2000);
  }
}
