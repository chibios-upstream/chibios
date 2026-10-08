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
 * @brief   STM32H7 CAN subsystem low level driver.
 *
 * @addtogroup CAN
 * @{
 */

#include "hal.h"

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Divide the shared 2560-word RAM among the enabled instances. The layout
   retains the HAL capacities, including reserved Rx buffers and Tx events.*/
#define FDCAN_INSTANCES                    (STM32_CAN_USE_FDCAN1 + \
                                            STM32_CAN_USE_FDCAN2 + \
                                            STM32_CAN_USE_FDCAN3)
#define STM32_FDCAN_FLS_NBR                 (64U / FDCAN_INSTANCES)
#define STM32_FDCAN_FLE_NBR                 (32U / FDCAN_INSTANCES)
#define STM32_FDCAN_RF0_NBR                 (32U / FDCAN_INSTANCES)
#define STM32_FDCAN_RF1_NBR                 (32U / FDCAN_INSTANCES)
#define STM32_FDCAN_RB_NBR                  (32U / FDCAN_INSTANCES)
#define STM32_FDCAN_TEF_NBR                 (16U / FDCAN_INSTANCES)
#define STM32_FDCAN_TB_NBR                  (32U / FDCAN_INSTANCES)
#define STM32_FDCAN_TM_NBR                  (32U / FDCAN_INSTANCES)

#define SRAMCAN_FLSSA                      0U
#define SRAMCAN_FLESA                      (SRAMCAN_FLSSA + STM32_FDCAN_FLS_NBR)
#define SRAMCAN_RF0SA                      (SRAMCAN_FLESA + 2U * STM32_FDCAN_FLE_NBR)
#define SRAMCAN_RF1SA                      (SRAMCAN_RF0SA + 18U * STM32_FDCAN_RF0_NBR)
#define SRAMCAN_RBSA                       (SRAMCAN_RF1SA + 18U * STM32_FDCAN_RF1_NBR)
#define SRAMCAN_TEFSA                      (SRAMCAN_RBSA + 18U * STM32_FDCAN_RB_NBR)
#define SRAMCAN_TBSA                       (SRAMCAN_TEFSA + 2U * STM32_FDCAN_TEF_NBR)
#define SRAMCAN_TMSA                       (SRAMCAN_TBSA + 18U * STM32_FDCAN_TB_NBR)
#define SRAMCAN_SIZE                       (SRAMCAN_TMSA + 2U * STM32_FDCAN_TM_NBR)

#if FDCAN_INSTANCES * SRAMCAN_SIZE > 2560U
#error "FDCAN message RAM allocation exceeds 10KB"
#endif

#define FDCAN_TIMEOUT                      TIME_MS2I(250U)
#define FDCAN_CCCR_OPTIONS                 (FDCAN_CCCR_ASM | FDCAN_CCCR_MON | \
                                            FDCAN_CCCR_DAR | FDCAN_CCCR_TEST | \
                                            FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | \
                                            FDCAN_CCCR_PXHD | FDCAN_CCCR_EFBI | \
                                            FDCAN_CCCR_TXP | FDCAN_CCCR_NISO)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if STM32_CAN_USE_FDCAN1 || defined(__DOXYGEN__)
hal_can_driver_c CAND1;
#endif
#if STM32_CAN_USE_FDCAN2 || defined(__DOXYGEN__)
hal_can_driver_c CAND2;
#endif
#if STM32_CAN_USE_FDCAN3 || defined(__DOXYGEN__)
hal_can_driver_c CAND3;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/* Accessed under the system lock; the RCC gate/reset is shared by all units.*/
static unsigned fdcan_users;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void fdcan_acquire(void) {

  chSysLock();
  if (fdcan_users == 0U) {
    rccEnableFDCAN(true);
    rccResetFDCAN();
  }
  fdcan_users++;
  chSysUnlock();
}

static void fdcan_release(void) {

  chSysLock();
  chDbgAssert(fdcan_users > 0U, "unbalanced FDCAN clock");
  fdcan_users--;
  if (fdcan_users == 0U) {
    rccDisableFDCAN();
  }
  chSysUnlock();
}

