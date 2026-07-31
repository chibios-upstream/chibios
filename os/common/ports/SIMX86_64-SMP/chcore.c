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

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

__thread core_id_t port_core_id;
__thread bool port_isr_context_flag;
__thread syssts_t port_irq_sts = (syssts_t)1;
__thread bool port_startup_unlock_pending;

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

#if CH_CFG_SMP_MODE == TRUE
static pthread_t port_core_threads[PORT_CORES_NUMBER];
static unsigned port_startup_state;
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

static void *port_core1_start(void *p) {
  extern void c1_main(void);

  (void)p;

  port_core_id = (core_id_t)1;
  port_irq_sts = (syssts_t)1;
  port_isr_context_flag = false;
  port_startup_unlock_pending = false;

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

    port_core_threads[0] = pthread_self();
    err = pthread_create(&port_core_threads[1], NULL,
                         port_core1_start, NULL);
    if (err != 0) {
      abort();
    }
  }
#else
  port_startup_unlock_pending = false;
#endif
}

/**
 * @brief   Performs the one-time host-core startup handshake.
 * @note    Called on each core by its first kernel unlock.
 */
void port_startup_unlock(void) {

#if CH_CFG_SMP_MODE == TRUE
  if (port_core_id == (core_id_t)0) {
    __atomic_store_n(&port_startup_state, PORT_STARTUP_PRIMARY_RELEASED,
                     __ATOMIC_RELEASE);
    port_wait_startup_state(PORT_STARTUP_SECONDARY_READY);
  }
  else {
    __atomic_store_n(&port_startup_state, PORT_STARTUP_SECONDARY_READY,
                     __ATOMIC_RELEASE);
  }
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
