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
 * @file    ARMv8-M-ML-ALT/smp/rp2/chcoresmp.c
 * @brief   ARMv8-M-ML-ALT RP2 SMP code.
 *
 * @addtogroup ARMV8M_ML_ALT_CORE_SMP_RP2
 * @{
 */

#include "ch.h"

#if (CH_CFG_SMP_MODE == TRUE) || defined(__DOXYGEN__)

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

/**
 * @brief   Cores which completed SMP initialization.
 * @note    A core which never started must not be waited for in the
 *          lockout handshake.
 */
static uint32_t port_lockout_ready[PORT_CORES_NUMBER];

/**
 * @brief   Per-core lockout-in-progress flags.
 * @details Published with interrupts masked BEFORE acquiring the lockout
 *          spinlock and cleared, also with interrupts masked, together
 *          with its release. A same-core caller finding its flag already
 *          set would otherwise spin on the hardware lock above the
 *          holder's priority forever; that is a design error (concurrent
 *          same-core lockouts must be serialized at a higher layer) and
 *          fails loudly instead.
 */
static volatile bool port_lockout_wanted[PORT_CORES_NUMBER];

/**
 * @brief   True when the current lockout actually parked the other core.
 * @note    Written only while the lockout spinlock is held; the unlock
 *          path must consume the decision made at lockout time, the
 *          ready flag may change in between.
 */
static volatile bool port_lockout_parked;

#if defined(PORT_HANDLE_FIFO_MESSAGE) || defined(__DOXYGEN__)
/**
 * @brief   Per-core application messages captured while the FIFO is being
 *          drained outside the FIFO handler.
 * @details While a core is parked its FIFO drain cannot run flash-resident
 *          handlers, and a core draining the FIFO during a lockout
 *          handshake is in thread context which the handler is not
 *          entitled to; messages below the reserved range are buffered
 *          here and delivered from the FIFO handler afterwards. Excess
 *          messages beyond the buffer depth are dropped. Each core only
 *          touches its own row, no cross-core synchronization is needed.
 */
#define PORT_LOCKOUT_MSG_BUFFER_DEPTH   8U
static volatile uint32_t port_lockout_msg_buffer[PORT_CORES_NUMBER]
                                                [PORT_LOCKOUT_MSG_BUFFER_DEPTH];
static volatile uint32_t port_lockout_msg_count[PORT_CORES_NUMBER];
#endif

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

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

/**
 * @brief   Parks the core while the other core has flash unavailable.
 * @details Called from the FIFO handler on reception of the lockout token.
 *          The whole wait executes from RAM with interrupts masked because
 *          the requesting core is about to disable XIP; any flash fetch on
 *          this core would fault or return garbage.
 */
CC_NO_INLINE CC_SECTION(".ramtext")
static void port_fifo_lockout_wait(void) {
  uint32_t primask = __get_PRIMASK();

  __disable_irq();

  /* Acknowledging the lockout, the requester waits for this before
     touching XIP.*/
  while ((SIO->FIFO_ST & SIO_FIFO_ST_RDY) == 0U) {
  }
  SIO->FIFO_WR = PORT_FIFO_LOCKOUT_ACK_MESSAGE;
  __SEV();

  /* Spinning on SIO registers only until released.*/
  while (true) {
    uint32_t message;

    while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD) == 0U) {
    }
    message = SIO->FIFO_RD;
    if (message == PORT_FIFO_UNLOCK_MESSAGE) {
      break;
    }
    if (message == PORT_FIFO_PANIC_MESSAGE) {
      /* Cannot reach the flash-resident halt path, parking here, the
         other core is halting anyway.*/
      while (true) {
      }
    }
#if defined(PORT_HANDLE_FIFO_MESSAGE)
    /* Application messages cannot be delivered while flash handlers are
       unreachable, buffering for delivery after unparking. Reschedule
       tokens are dropped, the ISR epilogue reschedules on return.*/
    if (message < PORT_FIFO_LOCKOUT_ACK_MESSAGE) {
      /* SIO->CPUID is read directly: port_get_core_id() is a static
         inline whose out-of-line copy would live in flash, which is
         unreachable here.*/
      uint32_t core = SIO->CPUID;

      if (port_lockout_msg_count[core] < PORT_LOCKOUT_MSG_BUFFER_DEPTH) {
        port_lockout_msg_buffer[core][port_lockout_msg_count[core]++] = message;
      }
    }
#else
    /* Anything else (reschedule tokens) is stale, the ISR epilogue
       reschedules on return anyway.*/
#endif
  }

  /* Acknowledging the unlock, XIP is available again at this point.*/
  while ((SIO->FIFO_ST & SIO_FIFO_ST_RDY) == 0U) {
  }
  SIO->FIFO_WR = PORT_FIFO_LOCKOUT_ACK_MESSAGE;
  __SEV();

  __set_PRIMASK(primask);