/* Thread-context handshakes, never called by X/I-class configuration APIs.*/
static bool fdcan_wait(hal_can_driver_c *canp, uint32_t mask, uint32_t value) {
  systime_t start = chVTGetSystemTimeX();

  while ((canp->fdcan->CCCR & mask) != value) {
    if (chTimeDiffX(start, chVTGetSystemTimeX()) >= FDCAN_TIMEOUT) {
      return false;
    }
    chThdSleep(1);
  }
  return true;
}

static void fdcan_disable_interrupts(hal_can_driver_c *canp) {

  canp->fdcan->IE = 0U;
  canp->fdcan->ILE = 0U;
  canp->fdcan->TXBTIE = 0U;
  canp->fdcan->TXBCIE = 0U;
  canp->fdcan->IR = 0xFFFFFFFFU;
}

/* Classic DLC 9..15 means eight bytes, including in an FD-capable instance.*/
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
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the CAN driver objects without accessing hardware.
 * @notapi
 */
void can_lld_init(void) {
  uint32_t *ram = (uint32_t *)SRAMCAN_BASE;

  fdcan_users = 0U;
#if STM32_CAN_USE_FDCAN1
  canObjectInit(&CAND1);
  CAND1.fdcan = FDCAN1;
  CAND1.ram_base = ram;
  CAND1.word_size = 0U;
  ram += SRAMCAN_SIZE;
#endif
#if STM32_CAN_USE_FDCAN2
  canObjectInit(&CAND2);
  CAND2.fdcan = FDCAN2;
  CAND2.ram_base = ram;
  CAND2.word_size = 0U;
  ram += SRAMCAN_SIZE;
#endif
#if STM32_CAN_USE_FDCAN3
  canObjectInit(&CAND3);
  CAND3.fdcan = FDCAN3;
  CAND3.ram_base = ram;
  CAND3.word_size = 0U;
#endif
  (void)ram;
}

/**
 * @brief   Starts one controller, preserving other active controllers.
 * @notapi
 */
