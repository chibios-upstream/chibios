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
 * EFL SMP lockout validation.
 *
 * Exercises the built-in RP_EFL_XIP_SAFETY_LOCKOUT strategy: one core
 * performs erase/program/verify cycles on the last flash sector while the
 * other core keeps executing flash-resident code, and a fast (above
 * kernel priority) interrupt with a flash-resident handler runs
 * throughout. Without the lockout the flash-resident core would fault or
 * read garbage as soon as XIP is disabled.
 *
 * Report is emitted on UART0 (GPIO0/GPIO1) at the SIO default bitrate.
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "efl_smp_lockout.h"

/*===========================================================================*/
/* Shared state, plain SRAM is coherent between the RP2350 cores.            */
/*===========================================================================*/

volatile uint32_t c0_heartbeat;
volatile uint32_t c1_heartbeat;
volatile uint32_t c1_cycles;
volatile uint32_t c1_errors;
volatile uint32_t c1_go;
volatile uint32_t c1_done;
volatile uint32_t c0_delay_armed;
volatile uint32_t c1_init_entered;
volatile uint32_t c1_init_release;
volatile uint32_t fastirq_count;

/* Statically initialized: core 1 can signal it before core 0's main()
   runs any code after chSysInit().*/
SEMAPHORE_DECL(c1_ready_sem, 0);

/*===========================================================================*/
/* Report helpers.                                                           */
/*===========================================================================*/

static BaseSequentialStream *chp = (BaseSequentialStream *)&SIOD0;

static unsigned pass_count;
static unsigned fail_count;

void eflSmpInstanceInitHook(void *oip) {

  (void)oip;
  if (SIO->CPUID == 1U) {
    c1_init_entered = 1U;
    while (c1_init_release == 0U) {
    }
  }
}

