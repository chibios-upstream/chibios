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
 * @file    FDCANv2/hal_can_lld.c
 * @brief   STM32 CAN subsystem low level driver source.
 *
 * @addtogroup CAN
 * @{
 */

#include "hal.h"

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*
 * TODO
 * The message received are stored only in RX FIFO 0 and RX FIFO 1, it depends on the configuration (GFC and SFEC/EFEC).
 * With this revision of driver, the filter is always active (es. FDCAN_SIDFC.LSS > 0).
 * The management of interrupt RX BUFFER (FDCAN_IR.DRX) is not implemented.
 * */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/
#define STM32_FDCAN_FIFO_WATERMARK          1U

/* Application-controlled CCCR bits. INIT, CCE and clock stop are owned by
   the driver; FDOE and BRSE are further restricted by op_mode.*/
#define FDCAN_CCCR_OPTIONS                 (FDCAN_CCCR_ASM | FDCAN_CCCR_MON | \
                                            FDCAN_CCCR_DAR | FDCAN_CCCR_TEST | \
                                            FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | \
                                            FDCAN_CCCR_PXHD | FDCAN_CCCR_EFBI | \
                                            FDCAN_CCCR_TXP | FDCAN_CCCR_NISO)

/* Size of element size (RAM words) */
#define CAN_SIZE_RAM_WORDS              (4U)
#define FDCAN_SIZE_RAM_WORDS            (18U)

/* Filter Standard Element MAX Size in WORDS for single element. */
#define SRAMCAN_FLS_SIZE                (1U)

/* Filter Extended Element MAX Size in WORDS for single element.*/
#define SRAMCAN_FLE_SIZE                (2U)

/* RX FIFO 0 Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_RF0_SIZE                (18U)

/* RX FIFO 1 Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_RF1_SIZE                (18U)

/* RX Buffer MAX Size in WORDS for single element.*/
#define SRAMCAN_RB_SIZE                 (18U)

/* TX Event FIFO Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_TEF_SIZE                (2U)

/* TX FIFO/Queue Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_TB_SIZE                 (18U)

/* Trigger Memory MAX Size in WORDS for single element.*/
#define SRAMCAN_TM_SIZE                 (2U)

/* Message RAM layout helpers.*/
#define FDCAN_RAM_SIZE(fls, fle, rf0, rf1, rb, tef, tb, tm)                 \
  ((fls) * SRAMCAN_FLS_SIZE + (fle) * SRAMCAN_FLE_SIZE +                    \
   (rf0) * SRAMCAN_RF0_SIZE + (rf1) * SRAMCAN_RF1_SIZE +                    \
   (rb) * SRAMCAN_RB_SIZE + (tef) * SRAMCAN_TEF_SIZE +                      \
   (tb) * SRAMCAN_TB_SIZE + (tm) * SRAMCAN_TM_SIZE)

#define FDCAN_RAM_FLESA(fls)                (fls)
#define FDCAN_RAM_RF0SA(fls, fle)           (FDCAN_RAM_FLESA(fls) +         \
                                             (fle) * SRAMCAN_FLE_SIZE)
#define FDCAN_RAM_RF1SA(fls, fle, rf0)      (FDCAN_RAM_RF0SA(fls, fle) +    \
                                             (rf0) * SRAMCAN_RF0_SIZE)
#define FDCAN_RAM_RBSA(fls, fle, rf0, rf1)  (FDCAN_RAM_RF1SA(fls, fle,      \
                                                             rf0) +         \
                                             (rf1) * SRAMCAN_RF1_SIZE)
#define FDCAN_RAM_TEFSA(fls, fle, rf0, rf1, rb)                             \
  (FDCAN_RAM_RBSA(fls, fle, rf0, rf1) + (rb) * SRAMCAN_RB_SIZE)
#define FDCAN_RAM_TBSA(fls, fle, rf0, rf1, rb, tef)                         \
  (FDCAN_RAM_TEFSA(fls, fle, rf0, rf1, rb) +                                \
   (tef) * SRAMCAN_TEF_SIZE)

#define FDCAN_RAM_CONFIG_VALID(fls, fle, rf0, rf1, rb, tef, tb, tm)         \
  (((fls) <= 128U) && ((fle) <= 64U) &&                                     \
   ((rf0) > 0U) && ((rf0) <= 64U) &&                                        \
   ((rf1) > 0U) && ((rf1) <= 64U) &&                                        \
   ((rb) <= 64U) && ((tef) <= 32U) &&                                       \
   ((tb) > 0U) && ((tb) <= 32U) && ((tm) <= 64U))

