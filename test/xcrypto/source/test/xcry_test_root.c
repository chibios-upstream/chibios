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

/**
 * @mainpage Test Suite Specification
 * Test suite for the ChibiOS/XHAL Crypto driver. The general sequence
 * checks the driver and API behavior, the other sequences check each
 * algorithm against published test vectors. Algorithms not supported
 * by the driver under test are skipped.
 *
 * <h2>Test Sequences</h2>
 * - @subpage xcry_test_sequence_001
 * - @subpage xcry_test_sequence_002
 * - @subpage xcry_test_sequence_003
 * - @subpage xcry_test_sequence_004
 * - @subpage xcry_test_sequence_005
 * - @subpage xcry_test_sequence_006
 * - @subpage xcry_test_sequence_007
 * - @subpage xcry_test_sequence_008
 * - @subpage xcry_test_sequence_009
 * - @subpage xcry_test_sequence_010
 * - @subpage xcry_test_sequence_011
 * .
 */

/**
 * @file    xcry_test_root.c
 * @brief   Test Suite root structures code.
 */

#include "hal.h"
#include "xcry_test_root.h"

#if !defined(__DOXYGEN__)

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Array of test sequences.
 */
const testsequence_t * const xcry_test_suite_array[] = {
  &xcry_test_sequence_001,
  &xcry_test_sequence_002,
  &xcry_test_sequence_003,
  &xcry_test_sequence_004,
  &xcry_test_sequence_005,
  &xcry_test_sequence_006,
  &xcry_test_sequence_007,
  &xcry_test_sequence_008,
  &xcry_test_sequence_009,
  &xcry_test_sequence_010,
  &xcry_test_sequence_011,
  NULL
};

/**
 * @brief   Test suite root structure.
 */
const testsuite_t xcry_test_suite = {
  "ChibiOS/XHAL Crypto Test Suite",
  xcry_test_suite_array
};

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

uint32_t __nocache_xcry_in[XCRY_BUFFER_SIZE / 4U];
uint32_t __nocache_xcry_out[XCRY_BUFFER_SIZE / 4U];

/* Returns true, reporting a skip, if the driver lacks the algorithm.*/
bool xcry_unsupported(cry_algorithm_t algorithm) {
  cry_capabilities_t caps;

  if (cryGetCapabilities(XCRY_DRIVER, algorithm, &caps) == HAL_RET_SUCCESS) {
    return false;
  }
  test_println("--- Skipped, algorithm not supported by the driver");

  return true;
}

/* Returns true if the driver supports a key size, in bytes.*/
bool xcry_key_supported(cry_algorithm_t algorithm, size_t size) {
  cry_capabilities_t caps;

  (void) cryGetCapabilities(XCRY_DRIVER, algorithm, &caps);

  return (size * 8U >= caps.min_key_bits) && (size * 8U <= caps.max_key_bits);
}

/* Loads raw key material into the transient key.*/
msg_t xcry_load_key(cry_key_type_t type, size_t size, const uint8_t *key) {
  cry_key_params_t params = {type, size * 8U, CRY_CURVE_NONE};

  return cryKeyLoad(XCRY_DRIVER, CRY_KEY_TRANSIENT, &params,
                    CRY_KEY_FORMAT_RAW, size, key);
}

/* Runs a cipher vector in fragments, each followed by an empty update,
   returns the first failure.*/
msg_t xcry_cipher(const xcry_cipher_vector_t *v, cry_direction_t direction,
                  const uint8_t *in, uint8_t *out, size_t frag) {
  cry_cipher_params_t params = {v->iv, v->iv_size};
  cry_operation_t op;
  size_t done, produced, n, len;
  msg_t msg;

  msg = xcry_load_key(CRY_KEY_AES, v->key_size, v->key);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  msg = cryCipherBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, v->algorithm,
                       direction, &params);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  done = 0U;
  produced = 0U;
  while (done < v->size) {
    n = (v->size - done) < frag ? (v->size - done) : frag;
    msg = cryCipherUpdate(&op, n, &in[done], v->size - produced,
                          &out[produced], &len);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    done += n;
    produced += len;
    msg = cryCipherUpdate(&op, 0U, NULL, 0U, NULL, &len);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
  }
  msg = cryCipherFinal(&op, v->size - produced, &out[produced], &len);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  produced += len;

  return produced == v->size ? HAL_RET_SUCCESS : CRY_ERR_FAILURE;
}

/* Runs an AEAD vector in fragments, each followed by an empty update, the
   tag is generated on encryption and verified on decryption.*/
