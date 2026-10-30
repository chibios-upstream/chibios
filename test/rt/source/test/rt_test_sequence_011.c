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

#include "hal.h"
#include "rt_test_root.h"

/**
 * @file    rt_test_sequence_011.c
 * @brief   Test Sequence 011 code.
 *
 * @page rt_test_sequence_011 [11] Dynamic threads
 *
 * File: @ref rt_test_sequence_011.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the dynamic thread
 * creation APIs.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_DYNAMIC == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage rt_test_011_001
 * - @subpage rt_test_011_002
 * - @subpage rt_test_011_003
 * - @subpage rt_test_011_004
 * - @subpage rt_test_011_005
 * .
 */

#if (CH_CFG_USE_DYNAMIC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#if CH_CFG_USE_HEAP
static memory_heap_t heap1;
#endif
#if CH_CFG_USE_MEMPOOLS
static memory_pool_t mp1;
static memory_pool_t mp2;
static CH_MEM_GLOBAL_COHERENT_BSS thread_t thd_objs[4];
#endif

static THD_FUNCTION(dyn_thread1, p) {

  test_emit_token(*(char *)p);
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_011_001 [11.1] Threads creation from Memory Heap
 *
 * <h2>Description</h2>
 * Two threads are started by allocating the memory from the Memory
 * Heap then a third thread is started with a huge stack
 * requirement.<br> The test expects the first two threads to
 * successfully start and the third one to fail.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_HEAP == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.1.1] Getting base priority for threads.
 * - [11.1.2] Getting heap info before the test.
 * - [11.1.3] Creating thread 1, it is expected to succeed.
 * - [11.1.4] Creating thread 2, it is expected to succeed.
 * - [11.1.5] Creating thread 3, it is expected to fail.
 * - [11.1.6] Letting threads execute then checking the start order and
 *   freeing memory.
 * - [11.1.7] Getting heap info again for verification.
 * .
 */

static void rt_test_011_001_setup(void) {
  chHeapObjectInit(&heap1, TEST_SHARED_BUFFER,
                   TEST_SHARED_BUFFER_SIZE);
}

static void rt_test_011_001_teardown(void) {
  chHeapObjectDispose(&heap1);
}

static void rt_test_011_001_execute(void) {
  size_t n1, total1, largest1;
  size_t n2, total2, largest2;
  tprio_t prio;

  /* [11.1.1] Getting base priority for threads.*/
  test_set_step(1);
  {
    prio = chThdGetPriorityX();
  }
  test_end_step(1);

  /* [11.1.2] Getting heap info before the test.*/
  test_set_step(2);
  {
    n1 = chHeapStatus(&heap1, &total1, &largest1);
    test_assert(n1 == 1, "heap fragmented");
  }
  test_end_step(2);

  /* [11.1.3] Creating thread 1, it is expected to succeed.*/
  test_set_step(3);
  {
    threads[0] = chThdCreateFromHeap(&heap1,
                                     THD_WORKING_AREA_SIZE(THREADS_STACK_SIZE),
                                     "dyn1",
                                     prio-1, dyn_thread1, "A");
    test_assert(threads[0] != NULL, "thread creation failed");
  }
  test_end_step(3);

  /* [11.1.4] Creating thread 2, it is expected to succeed.*/
  test_set_step(4);
  {
    threads[1] = chThdCreateFromHeap(&heap1,
                                     THD_WORKING_AREA_SIZE(THREADS_STACK_SIZE),
                                     "dyn2",
                                     prio-2, dyn_thread1, "B");
    test_assert(threads[1] != NULL, "thread creation failed");
  }
  test_end_step(4);

  /* [11.1.5] Creating thread 3, it is expected to fail.*/
  test_set_step(5);
  {
    threads[2] = chThdCreateFromHeap(&heap1,
                                     (((size_t)-1) >> 1U) + 1U,
                                     "dyn3",
                                     prio-3, dyn_thread1, "C");
    test_assert(threads[2] == NULL, "thread creation not failed");
  }
  test_end_step(5);

  /* [11.1.6] Letting threads execute then checking the start order and
     freeing memory.*/
  test_set_step(6);
  {
    test_wait_threads();
    test_assert_sequence("AB", "invalid sequence");
  }
  test_end_step(6);

  /* [11.1.7] Getting heap info again for verification.*/
  test_set_step(7);
  {
    n2 = chHeapStatus(&heap1, &total2, &largest2);
    test_assert(n1 == n2, "fragmentation changed");
    test_assert(total1 == total2, "total free space changed");
    test_assert(largest1 == largest2, "largest fragment size changed");
  }
  test_end_step(7);
}

