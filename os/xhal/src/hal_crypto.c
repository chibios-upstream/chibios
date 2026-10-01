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
 * @file        hal_crypto.c
 * @brief       Generated Crypto Driver source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_CRYPTO
 * @{
 */

#include "hal.h"
#include <string.h>

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module local macros.                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/**
 * @brief       Checks the mathematical description of key material.
 *
 * @param[in]     params        Key parameters.
 * @return                      True if the parameter combination is valid.
 *
 * @notapi
 */
static bool cry_key_params_valid(const cry_key_params_t *params) {
  size_t curve_bits;

  if (params->bits == 0U) {
    return false;
  }
  switch (params->type) {
  case CRY_KEY_AES:
    return (params->curve == CRY_CURVE_NONE) &&
           ((params->bits == 128U) || (params->bits == 192U) ||
            (params->bits == 256U));
  case CRY_KEY_HMAC:
  case CRY_KEY_DERIVATION:
    return (params->curve == CRY_CURVE_NONE) && (params->bits % 8U == 0U);
  case CRY_KEY_RSA_PUBLIC:
  case CRY_KEY_RSA_PAIR:
    return params->curve == CRY_CURVE_NONE;
  case CRY_KEY_ECC_PUBLIC:
  case CRY_KEY_ECC_PAIR:
    curve_bits = params->curve == CRY_CURVE_P256 ? 256U :
                 params->curve == CRY_CURVE_P384 ? 384U :
                 params->curve == CRY_CURVE_P521 ? 521U : 0U;
    return params->bits == curve_bits;
  default:
    return false;
  }
}

/**
 * @brief       Checks a pointer/length pair.
 *
 * @param[in]     buf           Buffer pointer, may be NULL only when the size
 *                              is zero.
 * @param[in]     size          Buffer size, in bytes.
 * @return                      True if the pair is consistent.
 *
 * @notapi
 */
static bool cry_buffer_valid(const void *buf, size_t size) {

  return (size == 0U) || (buf != NULL);
}

/**
 * @brief       Identifies known algorithms without consulting key metadata.
 *
 * @param[in]     algorithm     Algorithm selector.
 * @return                      The operation class, CRY_CLASS_NONE for unknown
 *                              selectors.
 *
 * @notapi
 */
static cry_class_t cry_algorithm_class(cry_algorithm_t algorithm) {

  switch (algorithm) {
  case CRY_ALG_AES_ECB:
  case CRY_ALG_AES_CBC:
  case CRY_ALG_AES_CFB128:
  case CRY_ALG_AES_CTR:
    return CRY_CLASS_CIPHER;
  case CRY_ALG_AES_GCM:
  case CRY_ALG_AES_CCM:
    return CRY_CLASS_AEAD;
  case CRY_ALG_AES_CMAC:
  case CRY_ALG_HMAC_SHA256:
  case CRY_ALG_HMAC_SHA384:
  case CRY_ALG_HMAC_SHA512:
    return CRY_CLASS_MAC;
  case CRY_ALG_SHA1:
  case CRY_ALG_SHA224:
  case CRY_ALG_SHA256:
  case CRY_ALG_SHA384:
  case CRY_ALG_SHA512:
    return CRY_CLASS_HASH;
  case CRY_ALG_RSA_PSS_SHA256:
  case CRY_ALG_RSA_PSS_SHA384:
  case CRY_ALG_RSA_PSS_SHA512:
  case CRY_ALG_RSA_PKCS1_SHA256:
  case CRY_ALG_RSA_PKCS1_SHA384:
  case CRY_ALG_RSA_PKCS1_SHA512:
  case CRY_ALG_ECDSA_SHA256:
  case CRY_ALG_ECDSA_SHA384:
  case CRY_ALG_ECDSA_SHA512:
    return CRY_CLASS_SIGNATURE;
  case CRY_ALG_RSA_OAEP_SHA256:
  case CRY_ALG_RSA_OAEP_SHA384:
  case CRY_ALG_RSA_OAEP_SHA512:
    return CRY_CLASS_ASYMMETRIC;
  case CRY_ALG_ECDH:
    return CRY_CLASS_AGREEMENT;
  case CRY_ALG_HKDF_SHA256:
  case CRY_ALG_HKDF_SHA384:
  case CRY_ALG_HKDF_SHA512:
    return CRY_CLASS_DERIVATION;
  default:
    return CRY_CLASS_NONE;
  }
}

/**
 * @brief       Returns the fixed digest or MAC size.
 *
 * @param[in]     algorithm     Algorithm selector.
 * @return                      The hash output size, zero for algorithms
 *                              without one.
 *
 * @notapi
 */
static size_t cry_digest_size(cry_algorithm_t algorithm) {

  switch (algorithm) {
  case CRY_ALG_SHA1:
    return 20U;
  case CRY_ALG_SHA224:
    return 28U;
  case CRY_ALG_AES_CMAC:
    return 16U;
  case CRY_ALG_HMAC_SHA256:
  case CRY_ALG_SHA256:
  case CRY_ALG_RSA_PSS_SHA256:
  case CRY_ALG_RSA_PKCS1_SHA256:
  case CRY_ALG_ECDSA_SHA256:
  case CRY_ALG_RSA_OAEP_SHA256:
  case CRY_ALG_HKDF_SHA256:
    return 32U;
  case CRY_ALG_HMAC_SHA384:
  case CRY_ALG_SHA384:
  case CRY_ALG_RSA_PSS_SHA384:
  case CRY_ALG_RSA_PKCS1_SHA384:
  case CRY_ALG_ECDSA_SHA384:
  case CRY_ALG_RSA_OAEP_SHA384:
  case CRY_ALG_HKDF_SHA384:
    return 48U;
  case CRY_ALG_HMAC_SHA512:
  case CRY_ALG_SHA512:
  case CRY_ALG_RSA_PSS_SHA512:
  case CRY_ALG_RSA_PKCS1_SHA512:
  case CRY_ALG_ECDSA_SHA512:
  case CRY_ALG_RSA_OAEP_SHA512:
  case CRY_ALG_HKDF_SHA512:
    return 64U;
  default:
    return 0U;
  }
}

/**
 * @brief       Checks signature parameters independently of the key.
 *
 * @param[in]     algorithm     Signature algorithm.
 * @param[in]     params        Signature parameters, or NULL for defaults.
 * @return                      True if the parameters are valid for the
 *                              algorithm.
 *
 * @notapi
 */
static bool cry_signature_valid(cry_algorithm_t algorithm,
                                const cry_signature_params_t *params) {
  bool pss;

  if (cry_algorithm_class(algorithm) != CRY_CLASS_SIGNATURE) {
    return false;
  }
  pss = (algorithm == CRY_ALG_RSA_PSS_SHA256) ||
        (algorithm == CRY_ALG_RSA_PSS_SHA384) ||
        (algorithm == CRY_ALG_RSA_PSS_SHA512);
  return (params == NULL) || pss || (params->salt_size == 0U);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       Initializes the Crypto subsystem.
 *
 * @init
 */
void cryInit(void) {

  cry_lld_init();
}

/**
 * @brief       Queries per-driver algorithm capabilities.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     algorithm     Explicit algorithm selector.
 * @param[out]    caps          Receives capabilities; cleared when the query
 *                              fails.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval HAL_RET_SUCCESS      The algorithm is supported, capabilities
 *                              returned.
 * @retval CRY_ERR_UNSUPPORTED  The algorithm is not supported by this driver.
 *
 * @api
 */
msg_t cryGetCapabilities(hal_crypto_driver_c *cryp, cry_algorithm_t algorithm,
                         cry_capabilities_t *caps) {
  msg_t msg;

  chDbgCheck((cryp != NULL) && (caps != NULL) &&
             (cry_algorithm_class(algorithm) != CRY_CLASS_NONE));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  memset(caps, 0, sizeof (*caps));
  msg = cry_lld_get_capabilities(cryp, algorithm, caps);
  if (msg != HAL_RET_SUCCESS) {
    memset(caps, 0, sizeof (*caps));
  }

  return msg;
}

/**
 * @brief       Loads a key.
 * @details     CRY_KEY_TRANSIENT can always be loaded: it is volatile and
 *              erased by drvStop(). Whether other identifiers can be loaded,
 *              and with which lifetime, is backend-defined; identifiers that
 *              cannot be loaded are rejected with CRY_ERR_UNSUPPORTED.
 *              Operations using a loadable key hold it, from begin to
 *              finalization or abort for streams and for the duration of the
 *              call for single-call operations; loading a key fails while it
 *              is held, so an operation always uses the key it started with.
 *              Loading and the begin using the key are not atomic: callers
 *              sharing a driver serialize them, for example with drvLock().
 *              The LLD consumes the material before returning and checks its
 *              encoding and mathematical validity. Key parameters are checked
 *              at runtime because they usually describe imported material.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a loadable
 *                              backend key.
 * @param[in]     params        Mathematical key parameters.
 * @param[in]     format        Explicit key-material encoding.
 * @param[in]     size          Encoded key material size, in bytes.
 * @param[in]     data          Encoded key material, consumed before return.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_ARGUMENT     Invalid key parameters or key material.
 * @retval CRY_ERR_UNSUPPORTED  The identifier cannot be loaded by this
 *                              backend.
 * @retval CRY_ERR_BUSY         The key is held by an operation.
 *
 * @api
 */
msg_t cryKeyLoad(hal_crypto_driver_c *cryp, crykey_t key,
                 const cry_key_params_t *params, cry_key_format_t format,
                 size_t size, const uint8_t *data) {
  msg_t msg;

  chDbgCheck((cryp != NULL) && (params != NULL) && (data != NULL) &&
             (size > 0U) &&
             ((unsigned)format <= (unsigned)CRY_KEY_FORMAT_BACKEND));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  if (!cry_key_params_valid(params)) {
    return CRY_ERR_ARGUMENT;
  }
  msg = cry_lld_key_load(cryp, key, params, format, size, data);

  return msg;
}

/**
 * @brief       Starts a hash stream.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[out]    op            Caller-owned operation context, initialized by
 *                              this call.
 * @param[in]     algorithm     Hash algorithm.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryHashBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                   cry_algorithm_t algorithm) {
  msg_t msg;

  chDbgCheck((cryp != NULL) && (op != NULL) &&
             (cry_algorithm_class(algorithm) == CRY_CLASS_HASH));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  msg = cry_lld_hash_begin(cryp, op, algorithm);
  if (msg != HAL_RET_SUCCESS) {
    /* Releasing any partial setup.*/
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Starts an unpadded symmetric cipher stream.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[out]    op            Caller-owned operation context, initialized by
 *                              this call.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Cipher algorithm.
 * @param[in]     direction     Encryption or decryption.
 * @param[in]     params        Cipher parameters, consumed before return.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryCipherBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                     crykey_t key, cry_algorithm_t algorithm,
                     cry_direction_t direction,
                     const cry_cipher_params_t *params) {
  msg_t msg;

  chDbgCheck((cryp != NULL) && (op != NULL));
  chDbgCheck((cry_algorithm_class(algorithm) == CRY_CLASS_CIPHER) &&
             ((direction == CRY_ENCRYPT) || (direction == CRY_DECRYPT)));
  chDbgCheck((params != NULL) && cry_buffer_valid(params->iv, params->iv_size));
  chDbgCheck(params->iv_size == (algorithm == CRY_ALG_AES_ECB ? 0U : 16U));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  msg = cry_lld_cipher_begin(cryp, op, key, algorithm, direction, params);
  if (msg != HAL_RET_SUCCESS) {
    /* Releasing any partial setup.*/
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Starts a MAC generation or verification stream.
 * @details     Tag size is explicit; the LLD reports unsupported tag sizes.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[out]    op            Caller-owned operation context, initialized by
 *                              this call.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     MAC algorithm.
 * @param[in]     verify        True for verification, false for generation.
 * @param[in]     tag_size      Tag size, from one byte up to the algorithm
 *                              output size.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryMacBegin(hal_crypto_driver_c *cryp, cry_operation_t *op, crykey_t key,
                  cry_algorithm_t algorithm, bool verify, size_t tag_size) {
  msg_t msg;

  chDbgCheck((cryp != NULL) && (op != NULL) &&
             (cry_algorithm_class(algorithm) == CRY_CLASS_MAC) &&
             (tag_size > 0U) && (tag_size <= cry_digest_size(algorithm)));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  msg = cry_lld_mac_begin(cryp, op, key, algorithm, verify, tag_size);
  if (msg != HAL_RET_SUCCESS) {
    /* Releasing any partial setup.*/
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Starts authenticated encryption or decryption.
 * @details     All lengths are bytes. CCM requires declared AAD and payload
 *              totals. GCM permits CRY_LENGTH_UNKNOWN. Backends enforce
 *              further limits. Decryption output must remain quarantined until
 *              cryAeadVerify() succeeds.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[out]    op            Caller-owned operation context, initialized by
 *                              this call.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     AEAD algorithm.
 * @param[in]     direction     Encryption or decryption.
 * @param[in]     params        AEAD parameters, consumed before return.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_ARGUMENT     The declared CCM payload size exceeds the
 *                              length field selected by the nonce size,
 *                              reported by the LLD.
 *
 * @api
 */
msg_t cryAeadBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                   crykey_t key, cry_algorithm_t algorithm,
                   cry_direction_t direction, const cry_aead_params_t *params) {
  msg_t msg;

  chDbgCheck((cryp != NULL) && (op != NULL));
  chDbgCheck((cry_algorithm_class(algorithm) == CRY_CLASS_AEAD) &&
             ((direction == CRY_ENCRYPT) || (direction == CRY_DECRYPT)));
  chDbgCheck((params != NULL) && (params->nonce != NULL) &&
             (params->nonce_size > 0U));
  chDbgCheck((algorithm != CRY_ALG_AES_GCM) ||
             (params->tag_size == 4U) || (params->tag_size == 8U) ||
             ((params->tag_size >= 12U) && (params->tag_size <= 16U)));
  chDbgCheck((algorithm != CRY_ALG_AES_CCM) ||
             ((params->nonce_size >= 7U) && (params->nonce_size <= 13U)));
  chDbgCheck((algorithm != CRY_ALG_AES_CCM) ||
             ((params->tag_size >= 4U) && (params->tag_size <= 16U) &&
              ((params->tag_size & 1U) == 0U)));
  chDbgCheck((algorithm != CRY_ALG_AES_CCM) ||
             ((params->aad_size != CRY_LENGTH_UNKNOWN) &&
              (params->data_size != CRY_LENGTH_UNKNOWN)));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  msg = cry_lld_aead_begin(cryp, op, key, algorithm, direction, params);
  if (msg != HAL_RET_SUCCESS) {
    /* Releasing any partial setup.*/
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Adds bytes to a hash stream.
 * @details     A backend error aborts the stream.
 *
 * @param[in,out] op            Active hash operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryHashUpdate(cry_operation_t *op, size_t size, const uint8_t *in) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(in, size));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  msg = cry_lld_hash_update(op, size, in);
  if (msg != HAL_RET_SUCCESS) {
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Adds bytes to a MAC stream.
 * @details     A backend error aborts the stream.
 *
 * @param[in,out] op            Active MAC operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryMacUpdate(cry_operation_t *op, size_t size, const uint8_t *in) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(in, size));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  msg = cry_lld_mac_update(op, size, in);
  if (msg != HAL_RET_SUCCESS) {
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Processes a cipher payload fragment.
 * @details     Input and output must not overlap unless the backend explicitly
 *              supports the overlap. A backend error aborts the stream.
 *
 * @param[in,out] op            Active cipher operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryCipherUpdate(cry_operation_t *op, size_t size, const uint8_t *in,
                      size_t out_size, uint8_t *out, size_t *out_length) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(in, size) &&
             cry_buffer_valid(out, out_size) && (out_length != NULL));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_cipher_update(op, size, in, out_size, out, out_length);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Processes an AEAD payload fragment.
 * @details     Input and output must not overlap unless the backend explicitly
 *              supports the overlap. A backend error aborts the stream. AEAD
 *              decryption output is provisional and must not be released to an
 *              untrusted consumer before verification.
 *
 * @param[in,out] op            Active AEAD operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryAeadUpdate(cry_operation_t *op, size_t size, const uint8_t *in,
                    size_t out_size, uint8_t *out, size_t *out_length) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(in, size) &&
             cry_buffer_valid(out, out_size) && (out_length != NULL));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_aead_update(op, size, in, out_size, out, out_length);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Adds associated data before the first nonempty payload
 *              fragment.
 * @details     A backend error aborts the stream.
 *
 * @param[in,out] op            Active AEAD operation context.
 * @param[in]     size          Number of associated-data bytes.
 * @param[in]     in            Associated data, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryAeadUpdateAAD(cry_operation_t *op, size_t size, const uint8_t *in) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(in, size));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  msg = cry_lld_aead_update_aad(op, size, in);
  if (msg != HAL_RET_SUCCESS) {
    cry_lld_abort(op);
  }

  return msg;
}

/**
 * @brief       Finalizes a hash stream and releases its resources.
 * @details     The context is idle on return, on success or failure.
 *
 * @param[in,out] op            Active hash operation context.
 * @param[in]     out_size      Output buffer capacity, at least the digest
 *                              size.
 * @param[out]    out           Digest output buffer.
 * @param[out]    out_length    Receives the digest size, or zero on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryHashFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                   size_t *out_length) {
  msg_t msg;

  chDbgCheck((op != NULL) && (out != NULL) && (out_length != NULL));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_hash_final(op, out_size, out, out_length);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  cry_lld_abort(op);

  return msg;
}

/**
 * @brief       Finalizes a cipher stream and releases its resources.
 * @details     The context is idle on return, on success or failure.
 *
 * @param[in,out] op            Active cipher operation context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryCipherFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(out, out_size) &&
             (out_length != NULL));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_cipher_final(op, out_size, out, out_length);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  cry_lld_abort(op);

  return msg;
}

/**
 * @brief       Finalizes a MAC generation stream and releases its resources.
 * @details     The context is idle on return, on success or failure.
 *
 * @param[in,out] op            Active MAC generation context.
 * @param[in]     out_size      Output buffer capacity, at least the tag size
 *                              selected at Begin.
 * @param[out]    out           Tag output buffer.
 * @param[out]    out_length    Receives the tag size, or zero on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryMacFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                  size_t *out_length) {
  msg_t msg;

  chDbgCheck((op != NULL) && (out != NULL) && (out_length != NULL));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_mac_final(op, out_size, out, out_length);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  cry_lld_abort(op);

  return msg;
}

/**
 * @brief       Verifies a MAC and retires the stream.
 * @details     The tag is message data: a tag whose size differs from the size
 *              selected at Begin is an authentication failure, not a
 *              programming error. The backend compares tags in constant time.
 *              The context is idle on return, on success or failure.
 *
 * @param[in,out] op            Active MAC verification context.
 * @param[in]     tag_size      Received tag size, in bytes.
 * @param[in]     tag           Received tag, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_AUTH_FAILED  The tag does not authenticate the message.
 *
 * @api
 */
msg_t cryMacVerify(cry_operation_t *op, size_t tag_size, const uint8_t *tag) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(tag, tag_size));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  msg = cry_lld_mac_verify(op, tag_size, tag);
  cry_lld_abort(op);

  return msg;
}

