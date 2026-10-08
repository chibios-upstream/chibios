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
 * @file    CANv1/hal_can_lld.c
 * @brief   STM32 CAN subsystem low level driver source.
 *
 * @addtogroup CAN
 * @{
 */

#include "hal.h"

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define CAN_START_TIMEOUT          TIME_MS2I(250U)
#define CAN_MCR_OPTIONS            (CAN_MCR_DBF | CAN_MCR_TTCM | \
                                    CAN_MCR_ABOM | CAN_MCR_AWUM | \
                                    CAN_MCR_NART | CAN_MCR_RFLM | CAN_MCR_TXFP)
#define CAN_BTR_OPTIONS            (CAN_BTR_SILM | CAN_BTR_LBKM | \
                                    CAN_BTR_SJW_Msk | CAN_BTR_TS2_Msk | \
                                    CAN_BTR_TS1_Msk | CAN_BTR_BRP_Msk)
#define CAN_TSR_COMPLETIONS        (CAN_TSR_RQCP0 | CAN_TSR_RQCP1 | CAN_TSR_RQCP2)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

hal_can_driver_c CAND1;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/* Thread-context startup handshakes, not used by X/I-class APIs.*/
static bool can_lld_wait(hal_can_driver_c *canp, uint32_t mask, uint32_t value) {
  systime_t start = chVTGetSystemTimeX();

  while ((canp->can->MSR & mask) != value) {
    if (chTimeDiffX(start, chVTGetSystemTimeX()) >= CAN_START_TIMEOUT) {
      return false;
    }
    chThdSleep(1);
  }
  return true;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the driver without accessing stopped hardware.
 * @notapi
 */
void can_lld_init(void) {

  canObjectInit(&CAND1);
  CAND1.can = CAN1;
}

/**
 * @brief   Configures and starts the controller.
 * @notapi
 */
msg_t can_lld_start(hal_can_driver_c *canp) {
  const hal_can_config_t *cfg = (const hal_can_config_t *)canp->config;

  if (cfg == NULL) {
    cfg = can_lld_selcfg(canp, 0U);
  }
  if ((cfg == NULL) || (can_lld_setcfg(canp, cfg) == NULL)) {
    return HAL_RET_CONFIG_ERROR;
  }
  canp->config = cfg;

  rccEnableCAN1(true);
  rccResetCAN1();
  canp->can->IER = 0U;

  /* Reset leaves bxCAN asleep. INRQ must be set with SLEEP cleared.*/
  canp->can->MCR = CAN_MCR_INRQ;
  if (!can_lld_wait(canp, CAN_MSR_INAK | CAN_MSR_SLAK, CAN_MSR_INAK)) {
    goto failed;
  }
  canp->can->BTR = cfg->btr;
  canp->can->MCR = cfg->mcr | CAN_MCR_INRQ;
  can_lld_set_filters(canp, 0U, NULL);
  can_lld_reset(canp);

  /* Exit requires bus synchronization (11 recessive bits). A disconnected
     or dominant RX input must not leave drvStart() blocked indefinitely.*/
  canp->can->MCR = cfg->mcr;
  if (!can_lld_wait(canp, CAN_MSR_INAK, 0U)) {
    goto failed;
  }

  canp->can->MSR = CAN_MSR_ERRI | CAN_MSR_WKUI | CAN_MSR_SLAKI;
  canp->can->TSR = CAN_TSR_COMPLETIONS;
  canp->can->IER = CAN_IER_TMEIE | CAN_IER_FMPIE0 | CAN_IER_FMPIE1 |
                   CAN_IER_ERRIE | CAN_IER_BOFIE | CAN_IER_EPVIE |
                   CAN_IER_EWGIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1
#if CAN_USE_SLEEP_MODE
                   | CAN_IER_WKUIE
#endif
#if STM32_CAN_REPORT_ALL_ERRORS
                   | CAN_IER_LECIE
#endif
                   ;

  return HAL_RET_SUCCESS;

failed:
  canp->can->IER = 0U;
  rccResetCAN1();
  rccDisableCAN1();
  return HAL_RET_HW_FAILURE;
}

/**
 * @brief   Disables interrupts and discards pending transfers.
 * @notapi
 */
void can_lld_stop(hal_can_driver_c *canp) {

  canp->can->IER = 0U;
  rccResetCAN1();
  rccDisableCAN1();
}

/**
 * @brief   Resets the frontend's cached notifications.
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
 * @note    Hardware changes require stop/start.
 * @notapi
 */
const hal_can_config_t *can_lld_setcfg(hal_can_driver_c *canp,
                                      const hal_can_config_t *config) {

  if (config == NULL) {
    return can_lld_selcfg(canp, 0U);
  }
  if (((config->mcr & ~CAN_MCR_OPTIONS) != 0U) ||
      ((config->btr & ~CAN_BTR_OPTIONS) != 0U) ||
      (((config->btr & CAN_BTR_SJW_Msk) >> CAN_BTR_SJW_Pos) >
       ((config->btr & CAN_BTR_TS2_Msk) >> CAN_BTR_TS2_Pos))) {
    return NULL;
  }
  if ((canp->state == HAL_DRV_STATE_READY) && (config != canp->config)) {
    return NULL;
  }
  return config;
}

/**
 * @brief   Selects a user configuration; no guessed default bit rate.
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
 * @brief   Callback association hook; notifications also serve waiters.
 * @notapi
 */
void can_lld_set_callback(hal_can_driver_c *canp, drv_cb_t cb) {

  (void)canp;
  (void)cb;
}

/**
 * @brief   Determines whether a frame can be transmitted.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval false        no space in the transmit queue.
 * @retval true         transmit slot available.
 *
 * @notapi
 */
bool can_lld_is_tx_empty(hal_can_driver_c *canp, canmbx_t mailbox) {

  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    return (canp->can->TSR & CAN_TSR_TME) != 0;
  case 1:
    return (canp->can->TSR & CAN_TSR_TME0) != 0;
  case 2:
    return (canp->can->TSR & CAN_TSR_TME1) != 0;
  case 3:
    return (canp->can->TSR & CAN_TSR_TME2) != 0;
  default:
    return false;
  }
}