static const testcase_t rt_test_011_001 = {
  "Threads creation from Memory Heap",
  rt_test_011_001_setup,
  rt_test_011_001_teardown,
  rt_test_011_001_execute
};
#endif /* CH_CFG_USE_HEAP == TRUE */

#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_011_002 [11.2] Threads creation from Memory Pool
 *
 * <h2>Description</h2>
 * Five thread creation are attempted from a pool containing only four
 * elements.<br> The test expects the first four threads to
 * successfully start and the last one to fail.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MEMPOOLS == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.2.1] Adding four working areas to the pool.
 * - [11.2.2] Getting base priority for threads.
 * - [11.2.3] Creating the five threads.
 * - [11.2.4] Testing that only the fifth thread creation failed.
 * - [11.2.5] Letting them run, free the memory then checking the
 *   execution sequence.
 * - [11.2.6] Testing that the pool contains four elements again.
 * .
 */

static void rt_test_011_002_setup(void) {
  chPoolObjectInitAligned(&mp1, THD_WORKING_AREA_SIZE(THREADS_STACK_SIZE),
                          PORT_WORKING_AREA_ALIGN, NULL);
}

static void rt_test_011_002_teardown(void) {
  chPoolObjectDispose(&mp1);
}

static void rt_test_011_002_execute(void) {
  unsigned i;
  tprio_t prio;

  /* [11.2.1] Adding four working areas to the pool.*/
  test_set_step(1);
  {
    for (i = 0; i < 4; i++)
      chPoolFree(&mp1, wa[i]);
  }
  test_end_step(1);

  /* [11.2.2] Getting base priority for threads.*/
  test_set_step(2);
  {
    prio = chThdGetPriorityX();
  }
  test_end_step(2);

  /* [11.2.3] Creating the five threads.*/
  test_set_step(3);
  {
    threads[0] = chThdCreateFromMemoryPool(&mp1, "dyn1", prio-1, dyn_thread1, "A");
    threads[1] = chThdCreateFromMemoryPool(&mp1, "dyn2", prio-2, dyn_thread1, "B");
    threads[2] = chThdCreateFromMemoryPool(&mp1, "dyn3", prio-3, dyn_thread1, "C");
    threads[3] = chThdCreateFromMemoryPool(&mp1, "dyn4", prio-4, dyn_thread1, "D");
    threads[4] = chThdCreateFromMemoryPool(&mp1, "dyn5", prio-5, dyn_thread1, "E");
  }
  test_end_step(3);

  /* [11.2.4] Testing that only the fifth thread creation failed.*/
  test_set_step(4);
  {
    test_assert((threads[0] != NULL) &&
                (threads[1] != NULL) &&
                (threads[2] != NULL) &&
                (threads[3] != NULL),
                "thread creation failed");
    test_assert(threads[4] == NULL,
                "thread creation not failed");
  }
  test_end_step(4);

  /* [11.2.5] Letting them run, free the memory then checking the
     execution sequence.*/
  test_set_step(5);
  {
    test_wait_threads();
    test_assert_sequence("ABCD", "invalid sequence");
  }
  test_end_step(5);

  /* [11.2.6] Testing that the pool contains four elements again.*/
  test_set_step(6);
  {
    for (i = 0; i < 4; i++)
      test_assert(chPoolAlloc(&mp1) != NULL, "pool list empty");
    test_assert(chPoolAlloc(&mp1) == NULL, "pool list not empty");
  }
  test_end_step(6);
}

static const testcase_t rt_test_011_002 = {
  "Threads creation from Memory Pool",
  rt_test_011_002_setup,
  rt_test_011_002_teardown,
  rt_test_011_002_execute
};
#endif /* CH_CFG_USE_MEMPOOLS == TRUE */

