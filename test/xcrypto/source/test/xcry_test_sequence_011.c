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
#include "xcry_test_root.h"

/**
 * @file    xcry_test_sequence_011.c
 * @brief   Test Sequence 011 code.
 *
 * @page xcry_test_sequence_011 [11] HKDF
 *
 * File: @ref xcry_test_sequence_011.c
 *
 * <h2>Description</h2>
 * HKDF-SHA256 key derivation with the RFC 5869 vectors.
 *
 * <h2>Test Cases</h2>
 * - @subpage xcry_test_011_001
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page xcry_test_011_001 [11.1] HKDF-SHA256 known answers
 *
 * <h2>Description</h2>
 * The input keying material is loaded as the transient key and output
 * bytes are derived.
 *
 * <h2>Test Steps</h2>
 * - [11.1.1] Checking HKDF-SHA256 support, the test is skipped if the
 *   driver does not support it.
 * - [11.1.2] Deriving, the output bytes are expected to match.
 * .
 */

static void xcry_test_011_001_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_011_001_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_011_001_execute(void) {
  cry_derivation_params_t params;
  unsigned i;
  msg_t msg;

  /* [11.1.1] Checking HKDF-SHA256 support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_HKDF_SHA256)) {
      return;
    }
  }
  test_end_step(1);

  /* [11.1.2] Deriving, the output bytes are expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_HKDF_VECTORS_COUNT; i++) {
      const xcry_kdf_vector_t *vp = &xcry_hkdf_vectors[i];

      msg = xcry_load_key(CRY_KEY_DERIVATION, vp->ikm_size, vp->ikm);
      test_assert(msg == HAL_RET_SUCCESS, "key load failed");
      params.salt = vp->salt;
      params.salt_size = vp->salt_size;
      params.info = vp->info;
      params.info_size = vp->info_size;
      msg = cryDeriveKey(XCRY_DRIVER, CRY_KEY_TRANSIENT, vp->algorithm,
                         &params, vp->size, xcry_out);
      test_assert(msg == HAL_RET_SUCCESS, "derivation failed");
      test_assert(memcmp(xcry_out, vp->okm, vp->size) == 0,
                  "output mismatch");
    }
  }
  test_end_step(2);
}

static const testcase_t xcry_test_011_001 = {
  "HKDF-SHA256 known answers",
  xcry_test_011_001_setup,
  xcry_test_011_001_teardown,
  xcry_test_011_001_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const xcry_test_sequence_011_array[] = {
  &xcry_test_011_001,
  NULL
};

/**
 * @brief   HKDF.
 */
const testsequence_t xcry_test_sequence_011 = {
  "HKDF",
  xcry_test_sequence_011_array
};