#if STM32_CAN_USE_FDCAN1
#define FDCAN1_RAM_SIZE                                                     \
  FDCAN_RAM_SIZE(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR,        \
                 STM32_CAN_FDCAN1_RF0_NBR, STM32_CAN_FDCAN1_RF1_NBR,        \
                 STM32_CAN_FDCAN1_RB_NBR, STM32_CAN_FDCAN1_TEF_NBR,         \
                 STM32_CAN_FDCAN1_TB_NBR, STM32_CAN_FDCAN1_TM_NBR)
#if !FDCAN_RAM_CONFIG_VALID(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR, \
                            STM32_CAN_FDCAN1_RF0_NBR, STM32_CAN_FDCAN1_RF1_NBR, \
                            STM32_CAN_FDCAN1_RB_NBR, STM32_CAN_FDCAN1_TEF_NBR, \
                            STM32_CAN_FDCAN1_TB_NBR, STM32_CAN_FDCAN1_TM_NBR)
#error "invalid FDCAN1 Message RAM configuration"
#endif
#else
#define FDCAN1_RAM_SIZE                     0U
#endif

#if STM32_CAN_USE_FDCAN2
#define FDCAN2_RAM_SIZE                                                     \
  FDCAN_RAM_SIZE(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR,        \
                 STM32_CAN_FDCAN2_RF0_NBR, STM32_CAN_FDCAN2_RF1_NBR,        \
                 STM32_CAN_FDCAN2_RB_NBR, STM32_CAN_FDCAN2_TEF_NBR,         \
                 STM32_CAN_FDCAN2_TB_NBR, STM32_CAN_FDCAN2_TM_NBR)
#if !FDCAN_RAM_CONFIG_VALID(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR, \
                            STM32_CAN_FDCAN2_RF0_NBR, STM32_CAN_FDCAN2_RF1_NBR, \
                            STM32_CAN_FDCAN2_RB_NBR, STM32_CAN_FDCAN2_TEF_NBR, \
                            STM32_CAN_FDCAN2_TB_NBR, STM32_CAN_FDCAN2_TM_NBR)
#error "invalid FDCAN2 Message RAM configuration"
#endif
#else
#define FDCAN2_RAM_SIZE                     0U
#endif

#if STM32_CAN_USE_FDCAN3
#define FDCAN3_RAM_SIZE                                                     \
  FDCAN_RAM_SIZE(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR,        \
                 STM32_CAN_FDCAN3_RF0_NBR, STM32_CAN_FDCAN3_RF1_NBR,        \
                 STM32_CAN_FDCAN3_RB_NBR, STM32_CAN_FDCAN3_TEF_NBR,         \
                 STM32_CAN_FDCAN3_TB_NBR, STM32_CAN_FDCAN3_TM_NBR)
#if !FDCAN_RAM_CONFIG_VALID(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR, \
                            STM32_CAN_FDCAN3_RF0_NBR, STM32_CAN_FDCAN3_RF1_NBR, \
                            STM32_CAN_FDCAN3_RB_NBR, STM32_CAN_FDCAN3_TEF_NBR, \
                            STM32_CAN_FDCAN3_TB_NBR, STM32_CAN_FDCAN3_TM_NBR)
#error "invalid FDCAN3 Message RAM configuration"
#endif
#else
#define FDCAN3_RAM_SIZE                     0U
#endif

#define FDCAN1_RAM_OFFSET                   0U
#define FDCAN2_RAM_OFFSET                   (FDCAN1_RAM_OFFSET +            \
                                             FDCAN1_RAM_SIZE)
#define FDCAN3_RAM_OFFSET                   (FDCAN2_RAM_OFFSET +            \
                                             FDCAN2_RAM_SIZE)
#define FDCAN_RAM_END                       (FDCAN3_RAM_OFFSET +            \
                                             FDCAN3_RAM_SIZE)

#if FDCAN_RAM_END > 2560U
#error "FDCAN Message RAM allocation exceeds 10KB"
#endif

#define TIMEOUT_INIT_MS                 250U
#define TIMEOUT_CSA_MS                  250U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief CAN1 driver identifier.*/
#if STM32_CAN_USE_FDCAN1 || defined(__DOXYGEN__)
CANDriver CAND1;
#endif

/** @brief CAN2 driver identifier.*/
#if STM32_CAN_USE_FDCAN2 || defined(__DOXYGEN__)
CANDriver CAND2;
#endif

/** @brief CAN3 driver identifier.*/
#if STM32_CAN_USE_FDCAN3 || defined(__DOXYGEN__)
CANDriver CAND3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