#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_011_003 [11.3] Detached dynamic thread recovery
 *
 * <h2>Description</h2>
 * A dynamic thread allocated from a heap is detached while still
 * active. After termination its memory is recovered by a registry
 * scan.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_HEAP == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.3.1] Getting heap status and base priority before the test.
 * - [11.3.2] A dynamic thread is created from the heap then its
 *   initial reference is released before it can run.
 * - [11.3.3] The detached thread is allowed to terminate, it is
 *   expected to keep its heap allocation until the registry is
 *   scanned.
 * - [11.3.4] The registry is scanned forward, the detached final
 *   thread is found and its memory is recovered.
 * .
 */

static void rt_test_011_003_setup(void) {
  chHeapObjectInit(&heap1, TEST_SHARED_BUFFER,
                   TEST_SHARED_BUFFER_SIZE);
}

static void rt_test_011_003_teardown(void) {
  {
    thread_t *tp, *ntp;

    tp = chRegFirstThread();
    do {
      ntp = chRegNextThread(tp);
      tp = ntp;
    } while (tp != NULL);
  }
  chHeapObjectDispose(&heap1);
}

static void rt_test_011_003_execute(void) {
  size_t n1, total1, largest1;
  size_t n2, total2, largest2;
  size_t n3, total3, largest3;
  thread_t *tp, *ntp, *dtp;
  tprio_t prio;
  bool found;

  /* [11.3.1] Getting heap status and base priority before the test.*/
  test_set_step(1);
  {
    prio = chThdGetPriorityX();
    n1 = chHeapStatus(&heap1, &total1, &largest1);
    test_assert(n1 == 1, "heap fragmented");
  }
  test_end_step(1);

  /* [11.3.2] A dynamic thread is created from the heap then its
     initial reference is released before it can run.*/
  test_set_step(2);
  {
    dtp = chThdCreateFromHeap(&heap1,
                                THD_WORKING_AREA_SIZE(THREADS_STACK_SIZE),
                                "dyn-detached",
                                prio-1, dyn_thread1, "D");
    test_assert(dtp != NULL, "thread creation failed");
    threads[0] = dtp;
    chThdRelease(dtp);
    threads[0] = NULL;
  }
  test_end_step(2);

  /* [11.3.3] The detached thread is allowed to terminate, it is
     expected to keep its heap allocation until the registry is
     scanned.*/
  test_set_step(3);
  {
    chThdSleepMilliseconds(50);
    test_assert_sequence("D", "invalid sequence");
    n2 = chHeapStatus(&heap1, &total2, &largest2);
    test_assert(total2 < total1, "memory already recovered");
    (void)n2;
    (void)largest2;
  }
  test_end_step(3);

  /* [11.3.4] The registry is scanned forward, the detached final
     thread is found and its memory is recovered.*/
  test_set_step(4);
  {
    found = false;
    tp = chRegFirstThread();
    do {
      if (tp == dtp) {
        found = true;
      }
      ntp = chRegNextThread(tp);
      tp = ntp;
    } while (tp != NULL);

    test_assert(found, "not found in registry scan");
    n3 = chHeapStatus(&heap1, &total3, &largest3);
    test_assert(n1 == n3, "fragmentation changed");
    test_assert(total1 == total3, "total free space changed");
    test_assert(largest1 == largest3, "largest fragment size changed");
  }
  test_end_step(4);
}

static const testcase_t rt_test_011_003 = {
  "Detached dynamic thread recovery",
  rt_test_011_003_setup,
  rt_test_011_003_teardown,
  rt_test_011_003_execute
};
#endif /* CH_CFG_USE_HEAP == TRUE */

#if ((CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE)) || defined(__DOXYGEN__)
/**
 * @page rt_test_011_004 [11.4] Threads spawning from Memory Pool and Memory Heap
 *
 * <h2>Description</h2>
 * Thread objects are allocated from a pool containing two elements and
 * stacks from the Memory Heap.<br> The test expects a spawn with a
 * huge stack requirement to fail without consuming a thread object,
 * then two spawns to succeed and a third one to fail because the pool
 * is empty.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - (CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE)
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.4.1] Getting base priority and heap info, adding two thread
 *   objects to the pool.
 * - [11.4.2] Spawning a thread with a huge stack, it is expected to
 *   fail.
 * - [11.4.3] Spawning three threads, the first two are expected to
 *   succeed and the third one to fail because the pool is empty.
 * - [11.4.4] Letting threads execute then checking the start order and
 *   freeing memory.
 * - [11.4.5] Getting heap info again for verification, testing that
 *   the pool contains two elements again.
 * .
 */