/**
 * @brief       Finalizes AEAD encryption and generates its tag.
 * @details     Tag_size must equal the size selected at Begin; output receives
 *              any deferred payload bytes. The context is idle on return, on
 *              success or failure.
 *
 * @param[in,out] op            Active AEAD encryption context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @param[in]     tag_size      Tag size selected at Begin, in bytes.
 * @param[out]    tag           Receives the generated tag.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryAeadFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                   size_t *out_length, size_t tag_size, uint8_t *tag) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(out, out_size) &&
             (out_length != NULL) && (tag != NULL) && (tag_size > 0U));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_aead_final(op, out_size, out, out_length, tag_size, tag);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  cry_lld_abort(op);

  return msg;
}

/**
 * @brief       Verifies the AEAD tag and retires the stream.
 * @details     Only successful verification authenticates all provisional
 *              plaintext from this operation. On failure the caller must
 *              discard and erase it. The tag is message data: a tag whose size
 *              differs from the size selected at Begin is an authentication
 *              failure. The context is idle on return, on success or failure.
 *
 * @param[in,out] op            Active AEAD decryption context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @param[in]     tag_size      Received tag size, in bytes.
 * @param[in]     tag           Received tag, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_AUTH_FAILED  The tag does not authenticate the message.
 *
 * @api
 */