struct fdcan_ram_config {
  uint32_t                  *base;
  uint16_t                  size;
  uint16_t                  flesa;
  uint16_t                  rf0sa;
  uint16_t                  rf1sa;
  uint16_t                  rbsa;
  uint16_t                  tefsa;
  uint16_t                  tbsa;
  uint8_t                   fls_nbr;
  uint8_t                   fle_nbr;
  uint8_t                   rf0_nbr;
  uint8_t                   rf1_nbr;
  uint8_t                   tef_nbr;
  uint8_t                   tb_nbr;
};

#if STM32_CAN_USE_FDCAN1
static const fdcan_ram_config_t fdcan1_ram_config = {
  (uint32_t *)SRAMCAN_BASE + FDCAN1_RAM_OFFSET,
  FDCAN1_RAM_SIZE,
  FDCAN_RAM_FLESA(STM32_CAN_FDCAN1_FLS_NBR),
  FDCAN_RAM_RF0SA(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR),
  FDCAN_RAM_RF1SA(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR,
                  STM32_CAN_FDCAN1_RF0_NBR),
  FDCAN_RAM_RBSA(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR,
                 STM32_CAN_FDCAN1_RF0_NBR, STM32_CAN_FDCAN1_RF1_NBR),
  FDCAN_RAM_TEFSA(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR,
                  STM32_CAN_FDCAN1_RF0_NBR, STM32_CAN_FDCAN1_RF1_NBR,
                  STM32_CAN_FDCAN1_RB_NBR),
  FDCAN_RAM_TBSA(STM32_CAN_FDCAN1_FLS_NBR, STM32_CAN_FDCAN1_FLE_NBR,
                 STM32_CAN_FDCAN1_RF0_NBR, STM32_CAN_FDCAN1_RF1_NBR,
                 STM32_CAN_FDCAN1_RB_NBR, STM32_CAN_FDCAN1_TEF_NBR),
  STM32_CAN_FDCAN1_FLS_NBR,
  STM32_CAN_FDCAN1_FLE_NBR,
  STM32_CAN_FDCAN1_RF0_NBR,
  STM32_CAN_FDCAN1_RF1_NBR,
  STM32_CAN_FDCAN1_TEF_NBR,
  STM32_CAN_FDCAN1_TB_NBR
};
#endif

#if STM32_CAN_USE_FDCAN2
static const fdcan_ram_config_t fdcan2_ram_config = {
  (uint32_t *)SRAMCAN_BASE + FDCAN2_RAM_OFFSET,
  FDCAN2_RAM_SIZE,
  FDCAN_RAM_FLESA(STM32_CAN_FDCAN2_FLS_NBR),
  FDCAN_RAM_RF0SA(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR),
  FDCAN_RAM_RF1SA(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR,
                  STM32_CAN_FDCAN2_RF0_NBR),
  FDCAN_RAM_RBSA(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR,
                 STM32_CAN_FDCAN2_RF0_NBR, STM32_CAN_FDCAN2_RF1_NBR),
  FDCAN_RAM_TEFSA(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR,
                  STM32_CAN_FDCAN2_RF0_NBR, STM32_CAN_FDCAN2_RF1_NBR,
                  STM32_CAN_FDCAN2_RB_NBR),
  FDCAN_RAM_TBSA(STM32_CAN_FDCAN2_FLS_NBR, STM32_CAN_FDCAN2_FLE_NBR,
                 STM32_CAN_FDCAN2_RF0_NBR, STM32_CAN_FDCAN2_RF1_NBR,
                 STM32_CAN_FDCAN2_RB_NBR, STM32_CAN_FDCAN2_TEF_NBR),
  STM32_CAN_FDCAN2_FLS_NBR,
  STM32_CAN_FDCAN2_FLE_NBR,
  STM32_CAN_FDCAN2_RF0_NBR,
  STM32_CAN_FDCAN2_RF1_NBR,
  STM32_CAN_FDCAN2_TEF_NBR,
  STM32_CAN_FDCAN2_TB_NBR
};
#endif

