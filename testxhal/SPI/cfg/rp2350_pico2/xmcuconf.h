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

/*
 * RP2350 drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in xhalconf.h.
 *
 * IRQ priorities:
 * 15...0       Lowest...Highest (4 bits on Cortex-M33).
 *
 * DMA priorities:
 * 0...1        Lowest...Highest.
 */

#ifndef XMCUCONF_H
#define XMCUCONF_H

#define __RP2350_XMCUCONF__

/*
 * HAL driver system settings.
 */
#define RP_NO_INIT                          FALSE
#define RP_CLOCK_DYNAMIC                    FALSE
#define RP_CORE1_START                      FALSE
#define RP_CORE1_VECTORS_TABLE              _vectors
#define RP_CORE1_ENTRY_POINT                _crt0_c1_entry
#define RP_CORE1_STACK_END                  __c1_main_stack_end__

/*
 * IRQ system settings.
 */
#define RP_IRQ_SYSTICK_PRIORITY             2
#define RP_IRQ_TIMER0_ALARM0_PRIORITY       2
#define RP_IRQ_TIMER0_ALARM1_PRIORITY       2
#define RP_IRQ_TIMER0_ALARM2_PRIORITY       2
#define RP_IRQ_TIMER0_ALARM3_PRIORITY       2
#define RP_IRQ_SPI0_PRIORITY                2
#define RP_IRQ_SPI1_PRIORITY                2
#define RP_IO_IRQ_BANK0_PRIORITY            2

/*
 * SPI driver system settings.
 */
#define RP_SPI_USE_SPI0                     TRUE
#define RP_SPI_USE_SPI1                     FALSE
#define RP_SPI_SPI0_RX_DMA_CHANNEL          RP_DMA_CHANNEL_ID_ANY
#define RP_SPI_SPI0_TX_DMA_CHANNEL          RP_DMA_CHANNEL_ID_ANY
#define RP_SPI_SPI1_RX_DMA_CHANNEL          RP_DMA_CHANNEL_ID_ANY
#define RP_SPI_SPI1_TX_DMA_CHANNEL          RP_DMA_CHANNEL_ID_ANY
#define RP_SPI_SPI0_DMA_PRIORITY            1
#define RP_SPI_SPI1_DMA_PRIORITY            1
#define RP_SPI_DMA_ERROR_HOOK(spip)

#endif /* XMCUCONF_H */
