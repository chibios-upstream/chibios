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
 * @file    TIMERv1/hal_st_lld.c
 * @brief   ST Driver subsystem low level driver code.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if CH_CFG_ST_TIMEDELTA > 0
#define ST_HANDLER                          SysTick_Handler
#endif /* CH_CFG_ST_TIMEDELTA > 0 */

#if CH_CFG_ST_TIMEDELTA == 0
#define ST_HANDLER                          SysTick_Handler
#endif /* CH_CFG_ST_TIMEDELTA == 0 */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

typedef struct {
  uint32_t n;
  uint32_t prio;
} alarm_irq_t;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if (CH_CFG_ST_TIMEDELTA > 0) || defined(__DOXYGEN__)
#if (ST_LLD_NUM_ALARMS > 1) || defined(__DOXYGEN__)
static const alarm_irq_t alarm_irqs[ST_LLD_NUM_ALARMS] = {
  {RP_TIMER0_IRQ0_NUMBER, RP_IRQ_TIMER0_ALARM0_PRIORITY},
  {RP_TIMER0_IRQ1_NUMBER, RP_IRQ_TIMER0_ALARM1_PRIORITY},
  {RP_TIMER0_IRQ2_NUMBER, RP_IRQ_TIMER0_ALARM2_PRIORITY},
  {RP_TIMER0_IRQ3_NUMBER, RP_IRQ_TIMER0_ALARM3_PRIORITY}
};
#endif
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (CH_CFG_ST_TIMEDELTA == 0) || defined(__DOXYGEN__)
#if !defined(ST_SYSTICK_SUPPRESS_ISR)
/**
 * @brief   SysTick interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SysTick_Handler) {

  CH_IRQ_PROLOGUE();

  chSysLockFromISR();
  chSysTimerHandlerI();
  chSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}
#endif
#endif /* CH_CFG_ST_TIMEDELTA == 0 */

#if (CH_CFG_ST_TIMEDELTA > 0) || defined(__DOXYGEN__)
#if !defined(ST_TIMER_ALARM0_SUPPRESS_ISR)
/**
 * @brief   TIMER alarm 0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_TIMER0_IRQ0_HANDLER) {
  uint32_t ints;

  CH_IRQ_PROLOGUE();

  ints = TIMER0->INTS;
  TIMER0->INTR = TIMER_INTR_ALARM0;

  /* Dismissing spurious wakeups, the alarm can be stopped from within a
     critical section while its interrupt is already latched and pended
     in the NVIC, in that case the callback must not be invoked.*/
  if ((ints & TIMER_INTS_ALARM0) != 0U) {
#if defined(ST_LLD_ALARM0_STATIC_CB)
    ST_LLD_ALARM0_STATIC_CB();
#else
    if (st_callbacks[0] != NULL) {
      st_callbacks[0](0U);
    }
#endif
  }

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(ST_TIMER_ALARM1_SUPPRESS_ISR)
/**
 * @brief   TIMER alarm 1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_TIMER0_IRQ1_HANDLER) {
  uint32_t ints;

  CH_IRQ_PROLOGUE();

  ints = TIMER0->INTS;
  TIMER0->INTR = TIMER_INTR_ALARM1;

  /* Dismissing spurious wakeups, the alarm can be stopped from within a
     critical section while its interrupt is already latched and pended
     in the NVIC, in that case the callback must not be invoked.*/
  if ((ints & TIMER_INTS_ALARM1) != 0U) {
#if defined(ST_LLD_ALARM1_STATIC_CB)
    ST_LLD_ALARM1_STATIC_CB();
#else
    if (st_callbacks[1] != NULL) {
      st_callbacks[1](1U);
    }
#endif
  }

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(ST_TIMER_ALARM2_SUPPRESS_ISR)
/**
 * @brief   TIMER alarm 2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_TIMER0_IRQ2_HANDLER) {
  uint32_t ints;

  CH_IRQ_PROLOGUE();

  ints = TIMER0->INTS;
  TIMER0->INTR = TIMER_INTR_ALARM2;

  /* Dismissing spurious wakeups, the alarm can be stopped from within a
     critical section while its interrupt is already latched and pended
     in the NVIC, in that case the callback must not be invoked.*/
  if ((ints & TIMER_INTS_ALARM2) != 0U) {
#if defined(ST_LLD_ALARM2_STATIC_CB)
    ST_LLD_ALARM2_STATIC_CB();
#else
    if (st_callbacks[2] != NULL) {
      st_callbacks[2](2U);
    }
#endif
  }

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(ST_TIMER_ALARM3_SUPPRESS_ISR)
/**
 * @brief   TIMER alarm 3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_TIMER0_IRQ3_HANDLER) {
  uint32_t ints;

  CH_IRQ_PROLOGUE();

  ints = TIMER0->INTS;
  TIMER0->INTR = TIMER_INTR_ALARM3;

  /* Dismissing spurious wakeups, the alarm can be stopped from within a
     critical section while its interrupt is already latched and pended
     in the NVIC, in that case the callback must not be invoked.*/
  if ((ints & TIMER_INTS_ALARM3) != 0U) {
#if defined(ST_LLD_ALARM3_STATIC_CB)
    ST_LLD_ALARM3_STATIC_CB();
#else
    if (st_callbacks[3] != NULL) {
      st_callbacks[3](3U);
    }
#endif
  }

  CH_IRQ_EPILOGUE();
}
#endif

