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
 * @brief   Missed deadline window.
 * @details The TIMER alarms fire on an exact 32-bit match, a deadline that
 *          the counter has already passed is only matched after the counter
 *          wraps. A deadline less than this many ticks behind the counter is
 *          taken as missed and re-armed, any other deadline is in the future,
 *          up to 2^32 - @p RP_ST_MISSED_WINDOW ticks ahead. The value is the
 *          headroom the kernel keeps below @p VT_MAX_DELAY.
 */
#define RP_ST_MISSED_WINDOW                 0x10000U

/**
 * @brief   Static callback for alarm 0.
 */
#define ST_LLD_ALARM0_STATIC_CB()                                           \
  do {                                                                      \
    osalSysLockFromISR();                                                   \
    osalOsTimerHandlerI();                                                  \
    osalSysUnlockFromISR();                                                 \
  } while (false)

/**
 * @brief   Static callback for alarm 1.
 */
#define ST_LLD_ALARM1_STATIC_CB()                                           \
  do {                                                                      \
    osalSysLockFromISR();                                                   \
    osalOsTimerHandlerI();                                                  \
    osalSysUnlockFromISR();                                                 \
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


#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) &&                           \
    (OSAL_ST_RESOLUTION != 32)
#error "OSAL_ST_RESOLUTION must be 32 in OSAL_ST_MODE_FREERUNNING mode"
#endif

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING

#define RP_ST_USE_SYSTICK                   FALSE
#define RP_ST_USE_TIMER                     TRUE

#elif OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC

#define RP_ST_USE_SYSTICK                   TRUE
#define RP_ST_USE_TIMER                     FALSE

#else

#define RP_ST_USE_SYSTICK                   FALSE
#define RP_ST_USE_TIMER                     FALSE

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
#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
  void st_lld_bind_alarm_n(unsigned alarm);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)
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
 * @brief   Determines if a deadline has been missed.
 *
 * @param[in] abstime   the deadline
 * @param[in] now       the current counter value
 * @return              The deadline status.
 * @retval false        if the deadline is in the future.
 * @retval true         if the deadline is less than @p RP_ST_MISSED_WINDOW
 *                      ticks behind @p now.
 *
 * @notapi
 */
__STATIC_INLINE bool st_lld_is_deadline_missed(uint32_t abstime,
                                               uint32_t now) {

  return (uint32_t)(now - abstime) < RP_ST_MISSED_WINDOW;
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
  /* Re-arm if abstime has been missed to avoid ~71 min wait for counter
     wrap, see RP_ST_MISSED_WINDOW. */
  if (st_lld_is_deadline_missed((uint32_t)abstime, TIMER0->TIMERAWL)) {
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
     either it is outside the missed window (see RP_ST_MISSED_WINDOW) or
     the compare has genuinely fired (ARMED bit cleared by hardware). ARMED is sampled immediately
     before deciding to re-arm so a firing that lands mid-check is seen
     in the narrowest possible window; the residual race, firing between
     that read and the store, can at worst produce one spurious alarm
     interrupt which the ST layer tolerates - the opposite failure, not
     re-arming a missed deadline, is the one that must not happen. */
  while (true) {
    uint32_t now = TIMER0->TIMERAWL;

    if (!st_lld_is_deadline_missed(target, now)) {
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
  /* Re-arm if abstime has been missed to avoid ~71 min wait for counter
     wrap, see RP_ST_MISSED_WINDOW. */
  if (st_lld_is_deadline_missed((uint32_t)abstime, TIMER0->TIMERAWL)) {
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

    if (!st_lld_is_deadline_missed(target, now)) {
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
#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#endif /* HAL_ST_LLD_H */

/** @} */
