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
 * @file    UARTv1/ti_uart.h
 * @brief   TI UART registers layout header.
 * @details 16550-compatible with TI extensions. All registers are 32 bits
 *          wide at stride 4 and must be accessed with full 32-bit reads and
 *          writes, a K3 interconnect requirement.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef TI_UART_H
#define TI_UART_H

/**
 * @brief   TI UART registers block.
 */
typedef struct {
  volatile uint32_t     RBR_THR_DLL;
  volatile uint32_t     IER_DLH;
  volatile uint32_t     IIR_FCR;
  volatile uint32_t     LCR;
  volatile uint32_t     MCR;
  volatile uint32_t     LSR;
  volatile uint32_t     MSR;
  volatile uint32_t     SCR;
  volatile uint32_t     MDR1;
  volatile uint32_t     resvd1[8];
  volatile uint32_t     SSR;
  volatile uint32_t     resvd2[3];
  volatile uint32_t     SYSC;
  volatile uint32_t     SYSS;
} TI_UART_TypeDef;

/**
 * @name    IER bits (DLAB = 0)
 * @{
 */
#define TI_UART_IER_ERBFI                   (1U << 0)
#define TI_UART_IER_ETBEI                   (1U << 1)
#define TI_UART_IER_ELSI                    (1U << 2)
#define TI_UART_IER_EDSSI                   (1U << 3)
/** @} */

/**
 * @name    IIR bits (read)
 * @{
 */
#define TI_UART_IIR_INTSTATUS               (1U << 0)
#define TI_UART_IIR_INTID_POS               1U
#define TI_UART_IIR_INTID_MASK              (7U << TI_UART_IIR_INTID_POS)
#define TI_UART_IIR_INTID_MSI               (0U << TI_UART_IIR_INTID_POS)
#define TI_UART_IIR_INTID_THRE              (1U << TI_UART_IIR_INTID_POS)
#define TI_UART_IIR_INTID_RDA               (2U << TI_UART_IIR_INTID_POS)
#define TI_UART_IIR_INTID_RLS               (3U << TI_UART_IIR_INTID_POS)
#define TI_UART_IIR_INTID_CTI               (6U << TI_UART_IIR_INTID_POS)
/** @} */

/**
 * @name    FCR bits (write)
 * @{
 */
#define TI_UART_FCR_FIFOEN                  (1U << 0)
#define TI_UART_FCR_RXRST                   (1U << 1)
#define TI_UART_FCR_TXRST                   (1U << 2)
#define TI_UART_FCR_RXTRIGGER_1             (0U << 6)
#define TI_UART_FCR_RXTRIGGER_4             (1U << 6)
#define TI_UART_FCR_RXTRIGGER_8             (2U << 6)
#define TI_UART_FCR_RXTRIGGER_14            (3U << 6)
/** @} */

/**
 * @name    LCR bits
 * @{
 */
#define TI_UART_LCR_WLS_5BIT                0U
#define TI_UART_LCR_WLS_6BIT                1U
#define TI_UART_LCR_WLS_7BIT                2U
#define TI_UART_LCR_WLS_8BIT                3U
#define TI_UART_LCR_STB                     (1U << 2)
#define TI_UART_LCR_PEN                     (1U << 3)
#define TI_UART_LCR_EPS                     (1U << 4)
#define TI_UART_LCR_BRK                     (1U << 6)
#define TI_UART_LCR_DLAB                    (1U << 7)
#define TI_UART_LCR_8N1                     TI_UART_LCR_WLS_8BIT
/** @} */

/**
 * @name    LSR bits
 * @{
 */
#define TI_UART_LSR_DR                      (1U << 0)
#define TI_UART_LSR_OE                      (1U << 1)
#define TI_UART_LSR_PE                      (1U << 2)
#define TI_UART_LSR_FE                      (1U << 3)
#define TI_UART_LSR_BI                      (1U << 4)
#define TI_UART_LSR_THRE                    (1U << 5)
#define TI_UART_LSR_TEMT                    (1U << 6)
#define TI_UART_LSR_RXFE                    (1U << 7)
#define TI_UART_LSR_RX_ERRORS               (TI_UART_LSR_OE | TI_UART_LSR_PE | \
                                             TI_UART_LSR_FE | TI_UART_LSR_BI)
/** @} */

/**
 * @name    SSR bits (TI extension)
 * @{
 */
#define TI_UART_SSR_TXFIFOFULL              (1U << 0)
/** @} */

/**
 * @name    MDR1 bits (TI extension)
 * @note    The UART is inert until MDR1 selects an operating mode, whatever
 *          the rest of the configuration says. Written last on start and
 *          first on stop.
 * @{
 */
#define TI_UART_MDR1_MODE_MASK              7U
#define TI_UART_MDR1_MODE_UART16X           0U
#define TI_UART_MDR1_MODE_DISABLE           7U
/** @} */

/**
 * @brief   TX FIFO depth.
 */
#define TI_UART_TX_FIFO_DEPTH               64U

#endif /* TI_UART_H */

/** @} */
