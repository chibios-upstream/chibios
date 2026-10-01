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
 * @file    xcry_test_sequence_001.c
 * @brief   Test Sequence 001 code.
 *
 * @page xcry_test_sequence_001 [1] Driver and API
 *
 * File: @ref xcry_test_sequence_001.c
 *
 * <h2>Description</h2>
 * Driver lifecycle, capabilities, transient key handling, stream
 * lifecycle and concurrency, independent of the algorithm.
 *
 * <h2>Test Cases</h2>
 * - @subpage xcry_test_001_001
 * - @subpage xcry_test_001_002
 * - @subpage xcry_test_001_003
 * - @subpage xcry_test_001_004
 * - @subpage xcry_test_001_005
 * - @subpage xcry_test_001_006
 * - @subpage xcry_test_001_007
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
 * @page xcry_test_001_001 [1.1] Driver lifecycle
 *
 * <h2>Description</h2>
 * The driver is started, stopped and restarted.
 *
 * <h2>Test Steps</h2>
 * - [1.1.1] Starting the driver, READY state is expected.
 * - [1.1.2] Stopping the driver, STOP state is expected.
 * - [1.1.3] Restarting the driver, READY state is expected.
 * .
 */

static void xcry_test_001_001_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_001_execute(void) {
  msg_t msg;

  /* [1.1.1] Starting the driver, READY state is expected.*/
  test_set_step(1);
  {
    msg = drvStart(XCRY_DRIVER, NULL);
    test_assert(msg == HAL_RET_SUCCESS, "start failed");
    test_assert(drvGetStateX(XCRY_DRIVER) == HAL_DRV_STATE_READY,
                "not ready");
  }
  test_end_step(1);

  /* [1.1.2] Stopping the driver, STOP state is expected.*/
  test_set_step(2);
  {
    drvStop(XCRY_DRIVER);
    test_assert(drvGetStateX(XCRY_DRIVER) == HAL_DRV_STATE_STOP,
                "not stopped");
  }
  test_end_step(2);

  /* [1.1.3] Restarting the driver, READY state is expected.*/
  test_set_step(3);
  {
    msg = drvStart(XCRY_DRIVER, NULL);
    test_assert(msg == HAL_RET_SUCCESS, "restart failed");
    test_assert(drvGetStateX(XCRY_DRIVER) == HAL_DRV_STATE_READY,
                "not ready");
  }
  test_end_step(3);
}

static const testcase_t xcry_test_001_001 = {
  "Driver lifecycle",
  NULL,
  xcry_test_001_001_teardown,
  xcry_test_001_001_execute
};

/**
 * @page xcry_test_001_002 [1.2] Capabilities report
 *
 * <h2>Description</h2>
 * Every algorithm is queried; supported algorithms report capability
 * flags, unsupported ones report CRY_ERR_UNSUPPORTED with cleared
 * capabilities.
 *
 * <h2>Test Steps</h2>
 * - [1.2.1] Querying all algorithms and printing the supported ones.
 * .
 */

static void xcry_test_001_002_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_001_002_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_002_execute(void) {
  cry_capabilities_t caps;
  cry_algorithm_t alg;
  unsigned n;
  msg_t msg;

  /* [1.2.1] Querying all algorithms and printing the supported ones.*/
  test_set_step(1);
  {
    n = 0U;
    for (alg = CRY_ALG_AES_ECB; alg <= CRY_ALG_HKDF_SHA512; alg++) {
      msg = cryGetCapabilities(XCRY_DRIVER, alg, &caps);
      if (msg == HAL_RET_SUCCESS) {
        test_assert(caps.flags != 0U, "no capability flags");
        test_assert(caps.min_key_bits <= caps.max_key_bits,
                    "invalid key size range");
        test_printf("--- Algorithm %u supported, flags 0x%x, key bits %u..%u"
                    TEST_CFG_EOL_STRING, (unsigned)alg, (unsigned)caps.flags,
                    (unsigned)caps.min_key_bits, (unsigned)caps.max_key_bits);
        n++;
      }
      else {
        test_assert(msg == CRY_ERR_UNSUPPORTED, "unexpected error");
        test_assert((caps.flags == 0U) && (caps.min_key_bits == 0U) &&
                    (caps.max_key_bits == 0U), "capabilities not cleared");
      }
    }
    test_printf("--- %u algorithms supported" TEST_CFG_EOL_STRING, n);
  }
  test_end_step(1);
}