msg_t cryAeadVerify(cry_operation_t *op, size_t out_size, uint8_t *out,
                    size_t *out_length, size_t tag_size, const uint8_t *tag) {
  msg_t msg;

  chDbgCheck((op != NULL) && cry_buffer_valid(out, out_size) &&
             (out_length != NULL) && cry_buffer_valid(tag, tag_size));

  chDbgAssert(cry_lld_operation_driver(op) != NULL, "idle operation");

  *out_length = 0U;
  msg = cry_lld_aead_verify(op, out_size, out, out_length, tag_size, tag);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  cry_lld_abort(op);

  return msg;
}

/**
 * @brief       Aborts any operation class; aborting an idle context is
 *              harmless.
 * @details     Valid on any context initialized by a begin function: active,
 *              finalized, failed or invalidated by drvStop(), while the driver
 *              object exists. Not valid on storage never passed to a begin
 *              function. No concurrent call may be executing on this context.
 *              On return all LLD accesses have completed, intermediate secrets
 *              are erased and the context is idle.
 *
 * @param[in,out] op            Operation context previously initialized by a
 *                              begin function.
 *
 * @api
 */
void cryOperationAbort(cry_operation_t *op) {

  chDbgCheck(op != NULL);

  cry_lld_abort(op);
}

/**
 * @brief       Signs a precomputed digest.
 * @details     The explicit algorithm includes its hash. RSA-PSS uses MGF1
 *              with that hash and an explicit salt length. RSA signatures are
 *              modulus-sized big-endian bytes; ECDSA is fixed-width big-endian
 *              r || s. Backends validate key-dependent lengths and scheme
 *              parameters.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Signature algorithm.
 * @param[in]     params        Signature parameters, or NULL for defaults
 *                              (zero salt).
 * @param[in]     size          Digest size, equal to the algorithm hash size.
 * @param[in]     in            Digest to sign.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the signature size, or zero on
 *                              failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_BUFFER       The key-dependent signature does not fit.
 *
 * @api
 */