#if STM32_CAN_USE_FDCAN3
static const fdcan_ram_config_t fdcan3_ram_config = {
  (uint32_t *)SRAMCAN_BASE + FDCAN3_RAM_OFFSET,
  FDCAN3_RAM_SIZE,
  FDCAN_RAM_FLESA(STM32_CAN_FDCAN3_FLS_NBR),
  FDCAN_RAM_RF0SA(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR),
  FDCAN_RAM_RF1SA(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR,
                  STM32_CAN_FDCAN3_RF0_NBR),
  FDCAN_RAM_RBSA(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR,
                 STM32_CAN_FDCAN3_RF0_NBR, STM32_CAN_FDCAN3_RF1_NBR),
  FDCAN_RAM_TEFSA(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR,
                  STM32_CAN_FDCAN3_RF0_NBR, STM32_CAN_FDCAN3_RF1_NBR,
                  STM32_CAN_FDCAN3_RB_NBR),
  FDCAN_RAM_TBSA(STM32_CAN_FDCAN3_FLS_NBR, STM32_CAN_FDCAN3_FLE_NBR,
                 STM32_CAN_FDCAN3_RF0_NBR, STM32_CAN_FDCAN3_RF1_NBR,
                 STM32_CAN_FDCAN3_RB_NBR, STM32_CAN_FDCAN3_TEF_NBR),
  STM32_CAN_FDCAN3_FLS_NBR,
  STM32_CAN_FDCAN3_FLE_NBR,
  STM32_CAN_FDCAN3_RF0_NBR,
  STM32_CAN_FDCAN3_RF1_NBR,
  STM32_CAN_FDCAN3_TEF_NBR,
  STM32_CAN_FDCAN3_TB_NBR
};
#endif

/* Includes controllers in CAN_STARTING. Accessed with the system locked.*/
static unsigned fdcan_users;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void fdcan_acquire(void) {

  if (fdcan_users == 0U) {
    rccEnableFDCAN(true);
    rccResetFDCAN();
  }
  fdcan_users++;
}

static void fdcan_release(void) {

  osalDbgAssert(fdcan_users > 0U, "unbalanced FDCAN clock");
  fdcan_users--;
  if (fdcan_users == 0U) {
    rccDisableFDCAN();
  }
}

static void fdcan_disable_interrupts(CANDriver *canp) {

  canp->fdcan->IE = 0U;
  canp->fdcan->ILE = 0U;
  canp->fdcan->TXBTIE = 0U;
  canp->fdcan->TXBCIE = 0U;
  canp->fdcan->IR = (uint32_t)-1;
}

static bool fdcan_clock_start(CANDriver *canp) {
  systime_t start, end;

  /* Leave clock stop before requesting initialization.*/
  canp->fdcan->CCCR &= ~FDCAN_CCCR_CSR;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_CSA_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_CSA) != 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

static bool fdcan_clock_stop(CANDriver *canp) {
  systime_t start, end;

  /* Requesting clock stop then waiting for it to happen.*/
  canp->fdcan->CCCR |= FDCAN_CCCR_CSR;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_CSA_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_CSA) == 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

static bool fdcan_init_mode(CANDriver *canp) {
  systime_t start, end;

  /* Going in initialization mode then waiting for it to happen.*/
  canp->fdcan->CCCR |= FDCAN_CCCR_INIT;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_INIT_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_INIT) == 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

static bool fdcan_active_mode(CANDriver *canp) {
  systime_t start, end;

  /* Going in active mode then waiting for it to happen.*/
  canp->fdcan->CCCR &= ~FDCAN_CCCR_INIT;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_INIT_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_INIT) != 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

/* Classic DLC 9..15 means eight bytes, also on an FD-capable controller.
   A remote frame has no data field.*/
