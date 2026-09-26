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
 * @file    RTCv2/hal_rtc_lld.h
 * @brief   STM32 RTCv2 low level driver header.
 *
 * @addtogroup HAL_RTC
 * @{
 */

#ifndef HAL_RTC_LLD_H
#define HAL_RTC_LLD_H

#if HAL_USE_RTC || defined(__DOXYGEN__)

#define RTC_ALARMS                          STM32_RTC_NUM_ALARMS

/**
 * @name    STM32 periodic-wakeup encoding
 * @details rtc_wakeup_t.wutr contains the 16-bit reload value in bits 15:0
 *          and CR.WUCKSEL in bits 18:16. Selectors 0..3 use RTCCLK divided by
 *          16, 8, 4, 2; 4..5 use ck_spre; 6..7 use ck_spre with 65536 added
 *          to the reload value. The period is (reload + 1) selected ticks.
 *          Selector 3 with reload zero is forbidden.
 * @{
 */
#define RTC_SUPPORTS_PERIODIC_WAKEUP        STM32_RTC_HAS_PERIODIC_WAKEUPS
#define RTC_WAKEUP(sel, reload)             (((sel) << 16) | (reload))
/** @} */

#define RTC_PRER(a, s)                      ((((a) - 1U) << 16) | ((s) - 1U))

#define RTC_ALRM_MSK4                       (1U << 31)
#define RTC_ALRM_WDSEL                      (1U << 30)
#define RTC_ALRM_DT(n)                      ((n) << 28)
#define RTC_ALRM_DU(n)                      ((n) << 24)
#define RTC_ALRM_MSK3                       (1U << 23)
#define RTC_ALRM_HT(n)                      ((n) << 20)
#define RTC_ALRM_HU(n)                      ((n) << 16)
#define RTC_ALRM_MSK2                       (1U << 15)
#define RTC_ALRM_MNT(n)                     ((n) << 12)
#define RTC_ALRM_MNU(n)                     ((n) << 8)
#define RTC_ALRM_MSK1                       (1U << 7)
#define RTC_ALRM_ST(n)                      ((n) << 4)
#define RTC_ALRM_SU(n)                      ((n) << 0)

#if !defined(STM32_EXTI_REQUIRED)
#define STM32_EXTI_REQUIRED
#endif

#if !defined(STM32_RTC_PRESA_VALUE) || defined(__DOXYGEN__)
#define STM32_RTC_PRESA_VALUE               32U
#endif

#if !defined(STM32_RTC_PRESS_VALUE) || defined(__DOXYGEN__)
#define STM32_RTC_PRESS_VALUE               1024U
#endif

#if !defined(STM32_RTC_CR_INIT) || defined(__DOXYGEN__)
#define STM32_RTC_CR_INIT                   0U
#endif

#if !defined(STM32_RTC_TAMPCR_INIT) || defined(__DOXYGEN__)
#define STM32_RTC_TAMPCR_INIT               0U
#endif

#if HAL_USE_RTC && !STM32_HAS_RTC
#error "RTC not present in the selected device"
#endif

#if defined(STM32_RTC_CK) && !defined(STM32_RTCCLK)
#define STM32_RTCCLK                        STM32_RTC_CK
#endif

#if !defined(STM32_RTCCLK)
#error "RTC clock not exported by HAL layer"
#endif

#if defined(STM32_RTC_FREQ)
#if STM32_RTC_FREQ == 0U
#error "RTC has no clock source selected"
#endif
#elif STM32_RTCCLK == 0
#error "RTC has no clock source selected"
#endif

#if defined(STM32_PCLK1_FREQ) && defined(STM32_RTC_FREQ)
#if STM32_PCLK1_FREQ < (STM32_RTC_FREQ * 7U)
#error "STM32_PCLK1 frequency is too low for RTC"
#endif
#elif STM32_PCLK1 < (STM32_RTCCLK * 7)
#error "STM32_PCLK1 frequency is too low for RTC"
#endif

#define STM32_RTC_PRER_BITS                 RTC_PRER(STM32_RTC_PRESA_VALUE,   \
                                                     STM32_RTC_PRESS_VALUE)

#define rtc_lld_driver_fields                                               \
  RTC_TypeDef               *rtc

#define rtc_lld_config_fields

#ifdef __cplusplus
extern "C" {
#endif
#if RTC_SUPPORTS_PERIODIC_WAKEUP
  msg_t rtc_lld_set_periodic_wakeup(hal_rtc_driver_c *rtcp,
                                   const rtc_wakeup_t *wakeupspec);
  msg_t rtc_lld_get_periodic_wakeup(hal_rtc_driver_c *rtcp,
                                   rtc_wakeup_t *wakeupspec);
#endif
  void rtc_lld_init(void);
  msg_t rtc_lld_start(hal_rtc_driver_c *rtcp);
  void rtc_lld_stop(hal_rtc_driver_c *rtcp);
  const hal_rtc_config_t *rtc_lld_setcfg(hal_rtc_driver_c *rtcp,
                                         const hal_rtc_config_t *config);
  const hal_rtc_config_t *rtc_lld_selcfg(hal_rtc_driver_c *rtcp,
                                         unsigned cfgnum);
  void rtc_lld_set_callback(hal_rtc_driver_c *rtcp, drv_cb_t cb);
  void rtc_lld_serve_interrupt(void);
  msg_t rtc_lld_set_datetime(hal_rtc_driver_c *rtcp,
                             const rtc_datetime_t *timespec);
  msg_t rtc_lld_get_datetime(hal_rtc_driver_c *rtcp,
                             rtc_datetime_t *timespec);
  msg_t rtc_lld_set_alarm(hal_rtc_driver_c *rtcp,
                          rtcalarm_t alarm,
                          const rtc_alarm_t *alarmspec);
  msg_t rtc_lld_get_alarm(hal_rtc_driver_c *rtcp,
                          rtcalarm_t alarm,
                          rtc_alarm_t *alarmspec);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */

#endif /* HAL_RTC_LLD_H */

/** @} */