#if defined(PORT_HANDLE_FIFO_MESSAGE)
  /* Back on flash, delivering the messages buffered during the park in
     arrival order; still in the FIFO ISR context.*/
  {
    core_id_t core = port_get_core_id();
    uint32_t i;

    for (i = 0U; i < port_lockout_msg_count[core]; i++) {
      PORT_HANDLE_FIFO_MESSAGE(core ^ 1U,
                               port_lockout_msg_buffer[core][i]);
    }
    port_lockout_msg_count[core] = 0U;
  }
#endif
}

/**
 * @brief   Sends a token to the other core and waits for its acknowledge.
 * @details The FIFO RX side is drained directly because this core's FIFO
 *          interrupt is masked during the handshake.
 *
 * @param[in] token     token to be sent
 * @return              @p true on acknowledge, @p false on timeout.
 */
static bool port_lockout_handshake(uint32_t token) {
  uint32_t start = TIMER0->TIMERAWL;

  while ((SIO->FIFO_ST & SIO_FIFO_ST_RDY) == 0U) {
    if ((TIMER0->TIMERAWL - start) > PORT_LOCKOUT_TIMEOUT_US) {
      return false;
    }
  }
  SIO->FIFO_WR = token;
  __SEV();

  while (true) {
    if ((SIO->FIFO_ST & SIO_FIFO_ST_VLD) != 0U) {
      uint32_t message = SIO->FIFO_RD;

      if (message == PORT_FIFO_LOCKOUT_ACK_MESSAGE) {
        return true;
      }
      if (message == PORT_FIFO_PANIC_MESSAGE) {
        port_local_halt();
      }
#if defined(PORT_HANDLE_FIFO_MESSAGE)
      /* Application messages cannot be delivered here: this is thread
         context with interrupts masked, not the ISR context the handler
         is entitled to (state checking, epilogue reschedule). Buffering
         for delivery from the FIFO handler pended after the handshake.*/
      if (message < PORT_FIFO_LOCKOUT_ACK_MESSAGE) {
        core_id_t core = port_get_core_id();

        if (port_lockout_msg_count[core] < PORT_LOCKOUT_MSG_BUFFER_DEPTH) {
          port_lockout_msg_buffer[core][port_lockout_msg_count[core]++] =
            message;
        }
      }
#endif
      /* Reschedule tokens are dropped here, a reschedule round is forced
         after the handshake.*/
    }
    if ((TIMER0->TIMERAWL - start) > PORT_LOCKOUT_TIMEOUT_US) {
      return false;
    }
  }
}

/*===========================================================================*/
/* Module interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   Single FIFO interrupt handler for both cores (RP2350).
 * @note    RP2350 uses a shared SIO_IRQ_FIFO (IRQ 25) unlike RP2040 which
 *          has separate IRQs per core.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorA4) {

  CH_IRQ_PROLOGUE();

  SIO->FIFO_ST = SIO_FIFO_ST_ROE | SIO_FIFO_ST_WOF;

#if defined(PORT_HANDLE_FIFO_MESSAGE)
  /* Messages buffered by a lockout handshake on this core are delivered
     first, they arrived before anything still in the FIFO. The producer
     runs in thread context with interrupts masked, it cannot race this
     handler.*/
  {
    core_id_t core = port_get_core_id();
    uint32_t i;

    for (i = 0U; i < port_lockout_msg_count[core]; i++) {
      PORT_HANDLE_FIFO_MESSAGE(core ^ 1U, port_lockout_msg_buffer[core][i]);
    }
    port_lockout_msg_count[core] = 0U;
  }
