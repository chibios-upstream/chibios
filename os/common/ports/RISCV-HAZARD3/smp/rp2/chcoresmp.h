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
 * @file    RISCV-HAZARD3/smp/rp2/chcoresmp.h
 * @brief   RISC-V Hazard3 RP2 SMP macros and structures.
 *
 * @addtogroup RISCV_HAZARD3_CORE_SMP_RP2
 * @{
 */

#ifndef CHCORESMP_H
#define CHCORESMP_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of SMP cores.
 */
#define PORT_CORES_NUMBER               2

/**
 * @name    IPC FIFO messages
 * @{
 */
#define PORT_FIFO_RESCHEDULE_MESSAGE    0xFFFFFFFFU
#define PORT_FIFO_PANIC_MESSAGE         0xFFFFFFFEU
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Spinlock to be used by the port layer.
 * @details RP2350-E2 permits unrelated SIO writes to release certain
 *          spinlocks on A2, A3, and A4 silicon. The default selects an
 *          unconditionally safe lock number.
 */
#if !defined(PORT_SPINLOCK_NUMBER)
#define PORT_SPINLOCK_NUMBER            31
#endif

/**
 * @brief   Raw Xh3irq priority level for the SIO FIFO inter-core IRQ.
 * @details Raw Hazard3 external interrupt priorities are 0-3 (higher = more
 *          urgent), unlike the ChibiOS/NVIC public ordering. Setting this to
 *          the maximum gives cross-core reschedule notifications the highest
 *          available urgency (tied with any maximum-urgency peripheral IRQ).
 */
#if !defined(PORT_FIFO_IRQ_PRIORITY)
#define PORT_FIFO_IRQ_PRIORITY          3
#endif

/* Note: the ram4/ram5 scratch banks used by the following sections also
   hold the per-core default stacks, annotated data competes with stack
   space and an overflow surfaces as a link-time region error.*/

/**
 * @brief   Marker enabling suffixed @p PORT_MEM_LOCAL_COHERENT_BSSn
 *          selection in chmem.h.
 */
#if !defined(PORT_MEM_LOCAL_COHERENT_BSS)
#define PORT_MEM_LOCAL_COHERENT_BSS     /* Enables suffixed PORT_MEM_LOCAL_COHERENT_BSSn selection in chmem.h.*/
#endif

/**
 * @brief   Preferential local and coherent BSS section for core zero.
 */
#if !defined(PORT_MEM_LOCAL_COHERENT_BSS0)
#define PORT_MEM_LOCAL_COHERENT_BSS0    CC_SECTION(".ram4_clear.core0")
#endif

/**
 * @brief   Preferential local and coherent BSS section for core one.
 */
#if !defined(PORT_MEM_LOCAL_COHERENT_BSS1)
#define PORT_MEM_LOCAL_COHERENT_BSS1    CC_SECTION(".ram5_clear.core1")
#endif

/**
 * @brief   Marker enabling suffixed @p PORT_MEM_LOCAL_BSSn selection
 *          in chmem.h.
 */
#if !defined(PORT_MEM_LOCAL_BSS)
#define PORT_MEM_LOCAL_BSS              /* Enables suffixed PORT_MEM_LOCAL_BSSn selection in chmem.h.*/
#endif

/**
 * @brief   Preferential local BSS section for core zero.
 */
#if !defined(PORT_MEM_LOCAL_BSS0)
#define PORT_MEM_LOCAL_BSS0             CC_SECTION(".ram4_clear.core0")
#endif

/**
 * @brief   Preferential local BSS section for core one.
 */
#if !defined(PORT_MEM_LOCAL_BSS1)
#define PORT_MEM_LOCAL_BSS1             CC_SECTION(".ram5_clear.core1")
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (PORT_SPINLOCK_NUMBER < 0) || (PORT_SPINLOCK_NUMBER > 31)
  #error "invalid PORT_SPINLOCK_NUMBER value"
#endif

/* RP2350-E2: only these spinlocks are immune to false releases caused by
   unrelated SIO writes on all affected mask revisions.*/
#if (PORT_SPINLOCK_NUMBER != 5)  && (PORT_SPINLOCK_NUMBER != 6)  &&       \
    (PORT_SPINLOCK_NUMBER != 7)  && (PORT_SPINLOCK_NUMBER != 10) &&       \
    (PORT_SPINLOCK_NUMBER != 11) && (PORT_SPINLOCK_NUMBER < 18)
  #error "PORT_SPINLOCK_NUMBER is unsafe on RP2350 A2/A3/A4 (RP2350-E2)"
#endif

#if (PORT_FIFO_IRQ_PRIORITY < 0) || (PORT_FIFO_IRQ_PRIORITY > 3)
  #error "invalid PORT_FIFO_IRQ_PRIORITY value"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Panic notification.
 * @note    The notification is durable even if the FIFO is full. It never
 *          polls because the other side could be unable to empty the FIFO
 *          after a catastrophic error.
 */
#define PORT_SYSTEM_HALT_HOOK() __port_smp_notify_panic()

/**
 * @brief   SMP-related port initialization.
 * @note    The port checks on presence of this macro so this
 *          must be a macro.
 * @post    The global kernel lock is acquired and is released by the final
 *          @p chSysUnlock() in the instance startup path.
 *
 * @param[in, out] oip  pointer to the @p os_instance_t structure
 */
#define port_smp_init(oip) __port_smp_init(oip)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void __port_smp_init(os_instance_t *oip);
  void __port_smp_notify_panic(void);
  void __port_spinlock_take(void);
  void __port_spinlock_release(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Triggers an inter-core notification.
 *
 * @param[in] oip       pointer to the @p os_instance_t structure
 */
static inline void port_notify_instance(os_instance_t *oip) {

  (void)oip;

  /* Sending a reschedule order to the other core if there is space in the FIFO. */
  if ((SIO->FIFO_ST & SIO_FIFO_ST_RDY) != 0U) {
    SIO->FIFO_WR = PORT_FIFO_RESCHEDULE_MESSAGE;
  }
}

/**
 * @brief   Takes the kernel spinlock.
 */
static inline void port_spinlock_take(void) {

  while (SIO->SPINLOCK[PORT_SPINLOCK_NUMBER] == 0U) {
  }
  __DMB();
}

/**
 * @brief   Releases the kernel spinlock.
 */
static inline void port_spinlock_release(void) {

  __DMB();
  SIO->SPINLOCK[PORT_SPINLOCK_NUMBER] = (uint32_t)SIO;
}

/**
 * @brief   Returns a core index.
 * @return  The core identifier from 0 to @p PORT_CORES_NUMBER - 1.
 */
static inline core_id_t port_get_core_id(void) {

  return SIO->CPUID;
}

/*===========================================================================*/
/* Module late inclusions.                                                   */
/*===========================================================================*/

#endif /* CHCORESMP_H */

/** @} */