msg_t crySignDigest(hal_crypto_driver_c *cryp, crykey_t key,
                    cry_algorithm_t algorithm,
                    const cry_signature_params_t *params, size_t size,
                    const uint8_t *in, size_t out_size, uint8_t *out,
                    size_t *out_length) {
  cry_job_t job = {0};
  msg_t msg;

  chDbgCheck((cryp != NULL) && cry_signature_valid(algorithm, params) &&
             (in != NULL) && (size == cry_digest_size(algorithm)) &&
             cry_buffer_valid(out, out_size) && (out_length != NULL));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  *out_length = 0U;
  job.kind = CRY_JOB_SIGN_DIGEST;
  job.input = in;
  job.input_size = size;
  if (params != NULL) {
    job.params.signature = *params;
  }
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  msg = cry_lld_execute(cryp, key, algorithm, &job);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }

  return msg;
}

/**
 * @brief       Signs a message using the selected signature scheme.
 * @details     The explicit algorithm includes its hash. RSA-PSS uses MGF1
 *              with that hash and an explicit salt length. RSA signatures are
 *              modulus-sized big-endian bytes; ECDSA is fixed-width big-endian
 *              r || s. Backends validate key-dependent lengths and scheme
 *              parameters.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Signature algorithm.
 * @param[in]     params        Signature parameters, or NULL for defaults
 *                              (zero salt).
 * @param[in]     size          Message size, in bytes.
 * @param[in]     in            Message, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the signature size, or zero on
 *                              failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_BUFFER       The key-dependent signature does not fit.
 *
 * @api
 */
