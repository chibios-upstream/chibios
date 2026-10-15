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
 * @file    xcry_test_sequence_003.c
 * @brief   Test Sequence 003 code.
 *
 * @page xcry_test_sequence_003 [3] AES-CBC
 *
 * File: @ref xcry_test_sequence_003.c
 *
 * <h2>Description</h2>
 * AES-CBC with the NIST SP 800-38A vectors and a long message.
 *
 * <h2>Test Cases</h2>
 * - @subpage xcry_test_003_001
 * - @subpage xcry_test_003_002
 * - @subpage xcry_test_003_003
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
 * @page xcry_test_003_001 [3.1] AES-CBC known answers
 *
 * <h2>Description</h2>
 * Each vector is encrypted and decrypted in a single update.
 *
 * <h2>Test Steps</h2>
 * - [3.1.1] Checking AES-CBC support, the test is skipped if the
 *   driver does not support it.
 * - [3.1.2] Encrypting, the ciphertext is expected to match.
 * - [3.1.3] Decrypting, the plaintext is expected to match.
 * .
 */

static void xcry_test_003_001_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_003_001_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_003_001_execute(void) {
  unsigned i;
  msg_t msg;

  /* [3.1.1] Checking AES-CBC support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_CBC)) {
      return;
    }
  }
  test_end_step(1);

  /* [3.1.2] Encrypting, the ciphertext is expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_CBC_VECTORS_COUNT; i++) {
      const xcry_cipher_vector_t *vp = &xcry_aes_cbc_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      msg = xcry_cipher(vp, CRY_ENCRYPT, vp->pt, xcry_out, vp->size);
      test_assert(msg == HAL_RET_SUCCESS, "encryption failed");
      test_assert(memcmp(xcry_out, vp->ct, vp->size) == 0,
                  "ciphertext mismatch");
    }
  }
  test_end_step(2);

  /* [3.1.3] Decrypting, the plaintext is expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_AES_CBC_VECTORS_COUNT; i++) {
      const xcry_cipher_vector_t *vp = &xcry_aes_cbc_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      msg = xcry_cipher(vp, CRY_DECRYPT, vp->ct, xcry_out, vp->size);
      test_assert(msg == HAL_RET_SUCCESS, "decryption failed");
      test_assert(memcmp(xcry_out, vp->pt, vp->size) == 0,
                  "plaintext mismatch");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_003_001 = {
  "AES-CBC known answers",
  xcry_test_003_001_setup,
  xcry_test_003_001_teardown,
  xcry_test_003_001_execute
};

/**
 * @page xcry_test_003_002 [3.2] AES-CBC fragmented
 *
 * <h2>Description</h2>
 * Each vector is processed in fragments not aligned to the block size,
 * and byte by byte.
 *
 * <h2>Test Steps</h2>
 * - [3.2.1] Checking AES-CBC support, the test is skipped if the
 *   driver does not support it.
 * - [3.2.2] Encrypting in 5 bytes fragments, the ciphertext is
 *   expected to match.
 * - [3.2.3] Decrypting byte by byte, the plaintext is expected to
 *   match.
 * .
 */

static void xcry_test_003_002_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_003_002_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_003_002_execute(void) {
  unsigned i;
  msg_t msg;

  /* [3.2.1] Checking AES-CBC support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_CBC)) {
      return;
    }
  }
  test_end_step(1);

  /* [3.2.2] Encrypting in 5 bytes fragments, the ciphertext is
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_AES_CBC_VECTORS_COUNT; i++) {
      const xcry_cipher_vector_t *vp = &xcry_aes_cbc_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      msg = xcry_cipher(vp, CRY_ENCRYPT, vp->pt, xcry_out, 5U);
      test_assert(msg == HAL_RET_SUCCESS, "encryption failed");
      test_assert(memcmp(xcry_out, vp->ct, vp->size) == 0,
                  "ciphertext mismatch");
    }
  }
  test_end_step(2);

  /* [3.2.3] Decrypting byte by byte, the plaintext is expected to
     match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_AES_CBC_VECTORS_COUNT; i++) {
      const xcry_cipher_vector_t *vp = &xcry_aes_cbc_vectors[i];

      if (!xcry_key_supported(vp->algorithm, vp->key_size)) {
        continue;
      }
      msg = xcry_cipher(vp, CRY_DECRYPT, vp->ct, xcry_out, 1U);
      test_assert(msg == HAL_RET_SUCCESS, "decryption failed");
      test_assert(memcmp(xcry_out, vp->pt, vp->size) == 0,
                  "plaintext mismatch");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_003_002 = {
  "AES-CBC fragmented",
  xcry_test_003_002_setup,
  xcry_test_003_002_teardown,
  xcry_test_003_002_execute
};

/**
 * @page xcry_test_003_003 [3.3] AES-CBC long message
 *
 * <h2>Description</h2>
 * A long message in shared buffers is processed in a single update,
 * backends may use DMA, then in fragments.
 *
 * <h2>Test Steps</h2>
 * - [3.3.1] Checking AES-CBC support, the test is skipped if the
 *   driver does not support it.
 * - [3.3.2] Encrypting in a single update, the ciphertext is expected
 *   to match.
 * - [3.3.3] Decrypting in 1000 bytes fragments, the plaintext is
 *   expected to match.
 * .
 */

static void xcry_test_003_003_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_003_003_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_003_003_execute(void) {
  const xcry_cipher_vector_t *vp = &xcry_aes_cbc_long[0];
  msg_t msg;

  /* [3.3.1] Checking AES-CBC support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_AES_CBC)) {
      return;
    }
  }
  test_end_step(1);

  /* [3.3.2] Encrypting in a single update, the ciphertext is expected
     to match.*/
  test_set_step(2);
  {
    memcpy(xcry_in, vp->pt, vp->size);
    msg = xcry_cipher(vp, CRY_ENCRYPT, xcry_in, xcry_out, vp->size);
    test_assert(msg == HAL_RET_SUCCESS, "encryption failed");
    test_assert(memcmp(xcry_out, vp->ct, vp->size) == 0,
                "ciphertext mismatch");
  }
  test_end_step(2);

  /* [3.3.3] Decrypting in 1000 bytes fragments, the plaintext is
     expected to match.*/
  test_set_step(3);
  {
    memcpy(xcry_in, vp->ct, vp->size);
    msg = xcry_cipher(vp, CRY_DECRYPT, xcry_in, xcry_out, 1000U);
    test_assert(msg == HAL_RET_SUCCESS, "decryption failed");
    test_assert(memcmp(xcry_out, vp->pt, vp->size) == 0,
                "plaintext mismatch");
  }
  test_end_step(3);
}

static const testcase_t xcry_test_003_003 = {
  "AES-CBC long message",
  xcry_test_003_003_setup,
  xcry_test_003_003_teardown,
  xcry_test_003_003_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const xcry_test_sequence_003_array[] = {
  &xcry_test_003_001,
  &xcry_test_003_002,
  &xcry_test_003_003,
  NULL
};

/**
 * @brief   AES-CBC.
 */
const testsequence_t xcry_test_sequence_003 = {
  "AES-CBC",
  xcry_test_sequence_003_array
};
