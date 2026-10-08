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
#include "oslib_test_root.h"

/**
 * @file    oslib_test_sequence_008.c
 * @brief   Test Sequence 008 code.
 *
 * @page oslib_test_sequence_008 [8] Memory Heaps
 *
 * File: @ref oslib_test_sequence_008.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS library functionalities related to
 * memory heaps.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_HEAP == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage oslib_test_008_001
 * - @subpage oslib_test_008_002
 * - @subpage oslib_test_008_003
 * - @subpage oslib_test_008_004
 * - @subpage oslib_test_008_005
 * .
 */

#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#define ALLOC_SIZE 16
#define HEAP_SIZE (ALLOC_SIZE * 8)

static memory_heap_t test_heap;
static uint8_t test_heap_buffer[HEAP_SIZE];
static ALIGNED_VAR(CH_HEAP_ALIGNMENT) heap_header_t integrity_buffer[8];

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page oslib_test_008_001 [8.1] Allocation and fragmentation
 *
 * <h2>Description</h2>
 * Series of allocations/deallocations are performed in carefully
 * designed sequences in order to stimulate all the possible code paths
 * inside the allocator. The test expects to find the heap back to the
 * initial status after each sequence.
 *
 * <h2>Test Steps</h2>
 * - [8.1.1] Testing initial conditions, the heap must not be
 *   fragmented and one free block present, finally, integrity is
 *   checked.
 * - [8.1.2] Trying to allocate an block bigger than available space,
 *   an error is expected, finally, integrity is checked.
 * - [8.1.3] Single block allocation using chHeapAlloc() then the block
 *   is freed using chHeapFree(), must not fail, finally, integrity is
 *   checked.
 * - [8.1.4] Using chHeapStatus() to assess the heap state. There must
 *   be at least one free block of sufficient size, finally, integrity
 *   is checked.
 * - [8.1.5] Allocating then freeing in the same order, finally,
 *   integrity is checked.
 * - [8.1.6] Allocating then freeing in reverse order, finally,
 *   integrity is checked.
 * - [8.1.7] Small fragments handling. Checking the behavior when
 *   allocating blocks with size not multiple of alignment unit,
 *   finally, integrity is checked.
 * - [8.1.8] Skipping a fragment, the first fragment in the list is too
 *   small so the allocator must pick the second one, finally,
 *   integrity is checked.
 * - [8.1.9] Allocating the whole available space, finally, integrity
 *   is checked.
 * - [8.1.10] Testing final conditions. The heap geometry must be the
 *   same than the one registered at beginning, finally, integrity is
 *   checked.
 * .
 */

static void oslib_test_008_001_setup(void) {
  chHeapObjectInit(&test_heap, test_heap_buffer, sizeof(test_heap_buffer));
}