/**
 * @brief   Inserts a frame into the transmit queue.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 * @param[in] ctfp      pointer to the CAN frame to be transmitted
 * @param[in] mailbox   mailbox number,  @p CAN_ANY_MAILBOX for any mailbox
 *
 * @notapi
 */
void can_lld_transmit(hal_can_driver_c *canp,
                      canmbx_t mailbox,
                      const CANTxFrame *ctfp) {
  uint32_t tir, data[2] = {0U, 0U};
  unsigned i, bytes;
  CAN_TxMailBox_TypeDef *tmbp;

  /* Pointer to a free transmission mailbox.*/
  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    tmbp = &canp->can->sTxMailBox[(canp->can->TSR & CAN_TSR_CODE) >> 24];
    break;
  case 1:
    tmbp = &canp->can->sTxMailBox[0];
    break;
  case 2:
    tmbp = &canp->can->sTxMailBox[1];
    break;
  case 3:
    tmbp = &canp->can->sTxMailBox[2];
    break;
  default:
    return;
  }

  /* Preparing the message.*/
  if (ctfp->IDE)
    tir = ((uint32_t)ctfp->EID << 3) | ((uint32_t)ctfp->RTR << 1) |
          CAN_TI0R_IDE;
  else
    tir = ((uint32_t)ctfp->SID << 21) | ((uint32_t)ctfp->RTR << 1);
  tmbp->TDTR = ctfp->DLC;
  bytes = ctfp->RTR ? 0U : (ctfp->DLC > 8U ? 8U : ctfp->DLC);
  for (i = 0U; i < bytes; i++) {
    data[i / 4U] |= (uint32_t)ctfp->data8[i] << ((i % 4U) * 8U);
  }
  tmbp->TDLR = data[0];
  tmbp->TDHR = data[1];
  tmbp->TIR  = tir | CAN_TI0R_TXRQ;
}