static void rt_test_011_004_setup(void) {
  chHeapObjectInit(&heap1, TEST_SHARED_BUFFER,
                   TEST_SHARED_BUFFER_SIZE);
  chPoolObjectInit(&mp1, sizeof (thread_t), NULL);
}

static void rt_test_011_004_teardown(void) {
  chPoolObjectDispose(&mp1);
  chHeapObjectDispose(&heap1);
}

static void rt_test_011_004_execute(void) {
  size_t n1, total1, largest1;
  size_t n2, total2, largest2;
  unsigned i;
  tprio_t prio;

  /* [11.4.1] Getting base priority and heap info, adding two thread
     objects to the pool.*/
  test_set_step(1);
  {
    prio = chThdGetPriorityX();
    n1 = chHeapStatus(&heap1, &total1, &largest1);
    test_assert(n1 == 1, "heap fragmented");
    for (i = 0; i < 2; i++)
      chPoolFree(&mp1, &thd_objs[i]);
  }
  test_end_step(1);

  /* [11.4.2] Spawning a thread with a huge stack, it is expected to
     fail.*/
  test_set_step(2);
  {
    THD_HEAP_DECL(td, "dyn1", &heap1, (((size_t)-1) >> 1U) + 1U,
                  prio-1, dyn_thread1, "X", NULL);

    threads[0] = chThdSpawnFromHeap(&mp1, &td);
    test_assert(threads[0] == NULL, "thread spawn not failed");
  }
  test_end_step(2);

  /* [11.4.3] Spawning three threads, the first two are expected to
     succeed and the third one to fail because the pool is empty.*/
  test_set_step(3);
  {
    thread_heap_descriptor_t td[3] = {
      __THD_HEAP_DECL_DATA("dyn1", &heap1, THD_STACK_SIZE(THREADS_STACK_SIZE),
                           prio-1, dyn_thread1, "A", NULL),
      __THD_HEAP_DECL_DATA("dyn2", &heap1, THD_STACK_SIZE(THREADS_STACK_SIZE),
                           prio-2, dyn_thread1, "B", NULL),
      __THD_HEAP_DECL_DATA("dyn3", &heap1, THD_STACK_SIZE(THREADS_STACK_SIZE),
                           prio-3, dyn_thread1, "C", NULL)
    };

    for (i = 0; i < 3; i++)
      threads[i] = chThdSpawnFromHeap(&mp1, &td[i]);
    test_assert((threads[0] != NULL) &&
                (threads[1] != NULL),
                "thread spawn failed");
    test_assert(threads[2] == NULL,
                "thread spawn not failed");
  }
  test_end_step(3);

  /* [11.4.4] Letting threads execute then checking the start order and
     freeing memory.*/
  test_set_step(4);
  {
    test_wait_threads();
    test_assert_sequence("AB", "invalid sequence");
  }
  test_end_step(4);

  /* [11.4.5] Getting heap info again for verification, testing that
     the pool contains two elements again.*/
  test_set_step(5);
  {
    n2 = chHeapStatus(&heap1, &total2, &largest2);
    test_assert(n1 == n2, "fragmentation changed");
    test_assert(total1 == total2, "total free space changed");
    test_assert(largest1 == largest2, "largest fragment size changed");
    for (i = 0; i < 2; i++)
      test_assert(chPoolAlloc(&mp1) != NULL, "pool list empty");
    test_assert(chPoolAlloc(&mp1) == NULL, "pool list not empty");
  }
  test_end_step(5);
}

static const testcase_t rt_test_011_004 = {
  "Threads spawning from Memory Pool and Memory Heap",
  rt_test_011_004_setup,
  rt_test_011_004_teardown,
  rt_test_011_004_execute
};
#endif /* (CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE) */

#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_011_005 [11.5] Threads spawning from Memory Pools
 *
 * <h2>Description</h2>
 * Thread objects are allocated from a pool containing four elements
 * and stacks from a pool containing three elements.<br> The test
 * expects the first three spawns to succeed and the fourth one to fail
 * without consuming a thread object.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MEMPOOLS == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.5.1] Adding four thread objects to the first pool and three
 *   stacks to the second pool.
 * - [11.5.2] Getting base priority for threads.
 * - [11.5.3] Spawning four threads, the first three are expected to
 *   succeed and the fourth one to fail because the stacks pool is
 *   empty.
 * - [11.5.4] Letting them run, free the memory then checking the
 *   execution sequence.
 * - [11.5.5] Testing that the pools contain four thread objects and
 *   three stacks again.
 * .
 */