#endif /* CH_CFG_ST_TIMEDELTA > 0 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void) {

#if CH_CFG_ST_TIMEDELTA > 0
  /* The timer needs to stop during debug or the virtual timers list would
     go out of sync.*/
  TIMER0->DBGPAUSE  = TIMER_DBGPAUSE_DBG0 | TIMER_DBGPAUSE_DBG1;

  /* Comparators and counter initially at zero.*/
  TIMER0->TIMELW    = 0U;
  TIMER0->TIMEHW    = 0U;
  TIMER0->ALARM[0]  = 0U;
  TIMER0->ALARM[1]  = 0U;
  TIMER0->ALARM[2]  = 0U;
  TIMER0->ALARM[3]  = 0U;
  TIMER0->INTE      = 0U;
  TIMER0->INTR      = TIMER_INTR_ALARM3 | TIMER_INTR_ALARM2 |
                      TIMER_INTR_ALARM1 | TIMER_INTR_ALARM0;

#endif /* CH_CFG_ST_TIMEDELTA > 0 */
}

/**
 * @brief   Enables an alarm interrupt on the invoking core.
 * @note    Must be called before any other alarm-related function.
 *
 * @notapi
 */
void st_lld_bind(void) {

#if CH_CFG_ST_TIMEDELTA > 0
  nvicEnableVector(RP_TIMER0_IRQ0_NUMBER, RP_IRQ_TIMER0_ALARM0_PRIORITY);
#endif
#if CH_CFG_ST_TIMEDELTA == 0
  uint32_t  timer_clk = RP_CORE_CLK;

  chDbgAssert(timer_clk % CH_CFG_ST_FREQUENCY == 0U,
              "division remainder");
  chDbgAssert((timer_clk / CH_CFG_ST_FREQUENCY) - 1U <= 0x00FFFFFFU,
              "prescaler range");

  /* Periodic systick mode, the Cortex-Mx internal systick timer is used
     in this mode.*/
  SysTick->LOAD = (timer_clk / CH_CFG_ST_FREQUENCY) - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;

  /* IRQ enabled.*/
  nvicSetSystemHandlerPriority(HANDLER_SYSTICK, RP_IRQ_SYSTICK_PRIORITY);
#endif /* CH_CFG_ST_TIMEDELTA == 0 */
}

#if (CH_CFG_ST_TIMEDELTA > 0) || defined(__DOXYGEN__)
#if (ST_LLD_NUM_ALARMS > 1) || defined(__DOXYGEN__)
/**
 * @brief   Enables an alarm interrupt on the invoking core.
 * @note    Must be called before any other alarm-related function.
 *
 * @param[in] alarm     alarm channel number (0..ST_LLD_NUM_ALARMS-1)
 *
 * @notapi
 */
void st_lld_bind_alarm_n(unsigned alarm) {

  nvicEnableVector(alarm_irqs[alarm].n, alarm_irqs[alarm].prio);
}
#endif /* ST_LLD_NUM_ALARMS > 1 */
#endif /* CH_CFG_ST_TIMEDELTA > 0 */

/** @} */