/**
 * @brief   Determines whether a frame has been received.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval false        no space in the transmit queue.
 * @retval true         transmit slot available.
 *
 * @notapi
 */
bool can_lld_is_rx_nonempty(hal_can_driver_c *canp, canmbx_t mailbox) {

  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    return ((canp->can->RF0R & CAN_RF0R_FMP0) != 0 ||
            (canp->can->RF1R & CAN_RF1R_FMP1) != 0);
  case 1:
    return (canp->can->RF0R & CAN_RF0R_FMP0) != 0;
  case 2:
    return (canp->can->RF1R & CAN_RF1R_FMP1) != 0;
  default:
    return false;
  }
}

/**
 * @brief   Receives a frame from the input queue.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 * @param[out] crfp     pointer to the buffer where the CAN frame is copied
 *
 * @notapi
 */
void can_lld_receive(hal_can_driver_c *canp,
                     canmbx_t mailbox,
                     CANRxFrame *crfp) {
  uint32_t rir, rdtr;

  if (mailbox == CAN_ANY_MAILBOX) {
    if ((canp->can->RF0R & CAN_RF0R_FMP0) != 0)
      mailbox = 1;
    else if ((canp->can->RF1R & CAN_RF1R_FMP1) != 0)
      mailbox = 2;
    else {
      /* Should not happen, do nothing.*/
      return;
    }
  }
  switch (mailbox) {
  case 1:
    /* Fetches the message.*/
    rir  = canp->can->sFIFOMailBox[0].RIR;
    rdtr = canp->can->sFIFOMailBox[0].RDTR;
    crfp->data32[0] = canp->can->sFIFOMailBox[0].RDLR;
    crfp->data32[1] = canp->can->sFIFOMailBox[0].RDHR;

    /* Releases the mailbox.*/
    canp->can->RF0R = CAN_RF0R_RFOM0;

    /* If the queue is empty re-enables the interrupt in order to generate
       events again.*/
    if ((canp->can->RF0R & CAN_RF0R_FMP0) == 0)
      canp->can->IER |= CAN_IER_FMPIE0;
    break;
  case 2:
    /* Fetches the message.*/
    rir  = canp->can->sFIFOMailBox[1].RIR;
    rdtr = canp->can->sFIFOMailBox[1].RDTR;
    crfp->data32[0] = canp->can->sFIFOMailBox[1].RDLR;
    crfp->data32[1] = canp->can->sFIFOMailBox[1].RDHR;

    /* Releases the mailbox.*/
    canp->can->RF1R = CAN_RF1R_RFOM1;

    /* If the queue is empty re-enables the interrupt in order to generate
       events again.*/
    if ((canp->can->RF1R & CAN_RF1R_FMP1) == 0)
      canp->can->IER |= CAN_IER_FMPIE1;
    break;
  default:
    /* Should not happen, do nothing.*/
    return;
  }

  /* Decodes the various fields in the RX frame.*/
  crfp->RTR = (rir & CAN_RI0R_RTR) >> 1;
  crfp->IDE = (rir & CAN_RI0R_IDE) >> 2;
  if (crfp->IDE)
    crfp->EID = rir >> 3;
  else
    crfp->SID = rir >> 21;
  crfp->DLC = rdtr & CAN_RDT0R_DLC;
  crfp->FMI = (uint8_t)(rdtr >> 8);
  crfp->TIME = (uint16_t)(rdtr >> 16);
}

/**
 * @brief   Tries to abort an ongoing transmission.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 * @param[in] mailbox   mailbox number
 *
 * @notapi
 */
void can_lld_abort(hal_can_driver_c *canp,
                   canmbx_t mailbox) {

  if ((mailbox >= 1U) && (mailbox <= CAN_TX_MAILBOXES)) {
    canp->can->TSR = CAN_TSR_ABRQ0 << ((mailbox - 1U) * 8U);
  }
}

#if CAN_USE_SLEEP_MODE || defined(__DOXYGEN__)
/**
 * @brief   Enters the sleep mode.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 *
 * @notapi
 */
