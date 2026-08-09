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
 * @file    TIMERv1/hal_st_lld.h
 * @brief   ST Driver subsystem low level driver header.
 * @details This header is designed to be include-able without having to
 *          include other files from the HAL.
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of supported alarms.
 */
#define ST_LLD_NUM_ALARMS                   RP_ST_NUM_ALARMS

/**
 * @brief   Static callback for alarm 0.
 */
#define ST_LLD_ALARM0_STATIC_CB()                                           \
  do {                                                                      \
    chSysLockFromISR();                                                     \
    chSysTimerHandlerI();                                                   \
    chSysUnlockFromISR();                                                   \
  } while (false)

/**
 * @brief   Static callback for alarm 1.
 */
#define ST_LLD_ALARM1_STATIC_CB()                                           \
  do {                                                                      \
    chSysLockFromISR();                                                     \
    chSysTimerHandlerI();                                                   \
    chSysUnlockFromISR();                                                   \
  } while (false)

/**
 * @brief   Defined for inclusion of the IRQ-binding API.
 */
#define ST_LLD_MULTICORE_SUPPORT

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   SysTick timer IRQ priority.
 */
#if !defined(RP_IRQ_SYSTICK_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_SYSTICK_PRIORITY             2
#endif

/**
 * @brief   TIMER0 alarm 0 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER0_ALARM0_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER0_ALARM0_PRIORITY        2
#endif

/**
 * @brief   TIMER0 alarm 1 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER0_ALARM1_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER0_ALARM1_PRIORITY        2
#endif

/**
 * @brief   TIMER0 alarm 2 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER0_ALARM2_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER0_ALARM2_PRIORITY        2
#endif

/**
 * @brief   TIMER0 alarm 3 IRQ priority.
 */
#if !defined(RP_IRQ_TIMER0_ALARM3_PRIORITY) || defined(__DOXYGEN__)
#define RP_IRQ_TIMER0_ALARM3_PRIORITY        2
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (CH_CFG_ST_TIMEDELTA > 0) && (CH_CFG_ST_RESOLUTION != 32)
#error "CH_CFG_ST_RESOLUTION must be 32 in free running mode"
#endif

#if CH_CFG_ST_TIMEDELTA > 0

#define RP_ST_USE_SYSTICK                   FALSE
#define RP_ST_USE_TIMER                     TRUE

#elif CH_CFG_ST_TIMEDELTA == 0

#define RP_ST_USE_SYSTICK                   TRUE
#define RP_ST_USE_TIMER                     FALSE

#else

#define RP_ST_USE_SYSTICK                   FALSE
#define RP_ST_USE_TIMER                     FALSE

#endif

#if RP_ST_USE_SYSTICK == TRUE
#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_SYSTICK_PRIORITY)
#error "invalid IRQ priority assigned to SysTick"
#endif
#endif

#if RP_ST_USE_TIMER == TRUE
#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_TIMER0_ALARM0_PRIORITY)
#error "invalid IRQ priority assigned to TIMER0 alarm 0"
#endif

#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_TIMER0_ALARM1_PRIORITY)
#error "invalid IRQ priority assigned to TIMER0 alarm 1"
#endif

#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_TIMER0_ALARM2_PRIORITY)
#error "invalid IRQ priority assigned to TIMER0 alarm 2"
#endif

#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_TIMER0_ALARM3_PRIORITY)
#error "invalid IRQ priority assigned to TIMER0 alarm 3"
#endif
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void st_lld_init(void);
  void st_lld_bind(void);
#if CH_CFG_ST_TIMEDELTA > 0
  void st_lld_bind_alarm_n(unsigned alarm);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

#if (CH_CFG_ST_TIMEDELTA > 0) || defined(__DOXYGEN__)
/**
 * @brief   Returns the time counter value.
 *
 * @return              The counter value.
 *
 * @notapi
 */
__STATIC_INLINE systime_t st_lld_get_counter(void) {

  return (systime_t)TIMER0->TIMERAWL;
}

/**
 * @brief   Starts the alarm.
 * @note    Makes sure that no spurious alarms are triggered after
 *          this call.
 * @note    Uses atomic SET/CLR registers for dual-core safe INTE access.
 *
 * @param[in] abstime   the time to be set for the first alarm
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_start_alarm(systime_t abstime) {

  TIMER0->ALARM[0]       = (uint32_t)abstime;
  TIMER0->INTR           = (1U << 0);
  TIMER0->SET.INTE       = (1U << 0);
  /* Re-arm if abstime already past to avoid ~71 min wait for counter wrap. */
  if ((int32_t)(abstime - TIMER0->TIMERAWL) <= 0) {
    TIMER0->ALARM[0]     = TIMER0->TIMERAWL + 2U;
  }
}

