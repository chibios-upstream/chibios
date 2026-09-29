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
 * @file    xcry_test_sequence_010.c
 * @brief   Test Sequence 010 code.
 *
 * @page xcry_test_sequence_010 [10] SHA
 *
 * File: @ref xcry_test_sequence_010.c
 *
 * <h2>Description</h2>
 * SHA-1 and SHA-2 hashes with the FIPS 180-4 examples.
 *
 * <h2>Test Cases</h2>
 * - @subpage xcry_test_010_001
 * - @subpage xcry_test_010_002
 * - @subpage xcry_test_010_003
 * - @subpage xcry_test_010_004
 * - @subpage xcry_test_010_005
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
 * @page xcry_test_010_001 [10.1] SHA-1
 *
 * <h2>Description</h2>
 * SHA-1 with the FIPS 180-4 examples, byte by byte and with one
 * million "a" in large fragments.
 *
 * <h2>Test Steps</h2>
 * - [10.1.1] Checking SHA-1 support, the test is skipped if the driver
 *   does not support it.
 * - [10.1.2] Hashing the examples in single updates, the digests are
 *   expected to match.
 * - [10.1.3] Hashing the examples byte by byte, the digests are
 *   expected to match.
 * - [10.1.4] Hashing one million "a" in 5000 bytes fragments from a
 *   shared buffer, the digest is expected to match.
 * .
 */

static void xcry_test_010_001_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_010_001_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_010_001_execute(void) {
  const xcry_hash_vector_t *mp;
  uint8_t digest[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [10.1.1] Checking SHA-1 support, the test is skipped if the driver
     does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_SHA1)) {
      return;
    }
  }
  test_end_step(1);

  /* [10.1.2] Hashing the examples in single updates, the digests are
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA1) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest,
                      XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(2);

  /* [10.1.3] Hashing the examples byte by byte, the digests are
     expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA1) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest, 1U);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(3);

  /* [10.1.4] Hashing one million "a" in 5000 bytes fragments from a
     shared buffer, the digest is expected to match.*/
  test_set_step(4);
  {
    mp = NULL;
    for (i = 0U; i < XCRY_SHA_MILLION_A_COUNT; i++) {
      if (xcry_sha_million_a[i].algorithm == CRY_ALG_SHA1) {
        mp = &xcry_sha_million_a[i];
      }
    }
    test_assert(mp != NULL, "missing vector");
    memset(xcry_in, 'a', 5000U);
    {
      cry_operation_t op;
      size_t len;

      msg = cryHashBegin(XCRY_DRIVER, &op, mp->algorithm);
      test_assert(msg == HAL_RET_SUCCESS, "begin failed");
      for (i = 0U; i < mp->size / 5000U; i++) {
        msg = cryHashUpdate(&op, 5000U, xcry_in);
        test_assert(msg == HAL_RET_SUCCESS, "update failed");
      }
      msg = cryHashFinal(&op, sizeof digest, digest, &len);
      test_assert(msg == HAL_RET_SUCCESS, "final failed");
    }
    test_assert(memcmp(digest, mp->digest, mp->digest_size) == 0,
                "digest mismatch");
  }
  test_end_step(4);
}

static const testcase_t xcry_test_010_001 = {
  "SHA-1",
  xcry_test_010_001_setup,
  xcry_test_010_001_teardown,
  xcry_test_010_001_execute
};

/**
 * @page xcry_test_010_002 [10.2] SHA-224
 *
 * <h2>Description</h2>
 * SHA-224 with the FIPS 180-4 examples, byte by byte and with one
 * million "a" in large fragments.
 *
 * <h2>Test Steps</h2>
 * - [10.2.1] Checking SHA-224 support, the test is skipped if the
 *   driver does not support it.
 * - [10.2.2] Hashing the examples in single updates, the digests are
 *   expected to match.
 * - [10.2.3] Hashing the examples byte by byte, the digests are
 *   expected to match.
 * - [10.2.4] Hashing one million "a" in 5000 bytes fragments from a
 *   shared buffer, the digest is expected to match.
 * .
 */

static void xcry_test_010_002_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_010_002_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_010_002_execute(void) {
  const xcry_hash_vector_t *mp;
  uint8_t digest[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [10.2.1] Checking SHA-224 support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_SHA224)) {
      return;
    }
  }
  test_end_step(1);

  /* [10.2.2] Hashing the examples in single updates, the digests are
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA224) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest,
                      XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(2);

  /* [10.2.3] Hashing the examples byte by byte, the digests are
     expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA224) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest, 1U);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(3);

  /* [10.2.4] Hashing one million "a" in 5000 bytes fragments from a
     shared buffer, the digest is expected to match.*/
  test_set_step(4);
  {
    mp = NULL;
    for (i = 0U; i < XCRY_SHA_MILLION_A_COUNT; i++) {
      if (xcry_sha_million_a[i].algorithm == CRY_ALG_SHA224) {
        mp = &xcry_sha_million_a[i];
      }
    }
    test_assert(mp != NULL, "missing vector");
    memset(xcry_in, 'a', 5000U);
    {
      cry_operation_t op;
      size_t len;

      msg = cryHashBegin(XCRY_DRIVER, &op, mp->algorithm);
      test_assert(msg == HAL_RET_SUCCESS, "begin failed");
      for (i = 0U; i < mp->size / 5000U; i++) {
        msg = cryHashUpdate(&op, 5000U, xcry_in);
        test_assert(msg == HAL_RET_SUCCESS, "update failed");
      }
      msg = cryHashFinal(&op, sizeof digest, digest, &len);
      test_assert(msg == HAL_RET_SUCCESS, "final failed");
    }
    test_assert(memcmp(digest, mp->digest, mp->digest_size) == 0,
                "digest mismatch");
  }
  test_end_step(4);
}