void can_lld_sleep(hal_can_driver_c *canp) {

  canp->can->MCR |= CAN_MCR_SLEEP;
}

/**
 * @brief   Enforces leaving the sleep mode.
 *
 * @param[in] canp      pointer to the @p hal_can_driver_c object
 *
 * @notapi
 */
void can_lld_wakeup(hal_can_driver_c *canp) {

  canp->can->MCR &= ~CAN_MCR_SLEEP;
}
#endif /* CAN_USE_SLEEP_MODE */

/**
 * @brief   Services transmission completions.
 * @notapi
 */
void can_lld_serve_tx_interrupt(hal_can_driver_c *canp) {
  uint32_t tsr = canp->can->TSR;
  canmbxmask_t flags = 0U;
  unsigned i;

  /* Only RQCP bits are W1C acknowledgements; never replay abort requests.*/
  canp->can->TSR = tsr & CAN_TSR_COMPLETIONS;
  if (((canp->can->IER & CAN_IER_TMEIE) == 0U) ||
      ((canp->state != HAL_DRV_STATE_READY) && (canp->state != CAN_SLEEP))) {
    return;
  }
  for (i = 0U; i < CAN_TX_MAILBOXES; i++) {
    uint32_t status = tsr >> (i * 8U);
    canmbxmask_t mask = CAN_MAILBOX_TO_MASK(i + 1U);

    if ((status & CAN_TSR_RQCP0) != 0U) {
      flags |= mask;
      /* A completed abort is not a successful transmission either.*/
      if ((status & CAN_TSR_TXOK0) == 0U) {
        flags |= mask << 16U;
      }
    }
  }
  if (flags != 0U) {
    _can_tx_empty_isr(canp, flags);
  }
}

/**
 * @brief   Services one receive FIFO.
 */
static void can_lld_serve_rx_interrupt(hal_can_driver_c *canp,
                                       unsigned fifo) {
  volatile uint32_t *rfr = fifo == 0U ? &canp->can->RF0R : &canp->can->RF1R;
  uint32_t status = *rfr;
  uint32_t fmpie = fifo == 0U ? CAN_IER_FMPIE0 : CAN_IER_FMPIE1;
  uint32_t fovie = fifo == 0U ? CAN_IER_FOVIE0 : CAN_IER_FOVIE1;
  uint32_t ier = canp->can->IER;

  /* FIFO register layouts match. Acknowledge overflow before callbacks,
     without releasing a frame or clearing unrelated W1C bits.*/
  if ((status & CAN_RF0R_FOVR0) != 0U) {
    *rfr = CAN_RF0R_FOVR0;
  }
  if ((canp->state != HAL_DRV_STATE_READY) && (canp->state != CAN_SLEEP)) {
    return;
  }
  if (((status & CAN_RF0R_FMP0) != 0U) && ((ier & fmpie) != 0U)) {
    /* The consumer must drain this FIFO. receive() rearms the notification
       once it is empty, matching the classic HAL notification model.*/
    canp->can->IER &= ~fmpie;
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(fifo + 1U));
  }
  if (((status & CAN_RF0R_FOVR0) != 0U) && ((ier & fovie) != 0U)) {
    _can_error_isr(canp, CAN_OVERFLOW_ERROR);
  }
}

/**
 * @brief   Services FIFO0.
 * @notapi
 */
void can_lld_serve_rx0_interrupt(hal_can_driver_c *canp) {

  can_lld_serve_rx_interrupt(canp, 0U);
}

/**
 * @brief   Services FIFO1.
 * @notapi
 */
void can_lld_serve_rx1_interrupt(hal_can_driver_c *canp) {

  can_lld_serve_rx_interrupt(canp, 1U);
}

/**
 * @brief   Services enabled status-change and error events.
 * @notapi
 */