static const testcase_t xcry_test_001_002 = {
  "Capabilities report",
  xcry_test_001_002_setup,
  xcry_test_001_002_teardown,
  xcry_test_001_002_execute
};

/**
 * @page xcry_test_001_003 [1.3] Transient key
 *
 * <h2>Description</h2>
 * Loading the transient key, invalid key parameters, a missing key and
 * loading a key held by a stream.
 *
 * <h2>Test Steps</h2>
 * - [1.3.1] Selecting a supported AES cipher, the test is skipped if
 *   there is none.
 * - [1.3.2] Beginning a stream before any key is loaded, CRY_ERR_KEY
 *   is expected.
 * - [1.3.3] Loading an AES key with an invalid size, CRY_ERR_ARGUMENT
 *   is expected.
 * - [1.3.4] Loading an AES-128 key and beginning a stream, success is
 *   expected.
 * - [1.3.5] Loading the key while the stream holds it, CRY_ERR_BUSY is
 *   expected.
 * - [1.3.6] Aborting the stream then loading the key again, success is
 *   expected.
 * .
 */

static void xcry_test_001_003_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_001_003_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_003_execute(void) {
  static const uint8_t key[16] = {0};
  static const uint8_t iv[16] = {0};
  cry_cipher_params_t params;
  cry_operation_t op;
  cry_algorithm_t alg;
  unsigned i;
  msg_t msg;

  /* [1.3.1] Selecting a supported AES cipher, the test is skipped if
     there is none.*/
  test_set_step(1);
  {
    alg = CRY_ALG_NONE;
    for (i = 0U; i < 4U; i++) {
      static const cry_algorithm_t algs[4] = {
        CRY_ALG_AES_ECB, CRY_ALG_AES_CBC, CRY_ALG_AES_CFB128, CRY_ALG_AES_CTR
      };
      cry_capabilities_t caps;

      if (cryGetCapabilities(XCRY_DRIVER, algs[i], &caps) == HAL_RET_SUCCESS) {
        alg = algs[i];
        break;
      }
    }
    if (alg == CRY_ALG_NONE) {
      test_println("--- Skipped, no AES cipher supported by the driver");
      return;
    }
    params.iv = alg == CRY_ALG_AES_ECB ? NULL : iv;
    params.iv_size = alg == CRY_ALG_AES_ECB ? 0U : sizeof iv;
  }
  test_end_step(1);

  /* [1.3.2] Beginning a stream before any key is loaded, CRY_ERR_KEY
     is expected.*/
  test_set_step(2);
  {
    msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                         CRY_ENCRYPT, &params);
    test_assert(msg == CRY_ERR_KEY, "missing key not detected");
  }
  test_end_step(2);

  /* [1.3.3] Loading an AES key with an invalid size, CRY_ERR_ARGUMENT
     is expected.*/
  test_set_step(3);
  {
    msg = xcry_load_key(CRY_KEY_AES, 12U, key);
    test_assert(msg == CRY_ERR_ARGUMENT, "invalid key accepted");
  }
  test_end_step(3);

  /* [1.3.4] Loading an AES-128 key and beginning a stream, success is
     expected.*/
  test_set_step(4);
  {
    msg = xcry_load_key(CRY_KEY_AES, sizeof key, key);
    test_assert(msg == HAL_RET_SUCCESS, "key load failed");
    msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                         CRY_ENCRYPT, &params);
    test_assert(msg == HAL_RET_SUCCESS, "begin failed");
  }
  test_end_step(4);

  /* [1.3.5] Loading the key while the stream holds it, CRY_ERR_BUSY is
     expected.*/
  test_set_step(5);
  {
    msg = xcry_load_key(CRY_KEY_AES, sizeof key, key);
    test_assert(msg == CRY_ERR_BUSY, "held key replaced");
  }
  test_end_step(5);

  /* [1.3.6] Aborting the stream then loading the key again, success is
     expected.*/
  test_set_step(6);
  {
    cryOperationAbort(&op);
    msg = xcry_load_key(CRY_KEY_AES, sizeof key, key);
    test_assert(msg == HAL_RET_SUCCESS, "key still held");
  }
  test_end_step(6);
}

static const testcase_t xcry_test_001_003 = {
  "Transient key",
  xcry_test_001_003_setup,
  xcry_test_001_003_teardown,
  xcry_test_001_003_execute
};

