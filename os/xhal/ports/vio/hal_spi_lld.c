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
 * @file    sandbox/hal_spi_lld.c
 * @brief   Sandbox SPI subsystem low level driver source.
 *
 * @addtogroup SPI
 * @{
 */

#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief SPI1 driver identifier.*/
#if VIO_SPI_USE_VSPI1 || defined(__DOXYGEN__)
hal_spi_driver_c SPID1;
#endif

/** @brief SPI2 driver identifier.*/
#if VIO_SPI_USE_VSPI2 || defined(__DOXYGEN__)
hal_spi_driver_c SPID2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared end-of-rx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] sts       VRQ flags
 */
static void spi_lld_serve_interrupt(hal_spi_driver_c *spip, uint32_t sts) {

  if ((sts & (1U << HAL_DRV_STATE_ERROR)) != 0U) {
    /* Transfer error interrupt, discards all others. The host has already
       stopped the transfer, the driver goes back to READY.*/
    _spi_isr_error_code(spip);
    return;
  }

  /* Operation finished interrupt.*/
  if ((sts & (1U << HAL_DRV_STATE_COMPLETE)) != 0U) {
    _spi_isr_complete_code(spip);
    return;
  }

  /* Circular buffer events from the same status snapshot.*/
  _spi_isr_circular_code(spip, (sts & (1U << HAL_DRV_STATE_HALF)) != 0U,
                         (sts & (1U << HAL_DRV_STATE_FULL)) != 0U);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if VIO_SPI_USE_VSPI1 || defined(__DOXYGEN__)
#if !defined(VIO_VSPI1_SUPPRESS_ISR)
SB_VRQ_HANDLER(MK_VECTOR(VIO_VSPI1_IRQ)) {

  CH_IRQ_PROLOGUE();

  spi_lld_serve_interrupt(&SPID1, flags);

  CH_IRQ_EPILOGUE();
}
#endif
#endif

#if VIO_SPI_USE_VSPI2 || defined(__DOXYGEN__)
#if !defined(VIO_VSPI2_SUPPRESS_ISR)
SB_VRQ_HANDLER(MK_VECTOR(VIO_VSPI2_IRQ)) {

  CH_IRQ_PROLOGUE();

  spi_lld_serve_interrupt(&SPID2, flags);

  CH_IRQ_EPILOGUE();
}
#endif
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

  /* Driver instances initialization.*/
#if VIO_SPI_USE_VSPI1 == TRUE
  spiObjectInit(&SPID1);
  SPID1.nvspi = 0U;
  __sb_vrq_seten(1U << VIO_VSPI1_IRQ);
#endif
#if VIO_SPI_USE_VSPI2 == TRUE
  spiObjectInit(&SPID2);
  SPID2.nvspi = 1U;
  __sb_vrq_seten(1U << VIO_VSPI2_IRQ);
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_start(hal_spi_driver_c *spip) {
  msg_t msg = HAL_RET_SUCCESS;

  /* Enables the peripheral.*/
  if (false) {
  }
#if VIO_SPI_USE_VSPI1 == TRUE
  else if (&SPID1 == spip) {
    msg = vspiInit(spip->nvspi, &spip->cfgbuf, sizeof (hal_spi_config_t));
  }
#endif
#if VIO_SPI_USE_VSPI2 == TRUE
  else if (&SPID2 == spip) {
    msg = vspiInit(spip->nvspi, &spip->cfgbuf, sizeof (hal_spi_config_t));
  }
#endif
  else {
    chDbgAssert(false, "invalid SPI instance");
  }

  if (msg == HAL_RET_SUCCESS) {
    spip->config = &spip->cfgbuf;
  }

  return msg;
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 *
 * @notapi
 */
void spi_lld_stop(hal_spi_driver_c *spip) {
  msg_t msg = HAL_RET_SUCCESS;

  /* Disables the peripheral.*/
  if (false) {
  }
#if VIO_SPI_USE_VSPI1 == TRUE
  else if (&SPID1 == spip) {
    msg = vspiDeinit(spip->nvspi);
  }
#endif
#if VIO_SPI_USE_VSPI2 == TRUE
  else if (&SPID2 == spip) {
    msg = vspiDeinit(spip->nvspi);
  }
#endif
  else {
    chDbgAssert(false, "invalid SIO instance");
  }

  chDbgAssert(msg == HAL_RET_SUCCESS, "unexpected failure");
}

/**
 * @brief   Applies an explicit SPI configuration.
 * @note    The VIO port does not accept arbitrary sandbox-provided
 *          configurations. Only host-approved predefined configurations
 *          selected using @p drvSelectCfgX() are supported.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] config    pointer to the @p hal_spi_config_t structure
 * @return              Always @p NULL in the VIO port.
 *
 * @notapi
 */
const hal_spi_config_t *spi_lld_setcfg(hal_spi_driver_c *spip,
                                       const hal_spi_config_t *config) {

  (void)spip;
  (void)config;

  /* Explicit configurations are intentionally rejected in this port. */
  return NULL;
}

/**
 * @brief       Selects one of the host-defined SPI configurations.
 * @note        The selected configuration is mirrored locally in
 *              @p spip->cfgbuf after host-side validation and selection.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer.
 *
 * @notapi
 */
const hal_spi_config_t *spi_lld_selcfg(hal_spi_driver_c *spip,
                                       unsigned cfgnum) {
  msg_t msg;

  msg = vspiSelectCfg(spip->nvspi, (uint32_t)cfgnum, &spip->cfgbuf,
                      sizeof (hal_spi_config_t));
  if (msg == HAL_RET_SUCCESS) {

    return &spip->cfgbuf;
  }

  return NULL;
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 *
 * @notapi
 */
void spi_lld_select(hal_spi_driver_c *spip) {

  (void)vspiSelect(spip->nvspi);
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 *
 * @notapi
 */
void spi_lld_unselect(hal_spi_driver_c *spip) {

  (void)vspiUnselect(spip->nvspi);
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This synchronous function performs the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @pre     In order to use this function the option @p SPI_USE_SYNCHRONIZATION
 *          must be enabled.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to be ignored
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_ignore(hal_spi_driver_c *spip, size_t n) {

  return vspiStartIgnore(spip->nvspi, n);
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_exchange(hal_spi_driver_c *spip, size_t n,
                       const void *txbuf, void *rxbuf) {

  return vspiStartExchange(spip->nvspi, n, txbuf, rxbuf);
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_send(hal_spi_driver_c *spip, size_t n, const void *txbuf) {

  return vspiStartSend(spip->nvspi, n, txbuf);
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_receive(hal_spi_driver_c *spip, size_t n, void *rxbuf) {

  return vspiStartReceive(spip->nvspi, n, rxbuf);
}

/**
 * @brief   Aborts the ongoing SPI operation, if any.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[out] sizep    pointer to the counter of frames not yet transferred
 *                      or @p NULL
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_stop_transfer(hal_spi_driver_c *spip, size_t *sizep) {
  size_t n;

  /* The host requires a valid counter pointer.*/
  return vspiStopTransfer(spip->nvspi, sizep != NULL ? sizep : &n);
}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 */
uint16_t spi_lld_polled_exchange(hal_spi_driver_c *spip, uint16_t frame) {
  uint16_t rxframe = 0U;

#if SPI_USE_SYNCHRONIZATION == TRUE
  msg_t msg;

  msg = spiExchange(spip, 1U, &frame, &rxframe);
  chDbgAssert(msg == HAL_RET_SUCCESS, "unexpected failure");
#else
  (void)spip;
  (void)frame;
  chDbgAssert(false, "SPI_USE_SYNCHRONIZATION required");
#endif

  return rxframe;
}

#endif /* HAL_USE_SPI */

/** @} */
