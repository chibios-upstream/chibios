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
 * @file    RTCv1/hal_rtc_lld.h
 * @brief   RP2040 RTC subsystem low level driver header.
 *
 * @section rtcv1_alarm_representation Alarm representation
 *
 * The portable @p rtc_alarm_t type carries a single 32-bit @p alrmr
 * word which, on STM32 parts, is an ALRMR register image. On this port
 * @p alrmr is NOT a register image: it is an ABSOLUTE alarm time
 * expressed in SECONDS since the Unix epoch (1970-01-01 00:00:00 UTC),
 * the same timescale carried by @p rtc_time64_t::tv_sec and used by
 * @p rtcSetTime64() and @p rtcGetTime64(), i.e. the timescale the
 * application uses when it sets the date/time. The driver expands
 * @p alrmr into a full broken-down date/time and programs a one-shot,
 * full-field date/time match in the RTC block.
 *
 * @note    The supported range ends on 2100-02-28T23:59:59Z: the
 *          RP2040 RTC hardware treats every year evenly divisible by
 *          4 as a leap year, including 2100 which is not leap in the
 *          Gregorian calendar. Date/times and alarm times at or after
 *          2100-03-01 are rejected with @p HAL_RET_CONFIG_ERROR; the
 *          @p CTRL.FORCE_NOTLEAPYEAR feature is kept cleared, which is
 *          correct within the supported range. The classic driver in
 *          os/hal/ports/RP/LLD/RTCv1 shares this hardware behavior.
 *          Alarm times before 1980-01-01 (the floor of the shared
 *          date/time representation) are also rejected with
 *          @p HAL_RET_CONFIG_ERROR.
 * @note    Repeating and field-masked alarms are not available through
 *          the portable API even though the RP2040 RTC block supports
 *          per-field match enables; see the classic driver in
 *          os/hal/ports/RP/LLD/RTCv1 for what the silicon can do.
 *          Dedicated rtcRP* extension functions are possible future
 *          work.
 * @note    The @p cr and @p prer fields of the shared
 *          @p hal_rtc_config_t are STM32 register images with no
 *          meaning on this port, they are accepted and ignored.
 *
 * @addtogroup HAL_RTC
 * @{
 */

#ifndef HAL_RTC_LLD_H
#define HAL_RTC_LLD_H

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Implementation capabilities
 * @{
 */
/**
 * @brief   Callback support in the driver.
 * @note    Kept for parity with the classic RP2040 driver which also
 *          supports alarm callbacks.
 */
#define RTC_SUPPORTS_CALLBACKS      TRUE

/**
 * @brief   Number of alarms available.
 */
#define RTC_ALARMS                  1

/**
 * @brief   Presence of a local persistent storage.
 */
#define RTC_HAS_STORAGE             FALSE
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/* Priority settings checks.*/
#if !defined(RP_IRQ_RTC_PRIORITY)
#error "RP_IRQ_RTC_PRIORITY not defined in mcuconf.h"
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* IRQ priority checks. The alarm handler interacts with the kernel,
   therefore a kernel-compatible priority is required.*/
#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_RTC_PRIORITY)
#error "Invalid IRQ priority assigned to RTC"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Implementation-specific @p hal_rtc_driver_c fields.
 */
#define rtc_lld_driver_fields                                               \
  /* Pointer to the RTC registers block.*/                                  \
  RTC_TypeDef               *rtc;                                           \
  /* Alarm time in seconds since the Unix epoch, zero when disarmed.*/      \
  uint32_t                  alrmr

/**
 * @brief   Implementation-specific @p hal_rtc_config_t fields.
 * @note    Empty, the shared @p cr and @p prer fields are accepted and
 *          ignored by this port.
 */
#define rtc_lld_config_fields

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
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

#endif /* HAL_USE_RTC == TRUE */

#endif /* HAL_RTC_LLD_H */

/** @} */
