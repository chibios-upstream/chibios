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
 * @file    TIMERv1/hal_gpt_lld.c
 * @brief   RP GPT subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "hal.h"

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   GPTD1 driver identifier.
 */
#if (RP_GPT_USE_GPT1 == TRUE) || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif

/**
 * @brief   GPTD2 driver identifier.
 */
#if (RP_GPT_USE_GPT2 == TRUE) || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif

/**
 * @brief   GPTD3 driver identifier.
 */
#if (RP_GPT_USE_GPT3 == TRUE) || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif

/**
 * @brief   GPTD4 driver identifier.
 */
#if (RP_GPT_USE_GPT4 == TRUE) || defined(__DOXYGEN__)
GPTDriver GPTD4;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Number of started GPT drivers sharing TIMER1.
 */
static unsigned gpt_started;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Arms the alarm on the current deadline.
 * @details The alarm fires on an exact match, a deadline the counter has
 *          already passed would only be matched after the counter wraps.
 *          In that case the alarm is disarmed and its interrupt is forced.
 * @note    Deadlines are at most 2^31 ticks ahead, see
 *          @p gpt_lld_start_timer().
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 */
static void gpt_lld_arm(GPTDriver *gptp) {
  uint32_t mask = 1U << gptp->alarm;

  TIMER1->ALARM[gptp->alarm] = gptp->deadline;

  /* A fired alarm clears its ARMED bit, it is sampled after the counter so
     a match happening between the two reads is not forced again.*/
  if (((int32_t)(gptp->deadline - TIMER1->TIMERAWL) <= 0) &&
      ((TIMER1->ARMED & mask) != 0U)) {
    TIMER1->ARMED    = mask;
    TIMER1->SET.INTF = mask;
  }
}

/**
 * @brief   Shared IRQ handler.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 */
static void gpt_lld_serve_interrupt(GPTDriver *gptp) {
  uint32_t mask = 1U << gptp->alarm;

  /* An alarm stopped or re-armed from a critical zone after firing can
     leave its interrupt pended in the NVIC with nothing pending in
     TIMER1, such an entry is dismissed.*/
  if ((TIMER1->INTS & mask) == 0U) {
    return;
  }
  TIMER1->CLR.INTF = mask;
  TIMER1->INTR     = mask;

  if (gptp->state == GPT_CONTINUOUS) {
    uint32_t elapsed;

    /* Next deadline on the period grid. Periods missed while the event
       was waiting are merged into this event, as a hardware timer would
       do.*/
    elapsed = TIMER1->TIMERAWL - gptp->deadline;
    gptp->deadline += ((elapsed / gptp->period) + 1U) * gptp->period;
    gpt_lld_arm(gptp);
  }

  _gpt_isr_invoke_cb(gptp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (RP_GPT_USE_GPT1 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   TIMER1 alarm 0 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_TIMER1_IRQ0_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD1);

  OSAL_IRQ_EPILOGUE();
}
#endif /* RP_GPT_USE_GPT1 == TRUE */

#if (RP_GPT_USE_GPT2 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   TIMER1 alarm 1 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_TIMER1_IRQ1_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD2);

  OSAL_IRQ_EPILOGUE();
}
#endif /* RP_GPT_USE_GPT2 == TRUE */

#if (RP_GPT_USE_GPT3 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   TIMER1 alarm 2 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_TIMER1_IRQ2_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD3);

  OSAL_IRQ_EPILOGUE();
}
#endif /* RP_GPT_USE_GPT3 == TRUE */

#if (RP_GPT_USE_GPT4 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   TIMER1 alarm 3 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_TIMER1_IRQ3_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD4);

  OSAL_IRQ_EPILOGUE();
}
#endif /* RP_GPT_USE_GPT4 == TRUE */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 *
 * @notapi
 */
