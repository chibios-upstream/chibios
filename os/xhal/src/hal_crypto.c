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
 * @brief       Resets HLD stream state without modifying LLD extension fields.
 *
 * @param[in]     op            Operation context.
 *
 * @notapi
 */
static void cry_reset_operation(cry_operation_t *op) {
  op->driver = NULL;
  op->algorithm = CRY_ALG_NONE;
  op->direction = CRY_ENCRYPT;
  op->tag_size = 0U;
  op->aad_expected = 0U;
  op->data_expected = 0U;
  op->aad_count = 0U;
  op->data_count = 0U;
}

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

  if ((params == NULL) || (params->bits == 0U)) {
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
 * @param[in]     buf           Buf.
 * @param[in]     size          Number of input bytes.
 * @return                      The requested result.
 *
 * @notapi
 */
static bool cry_buffer_valid(const void *buf, size_t size) {
  return (size == 0U) || (buf != NULL);
}

/**
 * @brief       Identifies known algorithms without consulting key metadata.
 *
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @return                      The requested result.
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
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @return                      The requested result.
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
 * @brief       Accounts for an active driver call or stream.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @notapi
 */
static msg_t cry_acquire(hal_crypto_driver_c *cryp) {
  msg_t msg = HAL_RET_SUCCESS;

  if (cryp == NULL) {
    return CRY_ERR_ARGUMENT;
  }
  chSysLock();
  if ((cryp->state != HAL_DRV_STATE_READY) &&
      (cryp->state != HAL_DRV_STATE_ACTIVE)) {
    msg = HAL_RET_INV_STATE;
  }
  else if (cryp->operations == SIZE_MAX) {
    msg = HAL_RET_NO_RESOURCE;
  }
  else {
    ++cryp->operations;
    cryp->state = HAL_DRV_STATE_ACTIVE;
  }
  chSysUnlock();
  return msg;
}

/**
 * @brief       Releases an active driver call or stream.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 *
 * @notapi
 */
static void cry_release(hal_crypto_driver_c *cryp) {
  chSysLock();
  chDbgAssert(cryp->operations > 0U, "unbalanced crypto release");
  --cryp->operations;
  if ((cryp->operations == 0U) && (cryp->state == HAL_DRV_STATE_ACTIVE)) {
    cryp->state = HAL_DRV_STATE_READY;
  }
  chSysUnlock();
}

/**
 * @brief       Releases LLD stream state and returns the operation to idle.
 *
 * @param[in]     op            Initialized, caller-owned operation context.
 *
 * @notapi
 */
static void cry_cleanup(cry_operation_t *op) {
  hal_crypto_driver_c *cryp = op->driver;

  /* Cleanup must also handle partial setup and successful finalization. */
  cry_lld_abort(op);
  cry_reset_operation(op);
  cry_release(cryp);
}

/**
 * @brief       Initializes HLD stream state and invokes the LLD.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     op            Initialized, caller-owned operation context.
 * @param[in]     cl            Cl.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     direction     Encryption or decryption.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @notapi
 */
static msg_t cry_stream_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                              cry_class_t cl, cry_algorithm_t algorithm,
                              crykey_t key, cry_direction_t direction,
                              const cry_stream_params_t *params) {
  msg_t msg;

  if ((op == NULL) || (cry_algorithm_class(algorithm) != cl)) {
    return CRY_ERR_ARGUMENT;
  }
  if (op->driver != NULL) {
    return CRY_ERR_STATE;
  }
  if ((cl != CRY_CLASS_HASH) && (key == CRY_KEY_INVALID)) {
    return CRY_ERR_KEY;
  }
  msg = cry_acquire(cryp);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  cry_reset_operation(op);
  op->driver = cryp;
  op->algorithm = algorithm;
  op->direction = direction;
  if (cl == CRY_CLASS_AEAD) {
    op->tag_size = params->aead.tag_size;
    op->aad_expected = params->aead.aad_size;
    op->data_expected = params->aead.data_size;
  }
  else if (cl == CRY_CLASS_MAC) {
    op->tag_size = params->mac_tag_size;
  }
  msg = cry_lld_begin(op, key, params);
  if (msg != HAL_RET_SUCCESS) {
    cry_cleanup(op);
  }
  return msg;
}