/**
 * @page xcry_test_001_004 [1.4] Stop invalidates streams
 *
 * <h2>Description</h2>
 * A stream open across drvStop() is invalidated and the transient key
 * is erased.
 *
 * <h2>Test Steps</h2>
 * - [1.4.1] Selecting a supported AES cipher, the test is skipped if
 *   there is none.
 * - [1.4.2] Loading a key and beginning a stream, success is expected.
 * - [1.4.3] Restarting the driver with the stream open, aborting the
 *   invalidated stream is expected to be harmless.
 * - [1.4.4] Beginning a stream on the erased transient key,
 *   CRY_ERR_KEY is expected.
 * - [1.4.5] Loading the key again, a new stream is expected to begin.
 * .
 */

static void xcry_test_001_004_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_001_004_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_004_execute(void) {
  static const uint8_t key[16] = {0};
  static const uint8_t iv[16] = {0};
  cry_cipher_params_t params;
  cry_operation_t op;
  cry_algorithm_t alg;
  unsigned i;
  msg_t msg;

  /* [1.4.1] Selecting a supported AES cipher, the test is skipped if
     there is none.*/
  test_set_step(1);
  {
    alg = CRY_ALG_NONE;
    for (i = 0U; i < 4U; i++) {
      static const cry_algorithm_t algs[4] = {
        CRY_ALG_AES_ECB, CRY_ALG_AES_CBC, CRY_ALG_AES_CFB128, CRY_ALG_AES_CTR
      };
      cry_capabilities_t caps;

      if (cryGetCapabilities(XCRY_DRIVER, algs[i], &caps) == HAL_RET_SUCCESS) {
        alg = algs[i];
        break;
      }
    }
    if (alg == CRY_ALG_NONE) {
      test_println("--- Skipped, no AES cipher supported by the driver");
      return;
    }
    params.iv = alg == CRY_ALG_AES_ECB ? NULL : iv;
    params.iv_size = alg == CRY_ALG_AES_ECB ? 0U : sizeof iv;
  }
  test_end_step(1);

  /* [1.4.2] Loading a key and beginning a stream, success is
     expected.*/
  test_set_step(2);
  {
    msg = xcry_load_key(CRY_KEY_AES, sizeof key, key);
    test_assert(msg == HAL_RET_SUCCESS, "key load failed");
    msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                         CRY_ENCRYPT, &params);
    test_assert(msg == HAL_RET_SUCCESS, "begin failed");
  }
  test_end_step(2);

  /* [1.4.3] Restarting the driver with the stream open, aborting the
     invalidated stream is expected to be harmless.*/
  test_set_step(3);
  {
    drvStop(XCRY_DRIVER);
    msg = drvStart(XCRY_DRIVER, NULL);
    test_assert(msg == HAL_RET_SUCCESS, "restart failed");
    cryOperationAbort(&op);
  }
  test_end_step(3);

  /* [1.4.4] Beginning a stream on the erased transient key,
     CRY_ERR_KEY is expected.*/
  test_set_step(4);
  {
    msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                         CRY_ENCRYPT, &params);
    test_assert(msg == CRY_ERR_KEY, "key survived the stop");
  }
  test_end_step(4);

  /* [1.4.5] Loading the key again, a new stream is expected to
     begin.*/
  test_set_step(5);
  {
    msg = xcry_load_key(CRY_KEY_AES, sizeof key, key);
    test_assert(msg == HAL_RET_SUCCESS, "key load failed");
    msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                         CRY_ENCRYPT, &params);
    test_assert(msg == HAL_RET_SUCCESS, "begin failed");
    cryOperationAbort(&op);
  }
  test_end_step(5);
}

static const testcase_t xcry_test_001_004 = {
  "Stop invalidates streams",
  xcry_test_001_004_setup,
  xcry_test_001_004_teardown,
  xcry_test_001_004_execute
};

/**
 * @page xcry_test_001_005 [1.5] Concurrent streams
 *
 * <h2>Description</h2>
 * Two hash streams are open at once; the second either begins, and
 * both produce correct digests, or is refused with CRY_ERR_BUSY.
 *
 * <h2>Test Steps</h2>
 * - [1.5.1] Selecting a supported hash, the test is skipped if there
 *   is none.
 * - [1.5.2] Beginning two streams, the second is allowed to be busy.
 * - [1.5.3] Interleaving updates and finalizing, the digests are
 *   expected to match.
 * .
 */

