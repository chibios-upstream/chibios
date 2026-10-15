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
 * @file    hal_safety.h
 * @brief   Safety manager macros and structures.
 *
 * @addtogroup HAL_SAFETY
 * @{
 */

#ifndef HAL_SAFETY_H
#define HAL_SAFETY_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Enables the safety-related features in HAL.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(HAL_USE_SAFETY) || defined(__DOXYGEN__)
#define HAL_USE_SAFETY                      FALSE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#if !defined(HAL_LLD_GET_CNT_VALUE) || defined(__DOXYGEN__)
/**
 * @brief   Timeout counter type, a stub if the platform does not implement
 *          the counter.
 */
typedef unsigned halcnt_t;
#endif

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Time measurement
 * @{
 */
/**
 * @brief   Converts from microseconds to timeout counter ticks.
 * @note    The result is rounded up.
 * @note    The computation is done in 32 bits, the frequency in MHz times
 *          the interval must not overflow.
 *
 * @param[in] freq      counter frequency in Hz
 * @param[in] usec      number of microseconds
 * @return              The number of ticks.
 */
#define HAL_SFT_US2CNT(freq, usec)                                          \
  (halcnt_t)((((uint32_t)(freq) + 999999U) / 1000000U) * (uint32_t)(usec))

/**
 * @brief   Converts from nanoseconds to timeout counter ticks.
 * @note    The result is rounded up.
 * @note    The computation is done in 32 bits, the frequency in MHz times
 *          the interval must not overflow.
 *
 * @param[in] freq      counter frequency in Hz
 * @param[in] nsec      number of nanoseconds
 * @return              The number of ticks.
 */
#define HAL_SFT_NS2CNT(freq, nsec)                                          \
  (halcnt_t)(((((uint32_t)(freq) + 999999U) / 1000000U) *                   \
              (uint32_t)(nsec) + 999U) / 1000U)
/** @} */

/**
 * @name    Function variants
 * @{
 */
/**
 * @brief   Sets bits into an 8 bits register.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] s         mask of bits to be set
 * @param[in] v         verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
#define halRegSet8X(p, s, v)        halRegModify8X(p, s, (uint8_t)0, v)

/**
 * @brief   Sets bits into a 16 bits register.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] s         mask of bits to be set
 * @param[in] v         verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
#define halRegSet16X(p, s, v)       halRegModify16X(p, s, (uint16_t)0, v)

/**
 * @brief   Sets bits into a 32 bits register.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] s         mask of bits to be set
 * @param[in] v         verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
#define halRegSet32X(p, s, v)       halRegModify32X(p, s, (uint32_t)0, v)

/**
 * @brief   Clears bits into an 8 bits register.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] c         mask of bits to be cleared
 * @param[in] v         verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
#define halRegClear8X(p, c, v)      halRegModify8X(p, (uint8_t)0, c, v)

/**
 * @brief   Clears bits into a 16 bits register.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] c         mask of bits to be cleared
 * @param[in] v         verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
#define halRegClear16X(p, c, v)     halRegModify16X(p, (uint16_t)0, c, v)

/**
 * @brief   Clears bits into a 32 bits register.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] c         mask of bits to be cleared
 * @param[in] v         verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
#define halRegClear32X(p, c, v)     halRegModify32X(p, (uint32_t)0, c, v)
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void halSftFail(const char *message);
  bool halRegWaitMatch8X(volatile uint8_t *p, uint8_t mask,
                         uint8_t match, uint32_t tmo, uint8_t *valp);
  bool halRegWaitMatch16X(volatile uint16_t *p, uint16_t mask,
                          uint16_t match, uint32_t tmo, uint16_t *valp);
  bool halRegWaitMatch32X(volatile uint32_t *p, uint32_t mask,
                          uint32_t match, uint32_t tmo, uint32_t *valp);
  bool halRegWaitAllSet8X(volatile uint8_t *p, uint8_t mask,
                          uint32_t tmo, uint8_t *valp);
  bool halRegWaitAllSet16X(volatile uint16_t *p, uint16_t mask,
                           uint32_t tmo, uint16_t *valp);
  bool halRegWaitAllSet32X(volatile uint32_t *p, uint32_t mask,
                           uint32_t tmo, uint32_t *valp);
  bool halRegWaitAnySet8X(volatile uint8_t *p, uint8_t mask,
                          uint32_t tmo, uint8_t *valp);
  bool halRegWaitAnySet16X(volatile uint16_t *p, uint16_t mask,
                           uint32_t tmo, uint16_t *valp);
  bool halRegWaitAnySet32X(volatile uint32_t *p, uint32_t mask,
                           uint32_t tmo, uint32_t *valp);
  bool halRegWaitAllClear8X(volatile uint8_t *p, uint8_t mask,
                            uint32_t tmo, uint8_t *valp);
  bool halRegWaitAllClear16X(volatile uint16_t *p, uint16_t mask,
                             uint32_t tmo, uint16_t *valp);
  bool halRegWaitAllClear32X(volatile uint32_t *p, uint32_t mask,
                             uint32_t tmo, uint32_t *valp);
  bool halRegWaitAnyClear8X(volatile uint8_t *p, uint8_t mask,
                            uint32_t tmo, uint8_t *valp);
  bool halRegWaitAnyClear16X(volatile uint16_t *p, uint16_t mask,
                             uint32_t tmo, uint16_t *valp);
  bool halRegWaitAnyClear32X(volatile uint32_t *p, uint32_t mask,
                             uint32_t tmo, uint32_t *valp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Inline functions.                                                         */
/*===========================================================================*/

/**
 * @brief   Enters a common safety fault handler on error.
 * @note    IF a custom handler is not defined then the default action is to
 *          call @p osalSysHalt().
 * @note    This function can potentially not return.
 *
 * @param[in] result    the error status, @p true if an error occurred
 * @param[in] message   an optional descriptive message
 *
 * @api
 */
static inline void halSftFailOnError(bool result, const char *message) {

  if (result) {
    halSftFail(message);
  }
}

/**
 * @brief   Returns the timeout counter value.
 * @note    The counter is implemented by the platform, it is free running
 *          and wraps around at the size of @p halcnt_t. Without a platform
 *          counter the value is always zero.
 *
 * @return              The counter value.
 *
 * @xclass
 */
static inline halcnt_t halSftGetCounterX(void) {

#if defined(HAL_LLD_GET_CNT_VALUE) && defined(HAL_LLD_GET_CNT_FREQUENCY)
  return HAL_LLD_GET_CNT_VALUE();
#else
  return (halcnt_t)0;
#endif
}

/**
 * @brief   Returns the timeout counter frequency.
 * @note    Zero without a platform counter, it can be used to check for
 *          the feature.
 *
 * @return              The counter frequency in Hz.
 *
 * @xclass
 */
static inline uint32_t halSftGetCounterFrequencyX(void) {

#if defined(HAL_LLD_GET_CNT_VALUE) && defined(HAL_LLD_GET_CNT_FREQUENCY)
  return HAL_LLD_GET_CNT_FREQUENCY();
#else
  return 0U;
#endif
}

/**
 * @brief   Checks if the timeout counter is within a window.
 * @note    The window can wrap around, its size must be smaller than the
 *          counter range.
 * @note    Without a platform counter the time does not advance, the
 *          counter is always within the window and the timeouts never
 *          expire.
 *
 * @param[in] start     start of the window, included
 * @param[in] end       end of the window, excluded
 * @return              The test result.
 * @retval false        if the counter is outside the window.
 * @retval true         if the counter is within the window.
 *
 * @xclass
 */
static inline bool halSftIsCounterWithinX(halcnt_t start, halcnt_t end) {
#if defined(HAL_LLD_GET_CNT_VALUE) && defined(HAL_LLD_GET_CNT_FREQUENCY)
  halcnt_t cnt = halSftGetCounterX();

  return (bool)((halcnt_t)(cnt - start) < (halcnt_t)(end - start));
#else

  (void)start;
  (void)end;

  return true;
#endif
}

#if (defined(HAL_LLD_GET_CNT_VALUE) &&                                     \
     defined(HAL_LLD_GET_CNT_FREQUENCY)) || defined(__DOXYGEN__)
/**
 * @brief   Waits for an interval from a timeout counter sample.
 * @details Returns after at least @p ticks counter periods since @p start
 *          was sampled, the phase of the sample within its period is
 *          accounted.
 * @note    Only available with a platform counter, see
 *          @p halSftGetCounterFrequencyX().
 * @note    The interval must be shorter than the counter range.
 *
 * @param[in] start     counter value sampled at the start of the interval
 * @param[in] ticks     interval in counter ticks
 *
 * @xclass
 */
static inline void halSftDelayFromX(halcnt_t start, halcnt_t ticks) {
  halcnt_t end = start + ticks + (halcnt_t)1;

  while (halSftIsCounterWithinX(start, end)) {
  }
}

/**
 * @brief   Waits for an interval.
 * @details Returns after at least @p ticks counter periods.
 * @note    Only available with a platform counter, see
 *          @p halSftGetCounterFrequencyX().
 * @note    The interval must be shorter than the counter range.
 *
 * @param[in] ticks     interval in counter ticks
 *
 * @xclass
 */
static inline void halSftDelayX(halcnt_t ticks) {

  halSftDelayFromX(halSftGetCounterX(), ticks);
}
#endif

/**
 * @brief   Writes an 8 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] value     value to be written
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegWrite8X(volatile uint8_t *p,
                                 uint8_t value,
                                 bool verify) {

  *p = value;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    if (*p != value) {
      halSftFail("w8");
    }
  }
#endif
}

/**
 * @brief   Writes a 16 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] value     value to be written
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegWrite16X(volatile uint16_t *p,
                                  uint16_t value,
                                  bool verify) {

  *p = value;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    if (*p != value) {
      halSftFail("w16");
    }
  }
#endif
}

/**
 * @brief   Writes a 32 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] value     value to be written
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegWrite32X(volatile uint32_t *p,
                                  uint32_t value,
                                  bool verify) {

  *p = value;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    if (*p != value) {
      halSftFail("w32");
    }
  }
#endif
}

/**
 * @brief   Modifies a 8 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] setmask   mask of bits to be set
 * @param[in] clrmask   mask of bits to be cleared
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegModify8X(volatile uint8_t *p,
                                  uint8_t setmask,
                                  uint8_t clrmask,
                                  bool verify) {
  uint8_t v = *p;
  v |= setmask;
  v &= ~clrmask;
  *p = v;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    uint8_t check = *p;

    if (((check & setmask) != setmask) || ((check & clrmask) != (uint8_t)0)) {
      halSftFail("m8");
    }
  }
#endif
}

/**
 * @brief   Modifies a 16 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] setmask   mask of bits to be set
 * @param[in] clrmask   mask of bits to be cleared
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegModify16X(volatile uint16_t *p,
                                   uint16_t setmask,
                                   uint16_t clrmask,
                                   bool verify) {
  uint16_t v = *p;
  v |= setmask;
  v &= ~clrmask;
  *p = v;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    uint16_t check = *p;

    if (((check & setmask) != setmask) || ((check & clrmask) != (uint16_t)0)) {
      halSftFail("m16");
    }
  }
#endif
}

/**
 * @brief   Modifies a 32 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] setmask   mask of bits to be set
 * @param[in] clrmask   mask of bits to be cleared
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegModify32X(volatile uint32_t *p,
                                   uint32_t setmask,
                                   uint32_t clrmask,
                                   bool verify) {
  uint32_t v = *p;
  v |= setmask;
  v &= ~clrmask;
  *p = v;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    uint32_t check = *p;

    if (((check & setmask) != setmask) || ((check & clrmask) != (uint32_t)0)) {
      halSftFail("m32");
    }
  }
#endif
}

/**
 * @brief   Writes a field into an 8 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] mask      mask of bits field to be written
 * @param[in] value     value to be written, must be pre-shifted to match the
 *                      mask
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegMaskedWrite8X(volatile uint8_t *p,
                                       uint8_t mask,
                                       uint8_t value,
                                       bool verify) {
  uint8_t v;

  osalDbgCheck((value & ~mask) == 0U);

  v = *p;
  v &= ~mask;
  v |= value;
  *p = v;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    uint8_t check = *p;

    if ((check & mask) != value) {
      halSftFail("mw8");
    }
  }
#endif
}

/**
 * @brief   Writes a field into a 16 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] mask      mask of bits field to be written
 * @param[in] value     value to be written, must be pre-shifted to match the
 *                      mask
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegMaskedWrite16X(volatile uint16_t *p,
                                        uint16_t mask,
                                        uint16_t value,
                                        bool verify) {
  uint16_t v;

  osalDbgCheck((value & ~mask) == 0U);

  v = *p;
  v &= ~mask;
  v |= value;
  *p = v;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    uint16_t check = *p;

    if ((check & mask) != value) {
      halSftFail("mw16");
    }
  }
#endif
}

/**
 * @brief   Writes a field into a 32 bits register.
 * @note    This code relies on compiler optimizations, the constant
 *          operations that cause no change to the final value are
 *          optimized out.
 * @note    There is a strong assumption for memory-mapped I/O, this is not
 *          necessarily true on all architectures.
 * @note    Verification failure results in entering the fault handler.
 *
 * @param[in] p         pointer to the register
 * @param[in] mask      mask of bits field to be written
 * @param[in] value     value to be written, must be pre-shifted to match the
 *                      mask
 * @param[in] verify    verification flag, register is read back if @p true,
 *                      this flag only has effect if @p HAL_USE_SAFETY is
 *                      set to @p TRUE
 *
 * @xclass
 */
static inline void halRegMaskedWrite32X(volatile uint32_t *p,
                                        uint32_t mask,
                                        uint32_t value,
                                        bool verify) {
  uint32_t v;

  osalDbgCheck((value & ~mask) == 0U);

  v = *p;
  v &= ~mask;
  v |= value;
  *p = v;

#if HAL_USE_SAFETY == FALSE
  (void)verify;
#else
  if (verify) {
    uint32_t check = *p;

    if ((check & mask) != value) {
      halSftFail("mw32");
    }
  }
#endif
}

#endif /* HAL_SAFETY_H */

/** @} */