static unsigned fdcan_payload_size(uint32_t dlc, bool fd, bool remote) {

  if (remote) {
    return 0U;
  }
  if (!fd && (dlc > 8U)) {
    return 8U;
  }
  return dlc_to_bytes[dlc];
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level CAN driver initialization.
 *
 * @notapi
 */
void can_lld_init(void) {

  /* Shared-clock ownership and unit reset.*/
  fdcan_users = 0U;
  rccResetFDCAN();

#if STM32_CAN_USE_FDCAN1
  /* Driver initialization.*/
  canObjectInit(&CAND1);
  CAND1.fdcan = FDCAN1;
  CAND1.ram = &fdcan1_ram_config;
#endif

#if STM32_CAN_USE_FDCAN2
  /* Driver initialization.*/
  canObjectInit(&CAND2);
  CAND2.fdcan = FDCAN2;
  CAND2.ram = &fdcan2_ram_config;
#endif

#if STM32_CAN_USE_FDCAN3
  /* Driver initialization.*/
  canObjectInit(&CAND3);
  CAND3.fdcan = FDCAN3;
  CAND3.ram = &fdcan3_ram_config;
#endif
}

/**
 * @brief   Configures and activates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @return              The operation result.
 * @retval HAL_RET_SUCCESS       if the operation succeeded.
 * @retval HAL_RET_CONFIG_ERROR  if the configuration is invalid.
 * @retval HAL_RET_HW_FAILURE    if a hardware handshake timed out.
 *
 * @notapi
 */
msg_t can_lld_start(CANDriver *canp) {
  uint32_t offset, cccr;
  volatile uint32_t *wp;

  if ((canp->config == NULL) ||
      (canp->ram == NULL) ||
      ((canp->config->op_mode != OPMODE_CAN) &&
       (canp->config->op_mode != OPMODE_FDCAN))) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Reserve the shared clock before any handshakes can release the lock.*/
  fdcan_acquire();
  fdcan_disable_interrupts(canp);

  if (fdcan_clock_start(canp)) {
    goto failed;
  }

  /* Going in initialization mode.*/
  if (fdcan_init_mode(canp)) {
    goto failed;
  }

  /* CCE resets this controller's FIFO status. Only clear its own RAM slice,
     after the controller has stopped accessing the old FIFO contents.*/
  canp->fdcan->CCCR |= FDCAN_CCCR_CCE;
  for (wp = canp->ram->base;
       wp < canp->ram->base + canp->ram->size;
       wp++) {
    *wp = 0U;
  }

  /* Configuration of element size (RAM words). */
  if (canp->config->op_mode == OPMODE_FDCAN) {
    canp->word_size = FDCAN_SIZE_RAM_WORDS;
  }
  else {
    canp->word_size = CAN_SIZE_RAM_WORDS;
  }

  /* Replace the previous options instead of retaining bits across starts.
     Classic mode must clear FDOE and BRSE even if supplied in CCCR.*/
  cccr = canp->config->CCCR & FDCAN_CCCR_OPTIONS;
  if (canp->config->op_mode == OPMODE_FDCAN) {
    cccr |= FDCAN_CCCR_FDOE;
  }
  else {
    cccr &= ~(FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE);
  }
  canp->fdcan->CCCR = FDCAN_CCCR_INIT | FDCAN_CCCR_CCE | cccr;
  canp->fdcan->NBTP = canp->config->NBTP;
  canp->fdcan->DBTP = canp->config->DBTP;
  canp->fdcan->TDCR = canp->config->TDCR;
  canp->fdcan->TSCC = canp->config->TSCC;

  /* TEST is only writable when FDCAN_CCCR_TEST is set and FDCAN is still in
   * configuration mode */
  if (canp->config->CCCR & FDCAN_CCCR_TEST) {
    canp->fdcan->TEST = canp->config->TEST;
  }

  canp->fdcan->GFC = canp->config->RXGFC;

  /* H7 FDCAN instances share one Message RAM, each with a packed slice.*/
  offset = (uint32_t)(canp->ram->base - (uint32_t *)SRAMCAN_BASE);
  canp->fdcan->SIDFC = FDCAN_CONFIG_SIDFC_LSS(canp->ram->fls_nbr) |
                       FDCAN_CONFIG_SIDFC_FLSSA(offset);
  canp->fdcan->XIDFC = FDCAN_CONFIG_XIDFC_LSE(canp->ram->fle_nbr) |
                       FDCAN_CONFIG_XIDFC_FLESA(offset + canp->ram->flesa);
  canp->fdcan->RXF0C = FDCAN_CONFIG_RXF0C_F0S(canp->ram->rf0_nbr) |
                       FDCAN_CONFIG_RXF0C_F0SA(offset + canp->ram->rf0sa) |
                       FDCAN_CONFIG_RXF0C_F0WM(STM32_FDCAN_FIFO_WATERMARK);
  canp->fdcan->RXF1C = FDCAN_CONFIG_RXF1C_F1S(canp->ram->rf1_nbr) |
                       FDCAN_CONFIG_RXF1C_F1SA(offset + canp->ram->rf1sa) |
                       FDCAN_CONFIG_RXF1C_F1WM(STM32_FDCAN_FIFO_WATERMARK);
  canp->fdcan->RXBC = FDCAN_CONFIG_RXBC_RBSA(offset + canp->ram->rbsa);
  canp->fdcan->TXEFC = FDCAN_CONFIG_TXEFC_EFS(canp->ram->tef_nbr) |
                       FDCAN_CONFIG_TXEFC_EFSA(offset + canp->ram->tefsa);
  /* NDTB remains zero, TFQS selects FIFO mode without dedicated buffers.*/
  canp->fdcan->TXBC = FDCAN_CONFIG_TXBC_TFQS(canp->ram->tb_nbr) |
                      FDCAN_CONFIG_TXBC_TBSA(offset + canp->ram->tbsa);

  /* Data field. Data field sizes > 8
     bytes are intended for CAN FD operation only. */
  if (canp->config->op_mode == OPMODE_FDCAN) {
    canp->fdcan->TXESC = FDCAN_CONFIG_TXESC_TBDS_64BDF;
    canp->fdcan->RXESC = FDCAN_CONFIG_RXESC_F0DS_64BDF | FDCAN_CONFIG_RXESC_F1DS_64BDF |
                         FDCAN_CONFIG_RXESC_RBDS_64BDF;
  }
  else {
    canp->fdcan->TXESC = FDCAN_CONFIG_TXESC_TBDS_8BDF;
    canp->fdcan->RXESC = FDCAN_CONFIG_RXESC_F0DS_8BDF | FDCAN_CONFIG_RXESC_F1DS_8BDF |
                         FDCAN_CONFIG_RXESC_RBDS_8BDF;
  }

  /* Enabling interrupts, only using interrupt zero.*/
  canp->fdcan->IR     = (uint32_t)-1;
  canp->fdcan->IE     = FDCAN_IE_RF1WE | FDCAN_IE_RF1LE |
                        FDCAN_IE_RF0WE | FDCAN_IE_RF0LE |
                        FDCAN_IE_TCE | FDCAN_IE_BOE;
  canp->fdcan->TXBTIE = FDCAN_TXBTIE_TIE;
  canp->fdcan->ILE    = FDCAN_ILE_EINT0;

  /* Going in active mode.*/
  if (fdcan_active_mode(canp)) {
    goto failed;
  }

  return HAL_RET_SUCCESS;

failed:
  fdcan_disable_interrupts(canp);
  canp->fdcan->CCCR |= FDCAN_CCCR_INIT | FDCAN_CCCR_CSR;
  fdcan_release();
  return HAL_RET_HW_FAILURE;
}

/**
 * @brief   Deactivates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_stop(CANDriver *canp) {

  /* If in ready state then disables the CAN peripheral.*/
  if (canp->state == CAN_READY) {
    /* Disabling and clearing interrupts.*/
    fdcan_disable_interrupts(canp);

    /* Disables the peripheral.*/
    (void) fdcan_clock_stop(canp);

    fdcan_release();
  }
}