static void rt_test_011_005_setup(void) {
  chPoolObjectInit(&mp1, sizeof (thread_t), NULL);
  chPoolObjectInitAligned(&mp2, THD_POOL_STACK_SIZE(THREADS_STACK_SIZE),
                          PORT_WORKING_AREA_ALIGN, NULL);
}

static void rt_test_011_005_teardown(void) {
  chPoolObjectDispose(&mp2);
  chPoolObjectDispose(&mp1);
}

static void rt_test_011_005_execute(void) {
  unsigned i;
  tprio_t prio;

  /* [11.5.1] Adding four thread objects to the first pool and three
     stacks to the second pool.*/
  test_set_step(1);
  {
    for (i = 0; i < 4; i++)
      chPoolFree(&mp1, &thd_objs[i]);
    for (i = 0; i < 3; i++)
      chPoolFree(&mp2, wa[i]);
  }
  test_end_step(1);

  /* [11.5.2] Getting base priority for threads.*/
  test_set_step(2);
  {
    prio = chThdGetPriorityX();
  }
  test_end_step(2);

  /* [11.5.3] Spawning four threads, the first three are expected to
     succeed and the fourth one to fail because the stacks pool is
     empty.*/
  test_set_step(3);
  {
    thread_pool_descriptor_t td[4] = {
      __THD_POOL_DECL_DATA("dyn1", &mp2, prio-1, dyn_thread1, "A", NULL),
      __THD_POOL_DECL_DATA("dyn2", &mp2, prio-2, dyn_thread1, "B", NULL),
      __THD_POOL_DECL_DATA("dyn3", &mp2, prio-3, dyn_thread1, "C", NULL),
      __THD_POOL_DECL_DATA("dyn4", &mp2, prio-4, dyn_thread1, "D", NULL)
    };

    for (i = 0; i < 4; i++)
      threads[i] = chThdSpawnFromMemoryPool(&mp1, &td[i]);
    test_assert((threads[0] != NULL) &&
                (threads[1] != NULL) &&
                (threads[2] != NULL),
                "thread spawn failed");
    test_assert(threads[3] == NULL,
                "thread spawn not failed");
  }
  test_end_step(3);

  /* [11.5.4] Letting them run, free the memory then checking the
     execution sequence.*/
  test_set_step(4);
  {
    test_wait_threads();
    test_assert_sequence("ABC", "invalid sequence");
  }
  test_end_step(4);

  /* [11.5.5] Testing that the pools contain four thread objects and
     three stacks again.*/
  test_set_step(5);
  {
    for (i = 0; i < 4; i++)
      test_assert(chPoolAlloc(&mp1) != NULL, "objects pool list empty");
    test_assert(chPoolAlloc(&mp1) == NULL, "objects pool list not empty");
    for (i = 0; i < 3; i++)
      test_assert(chPoolAlloc(&mp2) != NULL, "stacks pool list empty");
    test_assert(chPoolAlloc(&mp2) == NULL, "stacks pool list not empty");
  }
  test_end_step(5);
}

static const testcase_t rt_test_011_005 = {
  "Threads spawning from Memory Pools",
  rt_test_011_005_setup,
  rt_test_011_005_teardown,
  rt_test_011_005_execute
};
#endif /* CH_CFG_USE_MEMPOOLS == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const rt_test_sequence_011_array[] = {
#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
  &rt_test_011_001,
#endif
#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
  &rt_test_011_002,
#endif
#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
  &rt_test_011_003,
#endif
#if ((CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE)) || defined(__DOXYGEN__)
  &rt_test_011_004,
#endif
#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
  &rt_test_011_005,
#endif
  NULL
};

/**
 * @brief   Dynamic threads.
 */
const testsequence_t rt_test_sequence_011 = {
  "Dynamic threads",
  rt_test_sequence_011_array
};

#endif /* CH_CFG_USE_DYNAMIC == TRUE */
