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
 * @brief   RP2350 RTC subsystem low level driver header.
 * @details The driver is built on the POWMAN "Always-On" timer, the
 *          RP2350 has no dedicated RTC peripheral. The timer counts
 *          milliseconds since the Unix epoch (1970-01-01 00:00:00
 *          UTC), the timescale of @p rtc_time64_t.
 *
 * @section rtcv2_retained_time Retained time base
 *
 * The AON timer counter is retained across ordinary chip resets, it is
 * part of a persistent ABI shared with whatever firmware ran before.
 * The classic driver in os/hal/ports/RP/LLD/RTCv2 keeps the counter in
 * milliseconds since 1980-01-01T00:00:00Z (@p RTC_BASE_YEAR) while
 * this driver keeps it in milliseconds since the Unix epoch: the two
 * time bases differ by 315532800 seconds, a counter retained from
 * classic firmware through a warm reset would be read that far in the
 * past. Such a counter needs one of:
 * - @p RP_RTC_TIME_BASE_LEGACY_1980 set to @p TRUE, the on-silicon
 *   time base then stays at 1980-01-01 for coexistence with classic
 *   firmware and this driver translates on every counter access;
 * - a date/time set after the first boot of this firmware, which
 *   rebases the counter to the Unix epoch (the default base).
 *
 * @section rtcv2_alarm_representation Alarm representation
 *
 * The portable @p rtc_alarm_t type carries a single 32-bit @p alrmr
 * word which, on STM32 parts, is an ALRMR register image. On this port
 * @p alrmr is NOT a register image: it is an ABSOLUTE alarm time
 * expressed in SECONDS since the Unix epoch (1970-01-01 00:00:00 UTC),
 * the same timescale carried by @p rtc_time64_t::tv_sec and used by
 * @p rtcSetTime64() and @p rtcGetTime64(), i.e. the timescale the
 * application uses when it sets the date/time. The driver programs the
 * 64-bit POWMAN alarm comparator at @p alrmr * 1000 milliseconds, the
 * alarm fires once when the timer value in milliseconds divided by
 * 1000 reaches @p alrmr.
 *
 * @note    Being a 32-bit seconds count the representation wraps on
 *          2106-02-07T06:28:15Z; alarms beyond that instant cannot be
 *          expressed through the portable API. Alarm times before
 *          1980-01-01 (the floor of the shared date/time
 *          representation) are rejected with @p HAL_RET_CONFIG_ERROR.
 * @note    Repeating and field-masked alarms are not available through
 *          the portable API even though the classic driver in
 *          os/hal/ports/RP/LLD/RTCv2 implements mask-based repetition
 *          in software on top of the single POWMAN comparator.
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
 * @note    Kept for parity with the classic RP2350 driver which also
 *          supports alarm callbacks.
 */
#define RTC_SUPPORTS_CALLBACKS      TRUE

/**
 * @brief   Number of alarms available.
 * @note    The RP2350 POWMAN AON timer has a single 64-bit alarm
 *          comparator.
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
/**
 * @brief   Keeps the on-silicon time base at 1980-01-01.
 * @details When @p TRUE the retained AON counter holds milliseconds
 *          since 1980-01-01T00:00:00Z, the time base used by the
 *          classic RP2350 driver, and this driver translates to and
 *          from the Unix timescale on every counter access. When
 *          @p FALSE the counter holds milliseconds since the Unix
 *          epoch. See the retained time base section above.
 */
#if !defined(RP_RTC_TIME_BASE_LEGACY_1980) || defined(__DOXYGEN__)
#define RP_RTC_TIME_BASE_LEGACY_1980    FALSE
#endif

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
 * @note    There is a single AON timer, reached through the CMSIS
 *          @p POWMAN peripheral pointer, so no per-instance register
 *          pointer field is required.
 */
#define rtc_lld_driver_fields                                               \
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