/**
 * @brief   Determines whether a frame can be transmitted.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval false        no space in the transmit queue.
 * @retval true         transmit slot available.
 *
 * @notapi
 */
bool can_lld_is_tx_empty(CANDriver *canp, canmbx_t mailbox) {

  (void)mailbox;

  return (bool)(((canp->fdcan->TXFQS & FDCAN_TXFQS_TFQF) == 0U) &&
                ((canp->fdcan->TXFQS & FDCAN_TXFQS_TFFL) > 0U));
}

/**
 * @brief   Inserts a frame into the transmit queue.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] ctfp      pointer to the CAN frame to be transmitted
 * @param[in] mailbox   mailbox number,  @p CAN_ANY_MAILBOX for any mailbox
 *
 * @notapi
 */
void can_lld_transmit(CANDriver *canp, canmbx_t mailbox, const CANTxFrame *ctfp) {
  uint32_t put_index;
  volatile uint32_t *tx_address;
  unsigned bytes, i;
  bool fd;

  (void)mailbox;

  fd = (ctfp->FDF != 0U) && (canp->word_size == FDCAN_SIZE_RAM_WORDS);
  bytes = fdcan_payload_size(ctfp->DLC, fd,
                             !fd && (ctfp->common.RTR != 0U));

  /* Retrieve the TX FIFO put index.*/
  put_index = ((canp->fdcan->TXFQS & FDCAN_TXFQS_TFQPI) >> FDCAN_TXFQS_TFQPI_Pos);

  /* Writing frame. */
  tx_address = canp->ram->base + canp->ram->tbsa + put_index * canp->word_size;

  *tx_address++ = ctfp->header32[0];
  *tx_address++ = ctfp->header32[1];
  for (i = 0U; i < bytes; i += 4U) {
    *tx_address++ = ctfp->data32[i / 4U];
  }

  /* Starting transmission.*/
  canp->fdcan->TXBAR = ((uint32_t)1 << put_index);

}

/**
 * @brief   Determines whether a frame has been received.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval false        no new messages available.
 * @retval true         new messages available.
 *
 * @notapi
 */
bool can_lld_is_rx_nonempty(CANDriver *canp, canmbx_t mailbox) {

  switch (mailbox) {
    case CAN_ANY_MAILBOX:
      return can_lld_is_rx_nonempty(canp, 1U) ||
             can_lld_is_rx_nonempty(canp, 2U);
    case 1:
      return (bool)((canp->fdcan->RXF0S & FDCAN_RXF0S_F0FL) != 0U);
    case 2:
      return (bool)((canp->fdcan->RXF1S & FDCAN_RXF1S_F1FL) != 0U);
    default:
      return false;
  }
}