#endif

  while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD) != 0U) {
    uint32_t message = SIO->FIFO_RD;
    /* FIFO traffic always comes from the other core, so panic handling must
       be symmetric.*/
    if (message == PORT_FIFO_PANIC_MESSAGE) {
      port_local_halt();
    }
    /* The other core needs this core off flash, parking until released.*/
    if (message == PORT_FIFO_LOCKOUT_MESSAGE) {
      port_fifo_lockout_wait();
      continue;
    }
#if defined(PORT_HANDLE_FIFO_MESSAGE)
    if (message < PORT_FIFO_LOCKOUT_ACK_MESSAGE) {
      PORT_HANDLE_FIFO_MESSAGE(port_get_core_id() ^ 1U, message);
    }
#else
    (void)message;
#endif
  }

  __SEV();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

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

  /* FIFO handler for this core. RP2350 uses a single shared IRQ 25.*/
  SIO->FIFO_ST = SIO_FIFO_ST_ROE | SIO_FIFO_ST_WOF;
  NVIC_SetPriority(SIO_IRQ_FIFOn, CORTEX_MINIMUM_PRIORITY);
  NVIC_EnableIRQ(SIO_IRQ_FIFOn);

  (void)oip;
}

/**
 * @brief   Marks this core as able to service flash-lockout requests.
 * @details Called after the first startup unlock has lowered the local
 *          interrupt mask. Subsequent unlocks leave the one-shot state
 *          unchanged.
 */
CC_NO_INLINE CC_SECTION(".ramtext")
void __port_smp_startup_complete(void) {
  core_id_t core_id;
  uint32_t primask;

  core_id = (core_id_t)SIO->CPUID;
  if (__atomic_load_n(&port_lockout_ready[core_id],
                      __ATOMIC_RELAXED) == 0U) {
    primask = __get_PRIMASK();
    __disable_irq();

    if (__atomic_load_n(&port_lockout_ready[core_id],
                        __ATOMIC_RELAXED) == 0U) {
      /* Admission is held through the complete flash operation. Waiting
         here in RAM keeps this core off XIP until the operation ends. It
         is intentionally unbounded like port_fifo_lockout_wait(): the
         holder can be completing a legitimate long flash operation.*/
      while (SIO->SPINLOCK[PORT_LOCKOUT_SPINLOCK_NUMBER] == 0U) {
      }
      __DMB();

      __atomic_store_n(&port_lockout_ready[core_id], 1U, __ATOMIC_RELEASE);

      __DMB();
      SIO->SPINLOCK[PORT_LOCKOUT_SPINLOCK_NUMBER] = (uint32_t)SIO;
    }

    __set_PRIMASK(primask);
  }
}

/**
 * @brief   Parks the other core outside flash and masks local interrupts
 *          sourced from it.
 * @details On return the other core is spinning in RAM with interrupts
 *          disabled and stays there until @p __port_flash_unlockout() is
 *          called. Requests from both cores are serialized on a dedicated
 *          hardware spinlock which is spun with interrupts enabled so that
 *          a crossing request from the other core can park this core
 *          first instead of deadlocking.
 * @note    Must be called from thread context outside any critical
 *          section.
 */