static const testcase_t xcry_test_010_002 = {
  "SHA-224",
  xcry_test_010_002_setup,
  xcry_test_010_002_teardown,
  xcry_test_010_002_execute
};

/**
 * @page xcry_test_010_003 [10.3] SHA-256
 *
 * <h2>Description</h2>
 * SHA-256 with the FIPS 180-4 examples, byte by byte and with one
 * million "a" in large fragments.
 *
 * <h2>Test Steps</h2>
 * - [10.3.1] Checking SHA-256 support, the test is skipped if the
 *   driver does not support it.
 * - [10.3.2] Hashing the examples in single updates, the digests are
 *   expected to match.
 * - [10.3.3] Hashing the examples byte by byte, the digests are
 *   expected to match.
 * - [10.3.4] Hashing one million "a" in 5000 bytes fragments from a
 *   shared buffer, the digest is expected to match.
 * .
 */

static void xcry_test_010_003_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_010_003_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_010_003_execute(void) {
  const xcry_hash_vector_t *mp;
  uint8_t digest[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [10.3.1] Checking SHA-256 support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_SHA256)) {
      return;
    }
  }
  test_end_step(1);

  /* [10.3.2] Hashing the examples in single updates, the digests are
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA256) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest,
                      XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(2);

  /* [10.3.3] Hashing the examples byte by byte, the digests are
     expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA256) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest, 1U);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(3);

  /* [10.3.4] Hashing one million "a" in 5000 bytes fragments from a
     shared buffer, the digest is expected to match.*/
  test_set_step(4);
  {
    mp = NULL;
    for (i = 0U; i < XCRY_SHA_MILLION_A_COUNT; i++) {
      if (xcry_sha_million_a[i].algorithm == CRY_ALG_SHA256) {
        mp = &xcry_sha_million_a[i];
      }
    }
    test_assert(mp != NULL, "missing vector");
    memset(xcry_in, 'a', 5000U);
    {
      cry_operation_t op;
      size_t len;

      msg = cryHashBegin(XCRY_DRIVER, &op, mp->algorithm);
      test_assert(msg == HAL_RET_SUCCESS, "begin failed");
      for (i = 0U; i < mp->size / 5000U; i++) {
        msg = cryHashUpdate(&op, 5000U, xcry_in);
        test_assert(msg == HAL_RET_SUCCESS, "update failed");
      }
      msg = cryHashFinal(&op, sizeof digest, digest, &len);
      test_assert(msg == HAL_RET_SUCCESS, "final failed");
    }
    test_assert(memcmp(digest, mp->digest, mp->digest_size) == 0,
                "digest mismatch");
  }
  test_end_step(4);
}

static const testcase_t xcry_test_010_003 = {
  "SHA-256",
  xcry_test_010_003_setup,
  xcry_test_010_003_teardown,
  xcry_test_010_003_execute
};

/**
 * @page xcry_test_010_004 [10.4] SHA-384
 *
 * <h2>Description</h2>
 * SHA-384 with the FIPS 180-4 examples, byte by byte and with one
 * million "a" in large fragments.
 *
 * <h2>Test Steps</h2>
 * - [10.4.1] Checking SHA-384 support, the test is skipped if the
 *   driver does not support it.
 * - [10.4.2] Hashing the examples in single updates, the digests are
 *   expected to match.
 * - [10.4.3] Hashing the examples byte by byte, the digests are
 *   expected to match.
 * - [10.4.4] Hashing one million "a" in 5000 bytes fragments from a
 *   shared buffer, the digest is expected to match.
 * .
 */

static void xcry_test_010_004_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_010_004_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_010_004_execute(void) {
  const xcry_hash_vector_t *mp;
  uint8_t digest[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [10.4.1] Checking SHA-384 support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_SHA384)) {
      return;
    }
  }
  test_end_step(1);

  /* [10.4.2] Hashing the examples in single updates, the digests are
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA384) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest,
                      XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(2);

  /* [10.4.3] Hashing the examples byte by byte, the digests are
     expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA384) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest, 1U);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(3);

  /* [10.4.4] Hashing one million "a" in 5000 bytes fragments from a
     shared buffer, the digest is expected to match.*/
  test_set_step(4);
  {
    mp = NULL;
    for (i = 0U; i < XCRY_SHA_MILLION_A_COUNT; i++) {
      if (xcry_sha_million_a[i].algorithm == CRY_ALG_SHA384) {
        mp = &xcry_sha_million_a[i];
      }
    }
    test_assert(mp != NULL, "missing vector");
    memset(xcry_in, 'a', 5000U);
    {
      cry_operation_t op;
      size_t len;

      msg = cryHashBegin(XCRY_DRIVER, &op, mp->algorithm);
      test_assert(msg == HAL_RET_SUCCESS, "begin failed");
      for (i = 0U; i < mp->size / 5000U; i++) {
        msg = cryHashUpdate(&op, 5000U, xcry_in);
        test_assert(msg == HAL_RET_SUCCESS, "update failed");
      }
      msg = cryHashFinal(&op, sizeof digest, digest, &len);
      test_assert(msg == HAL_RET_SUCCESS, "final failed");
    }
    test_assert(memcmp(digest, mp->digest, mp->digest_size) == 0,
                "digest mismatch");
  }
  test_end_step(4);
}