/**
 * @brief   Receives a frame from the input queue.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 * @param[out] crfp     pointer to the buffer where the CAN frame is copied
 *
 * @notapi
 */
void can_lld_receive(CANDriver *canp, canmbx_t mailbox, CANRxFrame *crfp) {
  uint32_t get_index;
  volatile const uint32_t *rx_address;
  unsigned bytes, i;
  bool fd;

  if (mailbox == CAN_ANY_MAILBOX) {
    if (can_lld_is_rx_nonempty(canp, 1U)) {
      mailbox = 1U;
    }
    else if (can_lld_is_rx_nonempty(canp, 2U)) {
      mailbox = 2U;
    }
    else {
      return;
    }
  }

  if (mailbox == 1U) {
     /* GET index RXF0, add it and the length to the rx_address.*/
     get_index = (canp->fdcan->RXF0S & FDCAN_RXF0S_F0GI_Msk) >> FDCAN_RXF0S_F0GI_Pos;
     rx_address = canp->ram->base + canp->ram->rf0sa + get_index * canp->word_size;
  }
  else {
     /* GET index RXF1, add it and the length to the rx_address.*/
     get_index = (canp->fdcan->RXF1S & FDCAN_RXF1S_F1GI_Msk) >> FDCAN_RXF1S_F1GI_Pos;
     rx_address = canp->ram->base + canp->ram->rf1sa + get_index * canp->word_size;
  }
  crfp->header32[0] = *rx_address++;
  crfp->header32[1] = *rx_address++;
  fd = (crfp->FDF != 0U) && (canp->word_size == FDCAN_SIZE_RAM_WORDS);
  bytes = fdcan_payload_size(crfp->DLC, fd,
                             !fd && (crfp->common.RTR != 0U));

  /* Copy message from FDCAN peripheral's SRAM to structure. RAM is restricted
     to word aligned accesses, so up to 3 extra bytes may be copied.*/
  for (i = 0U; i < bytes; i += 4U) {
    crfp->data32[i / 4U] = *rx_address++;
  }

  /* Acknowledge receipt by writing the get-index to the acknowledge
     register RXFxA then re-enable RX FIFO message arrived interrupt once
     the FIFO is emptied.*/
  if (mailbox == 1U) {
    uint32_t rxf0a = canp->fdcan->RXF0A;
    rxf0a &= ~FDCAN_RXF0A_F0AI_Msk;
    rxf0a |= get_index << FDCAN_RXF0A_F0AI_Pos;
    canp->fdcan->RXF0A = rxf0a;
  }
  else {
    uint32_t rxf1a = canp->fdcan->RXF1A;
    rxf1a &= ~FDCAN_RXF1A_F1AI_Msk;
    rxf1a |= get_index << FDCAN_RXF1A_F1AI_Pos;
    canp->fdcan->RXF1A = rxf1a;
  }
}

/**
 * @brief   Tries to abort an ongoing transmission.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number
 *
 * @notapi
 */
void can_lld_abort(CANDriver *canp, canmbx_t mailbox) {

  (void)canp;
  (void)mailbox;
}

#if CAN_USE_SLEEP_MODE || defined(__DOXYGEN__)
/**
 * @brief   Enters the sleep mode.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_sleep(CANDriver *canp) {

  (void)canp;
}

/**
 * @brief   Enforces leaving the sleep mode.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_wakeup(CANDriver *canp) {

  (void)canp;
}
#endif /* CAN_USE_SLEEP_MODE */

/**
 * @brief   FDCAN IRQ0 service routine.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_serve_interrupt(CANDriver *canp) {
  uint32_t ir = 0;

  /* Getting and clearing active IRQs.*/
  ir = canp->fdcan->IR;
  canp->fdcan->IR = ir;

  /* RX events.*/
  if ((ir & FDCAN_IR_RF0W) != 0U) {
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(1U));
  }
  if ((ir & FDCAN_IR_RF1W) != 0U) {
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(2U));
  }

  /* Overflow events.*/
  if (((ir & FDCAN_IR_RF0L) != 0U) || ((ir & FDCAN_IR_RF1L) != 0U)) {
    _can_error_isr(canp, CAN_OVERFLOW_ERROR);
  }

  /* Bus_off events.*/
  if ((ir & FDCAN_IR_BO) != 0U)  {
    _can_error_isr(canp, CAN_BUS_OFF_ERROR);
  }

  /* TX events.*/
  if ((ir & FDCAN_IR_TC) != 0U) {
    eventflags_t flags = 0U;

    flags |= CAN_MAILBOX_TO_MASK(1U);
    _can_tx_empty_isr(canp, flags);
  }
}

