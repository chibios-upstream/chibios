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
 * @file    portab.c
 * @brief   Application portability module code.
 *
 * @addtogroup application_portability
 * @{
 */

#include "../stm32u575zi_nucleo144/portab.h"
//#include "hal.h"

#if defined(STM32_DEMO_ALIGNED_WFI)
#include "hal.h"
#include "stm32_lpw.h"
#endif

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void portab_setup(void) {

}

#if defined(STM32_DEMO_ALIGNED_WFI)
/**
 * @brief   RT idle-loop example using the STM32 aligned wait helper.
 * @details Build with USE_STM32_ALIGNED_WFI=yes. The demo leaves SLEEPDEEP
 *          clear and uses ordinary interrupts to wake from shallow sleep.
 *          It does not demonstrate a complete Stop-mode entry sequence.
 * @note    Before adapting this to Stop, Standby or Shutdown, enforce all
 *          ES0499 2.2.26 conditions in the platform entry path: exclude flash
 *          access by GPDMA, DMA2D and SDMMC and select the appropriate flash
 *          prefetch/latency workaround. Configure wake sources, power mode
 *          and clock restoration there too. Do not simply set SLEEPDEEP.
 * @note    chconf.h disables the earlier built-in port wait. This hook runs
 *          in the RT idle thread, not in CH_CFG_IDLE_ENTER_HOOK().
 */
void portab_idle(void) {

  __DSB();
  stm32WfiAligned();
  __ISB();
}
#endif

/** @} */