/**
 * @brief       Checks that the stream is active and belongs to the requested
 *              class.
 *
 * @param[in]     op            Initialized, caller-owned operation context.
 * @param[in]     cl            Cl.
 * @return                      The requested result.
 *
 * @notapi
 */
static bool cry_stream_valid(const cry_operation_t *op, cry_class_t cl) {
  return (op != NULL) && (op->driver != NULL) &&
         (cry_algorithm_class(op->algorithm) == cl);
}

/**
 * @brief       Dispatches a stream fragment with length and phase accounting.
 *
 * @param[in]     op            Initialized, caller-owned operation context.
 * @param[in]     cl            Cl.
 * @param[in]     aad           Aad.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[in]     out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @notapi
 */
static msg_t cry_stream_update(cry_operation_t *op, cry_class_t cl, bool aad,
                               size_t size, const uint8_t *in, size_t out_size,
                               uint8_t *out, size_t *out_length) {
  msg_t msg;
  size_t *count = NULL;
  size_t expected;

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (!cry_stream_valid(op, cl)) {
    return CRY_ERR_STATE;
  }
  if ((out_length == NULL) || !cry_buffer_valid(in, size) ||
      !cry_buffer_valid(out, out_size)) {
    return CRY_ERR_ARGUMENT;
  }
  if (cl == CRY_CLASS_AEAD) {
    count = aad ? &op->aad_count : &op->data_count;
    if (size > SIZE_MAX - *count) {
      return CRY_ERR_ARGUMENT;
    }
    expected = aad ? op->aad_expected : op->data_expected;
    if ((aad && (op->data_count != 0U)) ||
        (!aad && (size != 0U) &&
         (op->aad_expected != CRY_LENGTH_UNKNOWN) &&
         (op->aad_count != op->aad_expected)) ||
        ((expected != CRY_LENGTH_UNKNOWN) &&
         ((*count > expected) || (size > expected - *count)))) {
      return CRY_ERR_ARGUMENT;
    }
  }
  msg = cry_lld_update(op, aad, size, in, out_size, out, out_length);
  if ((msg == HAL_RET_SUCCESS) && (*out_length > out_size)) {
    msg = CRY_ERR_FAILURE;
  }
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
    cry_cleanup(op);
  }
  else if (count != NULL) {
    *count += size;
  }
  return msg;
}

/**
 * @brief       Finalizes and retires a stream on either backend success or
 *              failure.
 *
 * @param[in]     op            Initialized, caller-owned operation context.
 * @param[in]     cl            Cl.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[in]     out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @param[in]     tag           Authentication tag to generate or verify.
 * @param[in]     expected_tag  Expected tag.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @notapi
 */
static msg_t cry_stream_final(cry_operation_t *op, cry_class_t cl,
                              size_t out_size, uint8_t *out,
                              size_t *out_length, uint8_t *tag,
                              const uint8_t *expected_tag) {
  msg_t msg;

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (!cry_stream_valid(op, cl)) {
    return CRY_ERR_STATE;
  }
  if ((out_length == NULL) || !cry_buffer_valid(out, out_size)) {
    return CRY_ERR_ARGUMENT;
  }
  if (cl == CRY_CLASS_AEAD) {
    if (((op->aad_expected != CRY_LENGTH_UNKNOWN) &&
         (op->aad_count != op->aad_expected)) ||
        ((op->data_expected != CRY_LENGTH_UNKNOWN) &&
         (op->data_count != op->data_expected))) {
      return CRY_ERR_ARGUMENT;
    }
  }
  msg = cry_lld_final(op, out_size, out, out_length, tag, expected_tag);
  if ((msg == HAL_RET_SUCCESS) && (*out_length > out_size)) {
    msg = CRY_ERR_FAILURE;
  }
  if ((msg == HAL_RET_SUCCESS) &&
      (((cl == CRY_CLASS_HASH) &&
        (*out_length != cry_digest_size(op->algorithm))) ||
       ((cl == CRY_CLASS_MAC) && (expected_tag == NULL) &&
        (*out_length != op->tag_size)))) {
    msg = CRY_ERR_FAILURE;
  }
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  cry_cleanup(op);
  return msg;
}

/**
 * @brief       Executes a single-call operation through the LLD.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     cl            Cl.
 * @param[in]     job           Job.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @notapi
 */