/**
 * @brief   Replaces the standard and extended filter tables.
 * @note    This is an STM32-specific API.
 * @note    Traffic must be quiesced while replacing the filter tables.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] num       number of entries in the filters array
 * @param[in] cfp       pointer to the filters array
 *
 * @notapi
 */
void can_lld_set_filters(CANDriver *canp, uint8_t num, const CANFilter *cfp) {
  unsigned i, num_std_filter = 0U, num_ext_filter = 0U;
  uint32_t maxid;
  volatile CANRxStandardFilter *filter_std;
  volatile CANRxExtendedFilter *filter_ext;
  bool valid;

  valid = (num == 0U) || (cfp != NULL);
  osalDbgCheck(valid);
  if (!valid) {
    return;
  }

  /* Validate the whole replacement before changing message RAM.*/
  for (i = 0U; i < num; i++) {
    maxid = cfp[i].filter_type == CAN_FILTER_TYPE_STD ? 0x7FFU : 0x1FFFFFFFU;
    valid = valid && ((unsigned)cfp[i].filter_type <= CAN_FILTER_TYPE_EXT) &&
            ((unsigned)cfp[i].filter_mode <= CAN_FILTER_MODE_CLASSIC) &&
            ((unsigned)cfp[i].filter_cfg >= CAN_FILTER_CFG_FIFO_0) &&
            ((unsigned)cfp[i].filter_cfg <= CAN_FILTER_CFG_REJECT) &&
            (cfp[i].identifier1 <= maxid) && (cfp[i].identifier2 <= maxid);
    if ((cfp[i].filter_type) == CAN_FILTER_TYPE_STD) {
      num_std_filter++;
    }
    else {
      num_ext_filter++;
    }
  }

  valid = valid && (num_std_filter <= canp->ram->fls_nbr) &&
          (num_ext_filter <= canp->ram->fle_nbr);
  osalDbgAssert(valid, "invalid filters");
  if (!valid) {
    return;
  }

  /* Base address of standard filter. */
  filter_std = (CANRxStandardFilter *)canp->ram->base;
  /* Base address of extended filter. */
  filter_ext = (CANRxExtendedFilter *)(canp->ram->base + canp->ram->flesa);

  /* Unused entries must be disabled, including when num is zero. Otherwise
     filters left by the previous call remain active.*/
  for (i = 0U; i < canp->ram->fls_nbr; i++) {
    filter_std[i].data32 = 0U;
  }
  for (i = 0U; i < canp->ram->fle_nbr; i++) {
    filter_ext[i].data32[0] = 0U;
    filter_ext[i].data32[1] = 0U;
  }

  /* Scanning the filters array. */
  for (i = 0U; i < num; i++) {
    /* Standard filter configuration */
    if ((cfp[i].filter_type) == CAN_FILTER_TYPE_STD) {
      /* Configure */
      filter_std->data32 = (FDCAN_STD_FILTER_SFID2(cfp[i].identifier2) |
                            FDCAN_STD_FILTER_SFID1(cfp[i].identifier1) |
                            FDCAN_STD_FILTER_SFEC(cfp[i].filter_cfg) |
                            FDCAN_STD_FILTER_SFT(cfp[i].filter_mode));
      filter_std++;
    }
    /* Extended filter configuration */
    else {

      /* Configure */
      filter_ext->data32[0] = (FDCAN_EXT_FILTER_EFID1(cfp[i].identifier1) |
                               FDCAN_EXT_FILTER_EFEC(cfp[i].filter_cfg));
      filter_ext->data32[1] = (FDCAN_EXT_FILTER_EFID2(cfp[i].identifier2) |
                               FDCAN_EXT_FILTER_EFT(cfp[i].filter_mode));
      filter_ext++;
    }
  }
}
/**
 * @brief   Replaces the standard and extended filter tables.
 * @note    This is an STM32-specific API.
 * @note    Serialize with lifecycle operations and quiesce traffic while
 *          replacing the tables; hardware accesses to filter RAM are not
 *          made atomic by locking the CPU.
 * @note    A zero count disables all entries; @p cfp may be @p NULL then.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] num       number of entries in the filters array
 * @param[in] cfp       pointer to the filters array
 *
 * @api
 */
void canSTM32SetFilters(CANDriver *canp, uint8_t num, const CANFilter *cfp) {

  osalDbgCheck(canp != NULL);
  osalDbgAssert(canp->state == CAN_READY, "invalid state");

  can_lld_set_filters(canp, num, cfp);
}

#endif /* HAL_USE_CAN */

/** @} */