void can_lld_serve_sce_interrupt(hal_can_driver_c *canp) {
  uint32_t msr = canp->can->MSR;
  uint32_t ier = canp->can->IER;
  uint32_t esr = canp->can->ESR;
  canerror_t errors;

  canp->can->MSR = msr & (CAN_MSR_ERRI | CAN_MSR_WKUI | CAN_MSR_SLAKI);
  if ((canp->state != HAL_DRV_STATE_READY) && (canp->state != CAN_SLEEP)) {
    return;
  }
#if CAN_USE_SLEEP_MODE
  if (((msr & CAN_MSR_WKUI) != 0U) && ((ier & CAN_IER_WKUIE) != 0U) &&
      (canp->state == CAN_SLEEP)) {
    can_lld_wakeup(canp);
    _can_wakeup_isr(canp);
  }
#endif
  if (((msr & CAN_MSR_ERRI) != 0U) && ((ier & CAN_IER_ERRIE) != 0U)) {
    errors = (esr & (CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF));
#if STM32_CAN_REPORT_ALL_ERRORS
    if (((esr & CAN_ESR_LEC) != 0U) && ((esr & CAN_ESR_LEC) != CAN_ESR_LEC)) {
      errors |= CAN_FRAMING_ERROR;
    }
#endif
    /* Retain HAL's raw ESR low halfword in bits 31:16.*/
    _can_error_isr(canp, errors | (esr << 16U));
  }
}

/**
 * @brief   Replaces all filter banks; zero entries selects accept-all FIFO0.
 * @note    Caller serializes access and quiesces traffic during replacement.
 * @notapi
 */
void can_lld_set_filters(hal_can_driver_c *canp, uint8_t num,
                         const CANFilter *cfp) {
  uint32_t used = 0U;
  unsigned i;
  bool valid = (num <= STM32_CAN_MAX_FILTERS) && ((num == 0U) || (cfp != NULL));

  if (valid) {
    for (i = 0U; i < num; i++) {
      uint32_t mask;

      if (cfp[i].filter >= STM32_CAN_MAX_FILTERS) {
        valid = false;
        break;
      }
      mask = 1U << cfp[i].filter;
      if ((used & mask) != 0U) {
        valid = false;
        break;
      }
      used |= mask;
    }
  }
  chDbgAssert(valid, "invalid filters");
  if (!valid) {
    return;
  }

  canp->can->FMR |= CAN_FMR_FINIT;
  canp->can->FA1R = 0U;
  canp->can->FM1R = 0U;
  canp->can->FS1R = 0U;
  canp->can->FFA1R = 0U;
  for (i = 0U; i < STM32_CAN_MAX_FILTERS; i++) {
    canp->can->sFilterRegister[i].FR1 = 0U;
    canp->can->sFilterRegister[i].FR2 = 0U;
  }
  if (num == 0U) {
    canp->can->FS1R = 1U;
    canp->can->FA1R = 1U;
  }
  else {
    for (i = 0U; i < num; i++) {
      uint32_t mask = 1U << cfp[i].filter;

      if (cfp[i].mode != 0U) {
        canp->can->FM1R |= mask;
      }
      if (cfp[i].scale != 0U) {
        canp->can->FS1R |= mask;
      }
      if (cfp[i].assignment != 0U) {
        canp->can->FFA1R |= mask;
      }
      canp->can->sFilterRegister[cfp[i].filter].FR1 = cfp[i].register1;
      canp->can->sFilterRegister[cfp[i].filter].FR2 = cfp[i].register2;
    }
    canp->can->FA1R = used;
  }
  canp->can->FMR &= ~CAN_FMR_FINIT;
}

/**
 * @brief   Replaces the filter table of a ready controller.
 * @note    Serialize with lifecycle operations and quiesce bus traffic;
 *          reception is disabled while FINIT is set. Reapply after restart.
 * @api
 */
void canSTM32SetFilters(hal_can_driver_c *canp, uint8_t num,
                        const CANFilter *cfp) {

  chDbgCheck(canp != NULL);
  chSysLock();
  chDbgAssert(canp->state == HAL_DRV_STATE_READY, "invalid state");
  can_lld_set_filters(canp, num, cfp);
  chSysUnlock();
}

#endif /* HAL_USE_CAN */

/** @} */
