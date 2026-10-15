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
 * @file    TIMERv1/hal_gpt_lld.h
 * @brief   RP GPT subsystem low level driver header.
 * @details GPTD1 to GPTD4 use the alarms 0 to 3 of TIMER1, which counts
 *          microseconds. A continuous period is produced by re-arming the
 *          alarm on each event.
 *
 * @addtogroup GPT
 * @{
 */

#ifndef HAL_GPT_LLD_H
#define HAL_GPT_LLD_H

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   TIMER1 counter frequency.
 */
#define RP_GPT_TIMER_FREQUENCY              1000000U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP configuration options
 * @{
 */
/**
 * @brief   GPTD1 driver enable switch.
 * @details If set to @p TRUE the support for GPTD1 is included, it uses
 *          TIMER1 alarm 0.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_GPT_USE_GPT1) || defined(__DOXYGEN__)
#define RP_GPT_USE_GPT1                     FALSE
#endif

/**
 * @brief   GPTD2 driver enable switch.
 * @details If set to @p TRUE the support for GPTD2 is included, it uses
 *          TIMER1 alarm 1.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_GPT_USE_GPT2) || defined(__DOXYGEN__)
#define RP_GPT_USE_GPT2                     FALSE
#endif

/**
 * @brief   GPTD3 driver enable switch.
 * @details If set to @p TRUE the support for GPTD3 is included, it uses
 *          TIMER1 alarm 2.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_GPT_USE_GPT3) || defined(__DOXYGEN__)
#define RP_GPT_USE_GPT3                     FALSE
#endif

/**
 * @brief   GPTD4 driver enable switch.
 * @details If set to @p TRUE the support for GPTD4 is included, it uses
 *          TIMER1 alarm 3.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_GPT_USE_GPT4) || defined(__DOXYGEN__)
#define RP_GPT_USE_GPT4                     FALSE
#endif

/**
 * @brief   TIMER1 alarm 0 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER1_ALARM0_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER1_ALARM0_PRIORITY       3
#endif

/**
 * @brief   TIMER1 alarm 1 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER1_ALARM1_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER1_ALARM1_PRIORITY       3
#endif

/**
 * @brief   TIMER1 alarm 2 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER1_ALARM2_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER1_ALARM2_PRIORITY       3
#endif

/**
 * @brief   TIMER1 alarm 3 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER1_ALARM3_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER1_ALARM3_PRIORITY       3
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(RP_HAS_TIMER1)
#define RP_HAS_TIMER1                       FALSE
#endif

#if ((RP_GPT_USE_GPT1 == TRUE) || (RP_GPT_USE_GPT2 == TRUE) ||              \
     (RP_GPT_USE_GPT3 == TRUE) || (RP_GPT_USE_GPT4 == TRUE)) &&             \
    (RP_HAS_TIMER1 == FALSE)
#error "GPT drivers require TIMER1, not present in the selected device"
#endif

#if (RP_GPT_USE_GPT1 == FALSE) && (RP_GPT_USE_GPT2 == FALSE) &&             \
    (RP_GPT_USE_GPT3 == FALSE) && (RP_GPT_USE_GPT4 == FALSE)
#error "GPT driver activated but no GPT peripheral assigned"
#endif

#if (RP_GPT_USE_GPT1 == TRUE) &&                                            \
    !OSAL_IRQ_IS_VALID_PRIORITY(RP_IRQ_TIMER1_ALARM0_PRIORITY)
#error "Invalid IRQ priority assigned to GPTD1"
#endif

#if (RP_GPT_USE_GPT2 == TRUE) &&                                            \
    !OSAL_IRQ_IS_VALID_PRIORITY(RP_IRQ_TIMER1_ALARM1_PRIORITY)
#error "Invalid IRQ priority assigned to GPTD2"
#endif

#if (RP_GPT_USE_GPT3 == TRUE) &&                                            \
    !OSAL_IRQ_IS_VALID_PRIORITY(RP_IRQ_TIMER1_ALARM2_PRIORITY)
#error "Invalid IRQ priority assigned to GPTD3"
#endif

#if (RP_GPT_USE_GPT4 == TRUE) &&                                            \
    !OSAL_IRQ_IS_VALID_PRIORITY(RP_IRQ_TIMER1_ALARM3_PRIORITY)
#error "Invalid IRQ priority assigned to GPTD4"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   GPT frequency type.
 */
typedef uint32_t gptfreq_t;

/**
 * @brief   GPT counter type.
 */
typedef uint32_t gptcnt_t;

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Timer clock in Hz.
   * @note    It must be @p RP_GPT_TIMER_FREQUENCY or an exact divider of
   *          it, the low level uses assertions to catch invalid values.
   */
  gptfreq_t                 frequency;
  /**
   * @brief   Timer callback pointer.
   * @note    This callback is invoked on GPT counter events.
   */
  gptcallback_t             callback;
  /* End of the mandatory fields.*/
} GPTConfig;

/**
 * @brief   Structure representing a GPT driver.
 */
struct GPTDriver {
  /**
   * @brief Driver state.
   */
  gptstate_t                state;
  /**
   * @brief Current configuration data.
   */
  const GPTConfig           *config;
#if defined(GPT_DRIVER_EXT_FIELDS)
  GPT_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief   TIMER1 alarm used by this driver.
   */
  uint32_t                  alarm;
  /**
   * @brief   Interrupt number of the alarm.
   */
  uint32_t                  irqn;
  /**
   * @brief   Interrupt priority of the alarm.
   */
  uint32_t                  irqprio;
  /**
   * @brief   TIMER1 ticks per GPT tick.
   */
  uint32_t                  scale;
  /**
   * @brief   Current period in TIMER1 ticks.
   */
  uint32_t                  period;
  /**
   * @brief   Current deadline in TIMER1 ticks.
   */
  uint32_t                  deadline;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns the interval of GPT peripheral.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @return              The current interval.
 *
 * @notapi
 */
#define gpt_lld_get_interval(gptp) ((gptcnt_t)((gptp)->period / (gptp)->scale))

/**
 * @brief   Returns the counter value of GPT peripheral.
 * @details The counter counts up from zero to the interval within the
 *          current cycle.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @return              The current counter value.
 *
 * @notapi
 */
#define gpt_lld_get_counter(gptp)                                           \
  ((gptcnt_t)((TIMER1->TIMERAWL - ((gptp)->deadline - (gptp)->period)) /    \
              (gptp)->scale))

/**
 * @brief   Changes the interval of GPT peripheral.
 * @details This function changes the interval of a running GPT unit.
 * @pre     The GPT unit must have been activated using @p gptStart().
 * @pre     The GPT unit must have been running in continuous mode using
 *          @p gptStartContinuous().
 * @post    The GPT unit interval is changed to the new value.
 * @note    The function has effect at the next cycle start.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @param[in] interval  new cycle time in timer ticks
 * @notapi
 */
#define gpt_lld_change_interval(gptp, interval) {                           \
  (gptp)->period = (uint32_t)(interval) * (gptp)->scale;                    \
}

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_GPT_USE_GPT1 == TRUE) && !defined(__DOXYGEN__)
extern GPTDriver GPTD1;
#endif

#if (RP_GPT_USE_GPT2 == TRUE) && !defined(__DOXYGEN__)
extern GPTDriver GPTD2;
#endif

#if (RP_GPT_USE_GPT3 == TRUE) && !defined(__DOXYGEN__)
extern GPTDriver GPTD3;
#endif

#if (RP_GPT_USE_GPT4 == TRUE) && !defined(__DOXYGEN__)
extern GPTDriver GPTD4;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void gpt_lld_init(void);
  void gpt_lld_start(GPTDriver *gptp);
  void gpt_lld_stop(GPTDriver *gptp);
  void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t interval);
  void gpt_lld_stop_timer(GPTDriver *gptp);
  void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GPT == TRUE */

#endif /* HAL_GPT_LLD_H */

/** @} */