static void xcry_test_001_005_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_001_005_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_005_execute(void) {
  const xcry_hash_vector_t *vp;
  cry_operation_t op1, op2;
  cry_algorithm_t alg;
  bool second;
  uint8_t digest1[XCRY_DIGEST_MAX], digest2[XCRY_DIGEST_MAX];
  size_t len;
  unsigned i;
  msg_t msg;

  /* [1.5.1] Selecting a supported hash, the test is skipped if there
     is none.*/
  test_set_step(1);
  {
    alg = CRY_ALG_NONE;
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      cry_capabilities_t caps;

      if (cryGetCapabilities(XCRY_DRIVER, xcry_sha_vectors[i].algorithm,
                             &caps) == HAL_RET_SUCCESS) {
        alg = xcry_sha_vectors[i].algorithm;
        /* The "abc" example follows the empty message.*/
        vp = &xcry_sha_vectors[i + 1U];
        break;
      }
    }
    if (alg == CRY_ALG_NONE) {
      test_println("--- Skipped, no hash supported by the driver");
      return;
    }
  }
  test_end_step(1);

  /* [1.5.2] Beginning two streams, the second is allowed to be busy.*/
  test_set_step(2);
  {
    msg = cryHashBegin(XCRY_DRIVER, &op1, alg);
    test_assert(msg == HAL_RET_SUCCESS, "first begin failed");
    msg = cryHashBegin(XCRY_DRIVER, &op2, alg);
    test_assert((msg == HAL_RET_SUCCESS) || (msg == CRY_ERR_BUSY),
                "unexpected error");
    second = msg == HAL_RET_SUCCESS;
    if (!second) {
      test_println("--- Second stream refused, engine busy");
    }
  }
  test_end_step(2);

  /* [1.5.3] Interleaving updates and finalizing, the digests are
     expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < vp->size; i++) {
      msg = cryHashUpdate(&op1, 1U, &vp->msg[i]);
      test_assert(msg == HAL_RET_SUCCESS, "first update failed");
      if (second) {
        msg = cryHashUpdate(&op2, 1U, &vp->msg[i]);
        test_assert(msg == HAL_RET_SUCCESS, "second update failed");
      }
    }
    msg = cryHashFinal(&op1, sizeof digest1, digest1, &len);
    test_assert(msg == HAL_RET_SUCCESS, "first final failed");
    test_assert(memcmp(digest1, vp->digest, vp->digest_size) == 0,
                "first digest mismatch");
    if (second) {
      msg = cryHashFinal(&op2, sizeof digest2, digest2, &len);
      test_assert(msg == HAL_RET_SUCCESS, "second final failed");
      test_assert(memcmp(digest2, vp->digest, vp->digest_size) == 0,
                  "second digest mismatch");
    }
  }
  test_end_step(3);
}

static const testcase_t xcry_test_001_005 = {
  "Concurrent streams",
  xcry_test_001_005_setup,
  xcry_test_001_005_teardown,
  xcry_test_001_005_execute
};

/**
 * @page xcry_test_001_006 [1.6] Stream abort
 *
 * <h2>Description</h2>
 * Aborting finalized, idle and active contexts.
 *
 * <h2>Test Steps</h2>
 * - [1.6.1] Selecting a supported hash, the test is skipped if there
 *   is none.
 * - [1.6.2] Aborting an active stream then a finalized one, a new
 *   stream on the same context is expected to work.
 * .
 */

static void xcry_test_001_006_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_001_006_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_006_execute(void) {
  const xcry_hash_vector_t *vp;
  cry_operation_t op;
  cry_algorithm_t alg;
  uint8_t digest[XCRY_DIGEST_MAX];
  size_t len;
  unsigned i;
  msg_t msg;

  /* [1.6.1] Selecting a supported hash, the test is skipped if there
     is none.*/
  test_set_step(1);
  {
    alg = CRY_ALG_NONE;
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      cry_capabilities_t caps;

      if (cryGetCapabilities(XCRY_DRIVER, xcry_sha_vectors[i].algorithm,
                             &caps) == HAL_RET_SUCCESS) {
        alg = xcry_sha_vectors[i].algorithm;
        /* The "abc" example follows the empty message.*/
        vp = &xcry_sha_vectors[i + 1U];
        break;
      }
    }
    if (alg == CRY_ALG_NONE) {
      test_println("--- Skipped, no hash supported by the driver");
      return;
    }
  }
  test_end_step(1);

  /* [1.6.2] Aborting an active stream then a finalized one, a new
     stream on the same context is expected to work.*/
  test_set_step(2);
  {
    msg = cryHashBegin(XCRY_DRIVER, &op, alg);
    test_assert(msg == HAL_RET_SUCCESS, "begin failed");
    msg = cryHashUpdate(&op, vp->size, vp->msg);
    test_assert(msg == HAL_RET_SUCCESS, "update failed");
    cryOperationAbort(&op);
    cryOperationAbort(&op);
    msg = cryHashBegin(XCRY_DRIVER, &op, alg);
    test_assert(msg == HAL_RET_SUCCESS, "begin after abort failed");
    msg = cryHashUpdate(&op, vp->size, vp->msg);
    test_assert(msg == HAL_RET_SUCCESS, "update failed");
    msg = cryHashFinal(&op, sizeof digest, digest, &len);
    test_assert(msg == HAL_RET_SUCCESS, "final failed");
    test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                "digest mismatch");
    cryOperationAbort(&op);
  }
  test_end_step(2);
}

