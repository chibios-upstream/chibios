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
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    RTCv2/hal_rtc_lld.c
 * @brief   STM32 RTCv2 low level driver source.
 *
 * @addtogroup HAL_RTC
 * @{
 */

#include "hal.h"

#if HAL_USE_RTC || defined(__DOXYGEN__)

#define RTC_TR_HT_OFFSET                    RTC_TR_HT_Pos
#define RTC_TR_HU_OFFSET                    RTC_TR_HU_Pos
#define RTC_TR_MNT_OFFSET                   RTC_TR_MNT_Pos
#define RTC_TR_MNU_OFFSET                   RTC_TR_MNU_Pos
#define RTC_TR_ST_OFFSET                    RTC_TR_ST_Pos
#define RTC_TR_SU_OFFSET                    RTC_TR_SU_Pos

#define RTC_DR_YT_OFFSET                    RTC_DR_YT_Pos
#define RTC_DR_YU_OFFSET                    RTC_DR_YU_Pos
#define RTC_DR_WDU_OFFSET                   RTC_DR_WDU_Pos
#define RTC_DR_MT_OFFSET                    RTC_DR_MT_Pos
#define RTC_DR_MU_OFFSET                    RTC_DR_MU_Pos
#define RTC_DR_DT_OFFSET                    RTC_DR_DT_Pos
#define RTC_DR_DU_OFFSET                    RTC_DR_DU_Pos

#define RTC_CR_BKP_OFFSET                   RTC_CR_BKP_Pos

static const hal_rtc_config_t rtc_default_config = {
  .cr                        = STM32_RTC_CR_INIT,
  .prer                      = STM32_RTC_PRER_BITS
};

hal_rtc_driver_c RTCD1;

static void rtc_wpr_unlock(hal_rtc_driver_c *rtcp) {

  rtcp->rtc->WPR = 0xCAU;
  rtcp->rtc->WPR = 0x53U;
}

static void rtc_wpr_lock(hal_rtc_driver_c *rtcp) {

  rtcp->rtc->WPR = 0xFFU;
}

/* ISR event flags and RSF are cleared by writing zero. Preserve flags which
   were not captured, INIT and all reserved bits, even if an event arrives
   between reading ISR and acknowledging an earlier event.*/
static void rtc_clear_flags(hal_rtc_driver_c *rtcp, uint32_t flags) {

  rtcp->rtc->ISR = (STM32_RTC_ISR_W0C_MASK & ~flags) |
                   (rtcp->rtc->ISR & RTC_ISR_INIT);
}

static void rtc_enter_init(hal_rtc_driver_c *rtcp) {

  rtcp->rtc->ISR = STM32_RTC_ISR_W0C_MASK | RTC_ISR_INIT;
  while ((rtcp->rtc->ISR & RTC_ISR_INITF) == 0U) {
  }
}

static void rtc_exit_init(hal_rtc_driver_c *rtcp) {

  rtcp->rtc->ISR = STM32_RTC_ISR_W0C_MASK;
}

static void rtc_disable_interrupt_sources(hal_rtc_driver_c *rtcp) {

#if defined(RTC_CR_ALRAIE)
  rtcp->rtc->CR &= ~RTC_CR_ALRAIE;
#endif
#if defined(RTC_CR_ALRBIE)
  rtcp->rtc->CR &= ~RTC_CR_ALRBIE;
#endif
#if defined(RTC_CR_WUTIE)
  rtcp->rtc->CR &= ~RTC_CR_WUTIE;
#endif
#if defined(RTC_CR_TSIE)
  rtcp->rtc->CR &= ~RTC_CR_TSIE;
#endif
  rtcp->rtc->TAMPCR &= ~STM32_RTC_TAMPCR_IRQ_MASK;
}

/* NVIC vectors belong to the platform IRQ layer and remain enabled across
   driver stop/start. Only the peripheral interrupt sources and EXTI masks
   are disabled here.*/
static void rtc_disable_exti(void) {

#if defined(STM32_RTC_ALARM_EXTI) && defined(STM32_RTC_TAMP_STAMP_EXTI) && \
    defined(STM32_RTC_WKUP_EXTI)
  extiEnableGroup1(EXTI_MASK1(STM32_RTC_ALARM_EXTI) |
                   EXTI_MASK1(STM32_RTC_TAMP_STAMP_EXTI) |
                   EXTI_MASK1(STM32_RTC_WKUP_EXTI),
                   EXTI_MODE_DISABLED);
#endif
  STM32_RTC_CLEAR_ALL_EXTI();
}

static void rtc_decode_time(uint32_t tr, rtc_datetime_t *timespec) {
  uint32_t n;

  n  = ((tr >> RTC_TR_HT_OFFSET) & 3U)   * 36000000U;
  n += ((tr >> RTC_TR_HU_OFFSET) & 15U)  * 3600000U;
  n += ((tr >> RTC_TR_MNT_OFFSET) & 7U)  * 600000U;
  n += ((tr >> RTC_TR_MNU_OFFSET) & 15U) * 60000U;
  n += ((tr >> RTC_TR_ST_OFFSET) & 7U)   * 10000U;
  n += ((tr >> RTC_TR_SU_OFFSET) & 15U)  * 1000U;
  timespec->millisecond = n;
}

static void rtc_decode_date(uint32_t dr, rtc_datetime_t *timespec) {

  timespec->year = (uint16_t)((((dr >> RTC_DR_YT_OFFSET) & 15U) * 10U) +
                              ((dr >> RTC_DR_YU_OFFSET) & 15U));
  timespec->month = (uint8_t)((((dr >> RTC_DR_MT_OFFSET) & 1U) * 10U) +
                              ((dr >> RTC_DR_MU_OFFSET) & 15U));
  timespec->day = (uint8_t)((((dr >> RTC_DR_DT_OFFSET) & 3U) * 10U) +
                            ((dr >> RTC_DR_DU_OFFSET) & 15U));
  timespec->dayofweek = (uint8_t)((dr >> RTC_DR_WDU_OFFSET) & 7U);
}

static uint32_t rtc_encode_time(const rtc_datetime_t *timespec) {
  uint32_t n, tr = 0U;

  n = timespec->millisecond / 1000U;
  tr |= ((n % 10U) << RTC_TR_SU_OFFSET);
  n /= 10U;
  tr |= ((n % 6U) << RTC_TR_ST_OFFSET);
  n /= 6U;
  tr |= ((n % 10U) << RTC_TR_MNU_OFFSET);
  n /= 10U;
  tr |= ((n % 6U) << RTC_TR_MNT_OFFSET);
  n /= 6U;
  tr |= ((n % 10U) << RTC_TR_HU_OFFSET);
  n /= 10U;
  tr |= (n << RTC_TR_HT_OFFSET);

  return tr;
}

static uint32_t rtc_encode_date(const rtc_datetime_t *timespec) {
  uint32_t n, dr = 0U;

  n = timespec->year;
  dr |= ((n % 10U) << RTC_DR_YU_OFFSET);
  n /= 10U;
  dr |= ((n % 10U) << RTC_DR_YT_OFFSET);

  n = timespec->month;
  dr |= ((n % 10U) << RTC_DR_MU_OFFSET);
  n /= 10U;
  dr |= ((n % 10U) << RTC_DR_MT_OFFSET);

  n = timespec->day;
  dr |= ((n % 10U) << RTC_DR_DU_OFFSET);
  n /= 10U;
  dr |= ((n % 10U) << RTC_DR_DT_OFFSET);
  dr |= ((uint32_t)timespec->dayofweek << RTC_DR_WDU_OFFSET);

  return dr;
}

void rtc_lld_serve_interrupt(void) {
  uint32_t cr, isr, tampcr, clear;
  rtceventflags_t flags = 0U;
  syssts_t sts;

  /* Serialize the three RTC vectors, which can have different priorities.
     Never hold this lock while invoking the application callback.*/
  sts = chSysGetStatusAndLockX();
  cr = RTCD1.rtc->CR;
  isr = RTCD1.rtc->ISR;
  tampcr = RTCD1.rtc->TAMPCR;
  clear = isr & STM32_RTC_ISR_EVENT_MASK;
  if ((clear & (RTC_ISR_TSF | RTC_ISR_ITSF)) != 0U) {
    clear |= RTC_ISR_TSF | RTC_ISR_ITSF;
  }
  rtc_wpr_unlock(&RTCD1);
  rtc_clear_flags(&RTCD1, clear);
  rtc_wpr_lock(&RTCD1);
  STM32_RTC_CLEAR_ALL_EXTI();

  if (((cr & RTC_CR_ALRAIE) != 0U) && ((isr & RTC_ISR_ALRAF) != 0U)) {
    flags |= RTC_FLAGS_ALARM_A;
  }
  if (((cr & RTC_CR_ALRBIE) != 0U) && ((isr & RTC_ISR_ALRBF) != 0U)) {
    flags |= RTC_FLAGS_ALARM_B;
  }
  if ((cr & RTC_CR_TSIE) != 0U) {
    if ((isr & (RTC_ISR_TSF | RTC_ISR_ITSF)) != 0U) {
      flags |= RTC_FLAGS_TS;
    }
    if ((isr & RTC_ISR_TSOVF) != 0U) {
      flags |= RTC_FLAGS_TS_OVF;
    }
  }
  if (((cr & RTC_CR_WUTIE) != 0U) && ((isr & RTC_ISR_WUTF) != 0U)) {
    flags |= RTC_FLAGS_WAKEUP;
  }
  if (((tampcr & (RTC_TAMPCR_TAMPIE | RTC_TAMPCR_TAMP1IE)) != 0U) &&
      ((isr & RTC_ISR_TAMP1F) != 0U)) {
    flags |= RTC_FLAGS_TAMP1;
  }
#if STM32_RTC_HAS_TAMP2
  if (((tampcr & (RTC_TAMPCR_TAMPIE | RTC_TAMPCR_TAMP2IE)) != 0U) &&
      ((isr & RTC_ISR_TAMP2F) != 0U)) {
    flags |= RTC_FLAGS_TAMP2;
  }
#endif
  if (((tampcr & (RTC_TAMPCR_TAMPIE | RTC_TAMPCR_TAMP3IE)) != 0U) &&
      ((isr & RTC_ISR_TAMP3F) != 0U)) {
    flags |= RTC_FLAGS_TAMP3;
  }
  RTCD1.events |= flags;
  chSysRestoreStatusX(sts);

  if ((flags != 0U) && (RTCD1.cb != NULL)) {
    RTCD1.cb(&RTCD1);
  }
}

void rtc_lld_init(void) {

  rtcObjectInit(&RTCD1);
  RTCD1.rtc = RTC;
}

msg_t rtc_lld_start(hal_rtc_driver_c *rtcp) {
  const hal_rtc_config_t *cfg;

  cfg = (const hal_rtc_config_t *)rtcp->config;
  if (cfg == NULL) {
    cfg = rtc_lld_selcfg(rtcp, 0U);
  }
  if (cfg == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }

  rtcp->config = cfg;

  rtc_wpr_unlock(rtcp);

  if ((rtcp->rtc->ISR & RTC_ISR_INITS) == 0U) {
    rtc_enter_init(rtcp);
    rtcp->rtc->CR = (cfg->cr & STM32_RTC_CR_MASK) | RTC_CR_BYPSHAD;
    rtcp->rtc->PRER = cfg->prer & 0x7FFFU;
    rtcp->rtc->PRER = cfg->prer & STM32_RTC_PRER_MASK;
    rtcp->rtc->TAMPCR = STM32_RTC_TAMPCR_INIT & STM32_RTC_TAMPCR_MASK;
    rtc_exit_init(rtcp);
  }
  else {
    /* An initialized backup domain retains its calendar, prescalers, alarm
       and wakeup settings. Only select the direct calendar read path.*/
    rtcp->rtc->CR |= RTC_CR_BYPSHAD;
  }
  rtc_wpr_lock(rtcp);

  rtcp->events = 0U;
  STM32_RTC_ENABLE_ALL_EXTI();

  return HAL_RET_SUCCESS;
}

void rtc_lld_stop(hal_rtc_driver_c *rtcp) {

  rtc_wpr_unlock(rtcp);
  rtc_disable_interrupt_sources(rtcp);
  rtc_wpr_lock(rtcp);
  rtc_disable_exti();
  rtcp->cb = NULL;
  rtcp->events = 0U;
}

const hal_rtc_config_t *rtc_lld_setcfg(hal_rtc_driver_c *rtcp,
                                       const hal_rtc_config_t *config) {
  (void)rtcp;

  if (config == NULL) {
    return rtc_lld_selcfg(rtcp, 0U);
  }

  return config;
}

const hal_rtc_config_t *rtc_lld_selcfg(hal_rtc_driver_c *rtcp, unsigned cfgnum) {
  (void)rtcp;

  if (cfgnum != 0U) {
    return NULL;
  }

  return &rtc_default_config;
}

void rtc_lld_set_callback(hal_rtc_driver_c *rtcp, drv_cb_t cb) {

  (void)rtcp;
  (void)cb;
}

msg_t rtc_lld_set_datetime(hal_rtc_driver_c *rtcp,
                           const rtc_datetime_t *timespec) {
  uint32_t tr, dr;
  syssts_t sts;

  tr = rtc_encode_time(timespec);
  dr = rtc_encode_date(timespec);

  sts = chSysGetStatusAndLockX();
  rtc_wpr_unlock(rtcp);
  rtc_enter_init(rtcp);
  rtcp->rtc->TR = tr;
  rtcp->rtc->DR = dr;
  rtcp->rtc->CR = (rtcp->rtc->CR & ~(1U << RTC_CR_BKP_OFFSET)) |
                  ((uint32_t)timespec->dstflag << RTC_CR_BKP_OFFSET);
  rtc_exit_init(rtcp);
  rtc_wpr_lock(rtcp);
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}

msg_t rtc_lld_get_datetime(hal_rtc_driver_c *rtcp, rtc_datetime_t *timespec) {
  uint32_t cr, dr, tr, ssr, prev_dr, prev_tr, prev_ssr;
  uint32_t subs;
  syssts_t sts;

  sts = chSysGetStatusAndLockX();
  ssr = 0U;
  tr = 0U;
  dr = 0U;
  do {
    prev_ssr = ssr;
    prev_tr = tr;
    prev_dr = dr;
    ssr = rtcp->rtc->SSR;
    tr = rtcp->rtc->TR;
    dr = rtcp->rtc->DR;
  } while ((ssr != prev_ssr) || (tr != prev_tr) || (dr != prev_dr));
  cr = rtcp->rtc->CR;
  chSysRestoreStatusX(sts);

  rtc_decode_time(tr, timespec);
  subs = ((((rtcp->rtc->PRER & RTC_PRER_PREDIV_S_Msk) >> RTC_PRER_PREDIV_S_Pos) - ssr) * 1000U) /
         ((((rtcp->rtc->PRER & RTC_PRER_PREDIV_S_Msk) >> RTC_PRER_PREDIV_S_Pos) + 1U));
  timespec->millisecond += subs;
  rtc_decode_date(dr, timespec);
  timespec->dstflag = (uint8_t)((cr >> RTC_CR_BKP_OFFSET) & 1U);

  return HAL_RET_SUCCESS;
}

msg_t rtc_lld_set_alarm(hal_rtc_driver_c *rtcp,
                        rtcalarm_t alarm,
                        const rtc_alarm_t *alarmspec) {
  syssts_t sts;

  if (alarm >= (rtcalarm_t)RTC_ALARMS) {
    return HAL_RET_CONFIG_ERROR;
  }

  sts = chSysGetStatusAndLockX();
  rtc_wpr_unlock(rtcp);
  if (alarm == 0U) {
    if (alarmspec != NULL) {
      rtcp->rtc->CR &= ~RTC_CR_ALRAE;
#if defined(RTC_ISR_ALRAWF)
      while ((rtcp->rtc->ISR & RTC_ISR_ALRAWF) == 0U) {
      }
#endif
      rtcp->rtc->ALRMAR = alarmspec->alrmr;
      rtcp->rtc->CR |= RTC_CR_ALRAE;
      rtcp->rtc->CR |= RTC_CR_ALRAIE;
    }
    else {
      rtcp->rtc->CR &= ~RTC_CR_ALRAIE;
      rtcp->rtc->CR &= ~RTC_CR_ALRAE;
    }
  }
#if RTC_ALARMS > 1
  else {
    if (alarmspec != NULL) {
      rtcp->rtc->CR &= ~RTC_CR_ALRBE;
#if defined(RTC_ISR_ALRBWF)
      while ((rtcp->rtc->ISR & RTC_ISR_ALRBWF) == 0U) {
      }
#endif
      rtcp->rtc->ALRMBR = alarmspec->alrmr;
      rtcp->rtc->CR |= RTC_CR_ALRBE;
      rtcp->rtc->CR |= RTC_CR_ALRBIE;
    }
    else {
      rtcp->rtc->CR &= ~RTC_CR_ALRBIE;
      rtcp->rtc->CR &= ~RTC_CR_ALRBE;
    }
  }
#endif
  rtc_wpr_lock(rtcp);
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}

msg_t rtc_lld_get_alarm(hal_rtc_driver_c *rtcp,
                        rtcalarm_t alarm,
                        rtc_alarm_t *alarmspec) {

  if (alarm >= (rtcalarm_t)RTC_ALARMS) {
    return HAL_RET_CONFIG_ERROR;
  }

  if (alarm == 0U) {
    alarmspec->alrmr = rtcp->rtc->ALRMAR;
  }
#if RTC_ALARMS > 1
  else {
    alarmspec->alrmr = rtcp->rtc->ALRMBR;
  }
#endif

  return HAL_RET_SUCCESS;
}

#if RTC_SUPPORTS_PERIODIC_WAKEUP
/**
 * @brief   Programs the wakeup timer after disabling it and waiting for WUTWF.
 * @note    Cached events are not discarded by this operation.
 */
msg_t rtc_lld_set_periodic_wakeup(hal_rtc_driver_c *rtcp,
                                 const rtc_wakeup_t *wakeupspec) {
  syssts_t sts;

  if ((wakeupspec != NULL) &&
      (((wakeupspec->wutr & ~0x0007FFFFU) != 0U) ||
       (wakeupspec->wutr == 0x00030000U))) {
    return HAL_RET_CONFIG_ERROR;
  }

  sts = chSysGetStatusAndLockX();
  rtc_wpr_unlock(rtcp);
  rtcp->rtc->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
  while ((rtcp->rtc->ISR & RTC_ISR_WUTWF) == 0U) {
  }
  rtc_clear_flags(rtcp, RTC_ISR_WUTF);

  if (wakeupspec != NULL) {
    rtcp->rtc->WUTR = wakeupspec->wutr & 0xFFFFU;
    rtcp->rtc->CR = (rtcp->rtc->CR & ~RTC_CR_WUCKSEL) |
                    (wakeupspec->wutr >> 16);
    rtcp->rtc->CR |= RTC_CR_WUTIE | RTC_CR_WUTE;
  }
  rtc_wpr_lock(rtcp);
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}

msg_t rtc_lld_get_periodic_wakeup(hal_rtc_driver_c *rtcp,
                                 rtc_wakeup_t *wakeupspec) {
  syssts_t sts;

  sts = chSysGetStatusAndLockX();
  wakeupspec->wutr = (rtcp->rtc->WUTR & 0xFFFFU) |
                     ((rtcp->rtc->CR & RTC_CR_WUCKSEL) << 16);
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}
#endif /* RTC_SUPPORTS_PERIODIC_WAKEUP */

#endif /* HAL_USE_RTC */

/** @} */
