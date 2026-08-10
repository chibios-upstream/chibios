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
#define RP_CORE1_START                      TRUE
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
#define RP_IRQ_UART0_PRIORITY               3
#define RP_IRQ_UART1_PRIORITY               3
#define RP_IO_IRQ_BANK0_PRIORITY            2

/*
 * SIO driver system settings.
 */
#define RP_SIO_USE_UART0                    TRUE
#define RP_SIO_USE_UART1                    FALSE

/*
 * ADC driver system settings.
 */
#define RP_ADC_USE_ADC1                     TRUE
#define RP_ADC_ADC1_DMA_CHANNEL             RP_DMA_CHANNEL_ID_ANY
#define RP_ADC_ADC1_DMA_PRIORITY            0
#define RP_ADC_ADC1_DMA_IRQ_PRIORITY        3

/*
 * PWM driver system settings.
 */
#define RP_PWM_USE_PWM0                     FALSE
#define RP_PWM_USE_PWM1                     FALSE
#define RP_PWM_USE_PWM2                     FALSE
#define RP_PWM_USE_PWM3                     FALSE
#define RP_PWM_USE_PWM4                     TRUE
#define RP_PWM_USE_PWM5                     FALSE
#define RP_PWM_USE_PWM6                     FALSE
#define RP_PWM_USE_PWM7                     FALSE
#define RP_PWM_USE_PWM8                     FALSE
#define RP_PWM_USE_PWM9                     FALSE
#define RP_PWM_USE_PWM10                    FALSE
#define RP_PWM_USE_PWM11                    FALSE
#define RP_PWM_IRQ_WRAP_NUMBER_PRIORITY     3

#endif /* XMCUCONF_H */