static msg_t cry_execute(hal_crypto_driver_c *cryp, crykey_t key,
                         cry_algorithm_t algorithm, cry_class_t cl,
                         const cry_job_t *job) {
  msg_t msg;

  if (cry_algorithm_class(algorithm) != cl) {
    return CRY_ERR_ARGUMENT;
  }
  if (key == CRY_KEY_INVALID) {
    return CRY_ERR_KEY;
  }
  msg = cry_acquire(cryp);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }
  msg = cry_lld_execute(cryp, key, algorithm, job);
  if ((msg == HAL_RET_SUCCESS) && (job->output_length != NULL) &&
      ((*job->output_length > job->output_size) ||
       ((job->kind == CRY_JOB_DERIVE) &&
        (*job->output_length != job->output_size)))) {
    msg = CRY_ERR_FAILURE;
  }
  if ((msg != HAL_RET_SUCCESS) && (job->output_length != NULL)) {
    *job->output_length = 0U;
  }
  cry_release(cryp);
  return msg;
}

/**
 * @brief       Checks signature parameters independently of the key.
 *
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @return                      The requested result.
 *
 * @notapi
 */
static bool cry_signature_valid(cry_algorithm_t algorithm,
                                const cry_signature_params_t *params) {
  bool pss;

  if ((cry_algorithm_class(algorithm) != CRY_CLASS_SIGNATURE) || (params == NULL)) {
    return false;
  }
  pss = (algorithm == CRY_ALG_RSA_PSS_SHA256) ||
        (algorithm == CRY_ALG_RSA_PSS_SHA384) ||
        (algorithm == CRY_ALG_RSA_PSS_SHA512);
  return pss || (params->salt_size == 0U);
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
 * @brief       Initializes a caller-owned operation context.
 * @details     Use only on fresh storage or an already idle context. Live
 *              contexts must first be aborted. Initializes HLD state and
 *              invokes the LLD operation initializer.
 *
 * @param[out]    op            Initialized, caller-owned operation context.
 *
 * @api
 */
void cryOperationObjectInit(cry_operation_t *op) {
  chDbgCheck(op != NULL);
  cry_reset_operation(op);
  cry_lld_operation_init(op);
}

/**
 * @brief       Queries per-driver algorithm capabilities.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[out]    caps          Receives capabilities; cleared when the query
 *                              fails.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryGetCapabilities(hal_crypto_driver_c *cryp, cry_algorithm_t algorithm,
                         cry_capabilities_t *caps) {
  msg_t msg;

  if (caps == NULL) {
    return CRY_ERR_ARGUMENT;
  }
  memset(caps, 0, sizeof (*caps));
  if (cry_algorithm_class(algorithm) == CRY_CLASS_NONE) {
    return CRY_ERR_UNSUPPORTED;
  }
  msg = cry_acquire(cryp);
  if (msg == HAL_RET_SUCCESS) {
    msg = cry_lld_get_capabilities(cryp, algorithm, caps);
    if (msg != HAL_RET_SUCCESS) {
      memset(caps, 0, sizeof (*caps));
    }
    cry_release(cryp);
  }
  return msg;
}

/**
 * @brief       Loads key material for subsequent cryptographic operations.
 * @details     The LLD defines supported identifiers and loading or
 *              replacement behavior. It consumes the supplied material before
 *              returning and checks its encoding and mathematical validity.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     params        Mathematical key parameters.
 * @param[in]     format        Explicit key-material encoding.
 * @param[in]     size          Number of input bytes.
 * @param[in]     data          Encoded key material, consumed before return.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryKeyLoad(hal_crypto_driver_c *cryp, crykey_t key,
                 const cry_key_params_t *params, cry_key_format_t format,
                 size_t size, const uint8_t *data) {
  msg_t msg;

  if ((key == CRY_KEY_INVALID) || !cry_key_params_valid(params) ||
      !cry_buffer_valid(data, size) || (size == 0U) ||
      ((unsigned)format > (unsigned)CRY_KEY_FORMAT_BACKEND)) {
    return CRY_ERR_ARGUMENT;
  }
  msg = cry_acquire(cryp);
  if (msg == HAL_RET_SUCCESS) {
    msg = cry_lld_key_load(cryp, key, params, format, size, data);
    cry_release(cryp);
  }
  return msg;
}

/**
 * @brief       Generates key material for cryptographic operations.
 * @details     The LLD defines supported identifiers, key types and sizes. Key
 *              generation requires a suitable random source and reports
 *              entropy failures.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     params        Mathematical key parameters.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryKeyGenerate(hal_crypto_driver_c *cryp, crykey_t key,
                     const cry_key_params_t *params) {
  msg_t msg;

  if ((key == CRY_KEY_INVALID) || !cry_key_params_valid(params) ||
      (params->type == CRY_KEY_RSA_PUBLIC) ||
      (params->type == CRY_KEY_ECC_PUBLIC)) {
    return CRY_ERR_ARGUMENT;
  }
  msg = cry_acquire(cryp);
  if (msg == HAL_RET_SUCCESS) {
    msg = cry_lld_key_generate(cryp, key, params);
    cry_release(cryp);
  }
  return msg;
}

/**
 * @brief       Releases key material loaded or generated through the driver.
 * @details     The LLD defines which identifiers can be unloaded. Loading,
 *              generation and unloading must not change the key used by an
 *              already active operation.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryKeyUnload(hal_crypto_driver_c *cryp, crykey_t key) {
  msg_t msg;

  if (key == CRY_KEY_INVALID) {
    return CRY_ERR_KEY;
  }
  msg = cry_acquire(cryp);
  if (msg == HAL_RET_SUCCESS) {
    msg = cry_lld_key_unload(cryp, key);
    cry_release(cryp);
  }
  return msg;
}

/**
 * @brief       Exports an asymmetric public key; never private key material.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     format        Explicit key-material encoding.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryKeyExportPublic(hal_crypto_driver_c *cryp, crykey_t key,
                         cry_key_format_t format, size_t out_size,
                         uint8_t *out, size_t *out_length) {
  msg_t msg;

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if ((key == CRY_KEY_INVALID) || (out_length == NULL) ||
      !cry_buffer_valid(out, out_size) ||
      ((format != CRY_KEY_FORMAT_SEC1) && (format != CRY_KEY_FORMAT_PKCS1_DER))) {
    return CRY_ERR_ARGUMENT;
  }
  msg = cry_acquire(cryp);
  if (msg == HAL_RET_SUCCESS) {
    msg = cry_lld_key_export_public(cryp, key, format, out_size, out, out_length);
    if ((msg == HAL_RET_SUCCESS) && (*out_length > out_size)) {
      msg = CRY_ERR_FAILURE;
    }
    cry_release(cryp);
  }
  if (msg != HAL_RET_SUCCESS) {
    *out_length = 0U;
  }
  return msg;
}

/**
 * @brief       Starts a hash stream.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryHashBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                   cry_algorithm_t algorithm) {
  return cry_stream_begin(cryp, op, CRY_CLASS_HASH, algorithm,
                          CRY_KEY_INVALID, CRY_ENCRYPT, NULL);
}

/**
 * @brief       Starts an unpadded symmetric cipher stream.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     direction     Encryption or decryption.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryCipherBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                     crykey_t key, cry_algorithm_t algorithm,
                     cry_direction_t direction,
                     const cry_cipher_params_t *params) {
  cry_stream_params_t setup;

  if ((cry_algorithm_class(algorithm) != CRY_CLASS_CIPHER) ||
      (params == NULL) ||
      ((direction != CRY_ENCRYPT) && (direction != CRY_DECRYPT)) ||
      !cry_buffer_valid(params->iv, params->iv_size) ||
      (params->iv_size != (algorithm == CRY_ALG_AES_ECB ? 0U : 16U))) {
    return CRY_ERR_ARGUMENT;
  }
  setup.cipher = *params;
  return cry_stream_begin(cryp, op, CRY_CLASS_CIPHER, algorithm, key,
                          direction, &setup);
}

/**
 * @brief       Starts a MAC generation or verification stream.
 * @details     Tag size is explicit; the LLD reports unsupported tag sizes.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     verify        True for verification, false for generation.
 * @param[in]     tag_size      Authentication tag size, in bytes.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryMacBegin(hal_crypto_driver_c *cryp, cry_operation_t *op, crykey_t key,
                  cry_algorithm_t algorithm, bool verify, size_t tag_size) {
  cry_stream_params_t setup;

  if ((cry_algorithm_class(algorithm) != CRY_CLASS_MAC) ||
      (tag_size == 0U) || (tag_size > cry_digest_size(algorithm))) {
    return CRY_ERR_ARGUMENT;
  }
  setup.mac_tag_size = tag_size;
  return cry_stream_begin(cryp, op, CRY_CLASS_MAC, algorithm, key,
                          verify ? CRY_DECRYPT : CRY_ENCRYPT, &setup);
}

/**
 * @brief       Starts authenticated encryption or decryption.
 * @details     All lengths are bytes. CCM requires declared AAD and payload
 *              totals. GCM permits CRY_LENGTH_UNKNOWN. Backends enforce
 *              further limits. Decryption output must remain quarantined until
 *              cryAeadVerify() succeeds.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     direction     Encryption or decryption.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAeadBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                   crykey_t key, cry_algorithm_t algorithm,
                   cry_direction_t direction, const cry_aead_params_t *params) {
  cry_stream_params_t setup;
  size_t length_bytes;
  size_t remaining;

  if ((cry_algorithm_class(algorithm) != CRY_CLASS_AEAD) ||
      (params == NULL) ||
      ((direction != CRY_ENCRYPT) && (direction != CRY_DECRYPT)) ||
      !cry_buffer_valid(params->nonce, params->nonce_size) ||
      (params->nonce_size == 0U)) {
    return CRY_ERR_ARGUMENT;
  }
  if (algorithm == CRY_ALG_AES_GCM) {
    if ((params->tag_size != 4U) && (params->tag_size != 8U) &&
        ((params->tag_size < 12U) || (params->tag_size > 16U))) {
      return CRY_ERR_ARGUMENT;
    }
  }
  else {
    if ((params->nonce_size < 7U) || (params->nonce_size > 13U) ||
        (params->tag_size < 4U) || (params->tag_size > 16U) ||
        ((params->tag_size & 1U) != 0U) ||
        (params->aad_size == CRY_LENGTH_UNKNOWN) ||
        (params->data_size == CRY_LENGTH_UNKNOWN)) {
      return CRY_ERR_ARGUMENT;
    }
    /* Avoid word-width-dependent shifts in the CCM length-field check. */
    remaining = params->data_size;
    for (length_bytes = 15U - params->nonce_size; length_bytes > 0U; --length_bytes) {
      remaining >>= 8;
    }
    if (remaining != 0U) {
      return CRY_ERR_ARGUMENT;
    }
  }
  setup.aead = *params;
  return cry_stream_begin(cryp, op, CRY_CLASS_AEAD, algorithm, key,
                          direction, &setup);
}

