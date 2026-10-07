/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.
    SPDX-License-Identifier: GPL-3.0-only
*/

/* Real kernel objects, consumed as raw bytes by check_registry.py. No
   descriptor offsets are used to construct these fixtures.*/
#include "ch.h"

#define FIXTURE(n) __attribute__((section(".fixture." #n), used))

#if defined(TEST_PORT_ID)
#define INNER_REGISTERS                                                   \
  .r4 = 0x44U, .r5 = 0x55U, .r6 = 0x66U, .r7 = 0x77U,                   \
  .r8 = 0x88U, .r9 = 0x99U, .r10 = 0xAAU, .r11 = 0xBBU

const struct port_intctx inner FIXTURE(inner) = {
  INNER_REGISTERS,
#if (TEST_PORT_ID == 4) || (TEST_PORT_ID == 6)
  .basepri = 0x80U,
#if TEST_PORT_ID == 6
  .lr_exc = 0xFFFFFFBCU - (CORTEX_USE_FPU ? 0x10U : 0U),
#else
  .lr_exc = 0xFFFFFFFDU - (CORTEX_USE_FPU ? 0x10U : 0U),
#endif
#if PORT_SAVE_CONTROL == TRUE
  .control = 3U,
#endif
#else
  .lr = 0x12345679U,
#endif
#if (TEST_PORT_ID == 6) ||                                              \
    ((TEST_PORT_ID == 5) && (CH_DBG_ENABLE_STACK_CHECK == TRUE))
  .splim = 0x20001000U,
#endif
#if defined(CORTEX_USE_FPU) && (CORTEX_USE_FPU == TRUE)
  .s16 = 0x1616U, .s31 = 0x3131U,
#if (TEST_PORT_ID == 3) || (TEST_PORT_ID == 5)
  .fpscr = 0x01000000U,
#endif
#endif
};

const struct port_extctx outer FIXTURE(outer) = {
  .r0 = 0x00U, .r1 = 0x11U, .r2 = 0x22U, .r3 = 0x33U,
  .r12 = 0xCCU, .lr_thd = 0xDDU, .pc = 0xEEU, .xpsr = 0x01000200U,
#if defined(CORTEX_USE_FPU) && (CORTEX_USE_FPU == TRUE)
  .s0 = 0x1000U, .s15 = 0x1515U, .fpscr = 0x02000000U,
#endif
};

const struct port_context context FIXTURE(context) = {
  .sp = (void *)0x20002000U,
#if (TEST_PORT_ID == 4) || (TEST_PORT_ID == 6)
  .regs = {INNER_REGISTERS},
#if PORT_SWITCHED_REGIONS_NUMBER > 0
  .regions = (const port_mpureg_t *)0x20007000U,
#endif
#endif
};

#if ((TEST_PORT_ID == 4) || (TEST_PORT_ID == 6)) &&                        \
    (PORT_SWITCHED_REGIONS_NUMBER > 0)
const port_mpureg_t regions[PORT_SWITCHED_REGIONS_NUMBER] FIXTURE(regions) = {
  [0 ... PORT_SWITCHED_REGIONS_NUMBER - 1] = {
    .rbar = 0x1000U,
#if TEST_PORT_ID == 4
    .rasr = 0x2001U
#else
    .rlar = 0x2001U
#endif
  }
};
#endif
#else
const struct port_intctx inner FIXTURE(inner) = {0};
const struct port_context context FIXTURE(context) = {0};
#endif

const thread_t thread FIXTURE(thread) = {
  .hdr.pqueue.prio = 123U,
  .owner = (os_instance_t *)0x20003000U,
  .wabase = (stkline_t *)0x20001000U,
  .waend = (stkline_t *)0x20002800U,
  .state = CH_STATE_SLEEPING,
  .flags = 2U,
  .refs = 3U,
  .name = (const char *)0x08001000U,
  .rqueue.next = (ch_queue_t *)0x20004000U,
  .rqueue.prev = (ch_queue_t *)0x20005000U,
#if CH_CFG_TIME_QUANTUM > 0
  .ticks = 7U,
#endif
#if CH_DBG_THREADS_PROFILING == TRUE
  .time = 0x4321U,
#endif
};

const os_instance_t instance FIXTURE(instance) = {
  .rlist.current = (thread_t *)0x20006000U,
  .core_id = 1U,
#if CH_CFG_SMP_MODE == FALSE
  .reglist.queue.next = (ch_queue_t *)0x20004000U,
  .reglist.queue.prev = (ch_queue_t *)0x20005000U,
#endif
};

const ch_system_t system FIXTURE(system) = {
  .state = ch_sys_running,
  .instances[0] = (os_instance_t *)0x20003000U,
#if CH_CFG_SMP_MODE == TRUE
  .reglist.queue.next = (ch_queue_t *)0x20004000U,
  .reglist.queue.prev = (ch_queue_t *)0x20005000U,
#endif
};