void gpt_lld_init(void) {

  gpt_started = 0U;

#if RP_GPT_USE_GPT1 == TRUE
  gptObjectInit(&GPTD1);
  GPTD1.alarm   = 0U;
  GPTD1.irqn    = RP_TIMER1_IRQ0_NUMBER;
  GPTD1.irqprio = RP_IRQ_TIMER1_ALARM0_PRIORITY;
#endif

#if RP_GPT_USE_GPT2 == TRUE
  gptObjectInit(&GPTD2);
  GPTD2.alarm   = 1U;
  GPTD2.irqn    = RP_TIMER1_IRQ1_NUMBER;
  GPTD2.irqprio = RP_IRQ_TIMER1_ALARM1_PRIORITY;
#endif

#if RP_GPT_USE_GPT3 == TRUE
  gptObjectInit(&GPTD3);
  GPTD3.alarm   = 2U;
  GPTD3.irqn    = RP_TIMER1_IRQ2_NUMBER;
  GPTD3.irqprio = RP_IRQ_TIMER1_ALARM2_PRIORITY;
#endif

#if RP_GPT_USE_GPT4 == TRUE
  gptObjectInit(&GPTD4);
  GPTD4.alarm   = 3U;
  GPTD4.irqn    = RP_TIMER1_IRQ3_NUMBER;
  GPTD4.irqprio = RP_IRQ_TIMER1_ALARM3_PRIORITY;
#endif
}

/**
 * @brief   Configures and activates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_start(GPTDriver *gptp) {

  osalDbgAssert((gptp->config->frequency > 0U) &&
                (gptp->config->frequency <= RP_GPT_TIMER_FREQUENCY) &&
                ((RP_GPT_TIMER_FREQUENCY % gptp->config->frequency) == 0U),
                "invalid frequency");

  if (gptp->state == GPT_STOP) {
    /* TIMER1 is shared by all the GPT drivers, it is taken out of reset by
       the first one and paused while the cores are halted by a debugger.*/
    if (gpt_started == 0U) {
      rp_peripheral_unreset(RESETS_ALLREG_TIMER1);
      TIMER1->DBGPAUSE = TIMER_DBGPAUSE_DBG0 | TIMER_DBGPAUSE_DBG1;
    }
    gpt_started++;

    nvicEnableVector(gptp->irqn, gptp->irqprio);
  }

  gptp->scale = RP_GPT_TIMER_FREQUENCY / gptp->config->frequency;
}

/**
 * @brief   Deactivates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop(GPTDriver *gptp) {

  if (gptp->state == GPT_READY) {
    nvicDisableVector(gptp->irqn);

    /* The last driver puts TIMER1 back in reset.*/
    gpt_started--;
    if (gpt_started == 0U) {
      rp_peripheral_reset(RESETS_ALLREG_TIMER1);
    }
  }
}

/**
 * @brief   Starts the timer in continuous mode.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  period in ticks, the period in TIMER1 ticks must not
 *                      exceed 2^31 - 1
 *
 * @notapi
 */
void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t interval) {
  uint32_t mask = 1U << gptp->alarm;

  osalDbgAssert((interval > 0U) && (interval <= (0x7FFFFFFFU / gptp->scale)),
                "invalid interval");

  gptp->period = (uint32_t)interval * gptp->scale;

  /* Clearing any event left by a previous run.*/
  TIMER1->CLR.INTF = mask;
  TIMER1->INTR     = mask;
  TIMER1->SET.INTE = mask;

  gptp->deadline = TIMER1->TIMERAWL + gptp->period;
  gpt_lld_arm(gptp);
}

/**
 * @brief   Stops the timer.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop_timer(GPTDriver *gptp) {
  uint32_t mask = 1U << gptp->alarm;

  TIMER1->CLR.INTE = mask;
  TIMER1->ARMED    = mask;
  TIMER1->CLR.INTF = mask;
  TIMER1->INTR     = mask;
}

/**
 * @brief   Starts the timer in one shot mode and waits for completion.
 * @details This function specifically polls the timer waiting for completion
 *          in order to not have extra delays caused by interrupt servicing,
 *          this function is only recommended for short delays.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  time interval in ticks
 *
 * @notapi
 */
void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {
  uint32_t start, ticks;

  start = TIMER1->TIMERAWL;
  ticks = (uint32_t)interval * gptp->scale;
  while ((TIMER1->TIMERAWL - start) < ticks) {
  }
}

#endif /* HAL_USE_GPT == TRUE */

/** @} */