/**
 * @brief       Adds bytes to a hash stream.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryHashUpdate(cry_operation_t *op, size_t size, const uint8_t *in) {
  size_t ignored;

  return cry_stream_update(op, CRY_CLASS_HASH, false, size, in, 0U, NULL, &ignored);
}

/**
 * @brief       Adds bytes to a MAC stream.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryMacUpdate(cry_operation_t *op, size_t size, const uint8_t *in) {
  size_t ignored;

  return cry_stream_update(op, CRY_CLASS_MAC, false, size, in, 0U, NULL, &ignored);
}

/**
 * @brief       Processes a cipher payload fragment.
 * @details     Input and output must not overlap unless the backend explicitly
 *              supports the overlap. Backend errors abort the context. AEAD
 *              decryption output is provisional and must not be released to an
 *              untrusted consumer before verification.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryCipherUpdate(cry_operation_t *op, size_t size, const uint8_t *in,
                      size_t out_size, uint8_t *out, size_t *out_length) {
  return cry_stream_update(op, CRY_CLASS_CIPHER, false, size, in,
                           out_size, out, out_length);
}

/**
 * @brief       Processes an AEAD payload fragment.
 * @details     Input and output must not overlap unless the backend explicitly
 *              supports the overlap. Backend errors abort the context. AEAD
 *              decryption output is provisional and must not be released to an
 *              untrusted consumer before verification.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAeadUpdate(cry_operation_t *op, size_t size, const uint8_t *in,
                    size_t out_size, uint8_t *out, size_t *out_length) {
  return cry_stream_update(op, CRY_CLASS_AEAD, false, size, in,
                           out_size, out, out_length);
}

/**
 * @brief       Adds associated data before the first nonempty payload
 *              fragment.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAeadUpdateAAD(cry_operation_t *op, size_t size, const uint8_t *in) {
  size_t ignored;

  return cry_stream_update(op, CRY_CLASS_AEAD, true, size, in, 0U, NULL, &ignored);
}

/**
 * @brief       Finalizes a hash stream and releases its resources.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryHashFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                   size_t *out_length) {
  if (cry_stream_valid(op, CRY_CLASS_HASH) &&
      (out_size < cry_digest_size(op->algorithm))) {
    if (out_length != NULL) {
      *out_length = 0U;
    }
    return CRY_ERR_BUFFER;
  }
  return cry_stream_final(op, CRY_CLASS_HASH, out_size, out, out_length, NULL, NULL);
}

/**
 * @brief       Finalizes a cipher stream and releases its resources.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryCipherFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  return cry_stream_final(op, CRY_CLASS_CIPHER, out_size, out, out_length, NULL, NULL);
}

/**
 * @brief       Finalizes a MAC stream and releases its resources.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryMacFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                  size_t *out_length) {
  if (cry_stream_valid(op, CRY_CLASS_MAC)) {
    if (out_length != NULL) {
      *out_length = 0U;
    }
    if (op->direction != CRY_ENCRYPT) {
      return CRY_ERR_STATE;
    }
    if (out_size < op->tag_size) {
      return CRY_ERR_BUFFER;
    }
  }
  return cry_stream_final(op, CRY_CLASS_MAC, out_size, out, out_length, NULL, NULL);
}

/**
 * @brief       Verifies a MAC and retires the stream.
 * @details     The backend compares tags in constant time. A mismatching tag
 *              returns CRY_ERR_AUTH_FAILED.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     tag_size      Authentication tag size, in bytes.
 * @param[in]     tag           Authentication tag to generate or verify.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryMacVerify(cry_operation_t *op, size_t tag_size, const uint8_t *tag) {
  size_t ignored;

  if (!cry_stream_valid(op, CRY_CLASS_MAC) || (op->direction != CRY_DECRYPT)) {
    return CRY_ERR_STATE;
  }
  if ((tag == NULL) || (tag_size != op->tag_size)) {
    return CRY_ERR_ARGUMENT;
  }
  return cry_stream_final(op, CRY_CLASS_MAC, 0U, NULL, &ignored, NULL, tag);
}

/**
 * @brief       Finalizes AEAD encryption and generates its tag.
 * @details     Tag_size must equal the size selected at Begin; output receives
 *              any deferred payload bytes.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @param[in]     tag_size      Authentication tag size, in bytes.
 * @param[out]    tag           Authentication tag to generate or verify.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAeadFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                   size_t *out_length, size_t tag_size, uint8_t *tag) {
  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (!cry_stream_valid(op, CRY_CLASS_AEAD) ||
      (op->direction != CRY_ENCRYPT)) {
    return CRY_ERR_STATE;
  }
  if ((tag == NULL) || (tag_size != op->tag_size)) {
    return CRY_ERR_ARGUMENT;
  }
  return cry_stream_final(op, CRY_CLASS_AEAD, out_size, out, out_length,
                          tag, NULL);
}

/**
 * @brief       Verifies the AEAD tag and retires the stream.
 * @details     Only successful verification authenticates all provisional
 *              plaintext from this operation. On failure the caller must
 *              discard and erase it.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @param[in]     tag_size      Authentication tag size, in bytes.
 * @param[in]     tag           Authentication tag to generate or verify.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAeadVerify(cry_operation_t *op, size_t out_size, uint8_t *out,
                    size_t *out_length, size_t tag_size, const uint8_t *tag) {
  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (!cry_stream_valid(op, CRY_CLASS_AEAD) ||
      (op->direction != CRY_DECRYPT)) {
    return CRY_ERR_STATE;
  }
  if ((tag == NULL) || (tag_size != op->tag_size)) {
    return CRY_ERR_ARGUMENT;
  }
  return cry_stream_final(op, CRY_CLASS_AEAD, out_size, out, out_length,
                          NULL, tag);
}

/**
 * @brief       Aborts any operation class; aborting an idle context is
 *              harmless.
 * @details     No concurrent call may be executing on this context. On return
 *              all LLD accesses have completed and the context is reusable.
 *              The LLD owns cleanup of its extension fields.
 *
 * @param[in,out] op            Initialized, caller-owned operation context.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryOperationAbort(cry_operation_t *op) {
  if (op == NULL) {
    return CRY_ERR_ARGUMENT;
  }
  if (op->driver != NULL) {
    cry_cleanup(op);
  }
  return HAL_RET_SUCCESS;
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
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t crySignDigest(hal_crypto_driver_c *cryp, crykey_t key,
                    cry_algorithm_t algorithm,
                    const cry_signature_params_t *params, size_t size,
                    const uint8_t *in, size_t out_size, uint8_t *out,
                    size_t *out_length) {
  cry_job_t job = {0};

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (!cry_signature_valid(algorithm, params) || !cry_buffer_valid(in, size) ||
      (out_length == NULL) || !cry_buffer_valid(out, out_size) ||
      (size != cry_digest_size(algorithm))) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_SIGN_DIGEST;
  job.input = in;
  job.input_size = size;
  job.params.signature = *params;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_SIGNATURE, &job);
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
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t crySignMessage(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_signature_params_t *params, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  cry_job_t job = {0};

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (!cry_signature_valid(algorithm, params) || !cry_buffer_valid(in, size) ||
      (out_length == NULL) || !cry_buffer_valid(out, out_size)) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_SIGN_MESSAGE;
  job.input = in;
  job.input_size = size;
  job.params.signature = *params;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_SIGNATURE, &job);
}

/**
 * @brief       Verifies a precomputed digest.
 * @details     The explicit algorithm includes its hash. RSA-PSS uses MGF1
 *              with that hash and an explicit salt length. RSA signatures are
 *              modulus-sized big-endian bytes; ECDSA is fixed-width big-endian
 *              r || s. Backends validate key-dependent lengths and scheme
 *              parameters.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     signature_size Signature size, in bytes.
 * @param[in]     signature     Signature to verify.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryVerifyDigest(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm,
                      const cry_signature_params_t *params, size_t size,
                      const uint8_t *in, size_t signature_size,
                      const uint8_t *signature) {
  cry_job_t job = {0};

  if (!cry_signature_valid(algorithm, params) || !cry_buffer_valid(in, size) ||
      !cry_buffer_valid(signature, signature_size) || (signature_size == 0U) ||
      (size != cry_digest_size(algorithm))) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_VERIFY_DIGEST;
  job.input = in;
  job.input_size = size;
  job.params.signature = *params;
  job.signature = signature;
  job.signature_size = signature_size;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_SIGNATURE, &job);
}

/**
 * @brief       Verifies a message using the selected signature scheme.
 * @details     The explicit algorithm includes its hash. RSA-PSS uses MGF1
 *              with that hash and an explicit salt length. RSA signatures are
 *              modulus-sized big-endian bytes; ECDSA is fixed-width big-endian
 *              r || s. Backends validate key-dependent lengths and scheme
 *              parameters.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     signature_size Signature size, in bytes.
 * @param[in]     signature     Signature to verify.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryVerifyMessage(hal_crypto_driver_c *cryp, crykey_t key,
                       cry_algorithm_t algorithm,
                       const cry_signature_params_t *params, size_t size,
                       const uint8_t *in, size_t signature_size,
                       const uint8_t *signature) {
  cry_job_t job = {0};

  if (!cry_signature_valid(algorithm, params) || !cry_buffer_valid(in, size) ||
      !cry_buffer_valid(signature, signature_size) || (signature_size == 0U)) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_VERIFY_MESSAGE;
  job.input = in;
  job.input_size = size;
  job.params.signature = *params;
  job.signature = signature;
  job.signature_size = signature_size;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_SIGNATURE, &job);
}

/**
 * @brief       Encrypts using an asymmetric encryption scheme.
 * @details     RSA-OAEP uses the selected hash for both OAEP and MGF1. The
 *              backend enforces modulus-dependent bounds and rejects malformed
 *              encodings without reporting padding details.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAsymEncrypt(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_asymmetric_params_t *params, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  cry_job_t job = {0};

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if ((cry_algorithm_class(algorithm) != CRY_CLASS_ASYMMETRIC) ||
      (params == NULL) || !cry_buffer_valid(params->label, params->label_size) ||
      !cry_buffer_valid(in, size) || !cry_buffer_valid(out, out_size) ||
      (out_length == NULL)) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_ASYM_ENCRYPT;
  job.input = in;
  job.input_size = size;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  job.params.asymmetric = *params;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_ASYMMETRIC, &job);
}

/**
 * @brief       Decrypts using an asymmetric encryption scheme.
 * @details     RSA-OAEP uses the selected hash for both OAEP and MGF1. The
 *              backend enforces modulus-dependent bounds and rejects malformed
 *              encodings without reporting padding details.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of input bytes.
 * @param[in]     in            Input buffer, or NULL when its size is zero.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer, or NULL when its capacity is
 *                              zero.
 * @param[out]    out_length    Receives the number of bytes produced, or zero
 *                              on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryAsymDecrypt(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_asymmetric_params_t *params, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {
  cry_job_t job = {0};

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if ((cry_algorithm_class(algorithm) != CRY_CLASS_ASYMMETRIC) ||
      (params == NULL) || !cry_buffer_valid(params->label, params->label_size) ||
      !cry_buffer_valid(in, size) || !cry_buffer_valid(out, out_size) ||
      (out_length == NULL)) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_ASYM_DECRYPT;
  job.input = in;
  job.input_size = size;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  job.params.asymmetric = *params;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_ASYMMETRIC, &job);
}

/**
 * @brief       Computes an ECDH shared secret.
 * @details     Peer encoding is an uncompressed SEC1 point on the private key
 *              curve. The LLD validates the point and secret. Output is the
 *              full-width big-endian x coordinate. The caller owns the secret
 *              output and its lifetime.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     peer_size     Encoded peer public-key size, in bytes.
 * @param[in]     peer          Encoded peer public key.
 * @param[in]     out_size      Output buffer capacity, in bytes.
 * @param[out]    out           Output buffer for the shared secret.
 * @param[out]    out_length    Receives the secret length, or zero on failure.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryKeyAgreement(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm, size_t peer_size,
                      const uint8_t *peer, size_t out_size, uint8_t *out,
                      size_t *out_length) {
  cry_job_t job = {0};

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if ((cry_algorithm_class(algorithm) != CRY_CLASS_AGREEMENT) ||
      !cry_buffer_valid(peer, peer_size) || (peer_size == 0U) ||
      !cry_buffer_valid(out, out_size) || (out_length == NULL)) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_AGREEMENT;
  job.input = peer;
  job.input_size = peer_size;
  job.output = out;
  job.output_size = out_size;
  job.output_length = out_length;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_AGREEMENT, &job);
}

/**
 * @brief       Derives output bytes using HKDF.
 * @details     Performs extract and expand using the selected hash. The size
 *              parameter is the requested output length, not merely buffer
 *              capacity. The caller owns the derived bytes and their lifetime.
 *
 * @param[in]     cryp          Pointer to the Crypto driver.
 * @param[in]     key           Key identifier interpreted by the LLD.
 * @param[in]     algorithm     Explicit algorithm selector for this operation
 *                              class.
 * @param[in]     params        Parameters for the selected operation class and
 *                              algorithm.
 * @param[in]     size          Number of bytes to derive, from 1 to 255 times
 *                              the hash size.
 * @param[out]    out           Output buffer with at least size bytes.
 * @return                      HAL_RET_SUCCESS or a Crypto/HAL error code.
 *
 * @api
 */
