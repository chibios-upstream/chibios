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
 * @file    xcry_test_sequence_006.c
 * @brief   Test Sequence 006 code.
 *
 * @page xcry_test_sequence_006 [6] AES-GCM
 *
 * File: @ref xcry_test_sequence_006.c
 *
 * <h2>Description</h2>
 * AES-GCM with the GCM specification and NIST CAVP vectors.
 *
 * <h2>Test Cases</h2>
 * - @subpage xcry_test_006_001
 * - @subpage xcry_test_006_002
 * - @subpage xcry_test_006_003
 * - @subpage xcry_test_006_004
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
 * @page xcry_test_006_001 [6.1] AES-GCM known answers
 *
 * <h2>Description</h2>
 * Each vector is encrypted and decrypted with verification in single
 * updates.
 *
 * <h2>Test Steps</h2>
 * - [6.1.1] Checking AES-GCM support, the test is skipped if the
 *   driver does not support it.
 * - [6.1.2] Encrypting, the ciphertext and the tag are expected to
 *   match.
 * - [6.1.3] Decrypting with verification, the plaintext is expected to
 *   match.
 * .
 */

static void xcry_test_006_001_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_006_001_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_006_001_execute(void) {
  uint8_t tag[16];
  unsigned i;
  msg_t msg;

  /* [6.1.1] Checking AES-GCM support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_GCM)) {
      return;
    }
  }
  test_end_step(1);

  /* [6.1.2] Encrypting, the ciphertext and the tag are expected to
     match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_GCM_VECTORS_COUNT; i++) {
      const xcry_aead_vector_t *vp = &xcry_aes_gcm_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      msg = xcry_aead(vp, CRY_ENCRYPT, vp->pt, xcry_out, tag, vp->tag_size,
                      XCRY_BUFFER_SIZE, true);
      test_assert(msg == HAL_RET_SUCCESS, "encryption failed");
      test_assert(memcmp(xcry_out, vp->ct, vp->size) == 0,
                  "ciphertext mismatch");
      test_assert(memcmp(tag, vp->tag, vp->tag_size) == 0, "tag mismatch");
    }
  }
  test_end_step(2);

  /* [6.1.3] Decrypting with verification, the plaintext is expected to
     match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_AES_GCM_VECTORS_COUNT; i++) {
      const xcry_aead_vector_t *vp = &xcry_aes_gcm_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      memcpy(tag, vp->tag, vp->tag_size);
      msg = xcry_aead(vp, CRY_DECRYPT, vp->ct, xcry_out, tag, vp->tag_size,
                      XCRY_BUFFER_SIZE, true);
      test_assert(msg == HAL_RET_SUCCESS, "verification failed");
      test_assert(memcmp(xcry_out, vp->pt, vp->size) == 0,
                  "plaintext mismatch");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_006_001 = {
  "AES-GCM known answers",
  xcry_test_006_001_setup,
  xcry_test_006_001_teardown,
  xcry_test_006_001_execute
};

/**
 * @page xcry_test_006_002 [6.2] AES-GCM fragmented
 *
 * <h2>Description</h2>
 * Associated data and payload are processed in small fragments,
 * without declared lengths.
 *
 * <h2>Test Steps</h2>
 * - [6.2.1] Checking AES-GCM support, the test is skipped if the
 *   driver does not support it.
 * - [6.2.2] Encrypting in 5 bytes fragments, the ciphertext and the
 *   tag are expected to match.
 * - [6.2.3] Decrypting byte by byte with verification, the plaintext
 *   is expected to match.
 * .
 */

static void xcry_test_006_002_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_006_002_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_006_002_execute(void) {
  uint8_t tag[16];
  unsigned i;
  msg_t msg;

  /* [6.2.1] Checking AES-GCM support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_GCM)) {
      return;
    }
  }
  test_end_step(1);

  /* [6.2.2] Encrypting in 5 bytes fragments, the ciphertext and the
     tag are expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_GCM_VECTORS_COUNT; i++) {
      const xcry_aead_vector_t *vp = &xcry_aes_gcm_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      msg = xcry_aead(vp, CRY_ENCRYPT, vp->pt, xcry_out, tag, vp->tag_size,
                      5U, false);
      test_assert(msg == HAL_RET_SUCCESS, "encryption failed");
      test_assert(memcmp(xcry_out, vp->ct, vp->size) == 0,
                  "ciphertext mismatch");
      test_assert(memcmp(tag, vp->tag, vp->tag_size) == 0, "tag mismatch");
    }
  }
  test_end_step(2);

  /* [6.2.3] Decrypting byte by byte with verification, the plaintext
     is expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_AES_GCM_VECTORS_COUNT; i++) {
      const xcry_aead_vector_t *vp = &xcry_aes_gcm_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      memcpy(tag, vp->tag, vp->tag_size);
      msg = xcry_aead(vp, CRY_DECRYPT, vp->ct, xcry_out, tag, vp->tag_size,
                      1U, true);
      test_assert(msg == HAL_RET_SUCCESS, "verification failed");
      test_assert(memcmp(xcry_out, vp->pt, vp->size) == 0,
                  "plaintext mismatch");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_006_002 = {
  "AES-GCM fragmented",
  xcry_test_006_002_setup,
  xcry_test_006_002_teardown,
  xcry_test_006_002_execute
};

/**
 * @page xcry_test_006_003 [6.3] AES-GCM authentication failure
 *
 * <h2>Description</h2>
 * Altered tags, a tag of the wrong size and altered ciphertext are
 * expected to fail verification.
 *
 * <h2>Test Steps</h2>
 * - [6.3.1] Checking AES-GCM support, the test is skipped if the
 *   driver does not support it.
 * - [6.3.2] Verifying an altered tag, CRY_ERR_AUTH_FAILED is expected.
 * - [6.3.3] Verifying a truncated tag, CRY_ERR_AUTH_FAILED is
 *   expected.
 * - [6.3.4] Verifying altered ciphertext, CRY_ERR_AUTH_FAILED is
 *   expected.
 * .
 */

static void xcry_test_006_003_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_006_003_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_006_003_execute(void) {
  const xcry_aead_vector_t *vp = &xcry_aes_gcm_vectors[0];
  uint8_t tag[16];
  msg_t msg;

  /* [6.3.1] Checking AES-GCM support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_GCM)) {
      return;
    }
  }
  test_end_step(1);

  /* [6.3.2] Verifying an altered tag, CRY_ERR_AUTH_FAILED is
     expected.*/
  test_set_step(2);
  {
    memcpy(tag, vp->tag, vp->tag_size);
    tag[0] ^= 0x01U;
    msg = xcry_aead(vp, CRY_DECRYPT, vp->ct, xcry_out, tag, vp->tag_size,
                    XCRY_BUFFER_SIZE, true);
    test_assert(msg == CRY_ERR_AUTH_FAILED, "altered tag accepted");
  }
  test_end_step(2);

  /* [6.3.3] Verifying a truncated tag, CRY_ERR_AUTH_FAILED is
     expected.*/
  test_set_step(3);
  {
    memcpy(tag, vp->tag, vp->tag_size);
    msg = xcry_aead(vp, CRY_DECRYPT, vp->ct, xcry_out, tag, vp->tag_size - 1U,
                    XCRY_BUFFER_SIZE, true);
    test_assert(msg == CRY_ERR_AUTH_FAILED, "truncated tag accepted");
  }
  test_end_step(3);

  /* [6.3.4] Verifying altered ciphertext, CRY_ERR_AUTH_FAILED is
     expected.*/
  test_set_step(4);
  {
    memcpy(xcry_in, vp->ct, vp->size);
    xcry_in[vp->size - 1U] ^= 0x80U;
    memcpy(tag, vp->tag, vp->tag_size);
    msg = xcry_aead(vp, CRY_DECRYPT, xcry_in, xcry_out, tag, vp->tag_size,
                    XCRY_BUFFER_SIZE, true);
    test_assert(msg == CRY_ERR_AUTH_FAILED, "altered ciphertext accepted");
  }
  test_end_step(4);
}

static const testcase_t xcry_test_006_003 = {
  "AES-GCM authentication failure",
  xcry_test_006_003_setup,
  xcry_test_006_003_teardown,
  xcry_test_006_003_execute
};

/**
 * @page xcry_test_006_004 [6.4] AES-GCM long message
 *
 * <h2>Description</h2>
 * A long message in shared buffers is processed in a single update,
 * backends may use DMA.
 *
 * <h2>Test Steps</h2>
 * - [6.4.1] Checking AES-GCM support, the test is skipped if the
 *   driver does not support it.
 * - [6.4.2] Encrypting, the ciphertext and the tag are expected to
 *   match.
 * - [6.4.3] Decrypting with verification, the plaintext is expected to
 *   match.
 * .
 */

static void xcry_test_006_004_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_006_004_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_006_004_execute(void) {
  const xcry_aead_vector_t *vp = &xcry_aes_gcm_long[0];
  uint8_t tag[16];
  msg_t msg;

  /* [6.4.1] Checking AES-GCM support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_GCM)) {
      return;
    }
  }
  test_end_step(1);

  /* [6.4.2] Encrypting, the ciphertext and the tag are expected to
     match.*/
  test_set_step(2);
  {
    memcpy(xcry_in, vp->pt, vp->size);
    msg = xcry_aead(vp, CRY_ENCRYPT, xcry_in, xcry_out, tag, vp->tag_size,
                    XCRY_BUFFER_SIZE, true);
    test_assert(msg == HAL_RET_SUCCESS, "encryption failed");
    test_assert(memcmp(xcry_out, vp->ct, vp->size) == 0,
                "ciphertext mismatch");
    test_assert(memcmp(tag, vp->tag, vp->tag_size) == 0, "tag mismatch");
  }
  test_end_step(2);

  /* [6.4.3] Decrypting with verification, the plaintext is expected to
     match.*/
  test_set_step(3);
  {
    memcpy(xcry_in, vp->ct, vp->size);
    memcpy(tag, vp->tag, vp->tag_size);
    msg = xcry_aead(vp, CRY_DECRYPT, xcry_in, xcry_out, tag, vp->tag_size,
                    XCRY_BUFFER_SIZE, true);
    test_assert(msg == HAL_RET_SUCCESS, "verification failed");
    test_assert(memcmp(xcry_out, vp->pt, vp->size) == 0,
                "plaintext mismatch");
  }
  test_end_step(3);
}

static const testcase_t xcry_test_006_004 = {
  "AES-GCM long message",
  xcry_test_006_004_setup,
  xcry_test_006_004_teardown,
  xcry_test_006_004_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const xcry_test_sequence_006_array[] = {
  &xcry_test_006_001,
  &xcry_test_006_002,
  &xcry_test_006_003,
  &xcry_test_006_004,
  NULL
};

/**
 * @brief   AES-GCM.
 */
const testsequence_t xcry_test_sequence_006 = {
  "AES-GCM",
  xcry_test_sequence_006_array
};
