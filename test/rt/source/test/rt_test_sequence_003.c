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
 * @file    rt_test_sequence_003.c
 * @brief   Test Sequence 003 code.
 *
 * @page rt_test_sequence_003 [3] Time and Intervals Functionality
 *
 * File: @ref rt_test_sequence_003.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS/RT functionalities related to time
 * and intervals management.
 *
 * <h2>Test Cases</h2>
 * - @subpage rt_test_003_001
 * - @subpage rt_test_003_002
 * - @subpage rt_test_003_003
 * .
 */

/****************************************************************************
 * Shared code.
 ****************************************************************************/

#include "ch.h"

#if CH_CFG_USE_TM || defined(__DOXYGEN__)
static time_measurement_t tm1, tm2;
#endif

/****************************************************************************
 * Test cases.
 ****************************************************************************/

/**
 * @page rt_test_003_001 [3.1] System Tick Counter functionality
 *
 * <h2>Description</h2>
 * The functionality of the API @p chVTGetSystemTimeX() is tested.
 *
 * <h2>Test Steps</h2>
 * - [3.1.1] A System Tick Counter increment is expected, the test
 *   simply hangs if it does not happen.
 * .
 */

static void rt_test_003_001_execute(void) {

  /* [3.1.1] A System Tick Counter increment is expected, the test
     simply hangs if it does not happen.*/
  test_set_step(1);
  {
    systime_t time = chVTGetSystemTimeX();
    while (time == chVTGetSystemTimeX()) {
#if defined(SIMULATOR)
        _sim_check_for_interrupts();
#endif
    }
  }
  test_end_step(1);
}

static const testcase_t rt_test_003_001 = {
  "System Tick Counter functionality",
  NULL,
  NULL,
  rt_test_003_001_execute
};

/**
 * @page rt_test_003_002 [3.2] Time ranges functionality
 *
 * <h2>Description</h2>
 * The functionality of the API @p chTimeIsInRangeX() is tested.
 *
 * <h2>Test Steps</h2>
 * - [3.2.1] Checking case where start == end, it must always evaluate
 *   as not in range.
 * - [3.2.2] Checking boundaries for start < end.
 * - [3.2.3] Checking boundaries for start > end.
 * .
 */

static void rt_test_003_002_execute(void) {

  /* [3.2.1] Checking case where start == end, it must always evaluate
     as not in range.*/
  test_set_step(1);
  {
    bool b;

    b = chTimeIsInRangeX((systime_t)0, (systime_t)0, (systime_t)0);
    test_assert(b == false, "in range");
    b = chTimeIsInRangeX((systime_t)-1, (systime_t)0, (systime_t)0);
    test_assert(b == false, "in range");
    b = chTimeIsInRangeX((systime_t)0, (systime_t)-1, (systime_t)-1);
    test_assert(b == false, "in range");
    b = chTimeIsInRangeX((systime_t)-1, (systime_t)-1, (systime_t)-1);
    test_assert(b == false, "in range");
  }
  test_end_step(1);

  /* [3.2.2] Checking boundaries for start < end.*/
  test_set_step(2);
  {
    bool b;

    b = chTimeIsInRangeX((systime_t)10, (systime_t)10, (systime_t)100);
    test_assert(b == true, "not in range");
    b = chTimeIsInRangeX((systime_t)9, (systime_t)10, (systime_t)100);
    test_assert(b == false, "in range");
    b = chTimeIsInRangeX((systime_t)99, (systime_t)10, (systime_t)100);
    test_assert(b == true, "not in range");
    b = chTimeIsInRangeX((systime_t)100, (systime_t)10, (systime_t)100);
    test_assert(b == false, "in range");
  }
  test_end_step(2);

  /* [3.2.3] Checking boundaries for start > end.*/
  test_set_step(3);
  {
    bool b;

    b = chTimeIsInRangeX((systime_t)100, (systime_t)100, (systime_t)10);
    test_assert(b == true, "not in range");
    b = chTimeIsInRangeX((systime_t)99, (systime_t)100, (systime_t)10);
    test_assert(b == false, "in range");
    b = chTimeIsInRangeX((systime_t)9, (systime_t)100, (systime_t)10);
    test_assert(b == true, "not in range");
    b = chTimeIsInRangeX((systime_t)10, (systime_t)100, (systime_t)10);
    test_assert(b == false, "in range");
  }
  test_end_step(3);
}