msg_t cryDeriveKey(hal_crypto_driver_c *cryp, crykey_t key,
                   cry_algorithm_t algorithm,
                   const cry_derivation_params_t *params, size_t size,
                   uint8_t *out) {
  cry_job_t job = {0};
  size_t length = 0U;

  if ((cry_algorithm_class(algorithm) != CRY_CLASS_DERIVATION) ||
      (params == NULL) || !cry_buffer_valid(params->salt, params->salt_size) ||
      !cry_buffer_valid(params->info, params->info_size) ||
      (size == 0U) || (size > 255U * cry_digest_size(algorithm)) || (out == NULL)) {
    return CRY_ERR_ARGUMENT;
  }
  job.kind = CRY_JOB_DERIVE;
  job.params.derivation = *params;
  job.output = out;
  job.output_size = size;
  job.output_length = &length;
  return cry_execute(cryp, key, algorithm, CRY_CLASS_DERIVATION, &job);
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
  self->operations = 0U;
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
  chDbgAssert((self->state == HAL_DRV_STATE_STOP) && (self->operations == 0U),
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
  chDbgAssert(self->operations == 0U, "active crypto operations");
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
  if (self->operations != 0U) {
    return NULL;
  }
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
  if (self->operations != 0U) {
    return NULL;
  }
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
