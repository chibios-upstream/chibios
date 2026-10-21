/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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
 * @file    I2Cv1/hal_i2c_lld.c
 * @brief   AM67 (J722S) I2C subsystem low level driver source.
 * @details Interrupt-driven master-mode driver for the TI OMAP-style I2C
 *          controller. Init sequence, transfer state machine and the
 *          STOP/bus-free dance derived from NuttX
 *          arch/arm/src/am67/am67_i2c.c (Apache-2.0): STP is never
 *          pre-armed with a segment (writing it in the restart window
 *          fires an immediate STOP), every segment ends with the bus held
 *          and the final STOP is issued explicitly from the ARDY window.
 *
 *          Pads are not touched here. Which pads carry the bus, and whether
 *          the internal pull-ups are enabled on them, is a board decision,
 *          so the board configures them before the driver is started. See
 *          @p board_i2c0_pinmux() in the T3 Gemstone O1 board files for the
 *          reference implementation.
 *
 *          XHAL moves the thread-suspend/timeout bookkeeping the classic
 *          driver did itself (i2c_run_transfer()) up into the class layer
 *          (hal_i2c.c: i2cSynchronizeS() suspends and calls
 *          i2c_lld_stop_transfer() on timeout). This LLD only starts a
 *          transfer and reports completion through __i2c_complete_isr() /
 *          __i2c_error_isr(); it no longer owns a thread reference or a
 *          blocking wait of its own.
 *
 * @addtogroup I2C
 * @{
 */

#include "hal.h"

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Internal reference clock the prescaler divides down to, per the TRM
   recommendation for standard and fast mode.*/
#define I2C_INTERNAL_CLOCK          12000000U

/* Bound for the reset-done busy wait, avoids a silent hard hang if the
   module clock is gated.*/
#define I2C_RESET_WAIT_LOOPS        1000000U

/* Bound for the wait that observes the reset actually starting. Short: it
   is a race guard, not a completion wait, and falling through it is a
   legitimate outcome (see i2c_hw_init).*/
#define I2C_RESET_START_LOOPS       10000U

/* Bound for the wait that lets a STOP release a bus left held by a
   previous transaction.*/
#define I2C_BUS_FREE_LOOPS          100000U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief MCU_I2C0 I2C driver identifier.*/
#if (AM67_I2C_USE_MCU_I2C0 == TRUE) || defined(__DOXYGEN__)
hal_i2c_driver_c I2CD1;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline uint32_t i2c_getreg(hal_i2c_driver_c *i2cp, uint32_t offset) {

  return *(volatile uint32_t *)(i2cp->base + offset);
}

static inline void i2c_putreg(hal_i2c_driver_c *i2cp, uint32_t offset,
                              uint32_t value) {

  *(volatile uint32_t *)(i2cp->base + offset) = value;
}

/**
 * @brief   Controller reset and timing configuration.
 * @details Sequence from NuttX am67_i2c_init(): the TI I2C requires
 *          CON.EN=1 for the internal reset to complete, and no-idle mode
 *          with both clocks kept active so the module is not gated
 *          between transfers. Reads the SCL frequency from
 *          @p i2cp->config, which the caller (i2c_lld_start()) has
 *          already resolved and stored.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 */
static void i2c_hw_init(hal_i2c_driver_c *i2cp) {
  const hal_i2c_config_t *config = (const hal_i2c_config_t *)i2cp->config;
  uint32_t psc, d, scll, sclh, i;

  i2cp->ready      = false;
  i2cp->init_error = I2C_INIT_OK;

  /* Soft reset, completes only with the module enabled.

     RST_DONE still reads 1 for a short while after SRST is asserted, so
     polling it immediately can fall straight through while the reset has
     not even started. Everything written afterwards would then be wiped
     by the reset completing underneath us, leaving an enabled module with
     zeroed SCL timing: no clock is ever generated, START never completes
     and every transfer times out. Wait for RST_DONE to drop first, then
     for it to come back. Falling through the first wait is legitimate (a
     reset fast enough to miss), the readback below is the real guard.*/
  i2c_putreg(i2cp, I2C_CON_OFFSET, 0U);
  i2c_putreg(i2cp, I2C_SYSC_OFFSET, I2C_SYSC_SRST);
  i2c_putreg(i2cp, I2C_CON_OFFSET, I2C_CON_EN);
  for (i = 0U; i < I2C_RESET_START_LOOPS; i++) {
    if ((i2c_getreg(i2cp, I2C_SYSS_OFFSET) & I2C_SYSS_RST_DONE) == 0U) {
      break;
    }
  }
  for (i = 0U; i < I2C_RESET_WAIT_LOOPS; i++) {
    if ((i2c_getreg(i2cp, I2C_SYSS_OFFSET) & I2C_SYSS_RST_DONE) != 0U) {
      break;
    }
  }
  if (i >= I2C_RESET_WAIT_LOOPS) {
    i2cp->init_error = I2C_INIT_RESET_TIMEOUT;
    return;
  }

  i2c_putreg(i2cp, I2C_SYSC_OFFSET, I2C_SYSC_IDLE_NO | I2C_SYSC_CLK_BOTH);

  /* SCL timing, written with the module disabled. The prescaler divides
     the functional clock to the 12 MHz internal reference, then:
     SCL = 12 MHz / ((SCLL + 7) + (SCLH + 5)).*/
  i2c_putreg(i2cp, I2C_CON_OFFSET, 0U);

  if (config->frequency == 0U) {
    i2cp->init_error = I2C_INIT_BAD_FREQUENCY;
    return;
  }
  psc = (i2cp->clock / I2C_INTERNAL_CLOCK) - 1U;
  d   = I2C_INTERNAL_CLOCK / config->frequency;
  scll = (d / 2U) - 7U;
  sclh = (d / 2U) - 5U;
  if ((scll < 1U) || (scll > 255U) || (sclh > 255U)) {
    i2cp->init_error = I2C_INIT_BAD_FREQUENCY;
    return;
  }
  i2c_putreg(i2cp, I2C_PSC_OFFSET, psc);
  i2c_putreg(i2cp, I2C_SCLL_OFFSET, scll);
  i2c_putreg(i2cp, I2C_SCLH_OFFSET, sclh);

  /* Readback guard for the reset race described above: if the reset was
     still in flight these registers now read back as zero rather than
     what was just written.*/
  if ((i2c_getreg(i2cp, I2C_SCLL_OFFSET) != scll) ||
      (i2c_getreg(i2cp, I2C_SCLH_OFFSET) != sclh) ||
      (i2c_getreg(i2cp, I2C_PSC_OFFSET) != psc)) {
    i2cp->init_error = I2C_INIT_TIMING_LOST;
    return;
  }

  i2c_putreg(i2cp, I2C_CON_OFFSET, I2C_CON_EN);

  /* All wakeup sources on (the TI Linux driver does the same to stop the
     I2C freezing across WFI on AM6x-class devices).*/
  i2c_putreg(i2cp, I2C_WE_OFFSET, I2C_WE_ALLMASK);

  /* Clean slate: interrupts off, pending flags and FIFOs cleared.*/
  i2c_putreg(i2cp, I2C_IRQENABLE_CLR_OFFSET, I2C_IRQ_ALLMASK);
  i2c_putreg(i2cp, I2C_IRQSTATUS_OFFSET, I2C_IRQ_ALLMASK);
  i2c_putreg(i2cp, I2C_BUF_OFFSET, I2C_BUF_TXFIFO_CLR | I2C_BUF_RXFIFO_CLR);

  /* A soft reset resets this master but not the bus. If the previous
     transaction ended without a STOP (the NACK path releases the engine
     but leaves the bus held) the slave is still mid-transfer and BB stays
     set, so the next START would never be granted. Issue one STOP to
     release it before declaring the driver ready.*/
  if ((i2c_getreg(i2cp, I2C_IRQSTATUS_RAW_OFFSET) & I2C_IRQ_BB) != 0U) {
    i2c_putreg(i2cp, I2C_CON_OFFSET,
               I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
    for (i = 0U; i < I2C_BUS_FREE_LOOPS; i++) {
      if ((i2c_getreg(i2cp, I2C_IRQSTATUS_RAW_OFFSET) & I2C_IRQ_BB) == 0U) {
        break;
      }
    }
    i2c_putreg(i2cp, I2C_IRQSTATUS_OFFSET, I2C_IRQ_ALLMASK);
    i2c_putreg(i2cp, I2C_CON_OFFSET, I2C_CON_EN);

    if ((i2c_getreg(i2cp, I2C_IRQSTATUS_RAW_OFFSET) & I2C_IRQ_BB) != 0U) {
      /* Still held: a slave is clock stretching or holding SDA and only
         bit-banged clock pulses would recover it. Report rather than let
         every later transfer time out with no explanation.*/
      i2cp->init_error = I2C_INIT_BUS_STUCK;
      return;
    }
  }

  i2cp->ready = true;
}

/**
 * @brief   Programs and starts one transfer segment.
 * @details Issues a START (or repeated START when the bus is already
 *          held). STP is deliberately not pre-armed, see the file header.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @param[in] tx        true for the transmit segment, false for receive
 */
static void i2c_start_segment(hal_i2c_driver_c *i2cp, bool tx) {
  uint32_t con = I2C_CON_EN | I2C_CON_MST;
  size_t n = tx ? i2cp->txbytes : i2cp->rxbytes;

  i2c_putreg(i2cp, I2C_SA_OFFSET, (uint32_t)i2cp->addr);
  i2c_putreg(i2cp, I2C_CNT_OFFSET, (uint32_t)n);

  if (tx) {
    con |= I2C_CON_TRX;
  }
  if (i2cp->addr > 0x7FU) {
    con |= I2C_CON_XSA;
  }
  i2c_putreg(i2cp, I2C_CON_OFFSET, con);

  /* Errors, segment-done and the data direction in use; BF stays off
     until the final STOP is issued.*/
  i2c_putreg(i2cp, I2C_IRQENABLE_CLR_OFFSET,
             I2C_IRQ_XRDY | I2C_IRQ_RRDY | I2C_IRQ_BF);
  i2c_putreg(i2cp, I2C_IRQENABLE_SET_OFFSET,
             I2C_IRQ_FATALMASK | I2C_IRQ_ARDY |
             (tx ? I2C_IRQ_XRDY : I2C_IRQ_RRDY));

  i2c_putreg(i2cp, I2C_CON_OFFSET, con | I2C_CON_STT);
}

/**
 * @brief   Quiesces the controller after an error or a requested stop.
 * @details Mirrors the NuttX fatal-error path: release the bus and reset
 *          the transaction engine, otherwise the master is left
 *          mid-transaction and every later transfer times out.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 */
static void i2c_quiesce(hal_i2c_driver_c *i2cp) {

  i2c_putreg(i2cp, I2C_CON_OFFSET,
             i2c_getreg(i2cp, I2C_CON_OFFSET) | I2C_CON_STP);
  i2c_putreg(i2cp, I2C_IRQENABLE_CLR_OFFSET, I2C_IRQ_ALLMASK);
  i2c_putreg(i2cp, I2C_IRQSTATUS_OFFSET, I2C_IRQ_ALLMASK);
  i2c_putreg(i2cp, I2C_BUF_OFFSET, I2C_BUF_TXFIFO_CLR | I2C_BUF_RXFIFO_CLR);
  i2c_putreg(i2cp, I2C_CNT_OFFSET, 0U);
  i2cp->txbytes = 0U;
  i2cp->rxbytes = 0U;
}

/**
 * @brief   Shared interrupt service.
 * @details Ordered like the NuttX ISR: fatal errors first (checked in
 *          the raw status so they cannot hide behind masking), then data
 *          movement, then the segment/stop bookkeeping.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 */
static void i2c_serve_interrupt(hal_i2c_driver_c *i2cp) {
  uint32_t status = i2c_getreg(i2cp, I2C_IRQSTATUS_OFFSET);
  uint32_t raw    = i2c_getreg(i2cp, I2C_IRQSTATUS_RAW_OFFSET);
  uint32_t fatal  = raw & I2C_IRQ_FATALMASK;

  if (fatal != 0U) {
    if ((fatal & I2C_IRQ_NACK) != 0U) {
      i2cp->errors |= I2C_ACK_FAILURE;
    }
    if ((fatal & I2C_IRQ_AL) != 0U) {
      i2cp->errors |= I2C_ARBITRATION_LOST;
    }
    if ((fatal & I2C_IRQ_AERR) != 0U) {
      i2cp->errors |= I2C_BUS_ERROR;
    }
    i2c_quiesce(i2cp);
    __i2c_error_isr(i2cp);
    return;
  }

  /* A byte arrived: drain it before any phase decision so a pending
     restart/stop cannot strand it in the FIFO.*/
  if ((status & I2C_IRQ_RRDY) != 0U) {
    uint32_t data = i2c_getreg(i2cp, I2C_DATA_OFFSET);

    if (i2cp->rxbytes > 0U) {
      *i2cp->rxptr++ = (uint8_t)data;
      i2cp->rxbytes--;
    }
    i2c_putreg(i2cp, I2C_IRQSTATUS_OFFSET, I2C_IRQ_RRDY);
  }

  /* The transmitter wants a byte.*/
  if ((status & I2C_IRQ_XRDY) != 0U) {
    if (i2cp->txbytes > 0U) {
      i2c_putreg(i2cp, I2C_DATA_OFFSET, (uint32_t)*i2cp->txptr++);
      i2cp->txbytes--;
    }
    else {
      i2c_putreg(i2cp, I2C_IRQENABLE_CLR_OFFSET, I2C_IRQ_XRDY);
    }
    i2c_putreg(i2cp, I2C_IRQSTATUS_OFFSET, I2C_IRQ_XRDY);
  }

  /* Segment finished (ARDY) or bus released (BF).*/
  if ((status & (I2C_IRQ_ARDY | I2C_IRQ_BF)) != 0U) {
    i2c_putreg(i2cp, I2C_IRQSTATUS_OFFSET,
               status & (I2C_IRQ_ARDY | I2C_IRQ_BF));

    if ((i2cp->txbytes == 0U) && !i2cp->rx_started &&
        (i2cp->rxbytes > 0U)) {
      /* Transmit segment done, the receive segment follows with a
         repeated START, ARDY means the hardware is holding the bus.*/
      i2cp->rx_started = true;
      i2c_start_segment(i2cp, false);
    }
    else if ((i2cp->txbytes == 0U) && (i2cp->rxbytes == 0U)) {
      if (((raw & I2C_IRQ_BB) != 0U) && ((status & I2C_IRQ_BF) == 0U)) {
        /* All data done but the bus is still held: issue the final STOP
           (legal here, this is the ARDY window) and wait for the
           bus-free event. Skip STP if a STOP is already in flight.*/
        if ((i2c_getreg(i2cp, I2C_CON_OFFSET) & I2C_CON_STP) == 0U) {
          i2c_putreg(i2cp, I2C_CON_OFFSET,
                     i2c_getreg(i2cp, I2C_CON_OFFSET) | I2C_CON_STP);
        }
        i2c_putreg(i2cp, I2C_IRQENABLE_SET_OFFSET, I2C_IRQ_BF);
      }
      else {
        /* Transfer complete and bus free.*/
        i2c_putreg(i2cp, I2C_IRQENABLE_CLR_OFFSET, I2C_IRQ_ALLMASK);
        __i2c_complete_isr(i2cp);
      }
    }
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (AM67_I2C_USE_MCU_I2C0 == TRUE) || defined(__DOXYGEN__)
static bool mcu_i2c0_irq_handler(void *arg) {
  bool preemption_required;

  (void)arg;

  i2c_serve_interrupt(&I2CD1);

  chSysLockFromISR();
  preemption_required = chSchIsPreemptionRequired();
  chSysUnlockFromISR();

  return preemption_required;
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if AM67_I2C_USE_MCU_I2C0 == TRUE
  i2cObjectInit(&I2CD1);
  I2CD1.base  = AM67_MCU_I2C0_BASE;
  I2CD1.clock = AM67_MCU_I2C0_CLOCK;
  I2CD1.ready = false;
  vimSetHandler(AM67_MCU_I2C0_IRQ, mcu_i2c0_irq_handler, NULL);
  vimSetPriority(AM67_MCU_I2C0_IRQ, AM67_I2C_MCU_I2C0_IRQ_PRIORITY);
#endif
}

/**
 * @brief   Configures and activates the I2C peripheral.
 * @details Called once per STOP->READY transition. The class layer has
 *          already resolved @p i2cp->config through i2c_lld_setcfg()
 *          before this runs (see __i2c_start_impl()).
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t i2c_lld_start(hal_i2c_driver_c *i2cp) {
  const hal_i2c_config_t *config = (const hal_i2c_config_t *)i2cp->config;

  if (config == NULL) {
    config = i2c_lld_selcfg(i2cp, 0U);
  }
  if (config == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }
  i2cp->config = config;

#if AM67_I2C_USE_MCU_I2C0 == TRUE
  if (i2cp == &I2CD1) {
    i2c_hw_init(i2cp);
    if (!i2cp->ready) {
      if (i2cp->init_error == I2C_INIT_BAD_FREQUENCY) {
        return HAL_RET_CONFIG_ERROR;
      }
      if (i2cp->init_error == I2C_INIT_BUS_STUCK) {
        return HAL_RET_HW_BUSY;
      }
      return HAL_RET_HW_FAILURE;
    }
    vimEnableInterrupt(AM67_MCU_I2C0_IRQ);
  }
#endif

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 *
 * @notapi
 */
void i2c_lld_stop(hal_i2c_driver_c *i2cp) {

  if (i2cp->state != HAL_DRV_STATE_STOP) {
#if AM67_I2C_USE_MCU_I2C0 == TRUE
    if (i2cp == &I2CD1) {
      vimDisableInterrupt(AM67_MCU_I2C0_IRQ);
    }
#endif
    i2c_putreg(i2cp, I2C_IRQENABLE_CLR_OFFSET, I2C_IRQ_ALLMASK);
    i2c_putreg(i2cp, I2C_CON_OFFSET, 0U);
    i2cp->ready = false;
  }
}

/**
 * @brief   I2C configuration.
 * @details AM67 has a single configurable field (SCL frequency) and no
 *          board-independent default, so a @p NULL configuration always
 *          falls through to @p i2c_lld_selcfg().
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @param[in] config    pointer to the @p hal_i2c_config_t structure
 * @return              A pointer to the configuration to use.
 *
 * @notapi
 */
const hal_i2c_config_t *i2c_lld_setcfg(hal_i2c_driver_c *i2cp,
                                       const hal_i2c_config_t *config) {

  if (config == NULL) {
    return i2c_lld_selcfg(i2cp, 0U);
  }

  return config;
}

/**
 * @brief   Selects one of the pre-defined I2C configurations.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer, or @p NULL if invalid.
 *
 * @notapi
 */
const hal_i2c_config_t *i2c_lld_selcfg(hal_i2c_driver_c *i2cp,
                                       unsigned cfgnum) {
  (void)i2cp;

#if I2C_USE_CONFIGURATIONS == TRUE
  extern const i2c_configurations_t i2c_configurations;

  if (cfgnum < i2c_configurations.cfgsnum) {
    return &i2c_configurations.cfgs[cfgnum];
  }
#else
  (void)cfgnum;
#endif

  return NULL;
}

/**
 * @brief   Low level callback configuration hook.
 * @details No hardware side effect needed: this driver always runs
 *          interrupt-driven regardless of whether a callback is set, so
 *          there is no interrupt source to gate on it.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @param[in] cb        the callback, or @p NULL to disable it
 *
 * @notapi
 */
void i2c_lld_set_callback(hal_i2c_driver_c *i2cp, drv_cb_t cb) {

  (void)i2cp;
  (void)cb;
}

/**
 * @brief   Starts an I2C master transmit transaction.
 * @details When @p rxbytes is greater than zero, a receive phase follows
 *          the transmit phase using a repeated START.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @return              The operation status.
 *
 * @notapi
 */
msg_t i2c_lld_start_master_transmit(hal_i2c_driver_c *i2cp, i2caddr_t addr,
                                    const uint8_t *txbuf, size_t txbytes,
                                    uint8_t *rxbuf, size_t rxbytes) {

  if (!i2cp->ready) {
    return HAL_RET_HW_FAILURE;
  }

  i2cp->addr       = addr;
  i2cp->txptr      = txbuf;
  i2cp->txbytes    = txbytes;
  i2cp->rxptr      = rxbuf;
  i2cp->rxbytes    = rxbytes;
  i2cp->rx_started = false;

  i2c_start_segment(i2cp, true);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Starts an I2C master receive transaction.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @param[in] addr      slave device address
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @return              The operation status.
 *
 * @notapi
 */
msg_t i2c_lld_start_master_receive(hal_i2c_driver_c *i2cp, i2caddr_t addr,
                                   uint8_t *rxbuf, size_t rxbytes) {

  if (!i2cp->ready) {
    return HAL_RET_HW_FAILURE;
  }

  i2cp->addr       = addr;
  i2cp->txptr      = NULL;
  i2cp->txbytes    = 0U;
  i2cp->rxptr      = rxbuf;
  i2cp->rxbytes    = rxbytes;
  i2cp->rx_started = true;

  i2c_start_segment(i2cp, false);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops the ongoing I2C transfer, if any.
 * @details Called by the class layer both for an explicit @p
 *          i2cStopTransfer() and when @p i2cSynchronizeS() times out.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t i2c_lld_stop_transfer(hal_i2c_driver_c *i2cp) {

  i2c_quiesce(i2cp);

  return HAL_RET_SUCCESS;
}

#endif /* HAL_USE_I2C == TRUE */

/** @} */