static void report(const char *name, bool ok) {

  chprintf(chp, "  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
}

/*===========================================================================*/
/* Fast interrupt, priority above the kernel, handler in flash.              */
/*===========================================================================*/

/**
 * @brief   TIMER1 alarm 0 handler.
 * @note    Runs above the kernel priority ceiling so it is not masked by
 *          critical sections; the lockout must defer it with PRIMASK
 *          while XIP is off because this code executes from flash.
 */
void RP_TIMER1_IRQ0_HANDLER(void) {

  TIMER1->INTR = 1U;
  fastirq_count++;
  TIMER1->ALARM[0] = TIMER1->TIMERAWL + 1000U;
}

static void fastirq_start(void) {

  rp_peripheral_unreset(RESETS_ALLREG_TIMER1);
  TIMER1->INTE = 1U;
  nvicEnableVector(RP_TIMER1_IRQ0_NUMBER, 1U);
  TIMER1->ALARM[0] = TIMER1->TIMERAWL + 1000U;
}

/*===========================================================================*/
/* Flash exercise, shared by both cores.                                     */
/*===========================================================================*/

#define TEST_SECTOR         (RP_FLASH_SECTORS_COUNT - 1U)
#define TEST_OFFSET         ((flash_offset_t)TEST_SECTOR * RP_FLASH_SECTOR_SIZE)
#define TEST_PAGES          (RP_FLASH_SECTOR_SIZE / RP_FLASH_PAGE_SIZE)

uint32_t flash_cycle(uint8_t pattern) {
  static uint8_t buf[RP_FLASH_PAGE_SIZE];
  BaseFlash *bfp = (BaseFlash *)&EFLD1;
  uint32_t errors = 0U;
  flash_error_t err;
  unsigned page;

  err = flashStartEraseSector(bfp, TEST_SECTOR);
  if (err != FLASH_NO_ERROR) {
    return 1U;
  }
  {
    uint32_t polls = 0U;

    while ((err = flashQueryErase(bfp, NULL)) == FLASH_BUSY_ERASING) {
      if (++polls > 5000000U) {
        return 1U;
      }
    }
    if (err != FLASH_NO_ERROR) {
      return 1U;
    }
  }

  for (page = 0U; page < TEST_PAGES; page++) {
    memset(buf, (int)(uint8_t)(pattern + page), sizeof buf);
    err = flashProgram(bfp, TEST_OFFSET + (page * RP_FLASH_PAGE_SIZE),
                       RP_FLASH_PAGE_SIZE, buf);
    if (err != FLASH_NO_ERROR) {
      errors++;
    }
  }

  for (page = 0U; page < TEST_PAGES; page++) {
    uint8_t expected = (uint8_t)(pattern + page);
    unsigned i;

    err = flashRead(bfp, TEST_OFFSET + (page * RP_FLASH_PAGE_SIZE),
                    RP_FLASH_PAGE_SIZE, buf);
    if (err != FLASH_NO_ERROR) {
      errors++;
      continue;
    }
    for (i = 0U; i < RP_FLASH_PAGE_SIZE; i++) {
      if (buf[i] != expected) {
        errors++;
        break;
      }
    }
  }

  return errors;
}

/*===========================================================================*/
/* Core 0 heartbeat thread, flash-resident tight loop.                       */
/*===========================================================================*/

static volatile bool hb_stop;

static CH_MEM_PRIVATE_BSS(0) THD_WORKING_AREA(waHeartbeat, 256);
static THD_FUNCTION(HeartbeatThread, arg) {

  (void)arg;
  chRegSetThreadName("heartbeat");
  while (!hb_stop) {
    c0_heartbeat++;
  }
}

/*
 * Application entry point, core 0.
 */
int main(void) {
  uint32_t hb_before, fi_before, c1hb_before;
  uint32_t my_errors;
  bool init_entered, init_not_ready, not_ready_during_lockout;
  bool ready_after_unlock;
  unsigned i;

  halInit();
  chSysInit();

  /* Holding core 1 inside its instance hook beyond the lockout timeout.
     The system timer is masked because core 1 owns the kernel lock.*/
  nvicDisableVector(RP_TIMER0_IRQ0_NUMBER);
  c0_delay_armed = 1U;
  {
    uint32_t start;

    start = TIMER0->TIMERAWL;
    while ((c1_init_entered == 0U) &&
           ((TIMER0->TIMERAWL - start) <= PORT_LOCKOUT_TIMEOUT_US)) {
    }
    init_entered = c1_init_entered != 0U;

    start = TIMER0->TIMERAWL;
    while ((TIMER0->TIMERAWL - start) <=
           (PORT_LOCKOUT_TIMEOUT_US + 10000U)) {
    }
    init_not_ready = !__port_lockout_other_ready();

    /* Starting an actual lockout while core 1 is unready. It does not
       perform a FIFO handshake but keeps admission closed until unlock.*/
    __port_flash_lockout();
    c1_init_release = 1U;

    start = TIMER0->TIMERAWL;
    while ((TIMER0->TIMERAWL - start) <=
           (PORT_LOCKOUT_TIMEOUT_US + 10000U)) {
    }
    not_ready_during_lockout = !__port_lockout_other_ready();

    __port_flash_unlockout();
    start = TIMER0->TIMERAWL;
    while (!__port_lockout_other_ready() &&
           ((TIMER0->TIMERAWL - start) <= PORT_LOCKOUT_TIMEOUT_US)) {
    }
    ready_after_unlock = __port_lockout_other_ready();
  }
  nvicEnableVector(RP_TIMER0_IRQ0_NUMBER,
                   RP_IRQ_TIMER0_ALARM0_PRIORITY);

  /* UART0 console on GPIO0/GPIO1.*/
  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);

  palSetLineMode(25U, PAL_MODE_OUTPUT_PUSHPULL);

  chprintf(chp, "\r\n*** EFL SMP lockout validation\r\n");
  chprintf(chp, "*** Flash sector under test: %u\r\n", TEST_SECTOR);

  eflStart(&EFLD1, NULL);
  report("core 1 entered delayed init", init_entered);
  report("core 1 not ready during delayed init", init_not_ready);
  report("core 1 stayed unready during lockout",
         not_ready_during_lockout);
  report("core 1 ready after startup unlock", ready_after_unlock);

  fastirq_start();

  chThdCreateStatic(waHeartbeat, sizeof(waHeartbeat),
                    NORMALPRIO - 1, HeartbeatThread, NULL);

  /* Waiting for core 1 to come alive; a core that never starts must
     produce a report rather than an eternal hang.*/
  if (chSemWaitTimeout(&c1_ready_sem, TIME_S2I(5)) != MSG_OK) {
    report("core 1 became ready", false);
    chprintf(chp, "\r\nResults: %u pass, %u fail\r\n", pass_count, fail_count);
    chprintf(chp, "*** FAILURES DETECTED ***\r\n");
    while (true) {
      palToggleLine(25U);
      chThdSleepMilliseconds(100);
    }
  }

  /*
   * Phase A: core 1 flashes, core 0 executes from flash throughout.
   */
  chprintf(chp, "--- Phase A: core 1 flashing, core 0 on flash\r\n");
  hb_before = c0_heartbeat;
  fi_before = fastirq_count;
  c1_go = 1U;

  for (i = 0U; (c1_done == 0U) && (i < 1200U); i++) {
    chThdSleepMilliseconds(100);
  }

  report("phase A completed", c1_done != 0U);
  report("core 1 flash cycles clean",
         (c1_errors == 0U) && (c1_cycles == C1_FLASH_CYCLES));
  report("core 0 heartbeat advanced", c0_heartbeat != hb_before);
  report("fast IRQ serviced", fastirq_count != fi_before);

  /*
   * Phase B: mirrored, core 0 flashes while core 1 executes from flash.
   * Skipped if phase A never completed, core 1 could still be inside
   * flash_cycle().
   */
  if (c1_done != 0U) {
    chprintf(chp, "--- Phase B: core 0 flashing, core 1 on flash\r\n");
    c1hb_before = c1_heartbeat;
    fi_before = fastirq_count;
    my_errors = 0U;
    for (i = 0U; i < C0_FLASH_CYCLES; i++) {
      my_errors += flash_cycle((uint8_t)(0xA0U + i));
    }

    report("core 0 flash cycles clean", my_errors == 0U);
    report("core 1 heartbeat advanced", c1_heartbeat != c1hb_before);
    report("fast IRQ serviced", fastirq_count != fi_before);
  }
  else {
    report("phase B skipped, phase A incomplete", false);
  }

  chprintf(chp, "\r\nResults: %u pass, %u fail\r\n", pass_count, fail_count);
  if (fail_count == 0U) {
    chprintf(chp, "ALL TESTS PASSED\r\n");
  }
  else {
    chprintf(chp, "*** FAILURES DETECTED ***\r\n");
  }

  hb_stop = true;
  while (true) {
    palToggleLine(25U);
    chThdSleepMilliseconds(500);
  }
}
