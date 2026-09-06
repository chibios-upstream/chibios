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
 * @file    TI/AM67/am67_registry.h
 * @brief   AM67 capabilities registry.
 * @details Describes what the SoC implements and where it lives, as seen
 *          from an MCU-domain Cortex-R5F core. Board files select which of
 *          these the firmware actually owns; the Linux host owns the rest.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef AM67_REGISTRY_H
#define AM67_REGISTRY_H

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    AM67 capabilities
 * @{
 */
/* DMTIMER attributes.*/
#define AM67_HAS_MCU_TIMER0                 TRUE
#define AM67_MCU_TIMER0_BASE                0x04800000U
#define AM67_MCU_TIMER0_IRQ                 28U

/* McSPI attributes.*/
#define AM67_HAS_MCU_MCSPI0                 TRUE
#define AM67_MCU_MCSPI0_BASE                0x04B00000U
#define AM67_MCU_MCSPI0_IRQ                 207U
#define AM67_MCU_MCSPI0_CLOCK               48000000U

/* I2C attributes.*/
#define AM67_HAS_MCU_I2C0                   TRUE
#define AM67_MCU_I2C0_BASE                  0x04900000U
#define AM67_MCU_I2C0_IRQ                   197U

/* UART attributes.*/
#define AM67_HAS_MAIN_UART1                 TRUE
#define AM67_MAIN_UART1_BASE                0x02810000U
#define AM67_MAIN_UART1_IRQ                 211U

/* EHRPWM attributes.*/
#define AM67_HAS_EPWM0                      TRUE
#define AM67_EPWM0_BASE                     0x23000000U
#define AM67_HAS_EPWM1                      TRUE
#define AM67_EPWM1_BASE                     0x23010000U
#define AM67_EPWM_CHANNELS                  2U
#define AM67_EPWM_CLOCK                     250000000U

/* ECAP attributes.*/
#define AM67_HAS_ECAP0                      TRUE
#define AM67_ECAP0_BASE                     0x23100000U
#define AM67_HAS_ECAP1                      TRUE
#define AM67_ECAP1_BASE                     0x23110000U
#define AM67_HAS_ECAP2                      TRUE
#define AM67_ECAP2_BASE                     0x23120000U
/* Measured on hardware, not the 250 MHz the eHRPWM sees (DR-002).*/
#define AM67_ECAP_CLOCK                     125000000U

/* RTI watchdog attributes.*/
#define AM67_HAS_MCU_RTI0                   TRUE
#define AM67_MCU_RTI0_BASE                  0x04880000U

/* Mailbox attributes.*/
#define AM67_HAS_MAILBOX0                   TRUE
#define AM67_MAILBOX0_BASE                  0x29010000U
#define AM67_MAILBOX0_IRQ                   241U
/** @} */

/*===========================================================================*/
/* Platform memory map.                                                      */
/*===========================================================================*/

/**
 * @name    Core-local and shared memories
 * @note    Only one TCM is usable on an MCU-domain core: the device tree
 *          sets @p ti,loczrama = <0>, putting the BTCM at address 0, and
 *          @p ti,atcm-enable = <0> leaves the ATCM off. Address 0 is not
 *          negotiable, the Cortex-R5 has no VBAR so the vectors, the
 *          pre-MPU boot code and the banked mode stacks share this block.
 * @{
 */
#define AM67_TCM_BASE                       0x00000000U
#define AM67_TCM_SIZE                       (32U * 1024U)
#define AM67_MSRAM_BASE                     0x60000000U
#define AM67_MSRAM_SIZE                     (512U * 1024U)
#define AM67_DDR_BASE                       0x80000000U
/** @} */

#endif /* AM67_REGISTRY_H */

/** @} */