void __port_flash_lockout(void) {

  chDbgAssert(!port_is_isr_context() &&
              __port_irq_enabled(__port_get_irq_status()) &&
              ((__get_PRIMASK() & 1U) == 0U),
              "not in thread context");

  /* Publishing the lockout intent atomically before contending for the
     hardware lock. A same-core caller finding the flag set would spin
     on the spinlock above the holder's priority forever with no
     timeout; concurrent same-core lockouts must be serialized at a
     higher layer (the EFL state machine already does), so this is a
     design error and fails loudly even in release builds.*/
  __disable_irq();
  if (port_lockout_wanted[port_get_core_id()]) {
    __enable_irq();
    chSysHalt("lockout re-entered");
  }
  port_lockout_wanted[port_get_core_id()] = true;
  __enable_irq();

  /* Serializing requesters, interrupts stay enabled while spinning so a
     crossing request from the other core can park this core first.
     While the lockout is held the owning thread remains preemptible;
     this is deliberate (interrupt windows between flash pages keep
     watchdog feeding and IRQ latency alive on this core) at the
     documented cost of extending the other core's park time.*/
  while (SIO->SPINLOCK[PORT_LOCKOUT_SPINLOCK_NUMBER] == 0U) {
  }
  __DMB();

  /* A core which never initialized cannot acknowledge and does not need
     parking. The decision is recorded for the unlock side, the flag may
     rise in the meantime.*/
  if (!__port_lockout_other_ready()) {
    port_lockout_parked = false;
    return;
  }
  port_lockout_parked = true;

  /* Masking local interrupts so that the FIFO handler cannot steal the
     acknowledge token; no lockout traffic can be in flight here because
     the spinlock is held.*/
  __disable_irq();

  if (!port_lockout_handshake(PORT_FIFO_LOCKOUT_MESSAGE)) {
    chSysHalt("lockout timeout");
  }

#if defined(PORT_HANDLE_FIFO_MESSAGE)
  /* Messages captured during the handshake are delivered by the FIFO
     handler in its normal ISR context, pending it while still masked so
     delivery happens right at the enable below.*/
  if (port_lockout_msg_count[port_get_core_id()] > 0U) {
    NVIC_SetPendingIRQ(SIO_IRQ_FIFOn);
  }
#endif

  __enable_irq();
}

/**
 * @brief   Releases the core parked by @p __port_flash_lockout().
 */
void __port_flash_unlockout(void) {

  chDbgAssert((SIO->SPINLOCK_ST &
               (1UL << PORT_LOCKOUT_SPINLOCK_NUMBER)) != 0U,
              "lockout not active");

  /* Unlocking only if the matching lockout parked the other core, the
     ready flag alone could have risen since then.*/
  if (port_lockout_parked) {
    port_lockout_parked = false;
    __disable_irq();

    if (!port_lockout_handshake(PORT_FIFO_UNLOCK_MESSAGE)) {
      chSysHalt("unlock timeout");
    }

#if defined(PORT_HANDLE_FIFO_MESSAGE)
    /* Messages captured during the handshake are delivered by the FIFO
       handler in its normal ISR context, pending it while still masked
       so delivery happens right at the enable below.*/
    if (port_lockout_msg_count[port_get_core_id()] > 0U) {
      NVIC_SetPendingIRQ(SIO_IRQ_FIFOn);
    }
#endif

    __enable_irq();
  }

  /* Releasing the hardware lock and withdrawing the intent flag as one
     unit so a preempting same-core caller never observes them apart.*/
  __disable_irq();
  __DMB();
  SIO->SPINLOCK[PORT_LOCKOUT_SPINLOCK_NUMBER] = (uint32_t)SIO;
  port_lockout_wanted[port_get_core_id()] = false;
  __enable_irq();

  /* A reschedule token could have been consumed during the handshakes,
     forcing a reschedule round.*/
  chSysLock();
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Tells whether the other core completed SMP initialization.
 * @details Until this returns @p true the other core may be executing
 *          startup code from flash without being parkable; callers which
 *          know the core was started must wait for readiness before the
 *          first lockout.
 *
 * @return              @p true if the other core can be parked.
 */
bool __port_lockout_other_ready(void) {

  return __atomic_load_n(&port_lockout_ready[port_get_core_id() ^ 1U],
                         __ATOMIC_ACQUIRE) != 0U;
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

#endif /* CH_CFG_SMP_MODE == TRUE */

/** @} */