static const testcase_t rt_test_003_002 = {
  "Time ranges functionality",
  NULL,
  NULL,
  rt_test_003_002_execute
};

#if (CH_CFG_USE_TM == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_003_003 [3.3] Time Measurement functionality
 *
 * <h2>Description</h2>
 * The functionality of Time Measurement objects is tested.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_TM == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.3.1] The initialized measurement objects are verified.
 * - [3.3.2] A measurement is performed and its result is verified.
 * - [3.3.3] The first measurement is chained to the second one and
 *   both objects are verified.
 * .
 */

static void rt_test_003_003_setup(void) {
  chTMObjectInit(&tm1);
  chTMObjectInit(&tm2);
}

static void rt_test_003_003_teardown(void) {
  chTMObjectDispose(&tm1);
  chTMObjectDispose(&tm2);
}

static void rt_test_003_003_execute(void) {
  rtcnt_t first;

  /* [3.3.1] The initialized measurement objects are verified.*/
  test_set_step(1);
  {
    test_assert(tm1.best == (rtcnt_t)-1, "invalid best");
    test_assert(tm1.worst == (rtcnt_t)0, "invalid worst");
    test_assert(tm1.last == (rtcnt_t)0, "invalid last");
    test_assert(tm1.n == (ucnt_t)0, "invalid counter");
    test_assert(tm1.cumulative == (rttime_t)0, "invalid cumulative");
  }
  test_end_step(1);

  /* [3.3.2] A measurement is performed and its result is verified.*/
  test_set_step(2);
  {
    chTMStartMeasurementX(&tm1);
    chSysPolledDelayX((rtcnt_t)100);
    chTMStopMeasurementX(&tm1);

    test_assert(tm1.n == (ucnt_t)1, "invalid counter");
    test_assert(tm1.last > (rtcnt_t)0, "invalid last");
    test_assert(tm1.best == tm1.last, "invalid best");
    test_assert(tm1.worst == tm1.last, "invalid worst");
    test_assert(tm1.cumulative == (rttime_t)tm1.last, "invalid cumulative");
    first = tm1.last;
  }
  test_end_step(2);

  /* [3.3.3] The first measurement is chained to the second one and
     both objects are verified.*/
  test_set_step(3);
  {
    chTMChainMeasurementToX(&tm1, &tm2);
    chSysPolledDelayX((rtcnt_t)10);
    chTMStopMeasurementX(&tm2);

    test_assert(tm1.n == (ucnt_t)2, "invalid counter");
    test_assert(tm1.last > (rtcnt_t)0, "invalid last");
    test_assert(tm1.best <= tm1.worst, "invalid range");
    test_assert(tm1.cumulative >= (rttime_t)first, "invalid cumulative");
    test_assert(tm2.n == (ucnt_t)1, "invalid counter");
    test_assert(tm2.last > (rtcnt_t)0, "invalid last");
    test_assert(tm2.best == tm2.last, "invalid best");
    test_assert(tm2.worst == tm2.last, "invalid worst");
  }
  test_end_step(3);
}

static const testcase_t rt_test_003_003 = {
  "Time Measurement functionality",
  rt_test_003_003_setup,
  rt_test_003_003_teardown,
  rt_test_003_003_execute
};
#endif /* CH_CFG_USE_TM == TRUE */

/****************************************************************************
 * Exported data.
 ****************************************************************************/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const rt_test_sequence_003_array[] = {
  &rt_test_003_001,
  &rt_test_003_002,
#if (CH_CFG_USE_TM == TRUE) || defined(__DOXYGEN__)
  &rt_test_003_003,
#endif
  NULL
};

/**
 * @brief   Time and Intervals Functionality.
 */
const testsequence_t rt_test_sequence_003 = {
  "Time and Intervals Functionality",
  rt_test_sequence_003_array
};