static const testcase_t xcry_test_010_004 = {
  "SHA-384",
  xcry_test_010_004_setup,
  xcry_test_010_004_teardown,
  xcry_test_010_004_execute
};

/**
 * @page xcry_test_010_005 [10.5] SHA-512
 *
 * <h2>Description</h2>
 * SHA-512 with the FIPS 180-4 examples, byte by byte and with one
 * million "a" in large fragments.
 *
 * <h2>Test Steps</h2>
 * - [10.5.1] Checking SHA-512 support, the test is skipped if the
 *   driver does not support it.
 * - [10.5.2] Hashing the examples in single updates, the digests are
 *   expected to match.
 * - [10.5.3] Hashing the examples byte by byte, the digests are
 *   expected to match.
 * - [10.5.4] Hashing one million "a" in 5000 bytes fragments from a
 *   shared buffer, the digest is expected to match.
 * .
 */

static void xcry_test_010_005_setup(void) {
  drvStart(XCRY_DRIVER, NULL);
}

static void xcry_test_010_005_teardown(void) {
  drvStop(XCRY_DRIVER);
}

static void xcry_test_010_005_execute(void) {
  const xcry_hash_vector_t *mp;
  uint8_t digest[XCRY_DIGEST_MAX];
  unsigned i;
  msg_t msg;

  /* [10.5.1] Checking SHA-512 support, the test is skipped if the
     driver does not support it.*/
  test_set_step(1);
  {
    if (xcry_unsupported(CRY_ALG_SHA512)) {
      return;
    }
  }
  test_end_step(1);

  /* [10.5.2] Hashing the examples in single updates, the digests are
     expected to match.*/
  test_set_step(2);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA512) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest,
                      XCRY_BUFFER_SIZE);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(2);

  /* [10.5.3] Hashing the examples byte by byte, the digests are
     expected to match.*/
  test_set_step(3);
  {
    for (i = 0U; i < XCRY_SHA_VECTORS_COUNT; i++) {
      const xcry_hash_vector_t *vp = &xcry_sha_vectors[i];

      if (vp->algorithm != CRY_ALG_SHA512) {
        continue;
      }
      msg = xcry_hash(vp->algorithm, vp->msg, vp->size, digest, 1U);
      test_assert(msg == HAL_RET_SUCCESS, "hash failed");
      test_assert(memcmp(digest, vp->digest, vp->digest_size) == 0,
                  "digest mismatch");
    }
  }
  test_end_step(3);

  /* [10.5.4] Hashing one million "a" in 5000 bytes fragments from a
     shared buffer, the digest is expected to match.*/
  test_set_step(4);
  {
    mp = NULL;
    for (i = 0U; i < XCRY_SHA_MILLION_A_COUNT; i++) {
      if (xcry_sha_million_a[i].algorithm == CRY_ALG_SHA512) {
        mp = &xcry_sha_million_a[i];
      }
    }
    test_assert(mp != NULL, "missing vector");
    memset(xcry_in, 'a', 5000U);
    {
      cry_operation_t op;
      size_t len;

      msg = cryHashBegin(XCRY_DRIVER, &op, mp->algorithm);
      test_assert(msg == HAL_RET_SUCCESS, "begin failed");
      for (i = 0U; i < mp->size / 5000U; i++) {
        msg = cryHashUpdate(&op, 5000U, xcry_in);
        test_assert(msg == HAL_RET_SUCCESS, "update failed");
      }
      msg = cryHashFinal(&op, sizeof digest, digest, &len);
      test_assert(msg == HAL_RET_SUCCESS, "final failed");
    }
    test_assert(memcmp(digest, mp->digest, mp->digest_size) == 0,
                "digest mismatch");
  }
  test_end_step(4);
}

static const testcase_t xcry_test_010_005 = {
  "SHA-512",
  xcry_test_010_005_setup,
  xcry_test_010_005_teardown,
  xcry_test_010_005_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const xcry_test_sequence_010_array[] = {
  &xcry_test_010_001,
  &xcry_test_010_002,
  &xcry_test_010_003,
  &xcry_test_010_004,
  &xcry_test_010_005,
  NULL
};

/**
 * @brief   SHA.
 */
const testsequence_t xcry_test_sequence_010 = {
  "SHA",
  xcry_test_sequence_010_array
};