msg_t crySignMessage(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_signature_params_t *params, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  cry_job_t job = {0};
  msg_t msg;

  chDbgCheck((cryp != NULL) && cry_signature_valid(algorithm, params) &&
             cry_buffer_valid(in, size) && cry_buffer_valid(out, out_size) &&
             (out_length != NULL));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  *out_length = 0U;
  job.kind = CRY_JOB_SIGN_MESSAGE;
  job.input = in;
  job.input_size = size;
  if (params != NULL) {
    job.params.signature = *params;
  }
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  msg = cry_lld_execute(cryp, key, algorithm, &job);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }

  return msg;
}

/**
 * @brief       Verifies a signature over a precomputed digest.
 * @details     The explicit algorithm includes its hash. RSA-PSS uses MGF1
 *              with that hash and an explicit salt length. RSA signatures are
 *              modulus-sized big-endian bytes; ECDSA is fixed-width big-endian
 *              r || s. The signature is message data: malformed or wrongly
 *              sized signatures fail verification.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Signature algorithm.
 * @param[in]     params        Signature parameters, or NULL for defaults
 *                              (zero salt).
 * @param[in]     size          Digest size, equal to the algorithm hash size.
 * @param[in]     in            Digest to verify.
 * @param[in]     signature_size Received signature size, in bytes.
 * @param[in]     signature     Received signature, or NULL when its size is
 *                              zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_AUTH_FAILED  The signature does not verify.
 *
 * @api
 */