msg_t xcry_aead(const xcry_aead_vector_t *v, cry_direction_t direction,
                const uint8_t *in, uint8_t *out, uint8_t *tag,
                size_t tag_size, size_t frag, bool declare) {
  cry_aead_params_t params = {v->nonce, v->nonce_size, v->tag_size,
                              declare ? v->aad_size : CRY_LENGTH_UNKNOWN,
                              declare ? v->size : CRY_LENGTH_UNKNOWN};
  cry_operation_t op;
  size_t done, produced, n, len;
  msg_t msg;

  msg = xcry_load_key(CRY_KEY_AES, v->key_size, v->key);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  msg = cryAeadBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, v->algorithm,
                     direction, &params);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  for (done = 0U; done < v->aad_size; done += n) {
    n = (v->aad_size - done) < frag ? (v->aad_size - done) : frag;
    msg = cryAeadUpdateAAD(&op, n, &v->aad[done]);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    msg = cryAeadUpdateAAD(&op, 0U, NULL);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
  }
  produced = 0U;
  for (done = 0U; done < v->size; done += n) {
    n = (v->size - done) < frag ? (v->size - done) : frag;
    msg = cryAeadUpdate(&op, n, &in[done], v->size - produced,
                        &out[produced], &len);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    produced += len;
    msg = cryAeadUpdate(&op, 0U, NULL, 0U, NULL, &len);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
  }
  if (direction == CRY_ENCRYPT) {
    msg = cryAeadFinal(&op, v->size - produced, &out[produced], &len,
                       tag_size, tag);
  }
  else {
    msg = cryAeadVerify(&op, v->size - produced, &out[produced], &len,
                        tag_size, tag);
  }
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  produced += len;

  return produced == v->size ? HAL_RET_SUCCESS : CRY_ERR_FAILURE;
}

/* Returns true, reporting a skip, if an AEAD vector was rejected only
   because it uses parameters that are optional for backends: GCM nonces
   other than 96 bits.*/
bool xcry_aead_optional(const xcry_aead_vector_t *v, msg_t msg) {

  if ((msg != CRY_ERR_UNSUPPORTED) || (v->algorithm != CRY_ALG_AES_GCM) ||
      (v->nonce_size == 12U)) {
    return false;
  }
  test_print("--- Skipped, optional parameters: ");
  test_println(v->name);

  return true;
}

/* Runs a MAC vector in fragments, each followed by an empty update, the tag
   is generated or verified.*/
msg_t xcry_mac(const xcry_mac_vector_t *v, bool verify, uint8_t *tag,
               size_t tag_size, size_t frag) {
  cry_key_type_t type;
  cry_operation_t op;
  size_t done, n, len;
  msg_t msg;

  type = v->algorithm == CRY_ALG_AES_CMAC ? CRY_KEY_AES : CRY_KEY_HMAC;
  msg = xcry_load_key(type, v->key_size, v->key);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  msg = cryMacBegin(XCRY_DRIVER, &op, CRY_KEY_TRANSIENT, v->algorithm,
                    verify, v->tag_size);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  for (done = 0U; done < v->size; done += n) {
    n = (v->size - done) < frag ? (v->size - done) : frag;
    msg = cryMacUpdate(&op, n, &v->msg[done]);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    msg = cryMacUpdate(&op, 0U, NULL);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
  }
  if (verify) {
    return cryMacVerify(&op, tag_size, tag);
  }
  msg = cryMacFinal(&op, tag_size, tag, &len);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }

  return len == v->tag_size ? HAL_RET_SUCCESS : CRY_ERR_FAILURE;
}

/* Hashes a message in fragments, each followed by an empty update, the
   output buffer holds XCRY_DIGEST_MAX bytes.*/
msg_t xcry_hash(cry_algorithm_t algorithm, const uint8_t *msg, size_t size,
                uint8_t *out, size_t frag) {
  cry_operation_t op;
  size_t done, n, len;
  msg_t ret;

  ret = cryHashBegin(XCRY_DRIVER, &op, algorithm);
  if (ret != HAL_RET_SUCCESS) {
    return ret;
  }
  for (done = 0U; done < size; done += n) {
    n = (size - done) < frag ? (size - done) : frag;
    ret = cryHashUpdate(&op, n, &msg[done]);
    if (ret != HAL_RET_SUCCESS) {
      return ret;
    }
    ret = cryHashUpdate(&op, 0U, NULL);
    if (ret != HAL_RET_SUCCESS) {
      return ret;
    }
  }

  return cryHashFinal(&op, XCRY_DIGEST_MAX, out, &len);
}

#endif /* !defined(__DOXYGEN__) */