static const testcase_t xcry_test_001_006 = {
  "Stream abort",
  xcry_test_001_006_setup,
  xcry_test_001_006_teardown,
  xcry_test_001_006_execute
};

/**
 * @page xcry_test_001_007 [1.7] Unsupported operations
 *
 * <h2>Description</h2>
 * Beginning a stream for an algorithm the driver does not support is
 * expected to return CRY_ERR_UNSUPPORTED.
 *
 * <h2>Test Steps</h2>
 * - [1.7.1] Loading an AES key, when possible, so that key errors do
 *   not hide unsupported algorithms.
 * - [1.7.2] Beginning streams for all unsupported stream algorithms.
 * .
 */

static void xcry_test_001_007_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_001_007_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_001_007_execute(void) {
  static const uint8_t key[16] = {0};
  static const uint8_t iv[16] = {0};
  cry_cipher_params_t cparams = {iv, sizeof iv};
  cry_aead_params_t aparams = {iv, 12U, 16U, 0U, 0U};
  cry_capabilities_t caps;
  cry_operation_t op;
  cry_algorithm_t alg;
  msg_t msg;

  /* [1.7.1] Loading an AES key, when possible, so that key errors do
     not hide unsupported algorithms.*/
  test_set_step(1);
  {
    (void) xcry_load_key(CRY_KEY_AES, sizeof key, key);
  }
  test_end_step(1);

  /* [1.7.2] Beginning streams for all unsupported stream algorithms.*/
  test_set_step(2);
  {
    for (alg = CRY_ALG_AES_ECB; alg <= CRY_ALG_SHA512; alg++) {
      if (cryGetCapabilities(XCRY_DRIVER, alg, &caps) == HAL_RET_SUCCESS) {
        continue;
      }
      switch (alg) {
      case CRY_ALG_AES_ECB:
        cparams.iv = NULL;
        cparams.iv_size = 0U;
        msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                             CRY_ENCRYPT, &cparams);
        cparams.iv = iv;
        cparams.iv_size = sizeof iv;
        break;
      case CRY_ALG_AES_CBC:
      case CRY_ALG_AES_CFB128:
      case CRY_ALG_AES_CTR:
        msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                             CRY_ENCRYPT, &cparams);
        break;
      case CRY_ALG_AES_GCM:
      case CRY_ALG_AES_CCM:
        msg = cryAeadBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                           CRY_ENCRYPT, &aparams);
        break;
      case CRY_ALG_AES_CMAC:
      case CRY_ALG_HMAC_SHA256:
      case CRY_ALG_HMAC_SHA384:
      case CRY_ALG_HMAC_SHA512:
        msg = cryMacBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, alg,
                          false, 16U);
        break;
      default:
        msg = cryHashBegin(XCRY_DRIVER, &op, alg);
        break;
      }
      test_assert(msg == CRY_ERR_UNSUPPORTED, "unsupported algorithm began");
      cryOperationAbort(&op);
    }
  }
  test_end_step(2);
}

static const testcase_t xcry_test_001_007 = {
  "Unsupported operations",
  xcry_test_001_007_setup,
  xcry_test_001_007_teardown,
  xcry_test_001_007_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const xcry_test_sequence_001_array[] = {
  &xcry_test_001_001,
  &xcry_test_001_002,
  &xcry_test_001_003,
  &xcry_test_001_004,
  &xcry_test_001_005,
  &xcry_test_001_006,
  &xcry_test_001_007,
  NULL
};

/**
 * @brief   Driver and API.
 */
const testsequence_t xcry_test_sequence_001 = {
  "Driver and API",
  xcry_test_sequence_001_array
};
