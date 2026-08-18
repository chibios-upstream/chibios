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
 * @file    RISCV-HAZARD3/smp/rp2/chcoresmp.c
 * @brief   RISC-V Hazard3 RP2 SMP code.
 *
 * @addtogroup RISCV_HAZARD3_CORE_SMP_RP2
 * @{
 */

#include "ch.h"
#include "hazard3_irq.h"

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

/* This shared latch is zeroed once by the primary startup. It is deliberately
   not cleared during per-core initialization because doing so could erase a
   panic raised while the target core is starting. */
static uint32_t port_panic_pending[PORT_CORES_NUMBER];

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static bool port_is_panic_pending(void) {
  core_id_t core_id;

  core_id = port_get_core_id();

  return __atomic_load_n(&port_panic_pending[core_id],
                         __ATOMIC_ACQUIRE) != 0U;
}

static void port_local_halt(void) {
  os_instance_t *oip;
  const char *reason = "remote panic";

  port_disable();
  oip = currcore;

  __trace_halt("remote panic");

  if (oip != NULL) {
    oip->dbg.panic_msg = reason;
  }

  CH_CFG_SYSTEM_HALT_HOOK(reason);

  while (true) {
  }
}

/*===========================================================================*/
/* Module interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   Single FIFO interrupt handler for both cores.
 * @note    RP2350 uses a shared SIO_IRQ_FIFO IRQ 25
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorA4) {

  CH_IRQ_PROLOGUE();

  /* A startup-time latched panic can force this local source after the ROM
     has consumed its FIFO hint. Do not leave that forced level asserted. */
  hazard3_irq_force_clear(SIO_IRQ_FIFOn);

  /* The latch is authoritative; the FIFO message is only a wakeup hint. */
  if (port_is_panic_pending()) {
    port_local_halt();
  }

  SIO->FIFO_ST = SIO_FIFO_ST_ROE | SIO_FIFO_ST_WOF;

  while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD) != 0U) {
    uint32_t message;

    message = SIO->FIFO_RD;

    /* Reading frees FIFO space. Wake a peer blocked in h3.block even when
       this message is fatal and this core does not reach the ISR epilogue.*/
    __SEV();

    /* FIFO traffic always comes from the other core, so panic handling must
       be symmetric.*/
    if (message == PORT_FIFO_PANIC_MESSAGE) {
      port_local_halt();
    }
#if defined(PORT_HANDLE_FIFO_MESSAGE)
    if (message != PORT_FIFO_RESCHEDULE_MESSAGE) {
      PORT_HANDLE_FIFO_MESSAGE(port_get_core_id() ^ 1U, message);
    }
#else
    (void)message;
#endif
  }

  /* Order the FIFO drain before the second latch observation. This closes the
     race where the sender observed a full FIFO just before this core freed a
     slot and therefore could not enqueue the panic hint. */
  __asm__ volatile ("fence io, rw" : : : "memory");
  if (port_is_panic_pending()) {
    port_local_halt();
  }

  __SEV();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Notifies the other core about a local panic.
 * @note    The shared latch is authoritative. The FIFO write is a best-effort
 *          wakeup and is never allowed to block the panic path.
 */
void __port_smp_notify_panic(void) {
  core_id_t target;

  target = port_get_core_id() ^ 1U;

  /* Publish the durable indication before interacting with the FIFO. */
  __atomic_store_n(&port_panic_pending[target], 1U, __ATOMIC_RELEASE);
  __asm__ volatile ("fence rw, io" : : : "memory");

  if ((SIO->FIFO_ST & SIO_FIFO_ST_RDY) != 0U) {
    SIO->FIFO_WR = PORT_FIFO_PANIC_MESSAGE;
  }
}

/**
 * @brief   SMP-related port initialization.
 * @details Acquires the global kernel lock which is released by the final
 *          @p chSysUnlock() in the instance startup path.
 *
 * @param[in, out] oip  pointer to the @p os_instance_t structure
 */
void __port_smp_init(os_instance_t *oip) {

  /* Entering the initial global I-Lock state.*/
  port_spinlock_take();

#if CH_CFG_ST_TIMEDELTA > 0
  /* Activating timer for this instance.*/
  port_timer_enable(oip);
#endif

#if CH_CFG_SMP_MODE == TRUE
  SIO->FIFO_ST = SIO_FIFO_ST_ROE | SIO_FIFO_ST_WOF;
  hazard3_irq_set_priority(SIO_IRQ_FIFOn, PORT_FIFO_IRQ_PRIORITY);
  hazard3_irq_enable(SIO_IRQ_FIFOn);
  if (port_is_panic_pending()) {
    hazard3_irq_force(SIO_IRQ_FIFOn);
  }
#endif

  (void)oip;
}

/**
 * @brief   Takes the kernel spinlock.
 */
void __port_spinlock_take(void) {

  port_spinlock_take();
}

/**
 * @brief   Releases the kernel spinlock.
 */
void __port_spinlock_release(void) {

  port_spinlock_release();
}

/** @} */
