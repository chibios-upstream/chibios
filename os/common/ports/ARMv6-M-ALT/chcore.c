/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    ARMv6-M-ALT/chcore.c
 * @brief   ARMv6-M (alternate) port code.
 *
 * @addtogroup ARMV6M_ALT_CORE
 * @{
 */

#include "ch.h"

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
/* Module interrupt handlers.                                                */
/*===========================================================================*/

#if (CORTEX_ALTERNATE_SWITCH == FALSE) || defined(__DOXYGEN__)
/**
 * @brief   NMI vector.
 * @details The NMI vector is used for exception mode re-entering after a
 *          context switch.
 */
/*lint -save -e9075 [8.4] All symbols are invoked from asm context.*/
void __NMI_Handler(void) {
/*lint -restore*/

  /* The port_extctx structure is pointed by the PSP register.*/
  struct port_extctx *ctxp = (struct port_extctx *)__get_PSP();

  /* Discarding the current exception context and positioning the stack to
     point to the real one.*/
  ctxp++;

  /* Writing back the modified PSP value.*/
  __set_PSP((uint32_t)ctxp);

  /* Restoring the normal interrupts status.*/
  port_unlock_from_isr();
}
#endif /* !CORTEX_ALTERNATE_SWITCH */

#if (CORTEX_ALTERNATE_SWITCH == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   PendSV vector.
 * @details The PendSV vector is used for exception mode re-entering after a
 *          context switch.
 */
/*lint -save -e9075 [8.4] All symbols are invoked from asm context.*/
void __PendSV_Handler(void) {
/*lint -restore*/

  /* The port_extctx structure is pointed by the PSP register.*/
  struct port_extctx *ctxp = (struct port_extctx *)__get_PSP();

  /* Discarding the current exception context and positioning the stack to
     point to the real one.*/
  ctxp++;

  /* Writing back the modified PSP value.*/
  __set_PSP((uint32_t)ctxp);
}
#endif /* CORTEX_ALTERNATE_SWITCH */

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Port-related initialization code.
 *
 * @param[in, out] oip  pointer to the @p os_instance_t structure
 *
 * @notapi
 */
void port_init(os_instance_t *oip) {

  (void)oip;

#if defined(port_smp_init)
  port_smp_init(oip);
#endif

  NVIC_SetPriority(PendSV_IRQn, CORTEX_PRIORITY_PENDSV);
}

/** @} */