static void oslib_test_008_001_execute(void) {
  void *p1, *p2, *p3;
  size_t n, sz;

  /* [8.1.1] Testing initial conditions, the heap must not be
     fragmented and one free block present, finally, integrity is
     checked.*/
  test_set_step(1);
  {
    test_assert(chHeapStatus(&test_heap, &sz, NULL) == 1, "heap fragmented");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(1);

  /* [8.1.2] Trying to allocate an block bigger than available space,
     an error is expected, finally, integrity is checked.*/
  test_set_step(2);
  {
    p1 = chHeapAlloc(&test_heap, sizeof test_heap_buffer * 2);
    test_assert(p1 == NULL, "allocation not failed");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(2);

  /* [8.1.3] Single block allocation using chHeapAlloc() then the block
     is freed using chHeapFree(), must not fail, finally, integrity is
     checked.*/
  test_set_step(3);
  {
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    test_assert(p1 != NULL, "allocation failed");
    chHeapFree(p1);
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(3);

  /* [8.1.4] Using chHeapStatus() to assess the heap state. There must
     be at least one free block of sufficient size, finally, integrity
     is checked.*/
  test_set_step(4);
  {
    size_t total_size, largest_size;

    n = chHeapStatus(&test_heap, &total_size, &largest_size);
    test_assert(n == 1, "missing free block");
    test_assert(total_size >= ALLOC_SIZE, "unexpected heap state");
    test_assert(total_size == largest_size, "unexpected heap state");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(4);

  /* [8.1.5] Allocating then freeing in the same order, finally,
     integrity is checked.*/
  test_set_step(5);
  {
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    p2 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    p3 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    chHeapFree(p1);                                 /* Does not merge.*/
    chHeapFree(p2);                                 /* Merges backward.*/
    chHeapFree(p3);                                 /* Merges both sides.*/
    test_assert(chHeapStatus(&test_heap, &n, NULL) == 1, "heap fragmented");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(5);

  /* [8.1.6] Allocating then freeing in reverse order, finally,
     integrity is checked.*/
  test_set_step(6);
  {
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    p2 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    p3 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    chHeapFree(p3);                                 /* Merges forward.*/
    chHeapFree(p2);                                 /* Merges forward.*/
    chHeapFree(p1);                                 /* Merges forward.*/
    test_assert(chHeapStatus(&test_heap, &n, NULL) == 1, "heap fragmented");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(6);

  /* [8.1.7] Small fragments handling. Checking the behavior when
     allocating blocks with size not multiple of alignment unit,
     finally, integrity is checked.*/
  test_set_step(7);
  {
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE + 1);
    p2 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    chHeapFree(p1);
    test_assert(chHeapStatus(&test_heap, &n, NULL) == 2, "invalid state");
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    /* Note, the first situation happens when the alignment size is smaller
       than the header size, the second in the other cases.*/
    test_assert((chHeapStatus(&test_heap, &n, NULL) == 1) ||
                (chHeapStatus(&test_heap, &n, NULL) == 2), "heap fragmented");
    chHeapFree(p2);
    chHeapFree(p1);
    test_assert(chHeapStatus(&test_heap, &n, NULL) == 1, "heap fragmented");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(7);

  /* [8.1.8] Skipping a fragment, the first fragment in the list is too
     small so the allocator must pick the second one, finally,
     integrity is checked.*/
  test_set_step(8);
  {
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    p2 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    chHeapFree(p1);
    test_assert( chHeapStatus(&test_heap, &n, NULL) == 2, "invalid state");
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE * 2); /* Skips first fragment.*/
    chHeapFree(p1);
    chHeapFree(p2);
    test_assert(chHeapStatus(&test_heap, &n, NULL) == 1, "heap fragmented");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(8);

  /* [8.1.9] Allocating the whole available space, finally, integrity
     is checked.*/
  test_set_step(9);
  {
    (void)chHeapStatus(&test_heap, &n, NULL);
    p1 = chHeapAlloc(&test_heap, n);
    test_assert(p1 != NULL, "allocation failed");
    test_assert(chHeapStatus(&test_heap, NULL, NULL) == 0, "not empty");
    chHeapFree(p1);
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(9);

  /* [8.1.10] Testing final conditions. The heap geometry must be the
     same than the one registered at beginning, finally, integrity is
     checked.*/
  test_set_step(10);
  {
    test_assert(chHeapStatus(&test_heap, &n, NULL) == 1, "heap fragmented");
    test_assert(n == sz, "size changed");
    test_assert(!chHeapIntegrityCheck(&test_heap), "integrity failure");
  }
  test_end_step(10);
}

static const testcase_t oslib_test_008_001 = {
  "Allocation and fragmentation",
  oslib_test_008_001_setup,
  NULL,
  oslib_test_008_001_execute
};

/**
 * @page oslib_test_008_002 [8.2] Default Heap
 *
 * <h2>Description</h2>
 * The default heap is pre-allocated in the system. We test base
 * functionality.
 *
 * <h2>Test Steps</h2>
 * - [8.2.1] Single block allocation using chHeapAlloc() then the block
 *   is freed using chHeapFree(), must not fail.
 * - [8.2.2] Testing allocation failure.
 * .
 */

static void oslib_test_008_002_execute(void) {
  void *p1;
  size_t total_size, largest_size;

  /* [8.2.1] Single block allocation using chHeapAlloc() then the block
     is freed using chHeapFree(), must not fail.*/
  test_set_step(1);
  {
    (void)chHeapStatus(NULL, &total_size, &largest_size);
    p1 = chHeapAlloc(&test_heap, ALLOC_SIZE);
    test_assert(p1 != NULL, "allocation failed");
    chHeapFree(p1);
  }
  test_end_step(1);

  /* [8.2.2] Testing allocation failure.*/
  test_set_step(2);
  {
    p1 = chHeapAlloc(NULL, (size_t)-256);
    test_assert(p1 == NULL, "allocation not failed");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_008_002 = {
  "Default Heap",
  NULL,
  NULL,
  oslib_test_008_002_execute
};

/**
 * @page oslib_test_008_003 [8.3] Heap header bounds
 *
 * <h2>Description</h2>
 * The integrity checker rejects headers outside the heap or only
 * partially contained in it. All test headers have valid backing
 * storage, so no fault or kernel assertion is expected.
 *
 * <h2>Test Steps</h2>
 * - [8.3.1] The initial heap must pass the integrity check.
 * - [8.3.2] A header below the heap base must be rejected. Restore the
 *   link before checking the result.
 * - [8.3.3] A header at the first address beyond the heap must be
 *   rejected. Restore the link before checking the result.
 * - [8.3.4] A header whose final byte is outside the heap must be
 *   rejected. Restore the area before checking the result.
 * - [8.3.5] An out-of-area link after a valid header must also be
 *   rejected. Restore the link before checking the result.
 * .
 */

static void oslib_test_008_003_setup(void) {
  chHeapObjectInit(&test_heap, &integrity_buffer[1],
                   2U * sizeof (heap_header_t));
  integrity_buffer[0].free.next = NULL;
  integrity_buffer[0].free.pages = 0U;
  integrity_buffer[3].free.next = NULL;
  integrity_buffer[3].free.pages = 0U;
}

static void oslib_test_008_003_teardown(void) {
  chHeapObjectDispose(&test_heap);
}

static void oslib_test_008_003_execute(void) {
  heap_header_t *saved;
  size_t saved_size;
  bool failed;

  /* [8.3.1] The initial heap must pass the integrity check.*/
  test_set_step(1);
  {
    test_assert(!chHeapIntegrityCheck(&test_heap), "initial integrity failure");
  }
  test_end_step(1);

  /* [8.3.2] A header below the heap base must be rejected. Restore the
     link before checking the result.*/
  test_set_step(2);
  {
    saved = test_heap.header.free.next;
    test_heap.header.free.next = &integrity_buffer[0];
    failed = chHeapIntegrityCheck(&test_heap);
    test_heap.header.free.next = saved;
    test_assert(failed, "header below base accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(2);

  /* [8.3.3] A header at the first address beyond the heap must be
     rejected. Restore the link before checking the result.*/
  test_set_step(3);
  {
    saved = test_heap.header.free.next;
    test_heap.header.free.next = &integrity_buffer[3];
    failed = chHeapIntegrityCheck(&test_heap);
    test_heap.header.free.next = saved;
    test_assert(failed, "header beyond heap accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(3);

  /* [8.3.4] A header whose final byte is outside the heap must be
     rejected. Restore the area before checking the result.*/
  test_set_step(4);
  {
    saved_size = test_heap.area.size;
    test_heap.area.size = sizeof (heap_header_t) - 1U;
    failed = chHeapIntegrityCheck(&test_heap);
    test_heap.area.size = saved_size;
    test_assert(failed, "partial header accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(4);

  /* [8.3.5] An out-of-area link after a valid header must also be
     rejected. Restore the link before checking the result.*/
  test_set_step(5);
  {
    saved = integrity_buffer[1].free.next;
    integrity_buffer[1].free.next = &integrity_buffer[3];
    failed = chHeapIntegrityCheck(&test_heap);
    integrity_buffer[1].free.next = saved;
    test_assert(failed, "later out-of-area header accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(5);
}

static const testcase_t oslib_test_008_003 = {
  "Heap header bounds",
  oslib_test_008_003_setup,
  oslib_test_008_003_teardown,
  oslib_test_008_003_execute
};

/**
 * @page oslib_test_008_004 [8.4] Heap block sizes
 *
 * <h2>Description</h2>
 * Block sizes must be representable in bytes and fit the heap area.
 * Invalid page counts are restored before checking test results.
 *
 * <h2>Test Steps</h2>
 * - [8.4.1] The initial block ending exactly at the heap limit must
 *   pass the check.
 * - [8.4.2] A page count that would wrap the byte size to one header
 *   must be rejected.
 * - [8.4.3] A page count whose header addition would wrap must be
 *   rejected.
 * - [8.4.4] The first page count whose full byte size cannot be
 *   represented must be rejected.
 * - [8.4.5] A representable byte size that exceeds this heap must
 *   still be rejected.
 * - [8.4.6] A block exceeding the heap by one page must be rejected.
 * .
 */

static void oslib_test_008_004_setup(void) {
  chHeapObjectInit(&test_heap, integrity_buffer,
                   sizeof (integrity_buffer));
}

static void oslib_test_008_004_teardown(void) {
  chHeapObjectDispose(&test_heap);
}

static void oslib_test_008_004_execute(void) {
  heap_header_t *hp;
  size_t saved_pages;
  bool failed;

  hp = test_heap.header.free.next;
  saved_pages = hp->free.pages;

  /* [8.4.1] The initial block ending exactly at the heap limit must
     pass the check.*/
  test_set_step(1);
  {
    test_assert(!chHeapIntegrityCheck(&test_heap), "exact-fit block rejected");
  }
  test_end_step(1);

  /* [8.4.2] A page count that would wrap the byte size to one header
     must be rejected.*/
  test_set_step(2);
  {
    hp->free.pages = (SIZE_MAX / sizeof (heap_header_t)) + 1U;
    failed = chHeapIntegrityCheck(&test_heap);
    hp->free.pages = saved_pages;
    test_assert(failed, "wrapped block size accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(2);

  /* [8.4.3] A page count whose header addition would wrap must be
     rejected.*/
  test_set_step(3);
  {
    hp->free.pages = SIZE_MAX;
    failed = chHeapIntegrityCheck(&test_heap);
    hp->free.pages = saved_pages;
    test_assert(failed, "wrapped page count accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(3);

  /* [8.4.4] The first page count whose full byte size cannot be
     represented must be rejected.*/
  test_set_step(4);
  {
    hp->free.pages = SIZE_MAX / sizeof (heap_header_t);
    failed = chHeapIntegrityCheck(&test_heap);
    hp->free.pages = saved_pages;
    test_assert(failed, "unrepresentable block size accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(4);

  /* [8.4.5] A representable byte size that exceeds this heap must
     still be rejected.*/
  test_set_step(5);
  {
    hp->free.pages = (SIZE_MAX / sizeof (heap_header_t)) - 1U;
    failed = chHeapIntegrityCheck(&test_heap);
    hp->free.pages = saved_pages;
    test_assert(failed, "block outside heap accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(5);

  /* [8.4.6] A block exceeding the heap by one page must be rejected.*/
  test_set_step(6);
  {
    hp->free.pages = saved_pages + 1U;
    failed = chHeapIntegrityCheck(&test_heap);
    hp->free.pages = saved_pages;
    test_assert(failed, "block beyond heap limit accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(6);
}

static const testcase_t oslib_test_008_004 = {
  "Heap block sizes",
  oslib_test_008_004_setup,
  oslib_test_008_004_teardown,
  oslib_test_008_004_execute
};

/**
 * @page oslib_test_008_005 [8.5] Heap block overlap
 *
 * <h2>Description</h2>
 * Free block extents must not overlap, while separated blocks and
 * zero-page fragments remain valid. Link-order and loop checks must be
 * preserved.
 *
 * <h2>Test Steps</h2>
 * - [8.5.1] Ascending headers whose blocks overlap by one header must
 *   be rejected even though both blocks fit individually in the heap.
 * - [8.5.2] Separated free blocks, including one ending at the heap
 *   limit, must pass the check.
 * - [8.5.3] An allocation leaving a zero-page tail fragment must pass
 *   the check, as must the heap after freeing it.
 * - [8.5.4] Self-links and backward links must still be rejected
 *   without looping.
 * .
 */

static void oslib_test_008_005_setup(void) {
  chHeapObjectInit(&test_heap, integrity_buffer,
                   sizeof (integrity_buffer));
}

static void oslib_test_008_005_teardown(void) {
  chHeapObjectDispose(&test_heap);
}

static void oslib_test_008_005_execute(void) {
  heap_header_t *hp, *next;
  heap_header_t saved;
  void *p;
  bool failed;

  hp = test_heap.header.free.next;
  next = &integrity_buffer[4];
  saved = *hp;

  /* [8.5.1] Ascending headers whose blocks overlap by one header must
     be rejected even though both blocks fit individually in the
     heap.*/
  test_set_step(1);
  {
    hp->free.pages = 4U;
    hp->free.next = next;
    next->free.pages = 3U;
    next->free.next = NULL;
    failed = chHeapIntegrityCheck(&test_heap);
    *hp = saved;
    test_assert(failed, "overlapping blocks accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(1);

  /* [8.5.2] Separated free blocks, including one ending at the heap
     limit, must pass the check.*/
  test_set_step(2);
  {
    hp->free.pages = 1U;
    hp->free.next = next;
    next->free.pages = 3U;
    next->free.next = NULL;
    failed = chHeapIntegrityCheck(&test_heap);
    *hp = saved;
    test_assert(!failed, "separated blocks rejected");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(2);

  /* [8.5.3] An allocation leaving a zero-page tail fragment must pass
     the check, as must the heap after freeing it.*/
  test_set_step(3);
  {
    p = chHeapAlloc(&test_heap, 6U * sizeof (heap_header_t));
    test_assert(p != NULL, "allocation failed");
    test_assert(test_heap.header.free.next == &integrity_buffer[7], "wrong tail header");
    test_assert(integrity_buffer[7].free.pages == 0U, "tail is not zero-page");
    failed = chHeapIntegrityCheck(&test_heap);
    chHeapFree(p);
    test_assert(!failed, "zero-page fragment rejected");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(3);

  /* [8.5.4] Self-links and backward links must still be rejected
     without looping.*/
  test_set_step(4);
  {
    hp->free.next = hp;
    failed = chHeapIntegrityCheck(&test_heap);
    *hp = saved;
    test_assert(failed, "self-link accepted");

    hp->free.pages = 0U;
    hp->free.next = next;
    next->free.pages = 0U;
    next->free.next = hp;
    failed = chHeapIntegrityCheck(&test_heap);
    *hp = saved;
    test_assert(failed, "backward link accepted");
    test_assert(!chHeapIntegrityCheck(&test_heap), "restored heap rejected");
  }
  test_end_step(4);
}

static const testcase_t oslib_test_008_005 = {
  "Heap block overlap",
  oslib_test_008_005_setup,
  oslib_test_008_005_teardown,
  oslib_test_008_005_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const oslib_test_sequence_008_array[] = {
  &oslib_test_008_001,
  &oslib_test_008_002,
  &oslib_test_008_003,
  &oslib_test_008_004,
  &oslib_test_008_005,
  NULL
};

/**
 * @brief   Memory Heaps.
 */
const testsequence_t oslib_test_sequence_008 = {
  "Memory Heaps",
  oslib_test_sequence_008_array
};

#endif /* CH_CFG_USE_HEAP == TRUE */