msg_t can_lld_start(hal_can_driver_c *canp) {
  const hal_can_config_t *cfg = (const hal_can_config_t *)canp->config;
  uint32_t offset, cccr;
  volatile uint32_t *wp;

  if (cfg == NULL) {
    cfg = can_lld_selcfg(canp, 0U);
  }
  if ((cfg == NULL) || (can_lld_setcfg(canp, cfg) == NULL)) {
    return HAL_RET_CONFIG_ERROR;
  }
  canp->config = cfg;
  fdcan_acquire();
  fdcan_disable_interrupts(canp);

  /* Leave clock-stop before modifying INIT (RM0433, power-down procedure).*/
  canp->fdcan->CCCR &= ~FDCAN_CCCR_CSR;
  if (!fdcan_wait(canp, FDCAN_CCCR_CSA, 0U)) {
    goto failed;
  }
  canp->fdcan->CCCR |= FDCAN_CCCR_INIT;
  if (!fdcan_wait(canp, FDCAN_CCCR_INIT, FDCAN_CCCR_INIT)) {
    goto failed;
  }
  canp->fdcan->CCCR |= FDCAN_CCCR_CCE;

  /* CCE resets this controller's FIFO status. Initialize only its RAM slice.*/
  for (wp = canp->ram_base; wp < canp->ram_base + SRAMCAN_SIZE; wp++) {
    *wp = 0U;
  }
  canp->word_size = cfg->op_mode == OPMODE_FDCAN ? 18U : 4U;
  offset = (uint32_t)(canp->ram_base - (uint32_t *)SRAMCAN_BASE);

  cccr = cfg->CCCR & ~(FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE);
  if (cfg->op_mode == OPMODE_FDCAN) {
    cccr |= FDCAN_CCCR_FDOE | (cfg->CCCR & FDCAN_CCCR_BRSE);
  }
  canp->fdcan->CCCR = FDCAN_CCCR_INIT | FDCAN_CCCR_CCE | cccr;
  canp->fdcan->NBTP = cfg->NBTP;
  canp->fdcan->DBTP = cfg->DBTP;
  canp->fdcan->TDCR = cfg->TDCR;
  canp->fdcan->TSCC = cfg->TSCC;
  if ((cccr & FDCAN_CCCR_TEST) != 0U) {
    canp->fdcan->TEST = cfg->TEST;
  }
  canp->fdcan->GFC = cfg->RXGFC;
  canp->fdcan->XIDAM = 0x1FFFFFFFU;

  canp->fdcan->SIDFC = FDCAN_CONFIG_SIDFC_LSS(STM32_FDCAN_FLS_NBR) |
                       FDCAN_CONFIG_SIDFC_FLSSA(offset + SRAMCAN_FLSSA);
  canp->fdcan->XIDFC = FDCAN_CONFIG_XIDFC_LSE(STM32_FDCAN_FLE_NBR) |
                       FDCAN_CONFIG_XIDFC_FLESA(offset + SRAMCAN_FLESA);
  canp->fdcan->RXF0C = FDCAN_CONFIG_RXF0C_F0S(STM32_FDCAN_RF0_NBR) |
                       FDCAN_CONFIG_RXF0C_F0SA(offset + SRAMCAN_RF0SA) |
                       FDCAN_CONFIG_RXF0C_F0WM(1U);
  canp->fdcan->RXF1C = FDCAN_CONFIG_RXF1C_F1S(STM32_FDCAN_RF1_NBR) |
                       FDCAN_CONFIG_RXF1C_F1SA(offset + SRAMCAN_RF1SA) |
                       FDCAN_CONFIG_RXF1C_F1WM(1U);
  canp->fdcan->RXBC = FDCAN_CONFIG_RXBC_RBSA(offset + SRAMCAN_RBSA);
  canp->fdcan->TXEFC = FDCAN_CONFIG_TXEFC_EFS(STM32_FDCAN_TEF_NBR) |
                       FDCAN_CONFIG_TXEFC_EFSA(offset + SRAMCAN_TEFSA);
  canp->fdcan->TXBC = FDCAN_CONFIG_TXBC_TFQS(STM32_FDCAN_TB_NBR) |
                      FDCAN_CONFIG_TXBC_TBSA(offset + SRAMCAN_TBSA);
  if (cfg->op_mode == OPMODE_FDCAN) {
    canp->fdcan->TXESC = FDCAN_CONFIG_TXESC_TBDS_64BDF;
    canp->fdcan->RXESC = FDCAN_CONFIG_RXESC_F0DS_64BDF |
                         FDCAN_CONFIG_RXESC_F1DS_64BDF |
                         FDCAN_CONFIG_RXESC_RBDS_64BDF;
  }
  else {
    canp->fdcan->TXESC = FDCAN_CONFIG_TXESC_TBDS_8BDF;
    canp->fdcan->RXESC = FDCAN_CONFIG_RXESC_F0DS_8BDF |
                         FDCAN_CONFIG_RXESC_F1DS_8BDF |
                         FDCAN_CONFIG_RXESC_RBDS_8BDF;
  }

  can_lld_reset(canp);
  canp->fdcan->ILS = 0U;
  canp->fdcan->IR = 0xFFFFFFFFU;
  canp->fdcan->IE = FDCAN_IE_RF0WE | FDCAN_IE_RF1WE |
                    FDCAN_IE_RF0LE | FDCAN_IE_RF1LE |
                    FDCAN_IE_TCE | FDCAN_IE_TCFE | FDCAN_IE_BOE |
                    FDCAN_IE_EWE | FDCAN_IE_EPE |
                    FDCAN_IE_PEAE | FDCAN_IE_PEDE;
  canp->fdcan->TXBTIE = 0xFFFFFFFFU;
  canp->fdcan->TXBCIE = 0xFFFFFFFFU;
  canp->fdcan->ILE = FDCAN_ILE_EINT0;

  canp->fdcan->CCCR &= ~FDCAN_CCCR_INIT;
  if (!fdcan_wait(canp, FDCAN_CCCR_INIT, 0U)) {
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
 * @brief   Stops a controller without gating clocks used by its peers.
 * @notapi
 */
void can_lld_stop(hal_can_driver_c *canp) {

  fdcan_disable_interrupts(canp);
  canp->fdcan->TXBCR = canp->fdcan->TXBRP;
  canp->fdcan->CCCR |= FDCAN_CCCR_INIT;
  (void)fdcan_wait(canp, FDCAN_CCCR_INIT, FDCAN_CCCR_INIT);
  canp->fdcan->CCCR |= FDCAN_CCCR_CSR;
  (void)fdcan_wait(canp, FDCAN_CCCR_CSA, FDCAN_CCCR_CSA);
  fdcan_release();
}

/**
 * @brief   Resets cached events.
 * @notapi
 */
void can_lld_reset(hal_can_driver_c *canp) {

  canp->events = 0U;
  canp->rx_mailbox_mask = 0U;
  canp->tx_mailbox_mask = 0U;
  canp->tx_error_mask = 0U;
  canp->errors = 0U;
}

/**
 * @brief   Validates a configuration without touching hardware.
 * @note    Hardware reconfiguration requires stop/start.
 * @notapi
 */
const hal_can_config_t *can_lld_setcfg(hal_can_driver_c *canp,
                                      const hal_can_config_t *config) {

  if (config == NULL) {
    return can_lld_selcfg(canp, 0U);
  }
  if (((config->op_mode != OPMODE_CAN) &&
       (config->op_mode != OPMODE_FDCAN)) ||
      ((config->CCCR & ~FDCAN_CCCR_OPTIONS) != 0U) ||
      ((config->RXGFC & ~0x3FU) != 0U)) {
    return NULL;
  }
  if ((canp->state == HAL_DRV_STATE_READY) && (config != canp->config)) {
    return NULL;
  }
  return config;
}

/**
 * @brief   Selects a user configuration.
 * @notapi
 */
const hal_can_config_t *can_lld_selcfg(hal_can_driver_c *canp,
                                      unsigned cfgnum) {
#if CAN_USE_CONFIGURATIONS
  extern const can_configurations_t can_configurations;

  if (cfgnum < can_configurations.cfgsnum) {
    return can_lld_setcfg(canp, &can_configurations.cfgs[cfgnum]);
  }
#else
  (void)canp;
  (void)cfgnum;
#endif
  return NULL;
}

/**
 * @brief   Callback association hook, hardware events remain enabled.
 * @notapi
 */
void can_lld_set_callback(hal_can_driver_c *canp, drv_cb_t cb) {

  (void)canp;
  (void)cb;
}

/**
 * @brief   Tests space in the transmit FIFO, exposed as one logical mailbox.
 * @notapi
 */
bool can_lld_is_tx_empty(hal_can_driver_c *canp, canmbx_t mailbox) {

  (void)mailbox;
  return ((canp->fdcan->TXFQS & FDCAN_TXFQS_TFQF) == 0U) &&
         ((canp->fdcan->TXFQS & FDCAN_TXFQS_TFFL) != 0U);
}

/**
 * @brief   Queues a frame using word-aligned message RAM accesses.
 * @notapi
 */
void can_lld_transmit(hal_can_driver_c *canp, canmbx_t mailbox,
                      const CANTxFrame *ctfp) {
  uint32_t index;
  unsigned bytes, i;
  volatile uint32_t *wp;
  bool fd = (ctfp->FDF != 0U) && (canp->word_size == 18U);

  (void)mailbox;
  index = (canp->fdcan->TXFQS & FDCAN_TXFQS_TFQPI) >>
          FDCAN_TXFQS_TFQPI_Pos;
  chDbgAssert(index < STM32_FDCAN_TB_NBR, "invalid TX FIFO index");
  bytes = fdcan_payload_size(ctfp->DLC, fd, !fd && (ctfp->common.RTR != 0U));
  wp = canp->ram_base + SRAMCAN_TBSA + index * canp->word_size;
  *wp++ = ctfp->header32[0];
  *wp++ = ctfp->header32[1];
  for (i = 0U; i < bytes; i += 4U) {
    *wp++ = ctfp->data32[i / 4U];
  }
  canp->fdcan->TXBAR = 1U << index;
}

/**
 * @brief   Tests the two receive FIFOs.
 * @notapi
 */
bool can_lld_is_rx_nonempty(hal_can_driver_c *canp, canmbx_t mailbox) {

  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    return can_lld_is_rx_nonempty(canp, 1U) ||
           can_lld_is_rx_nonempty(canp, 2U);
  case 1U:
    return (canp->fdcan->RXF0S & FDCAN_RXF0S_F0FL) != 0U;
  case 2U:
    return (canp->fdcan->RXF1S & FDCAN_RXF1S_F1FL) != 0U;
  default:
    return false;
  }
}

/**
 * @brief   Receives and acknowledges a FIFO element.
 * @notapi
 */
void can_lld_receive(hal_can_driver_c *canp, canmbx_t mailbox, CANRxFrame *crfp) {
  uint32_t index, offset;
  unsigned bytes, i;
  volatile const uint32_t *rp;
  bool fd;

  if (mailbox == CAN_ANY_MAILBOX) {
    mailbox = can_lld_is_rx_nonempty(canp, 1U) ? 1U : 2U;
  }
  if (mailbox == 1U) {
    index = (canp->fdcan->RXF0S & FDCAN_RXF0S_F0GI) >> FDCAN_RXF0S_F0GI_Pos;
    offset = SRAMCAN_RF0SA;
  }
  else {
    index = (canp->fdcan->RXF1S & FDCAN_RXF1S_F1GI) >> FDCAN_RXF1S_F1GI_Pos;
    offset = SRAMCAN_RF1SA;
  }
  chDbgAssert(index < STM32_FDCAN_RF0_NBR, "invalid RX FIFO index");
  rp = canp->ram_base + offset + index * canp->word_size;
  crfp->header32[0] = *rp++;
  crfp->header32[1] = *rp++;
  fd = (crfp->FDF != 0U) && (canp->word_size == 18U);
  bytes = fdcan_payload_size(crfp->DLC, fd, !fd && (crfp->common.RTR != 0U));
  for (i = 0U; i < bytes; i += 4U) {
    crfp->data32[i / 4U] = *rp++;
  }
  if (mailbox == 1U) {
    canp->fdcan->RXF0A = index;
  }
  else {
    canp->fdcan->RXF1A = index;
  }
}

/**
 * @brief   Requests cancellation of all pending frames in the logical mailbox.
 * @notapi
 */
void can_lld_abort(hal_can_driver_c *canp, canmbx_t mailbox) {

  (void)mailbox;
  canp->fdcan->TXBCR = canp->fdcan->TXBRP;
}

/**
 * @brief   Dispatches enabled events outside the system lock.
 * @notapi
 */
void can_lld_serve_interrupt(hal_can_driver_c *canp) {
  uint32_t ir = canp->fdcan->IR & canp->fdcan->IE;
  canerror_t errors = 0U;

  canp->fdcan->IR = ir;
  if (canp->state != HAL_DRV_STATE_READY) {
    return;
  }
  if ((ir & FDCAN_IR_RF0W) != 0U) {
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(1U));
  }
  if ((ir & FDCAN_IR_RF1W) != 0U) {
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(2U));
  }
  if ((ir & (FDCAN_IR_RF0L | FDCAN_IR_RF1L)) != 0U) {
    errors |= CAN_OVERFLOW_ERROR;
  }
  if (((ir & FDCAN_IR_BO) != 0U) &&
      ((canp->fdcan->PSR & FDCAN_PSR_BO) != 0U)) {
    errors |= CAN_BUS_OFF_ERROR;
  }
  if ((ir & FDCAN_IR_EW) != 0U) {
    errors |= CAN_LIMIT_WARNING;
  }
  if ((ir & FDCAN_IR_EP) != 0U) {
    errors |= CAN_LIMIT_ERROR;
  }
  if ((ir & (FDCAN_IR_PEA | FDCAN_IR_PED)) != 0U) {
    errors |= CAN_FRAMING_ERROR;
  }
  if (errors != 0U) {
    _can_error_isr(canp, errors);
  }
  if ((ir & (FDCAN_IR_TC | FDCAN_IR_TCF)) != 0U) {
    canmbxmask_t flags = CAN_MAILBOX_TO_MASK(1U);

    if (((ir & FDCAN_IR_TCF) != 0U) &&
        ((canp->fdcan->TXBCF & ~canp->fdcan->TXBTO) != 0U)) {
      flags |= CAN_MAILBOX_TO_MASK(1U) << 16U;
    }
    _can_tx_empty_isr(canp, flags);
  }
}