/**
 * @brief   Stops the alarm interrupt.
 * @note    Uses atomic CLR register for dual-core safe INTE access.
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_stop_alarm(void) {

  TIMER0->CLR.INTE       = (1U << 0);
}

/**
 * @brief   Sets the alarm time.
 *
 * @param[in] abstime   the time to be set for the next alarm
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_set_alarm(systime_t abstime) {
  uint32_t target = (uint32_t)abstime;

  TIMER0->ALARM[0] = target;
  /* Verify-and-retry: the counter can pass the programmed value between
     the check and the store (an interrupt or bus stall in the window
     recreates the ~71 min wrap wait), so the deadline is re-armed until
     either it is provably in the future or the compare has genuinely
     fired (ARMED bit cleared by hardware). ARMED is sampled immediately
     before deciding to re-arm so a firing that lands mid-check is seen
     in the narrowest possible window; the residual race, firing between
     that read and the store, can at worst produce one spurious alarm
     interrupt which the ST layer tolerates - the opposite failure, not
     re-arming a missed deadline, is the one that must not happen. */
  while (true) {
    uint32_t now = TIMER0->TIMERAWL;

    if ((int32_t)(target - now) > 0) {
      break;
    }
    if ((TIMER0->ARMED & 1U) == 0U) {
      break;
    }
    target = now + 2U;
    TIMER0->ALARM[0] = target;
  }
}

/**
 * @brief   Returns the current alarm time.
 *
 * @return              The currently set alarm time.
 *
 * @notapi
 */
__STATIC_INLINE systime_t st_lld_get_alarm(void) {

  return (systime_t)TIMER0->ALARM[0];
}

/**
 * @brief   Determines if the alarm is active.
 *
 * @return              The alarm status.
 * @retval false        if the alarm is not active.
 * @retval true         is the alarm is active
 *
 * @notapi
 */
__STATIC_INLINE bool st_lld_is_alarm_active(void) {

  return (bool)((TIMER0->INTE & (1U << 0)) != 0U);
}

#if (ST_LLD_NUM_ALARMS > 1) || defined(__DOXYGEN__)
/**
 * @brief   Starts an alarm.
 * @note    Makes sure that no spurious alarms are triggered after
 *          this call.
 * @note    This functionality is only available in free running mode, the
 *          behavior in periodic mode is undefined.
 *
 * @param[in] abstime   the time to be set for the first alarm
 * @param[in] alarm     alarm channel number
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_start_alarm_n(unsigned alarm, systime_t abstime) {

  TIMER0->ALARM[alarm]   = (uint32_t)abstime;
  TIMER0->INTR           = (1U << alarm);
  TIMER0->SET.INTE       = (1U << alarm);
  /* Re-arm if abstime already past to avoid ~71 min wait for counter wrap. */
  if ((int32_t)(abstime - TIMER0->TIMERAWL) <= 0) {
    TIMER0->ALARM[alarm] = TIMER0->TIMERAWL + 2U;
  }
}

/**
 * @brief   Stops an alarm interrupt.
 * @note    This functionality is only available in free running mode, the
 *          behavior in periodic mode is undefined.
 *
 * @param[in] alarm     alarm channel number
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_stop_alarm_n(unsigned alarm) {

  TIMER0->CLR.INTE       = (1U << alarm);
}

/**
 * @brief   Sets an alarm time.
 * @note    This functionality is only available in free running mode, the
 *          behavior in periodic mode is undefined.
 *
 * @param[in] alarm     alarm channel number
 * @param[in] abstime   the time to be set for the next alarm
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_set_alarm_n(unsigned alarm, systime_t abstime) {
  uint32_t target = (uint32_t)abstime;

  TIMER0->ALARM[alarm] = target;
  /* Verify-and-retry, see st_lld_set_alarm(). */
  while (true) {
    uint32_t now = TIMER0->TIMERAWL;

    if ((int32_t)(target - now) > 0) {
      break;
    }
    if ((TIMER0->ARMED & (1U << alarm)) == 0U) {
      break;
    }
    target = now + 2U;
    TIMER0->ALARM[alarm] = target;
  }
}

/**
 * @brief   Returns an alarm current time.
 * @note    This functionality is only available in free running mode, the
 *          behavior in periodic mode is undefined.
 *
 * @param[in] alarm     alarm channel number
 * @return              The currently set alarm time.
 *
 * @notapi
 */
__STATIC_INLINE systime_t st_lld_get_alarm_n(unsigned alarm) {

  return (systime_t)TIMER0->ALARM[alarm];
}

/**
 * @brief   Determines if an alarm is active.
 *
 * @param[in] alarm     alarm channel number
 * @return              The alarm status.
 * @retval false        if the alarm is not active.
 * @retval true         is the alarm is active
 *
 * @notapi
 */
static inline bool st_lld_is_alarm_active_n(unsigned alarm) {

  return (bool)((TIMER0->INTE & (1U << alarm)) != 0U);
}
#endif /* ST_LLD_NUM_ALARMS > 1 */
#endif /* CH_CFG_ST_TIMEDELTA > 0 */

#endif /* HAL_ST_LLD_H */

/** @} */