msg_t cryVerifyDigest(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm,
                      const cry_signature_params_t *params, size_t size,
                      const uint8_t *in, size_t signature_size,
                      const uint8_t *signature) {
  cry_job_t job = {0};

  chDbgCheck((cryp != NULL) && cry_signature_valid(algorithm, params) &&
             (in != NULL) && (size == cry_digest_size(algorithm)) &&
             cry_buffer_valid(signature, signature_size));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  job.kind = CRY_JOB_VERIFY_DIGEST;
  job.input = in;
  job.input_size = size;
  if (params != NULL) {
    job.params.signature = *params;
  }
  job.signature = signature;
  job.signature_size = signature_size;
  return cry_lld_execute(cryp, key, algorithm, &job);
}

/**
 * @brief       Verifies a signature over a message.
 * @details     The explicit algorithm includes its hash. RSA-PSS uses MGF1
 *              with that hash and an explicit salt length. RSA signatures are
 *              modulus-sized big-endian bytes; ECDSA is fixed-width big-endian
 *              r || s. The signature is message data: malformed or wrongly
 *              sized signatures fail verification.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Signature algorithm.
 * @param[in]     params        Signature parameters, or NULL for defaults
 *                              (zero salt).
 * @param[in]     size          Message size, in bytes.
 * @param[in]     in            Message, or NULL when its size is zero.
 * @param[in]     signature_size Received signature size, in bytes.
 * @param[in]     signature     Received signature, or NULL when its size is
 *                              zero.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_AUTH_FAILED  The signature does not verify.
 *
 * @api
 */
msg_t cryVerifyMessage(hal_crypto_driver_c *cryp, crykey_t key,
                       cry_algorithm_t algorithm,
                       const cry_signature_params_t *params, size_t size,
                       const uint8_t *in, size_t signature_size,
                       const uint8_t *signature) {
  cry_job_t job = {0};

  chDbgCheck((cryp != NULL) && cry_signature_valid(algorithm, params) &&
             cry_buffer_valid(in, size) &&
             cry_buffer_valid(signature, signature_size));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  job.kind = CRY_JOB_VERIFY_MESSAGE;
  job.input = in;
  job.input_size = size;
  if (params != NULL) {
    job.params.signature = *params;
  }
  job.signature = signature;
  job.signature_size = signature_size;
  return cry_lld_execute(cryp, key, algorithm, &job);
}

