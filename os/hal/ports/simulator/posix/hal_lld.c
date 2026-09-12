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

/**
 * @file    simulator/posix/hal_lld.c
 * @brief   Posix simulator HAL subsystem low level driver code.
 *
 * @addtogroup POSIX_HAL
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Monotonic clock resolution used for tick scheduling.*/
#define SIM_NANOSECONDS_PER_SECOND      1000000000ULL

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/* Next tick deadline and fractional nanoseconds carried across periods.*/
static uint64_t nextcnt;
static uint64_t tick_fraction;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static uint64_t get_monotonic_time_ns(void) {
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    abort();
  }

  return ((uint64_t)ts.tv_sec * SIM_NANOSECONDS_PER_SECOND) +
         (uint64_t)ts.tv_nsec;
}

static void advance_tick(void) {

  nextcnt += SIM_NANOSECONDS_PER_SECOND / (uint64_t)OSAL_ST_FREQUENCY;
  tick_fraction += SIM_NANOSECONDS_PER_SECOND %
                   (uint64_t)OSAL_ST_FREQUENCY;
  if (tick_fraction >= (uint64_t)OSAL_ST_FREQUENCY) {
    nextcnt++;
    tick_fraction -= (uint64_t)OSAL_ST_FREQUENCY;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief Low level HAL driver initialization.
 */
void hal_lld_init(void) {

#if defined(__APPLE__)
  puts("ChibiOS/RT simulator (OS X)\n");
#else
  puts("ChibiOS/RT simulator (Linux)\n");
#endif
  nextcnt = get_monotonic_time_ns();
  tick_fraction = 0U;
  advance_tick();
}

/**
 * @brief   Interrupt simulation.
 */
void _sim_check_for_interrupts(void) {
  uint64_t now;
  bool int_occurred = false;

#if HAL_USE_SERIAL
  while (sd_lld_interrupt_pending()) {
    int_occurred = true;
  }
#endif

  now = get_monotonic_time_ns();
  if (now >= nextcnt) {
    int_occurred = true;
    advance_tick();

    CH_IRQ_PROLOGUE();

    chSysLockFromISR();
    chSysTimerHandlerI();
    chSysUnlockFromISR();

    CH_IRQ_EPILOGUE();
  }

  if (int_occurred) {
    __dbg_check_lock();
    if (chSchIsPreemptionRequired())
      chSchDoPreemption();
    __dbg_check_unlock();
  }
}

/** @} */