/**
 * @brief   Replaces the standard and extended filter tables.
 * @note    Call only on a ready, externally quiesced controller. The hardware
 *          can read filter RAM concurrently, so replacement is not atomic.
 * @notapi
 */
void can_lld_set_filters(hal_can_driver_c *canp, uint8_t num,
                         const CANFilter *cfp) {
  unsigned i, nstd = 0U, next = 0U;
  volatile uint32_t *stdp, *extp;
  bool valid = (num == 0U) || (cfp != NULL);

  if (!valid) {
    chDbgAssert(false, "null filters");
    return;
  }
  for (i = 0U; i < num; i++) {
    uint32_t maxid = cfp[i].filter_type == CAN_FILTER_TYPE_STD ?
                    0x7FFU : 0x1FFFFFFFU;

    valid = valid && (cfp[i].filter_type <= CAN_FILTER_TYPE_EXT) &&
            (cfp[i].filter_mode <= CAN_FILTER_MODE_CLASSIC) &&
            (cfp[i].filter_cfg >= CAN_FILTER_CFG_FIFO_0) &&
            (cfp[i].filter_cfg <= CAN_FILTER_CFG_REJECT) &&
            (cfp[i].identifier1 <= maxid) && (cfp[i].identifier2 <= maxid);
    if (cfp[i].filter_type == CAN_FILTER_TYPE_STD) {
      nstd++;
    }
    else {
      next++;
    }
  }
  valid = valid && (nstd <= STM32_FDCAN_FLS_NBR) &&
          (next <= STM32_FDCAN_FLE_NBR);
  chDbgAssert(valid, "invalid filters");
  if (!valid) {
    return;
  }

  stdp = canp->ram_base + SRAMCAN_FLSSA;
  extp = canp->ram_base + SRAMCAN_FLESA;
  for (i = 0U; i < STM32_FDCAN_FLS_NBR; i++) {
    stdp[i] = 0U;
  }
  for (i = 0U; i < 2U * STM32_FDCAN_FLE_NBR; i++) {
    extp[i] = 0U;
  }
  for (i = 0U; i < num; i++) {
    if (cfp[i].filter_type == CAN_FILTER_TYPE_STD) {
      *stdp++ = FDCAN_STD_FILTER_SFID1(cfp[i].identifier1) |
                FDCAN_STD_FILTER_SFID2(cfp[i].identifier2) |
                FDCAN_STD_FILTER_SFEC(cfp[i].filter_cfg) |
                FDCAN_STD_FILTER_SFT(cfp[i].filter_mode);
    }
    else {
      *extp++ = FDCAN_EXT_FILTER_EFID1(cfp[i].identifier1) |
                FDCAN_EXT_FILTER_EFEC(cfp[i].filter_cfg);
      *extp++ = FDCAN_EXT_FILTER_EFID2(cfp[i].identifier2) |
                FDCAN_EXT_FILTER_EFT(cfp[i].filter_mode);
    }
  }
}

/**
 * @brief   STM32-specific filter table replacement.
 * @note    Serialize this thread-context API with lifecycle operations.
 * @note    Traffic must be quiesced while the filter table is replaced.
 * @api
 */
void canSTM32SetFilters(hal_can_driver_c *canp, uint8_t num,
                        const CANFilter *cfp) {

  chDbgCheck(canp != NULL);
  chDbgAssert(canp->state == HAL_DRV_STATE_READY, "invalid state");
  can_lld_set_filters(canp, num, cfp);
}

#endif /* HAL_USE_CAN */

/** @} */