/**
 * @brief       Encrypts using an asymmetric encryption scheme.
 * @details     RSA-OAEP uses the selected hash for both OAEP and MGF1. The
 *              backend enforces modulus-dependent bounds.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Asymmetric encryption algorithm.
 * @param[in]     params        OAEP parameters, or NULL for no label.
 * @param[in]     size          Plaintext size, in bytes.
 * @param[in]     in            Plaintext, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the ciphertext size, or zero on
 *                              failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_ARGUMENT     The plaintext exceeds the key-dependent limit.
 * @retval CRY_ERR_BUFFER       The key-dependent ciphertext does not fit.
 *
 * @api
 */
msg_t cryAsymEncrypt(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_asymmetric_params_t *params, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  cry_job_t job = {0};
  msg_t msg;

  chDbgCheck((cryp != NULL) &&
             (cry_algorithm_class(algorithm) == CRY_CLASS_ASYMMETRIC));
  chDbgCheck((params == NULL) ||
             cry_buffer_valid(params->label, params->label_size));
  chDbgCheck(cry_buffer_valid(in, size) && cry_buffer_valid(out, out_size) &&
             (out_length != NULL));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  *out_length = 0U;
  job.kind = CRY_JOB_ASYM_ENCRYPT;
  job.input = in;
  job.input_size = size;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  if (params != NULL) {
    job.params.asymmetric = *params;
  }
  msg = cry_lld_execute(cryp, key, algorithm, &job);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }

  return msg;
}

/**
 * @brief       Decrypts using an asymmetric encryption scheme.
 * @details     RSA-OAEP uses the selected hash for both OAEP and MGF1. The
 *              ciphertext is message data: the backend rejects malformed
 *              encodings with CRY_ERR_ARGUMENT without reporting padding
 *              details.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Asymmetric encryption algorithm.
 * @param[in]     params        OAEP parameters, or NULL for no label.
 * @param[in]     size          Ciphertext size, in bytes.
 * @param[in]     in            Ciphertext, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the plaintext size, or zero on
 *                              failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_ARGUMENT     Malformed ciphertext.
 * @retval CRY_ERR_BUFFER       The plaintext does not fit.
 *
 * @api
 */
msg_t cryAsymDecrypt(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_asymmetric_params_t *params, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  cry_job_t job = {0};
  msg_t msg;

  chDbgCheck((cryp != NULL) &&
             (cry_algorithm_class(algorithm) == CRY_CLASS_ASYMMETRIC));
  chDbgCheck((params == NULL) ||
             cry_buffer_valid(params->label, params->label_size));
  chDbgCheck(cry_buffer_valid(in, size) && cry_buffer_valid(out, out_size) &&
             (out_length != NULL));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  *out_length = 0U;
  job.kind = CRY_JOB_ASYM_DECRYPT;
  job.input = in;
  job.input_size = size;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  if (params != NULL) {
    job.params.asymmetric = *params;
  }
  msg = cry_lld_execute(cryp, key, algorithm, &job);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }

  return msg;
}

/**
 * @brief       Computes an ECDH shared secret.
 * @details     Peer encoding is an uncompressed SEC1 point on the private key
 *              curve. The peer point is message data: the LLD validates it and
 *              reports invalid points with CRY_ERR_ARGUMENT. Output is the
 *              full-width big-endian x coordinate. The caller owns the secret
 *              output and its lifetime.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Key agreement algorithm.
 * @param[in]     peer_size     Encoded peer public-key size, in bytes.
 * @param[in]     peer          Encoded peer public key, or NULL when its size
 *                              is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer for the shared secret.
 * @param[out]    out_length    Receives the secret length, or zero on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 * @retval CRY_ERR_ARGUMENT     Invalid peer public key.
 * @retval CRY_ERR_BUFFER       The curve-dependent secret does not fit.
 *
 * @api
 */
msg_t cryKeyAgreement(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm, size_t peer_size,
                      const uint8_t *peer, size_t out_size, uint8_t *out,
                      size_t *out_length) {
  cry_job_t job = {0};
  msg_t msg;

  chDbgCheck((cryp != NULL) &&
             (cry_algorithm_class(algorithm) == CRY_CLASS_AGREEMENT));
  chDbgCheck(cry_buffer_valid(peer, peer_size) && (out != NULL) &&
             (out_length != NULL));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  *out_length = 0U;
  job.kind = CRY_JOB_AGREEMENT;
  job.input = peer;
  job.input_size = peer_size;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  msg = cry_lld_execute(cryp, key, algorithm, &job);
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }

  return msg;
}

/**
 * @brief       Derives output bytes using HKDF.
 * @details     Performs extract and expand using the selected hash. The size
 *              parameter is the requested output length, not merely buffer
 *              capacity. The caller owns the derived bytes and their lifetime.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier, CRY_KEY_TRANSIENT or a backend
 *                              key.
 * @param[in]     algorithm     Key derivation algorithm.
 * @param[in]     params        HKDF parameters, or NULL for no salt and no
 *                              context information.
 * @param[in]     size          Number of bytes to derive, from 1 to 255 times
 *                              the hash size.
 * @param[out]    out           Output buffer with at least size bytes.
 * @return                      HAL_RET_SUCCESS or a Crypto error code.
 *
 * @api
 */
