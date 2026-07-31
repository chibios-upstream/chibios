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
 * @file    SIMX86_64-SMP/chcore.c
 * @brief   SMP simulator on x86-64 port code.
 *
 * @addtogroup SIMX86_64_SMP_GCC_CORE
 * @{
 */

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ch.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*
 * RTOS-specific context offset.
 */
#if defined(_CHIBIOS_RT_CONF_) || defined(_CHIBIOS_NIL_CONF_)
#define CONTEXT_OFFSET      __CH_OFFSETOF(thread_t, ctx)
#else
#error "invalid chconf.h"
#endif

#define PORT_STARTUP_PRIMARY_RELEASED    1U
#define PORT_STARTUP_SECONDARY_READY     2U

#if !defined(PORT_SMP_IPI_SIGNAL)
#define PORT_SMP_IPI_SIGNAL             SIGUSR1
#endif

#if (__GCC_ATOMIC_CHAR_LOCK_FREE != 2)
#error "the SMP simulator requires lock-free character atomics"
#endif

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

__thread core_id_t port_core_id;
__thread bool port_isr_context_flag;
__thread syssts_t port_irq_sts = (syssts_t)1;

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

#if CH_CFG_SMP_MODE == TRUE
static pthread_t port_core_threads[PORT_CORES_NUMBER];
static sigset_t port_ipi_sigset;
static unsigned char port_kernel_spinlock;
static unsigned port_startup_state;
static __thread bool port_kernel_lock_held;
static __thread bool port_startup_unlock_pending;
#endif

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

#if CH_CFG_SMP_MODE == TRUE
static void port_wait_startup_state(unsigned state) {

  while (__atomic_load_n(&port_startup_state,
                         __ATOMIC_ACQUIRE) < state) {
    sched_yield();
  }
}

static void port_mask_ipi(void) {

  if (pthread_sigmask(SIG_BLOCK, &port_ipi_sigset, NULL) != 0) {
    abort();
  }
}

static void port_unmask_ipi(void) {

  if (pthread_sigmask(SIG_UNBLOCK, &port_ipi_sigset, NULL) != 0) {
    abort();
  }
}

static void port_spinlock_take(void) {

  if (port_kernel_lock_held) {
    abort();
  }

  while (__atomic_exchange_n(&port_kernel_spinlock, (unsigned char)1,
                             __ATOMIC_ACQUIRE) != (unsigned char)0) {
    while (__atomic_load_n(&port_kernel_spinlock,
                           __ATOMIC_RELAXED) != (unsigned char)0) {
      __asm__ volatile ("pause");
    }
  }
  port_kernel_lock_held = true;
}

static void port_spinlock_release(void) {

  if (!port_kernel_lock_held) {
    abort();
  }

  port_kernel_lock_held = false;
  __atomic_store_n(&port_kernel_spinlock, (unsigned char)0,
                   __ATOMIC_RELEASE);
}

static void port_startup_unlock(void) {

  if (port_core_id == (core_id_t)0) {
    __atomic_store_n(&port_startup_state, PORT_STARTUP_PRIMARY_RELEASED,
                     __ATOMIC_RELEASE);
    port_wait_startup_state(PORT_STARTUP_SECONDARY_READY);
  }
  else {
    __atomic_store_n(&port_startup_state, PORT_STARTUP_SECONDARY_READY,
                     __ATOMIC_RELEASE);
  }
}

static void *port_core1_start(void *p) {
  extern void c1_main(void);

  (void)p;

  port_core_id = (core_id_t)1;
  port_irq_sts = (syssts_t)1;
  port_isr_context_flag = false;
  port_startup_unlock_pending = false;
  port_kernel_lock_held = false;

  port_wait_startup_state(PORT_STARTUP_PRIMARY_RELEASED);
  c1_main();

  abort();
}
#endif

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Port-related initialization code.
 *
 * @param[in, out] oip  pointer to the OS instance
 */
void port_init(os_instance_t *oip) {

  (void)oip;

  port_irq_sts = (syssts_t)1;
  port_isr_context_flag = false;

#if CH_CFG_SMP_MODE == TRUE
  port_startup_unlock_pending = true;
  if (port_core_id == (core_id_t)0) {
    int err;

    if ((sigemptyset(&port_ipi_sigset) != 0) ||
        (sigaddset(&port_ipi_sigset, PORT_SMP_IPI_SIGNAL) != 0)) {
      abort();
    }
    port_mask_ipi();

    port_core_threads[0] = pthread_self();
    err = pthread_create(&port_core_threads[1], NULL,
                         port_core1_start, NULL);
    if (err != 0) {
      abort();
    }
  }
  else {
    port_mask_ipi();
  }
#else
  port_irq_sts = (syssts_t)1;
#endif
}

/**
 * @brief   Kernel-lock action.
 * @details Masks the virtual IPI source and takes the shared kernel spinlock.
 */
void port_lock(void) {

#if CH_CFG_SMP_MODE == TRUE
  port_mask_ipi();
  port_spinlock_take();
#endif
  port_irq_sts = (syssts_t)1;
}

/**
 * @brief   Kernel-unlock action.
 * @details Releases the shared kernel spinlock and unmasks virtual IPIs.
 */
void port_unlock(void) {

#if CH_CFG_SMP_MODE == TRUE
  if (port_startup_unlock_pending) {
    port_startup_unlock_pending = false;
    port_startup_unlock();
  }
  else {
    port_spinlock_release();
  }
#endif
  port_irq_sts = (syssts_t)0;
#if CH_CFG_SMP_MODE == TRUE
  port_unmask_ipi();
#endif
}

/**
 * @brief   Kernel-lock action from an interrupt handler.
 */
void port_lock_from_isr(void) {

  port_lock();
}

/**
 * @brief   Kernel-unlock action from an interrupt handler.
 */
void port_unlock_from_isr(void) {

  port_unlock();
}

/**
 * @brief   Disables all interrupt sources.
 */
void port_disable(void) {

#if CH_CFG_SMP_MODE == TRUE
  port_mask_ipi();
#endif
  port_irq_sts = (syssts_t)1;
}

/**
 * @brief   Disables interrupt sources at kernel level.
 */
void port_suspend(void) {

  port_disable();
}

/**
 * @brief   Enables all interrupt sources.
 */
void port_enable(void) {

  port_irq_sts = (syssts_t)0;
#if CH_CFG_SMP_MODE == TRUE
  port_unmask_ipi();
#endif
}

/**
 * Performs a context switch between two threads.
 * @param otp the thread to be switched out
 * @param ntp the thread to be switched in
 */
__attribute__((used))
static void __dummy(thread_t *ntp, thread_t *otp) {
  (void)ntp; (void)otp;

  asm volatile (
                ".globl port_switch                             \n\t"
                "port_switch:"
                "push    %%rsi                                  \n\t"
                "push    %%rdi                                  \n\t"
                "push    %%r15                                  \n\t"
                "push    %%r14                                  \n\t"
                "push    %%r13                                  \n\t"
                "push    %%r12                                  \n\t"
                "push    %%rbx                                  \n\t"
                "push    %%rbp                                  \n\t"
                "movq    %%rsp, %c[ctx](%%rsi)                  \n\t"
                "movq    %c[ctx](%%rdi), %%rsp                  \n\t"
                "pop     %%rbp                                  \n\t"
                "pop     %%rbx                                  \n\t"
                "pop     %%r12                                  \n\t"
                "pop     %%r13                                  \n\t"
                "pop     %%r14                                  \n\t"
                "pop     %%r15                                  \n\t"
                "pop     %%rdi                                  \n\t"
                "pop     %%rsi                                  \n\t"
                "ret"
                :
                : [ctx] "i" (CONTEXT_OFFSET));
}

/**
 * @brief   Start a thread by invoking its work function.
 * @details If the work function returns @p chThdExit() is automatically
 *          invoked.
 */
__attribute__((noreturn))
void _port_thread_start(msg_t (*pf)(void *), void *p) {

  chSysUnlock();
  pf(p);
  chThdExit(0);
  while (1);
}

/**
 * @brief   Returns the current value of the realtime counter.
 *
 * @return              The realtime counter value.
 */
rtcnt_t port_rt_get_counter_value(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return ((rtcnt_t)tv.tv_sec * (rtcnt_t)1000000) + (rtcnt_t)tv.tv_usec;
}

/** @} */
