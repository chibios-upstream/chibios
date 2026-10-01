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
 * @file    xcry_test_sequence_008.c
 * @brief   Test Sequence 008 code.
 *
 * @page xcry_test_sequence_008 [8] AES-CMAC
 *
 * File: @ref xcry_test_sequence_008.c
 *
 * <h2>Description</h2>
 * AES-CMAC with the NIST SP 800-38B vectors.
 *
 * <h2>Test Cases</h2>
 * - @subpage xcry_test_008_001
 * - @subpage xcry_test_008_002
 * - @subpage xcry_test_008_003
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

/* Returns true if the driver does not support the algorithm.*/
static bool xcry_mac_unsupported(cry_algorithm_t algorithm) {
  cry_capabilities_t caps;

  return cryGetCapabilities(XCRY_DRIVER, algorithm, &caps) != HAL_RET_SUCCESS;
}

/* Returns true if the driver does not support the vector algorithm or,
   for CMAC, its key size.*/
static bool xcry_mac_skip(const xcry_mac_vector_t *vp) {

  if (xcry_mac_unsupported(vp->algorithm)) {
    return true;
  }

  return (vp->algorithm == CRY_ALG_AES_CMAC) &&
         !xcry_key_supported(vp->algorithm, vp->key_size);
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page xcry_test_008_001 [8.1] AES-CMAC known answers
 *
 * <h2>Description</h2>
 * Each vector is authenticated and verified in single updates;
 * algorithms not supported by the driver are skipped.
 *
 * <h2>Test Steps</h2>
 * - [8.1.1] Checking AES-CMAC support, the test is skipped if the
 *   driver does not support any of the algorithms.
 * - [8.1.2] Generating tags, the tags are expected to match.
 * - [8.1.3] Verifying tags, success is expected.
 * .
 */

static void xcry_test_008_001_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_008_001_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_008_001_execute(void) {
  uint8_t tag[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [8.1.1] Checking AES-CMAC support, the test is skipped if the
     driver does not support any of the algorithms.*/
  test_set_step(1);
  {
    if (xcry_mac_unsupported(CRY_ALG_AES_CMAC)) {
      test_println("--- Skipped, algorithm not supported by the driver");
      return;
    }
  }
  test_end_step(1);

  /* [8.1.2] Generating tags, the tags are expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_CMAC_VECTORS_COUNT; i++) {
      const xcry_mac_vector_t *vp = &xcry_aes_cmac_vectors[i];

      if (xcry_mac_skip(vp)) {
        continue;
      }
      msg = xcry_mac(vp, false, tag, vp->tag_size, XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "tag generation failed");
      test_assert(memcmp(tag, vp->tag, vp->tag_size) == 0, "tag mismatch");
    }
  }
  test_end_step(2);

  /* [8.1.3] Verifying tags, success is expected.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_AES_CMAC_VECTORS_COUNT; i++) {
      const xcry_mac_vector_t *vp = &xcry_aes_cmac_vectors[i];

      if (xcry_mac_skip(vp)) {
        continue;
      }
      memcpy(tag, vp->tag, vp->tag_size);
      msg = xcry_mac(vp, true, tag, vp->tag_size, XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "verification failed");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_008_001 = {
  "AES-CMAC known answers",
  xcry_test_008_001_setup,
  xcry_test_008_001_teardown,
  xcry_test_008_001_execute
};

/**
 * @page xcry_test_008_002 [8.2] AES-CMAC fragmented
 *
 * <h2>Description</h2>
 * Messages are processed in small fragments.
 *
 * <h2>Test Steps</h2>
 * - [8.2.1] Checking AES-CMAC support, the test is skipped if the
 *   driver does not support any of the algorithms.
 * - [8.2.2] Generating tags in 7 bytes fragments, the tags are
 *   expected to match.
 * .
 */

static void xcry_test_008_002_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_008_002_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_008_002_execute(void) {
  uint8_t tag[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [8.2.1] Checking AES-CMAC support, the test is skipped if the
     driver does not support any of the algorithms.*/
  test_set_step(1);
  {
    if (xcry_mac_unsupported(CRY_ALG_AES_CMAC)) {
      test_println("--- Skipped, algorithm not supported by the driver");
      return;
    }
  }
  test_end_step(1);

  /* [8.2.2] Generating tags in 7 bytes fragments, the tags are
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_CMAC_VECTORS_COUNT; i++) {
      const xcry_mac_vector_t *vp = &xcry_aes_cmac_vectors[i];

      if (xcry_mac_skip(vp)) {
        continue;
      }
      msg = xcry_mac(vp, false, tag, vp->tag_size, 7U);
      test_assert(msg == HAL_RET_SUCCESS, "tag generation failed");
      test_assert(memcmp(tag, vp->tag, vp->tag_size) == 0, "tag mismatch");
    }
  }
  test_end_step(2);
}

static const testcase_t xcry_test_008_002 = {
  "AES-CMAC fragmented",
  xcry_test_008_002_setup,
  xcry_test_008_002_teardown,
  xcry_test_008_002_execute
};

/**
 * @page xcry_test_008_003 [8.3] AES-CMAC verification failure
 *
 * <h2>Description</h2>
 * Altered tags and tags of the wrong size are expected to fail
 * verification.
 *
 * <h2>Test Steps</h2>
 * - [8.3.1] Checking AES-CMAC support, the test is skipped if the
 *   driver does not support any of the algorithms.
 * - [8.3.2] Verifying altered tags, CRY_ERR_AUTH_FAILED is expected.
 * - [8.3.3] Verifying truncated tags, CRY_ERR_AUTH_FAILED is expected.
 * .
 */

static void xcry_test_008_003_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_008_003_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_008_003_execute(void) {
  uint8_t tag[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [8.3.1] Checking AES-CMAC support, the test is skipped if the
     driver does not support any of the algorithms.*/
  test_set_step(1);
  {
    if (xcry_mac_unsupported(CRY_ALG_AES_CMAC)) {
      test_println("--- Skipped, algorithm not supported by the driver");
      return;
    }
  }
  test_end_step(1);

  /* [8.3.2] Verifying altered tags, CRY_ERR_AUTH_FAILED is expected.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_CMAC_VECTORS_COUNT; i++) {
      const xcry_mac_vector_t *vp = &xcry_aes_cmac_vectors[i];

      if (xcry_mac_skip(vp)) {
        continue;
      }
      memcpy(tag, vp->tag, vp->tag_size);
      tag[vp->tag_size - 1U] ^= 0x01U;
      msg = xcry_mac(vp, true, tag, vp->tag_size, XCRY_BUFFER_SIZE);
      test_assert(msg == CRY_ERR_AUTH_FAILED, "altered tag accepted");
    }
  }
  test_end_step(2);

  /* [8.3.3] Verifying truncated tags, CRY_ERR_AUTH_FAILED is
     expected.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_AES_CMAC_VECTORS_COUNT; i++) {
      const xcry_mac_vector_t *vp = &xcry_aes_cmac_vectors[i];

      if (xcry_mac_skip(vp)) {
        continue;
      }
      memcpy(tag, vp->tag, vp->tag_size);
      msg = xcry_mac(vp, true, tag, vp->tag_size - 1U, XCRY_BUFFER_SIZE);
      test_assert(msg == CRY_ERR_AUTH_FAILED, "truncated tag accepted");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_008_003 = {
  "AES-CMAC verification failure",
  xcry_test_008_003_setup,
  xcry_test_008_003_teardown,
  xcry_test_008_003_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const xcry_test_sequence_008_array[] = {
  &xcry_test_008_001,
  &xcry_test_008_002,
  &xcry_test_008_003,
  NULL
};

/**
 * @brief   AES-CMAC.
 */
const testsequence_t xcry_test_sequence_008 = {
  "AES-CMAC",
  xcry_test_sequence_008_array
};