msg_t cryDeriveKey(hal_crypto_driver_c *cryp, crykey_t key,
                   cry_algorithm_t algorithm,
                   const cry_derivation_params_t *params, size_t size,
                   uint8_t *out) {
  cry_job_t job = {0};
  size_t length = 0U;

  chDbgCheck((cryp != NULL) &&
             (cry_algorithm_class(algorithm) == CRY_CLASS_DERIVATION));
  chDbgCheck((params == NULL) ||
             (cry_buffer_valid(params->salt, params->salt_size) &&
              cry_buffer_valid(params->info, params->info_size)));
  chDbgCheck((size > 0U) && (size <= 255U * cry_digest_size(algorithm)) &&
             (out != NULL));

  chDbgAssert(cryp->state == HAL_DRV_STATE_READY, "not ready");

  job.kind = CRY_JOB_DERIVE;
  if (params != NULL) {
    job.params.derivation = *params;
  }
  job.output = out;
  job.output_size = size;
  job.output_length = &length;
  return cry_lld_execute(cryp, key, algorithm, &job);
}

/*===========================================================================*/
/* Module class "hal_crypto_driver_c" methods.                               */
/*===========================================================================*/

/**
 * @name        Methods implementations of hal_crypto_driver_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p hal_crypto_driver_c instance to
 *                              be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @return                      A new reference to the object.
 */
void *__cry_objinit_impl(void *ip, const void *vmt) {
  hal_crypto_driver_c *self = (hal_crypto_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __drv_objinit_impl(self, vmt);

  /* Initialization code.*/
  cry_lld_object_init(self);

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p hal_crypto_driver_c instance to
 *                              be disposed.
 */
void __cry_dispose_impl(void *ip) {
  hal_crypto_driver_c *self = (hal_crypto_driver_c *)ip;

  /* Finalization code.*/
  chDbgAssert(self->state == HAL_DRV_STATE_STOP,
              "dispose of active crypto driver");

  /* Finalization of the ancestors-defined parts.*/
  __drv_dispose_impl(self);
}

/**
 * @brief       Override of method @p __drv_start().
 *
 * @param[in,out] ip            Pointer to a @p hal_crypto_driver_c instance.
 * @param[in]     config        Driver configuration or @p NULL.
 * @return                      The operation status.
 */
msg_t __cry_start_impl(void *ip, const void *config) {
  hal_crypto_driver_c *self = (hal_crypto_driver_c *)ip;

  if (config != NULL) {
    self->config = __cry_setcfg_impl(self, config);
  }
  else {
    self->config = __cry_selcfg_impl(self, 0U);
  }
  if (self->config == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }
  return cry_lld_start(self);
}

/**
 * @brief       Override of method @p __drv_stop().
 *
 * @param[in,out] ip            Pointer to a @p hal_crypto_driver_c instance.
 */
void __cry_stop_impl(void *ip) {
  hal_crypto_driver_c *self = (hal_crypto_driver_c *)ip;

  /* The LLD reclaims all resources and invalidates open streams.*/
  cry_lld_stop(self);
}

/**
 * @brief       Override of method @p __drv_set_cfg().
 *
 * @param[in,out] ip            Pointer to a @p hal_crypto_driver_c instance.
 * @param[in]     config        New driver configuration.
 * @return                      The configuration pointer.
 */
const void *__cry_setcfg_impl(void *ip, const void *config) {
  hal_crypto_driver_c *self = (hal_crypto_driver_c *)ip;

  /* The LLD rejects live reconfiguration while resources are in use.*/
  return cry_lld_setcfg(self, (const hal_crypto_config_t *)config);
}

/**
 * @brief       Override of method @p __drv_sel_cfg().
 *
 * @param[in,out] ip            Pointer to a @p hal_crypto_driver_c instance.
 * @param[in]     cfgnum        Driver configuration number.
 * @return                      The configuration pointer.
 */
const void *__cry_selcfg_impl(void *ip, unsigned cfgnum) {
  hal_crypto_driver_c *self = (hal_crypto_driver_c *)ip;

  /* The LLD rejects live reconfiguration while resources are in use.*/
  return cry_lld_selcfg(self, cfgnum);
}
/** @} */

/**
 * @brief       VMT structure of Crypto driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct hal_crypto_driver_vmt __hal_crypto_driver_vmt = {
  .dispose                  = __cry_dispose_impl,
  .start                    = __cry_start_impl,
  .stop                     = __cry_stop_impl,
  .setcfg                   = __cry_setcfg_impl,
  .selcfg                   = __cry_selcfg_impl
};

#endif /* HAL_USE_CRY == TRUE */

/** @} */
