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
 * @file    hal_crypto_lld.c
 * @brief   Crypto backend template. No cryptographic capabilities are provided.
 * @addtogroup HAL_CRYPTO
 * @{
 */
#include "hal.h"
#include <string.h>

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

static const hal_crypto_config_t cry_default_config = {0};

/**
 * @brief   Initializes the Crypto low-level driver.
 * @details Initialize any driver objects provided by the port using
 *          cryObjectInit(). Hardware is started through the driver start
 *          hook.
 *
 * @notapi
 */
void cry_lld_init(void) {

}

/**
 * @brief   Initializes LLD-specific driver fields.
 * @details Called for every object initialized with cryObjectInit().
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @notapi
 */
void cry_lld_object_init(hal_crypto_driver_c *cryp) {

  (void)cryp;
}

/**
 * @brief   Initializes LLD-specific operation fields.
 * @details Called by cryOperationObjectInit() after common fields are
 *          initialized. The HLD leaves extension fields untouched during
 *          begin, finalization and abort; their initialization, reuse and
 *          cleanup belong to the LLD.
 *
 * @param[in,out] op Initialized operation context.
 * @notapi
 */
void cry_lld_operation_init(cry_operation_t *op) {

  (void)op;
}

/**
 * @brief   Starts the Crypto driver.
 * @details Called in unlocked thread context with the driver in STARTING
 *          state and its configuration selected. The template provides no
 *          implementation.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_start(hal_crypto_driver_c *cryp) {

  (void)cryp;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Stops an idle Crypto driver.
 * @details Called in thread context after all driver calls and streams have
 *          finished. Release resources owned by this driver.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @notapi
 */
void cry_lld_stop(hal_crypto_driver_c *cryp) {

  (void)cryp;
}

/**
 * @brief   Selects a Crypto configuration.
 * @details Called during start, unlocked and with no stream or call active,
 *          or through the base-driver live configuration APIs with the system
 *          lock held. Must not block. Return NULL for unsupported
 *          configurations.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] config Requested configuration.
 * @return  The selected configuration, or NULL.
 * @notapi
 */
const hal_crypto_config_t *cry_lld_setcfg(hal_crypto_driver_c *cryp,
                                          const hal_crypto_config_t *config) {

  (void)cryp;

  return config;
}

/**
 * @brief   Selects a configuration by index.
 * @details Same nonblocking contract as cry_lld_setcfg(). Index zero selects
 *          the default configuration.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] cfgnum Configuration index.
 * @return  The selected configuration, or NULL.
 * @notapi
 */
const hal_crypto_config_t *cry_lld_selcfg(hal_crypto_driver_c *cryp,
                                          unsigned cfgnum) {

  (void)cryp;

  return cfgnum == 0U ? &cry_default_config : NULL;
}

/**
 * @brief   Queries algorithm capabilities.
 * @details Report capabilities supported by this driver. The template
 *          advertises none.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] algorithm Explicit algorithm selector.
 * @param[out] caps Algorithm capabilities.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_get_capabilities(hal_crypto_driver_c *cryp,
                               cry_algorithm_t algorithm,
                               cry_capabilities_t *caps) {

  (void)cryp;
  (void)algorithm;

  memset(caps, 0, sizeof (*caps));
  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Loads key material for cryptographic operations.
 * @details Interpret the key identifier and validate material, encoding and
 *          parameters. Consume the input before return. Supported identifiers
 *          and loading or replacement behavior are defined by the LLD.
 *          Loading must not change the key used by an active operation.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier interpreted by the LLD.
 * @param[in] params Mathematical key parameters.
 * @param[in] format Key material encoding.
 * @param[in] size Encoded key length, in bytes.
 * @param[in] data Encoded key material.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_key_load(hal_crypto_driver_c *cryp, crykey_t key,
                       const cry_key_params_t *params,
                       cry_key_format_t format, size_t size,
                       const uint8_t *data) {

  (void)cryp;
  (void)key;
  (void)params;
  (void)format;
  (void)size;
  (void)data;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Generates key material.
 * @details Validate supported key type and size and use a suitable random
 *          source. Propagate entropy failures. Generation must not change the
 *          key used by an active operation.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier interpreted by the LLD.
 * @param[in] params Mathematical key parameters.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_key_generate(hal_crypto_driver_c *cryp, crykey_t key,
                           const cry_key_params_t *params) {

  (void)cryp;
  (void)key;
  (void)params;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Releases loaded or generated key material.
 * @details The LLD defines supported identifiers and cleanup behavior.
 *          Unloading must not invalidate an active operation.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier interpreted by the LLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_key_unload(hal_crypto_driver_c *cryp, crykey_t key) {

  (void)cryp;
  (void)key;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Exports the public component of an asymmetric key.
 * @details Check the key type, requested format and output bounds. This
 *          operation never exports private or symmetric key material.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier interpreted by the LLD.
 * @param[in] format Key material encoding.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer.
 * @param[out] out_length Actual output length, or zero on failure.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_key_export_public(hal_crypto_driver_c *cryp, crykey_t key,
                                cry_key_format_t format, size_t out_size,
                                uint8_t *out, size_t *out_length) {

  (void)cryp;
  (void)key;
  (void)format;
  (void)out_size;
  (void)out;
  *out_length = 0U;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Begins a cipher, AEAD, MAC or hash operation.
 * @details Select parameters from op->algorithm; params is NULL and key is
 *          CRY_KEY_INVALID for hashes. For keyed operations, validate key
 *          type, size and suitability for the requested algorithm and
 *          direction. The same key must be used throughout the stream; the
 *          LLD chooses how to guarantee this. Consume pointer-backed
 *          parameters before return. Preserve operation state in LLD
 *          extension fields or other LLD-owned storage. Return CRY_ERR_BUSY
 *          if required resources are unavailable. The HLD calls abort after a
 *          failed begin, allowing partial setup to be released.
 *
 * @param[in,out] op Initialized operation context.
 * @param[in] key Key identifier interpreted by the LLD.
 * @param[in] params Parameters for the selected operation class.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_begin(cry_operation_t *op, crykey_t key,
                    const cry_stream_params_t *params) {

  (void)op;
  (void)key;
  (void)params;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Processes associated data or a payload fragment.
 * @details Consume input before return and never exceed out_size. Preserve
 *          algorithm state and partial blocks between calls. AEAD decryption
 *          output remains provisional until successful verification; the
 *          caller must quarantine it.
 *
 * @param[in,out] op Initialized operation context.
 * @param[in] aad True for associated data, false for payload.
 * @param[in] size Input length, in bytes.
 * @param[in] in Input buffer.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer.
 * @param[out] out_length Actual output length, or zero on failure.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_update(cry_operation_t *op, bool aad, size_t size,
                     const uint8_t *in, size_t out_size, uint8_t *out,
                     size_t *out_length) {

  (void)op;
  (void)aad;
  (void)size;
  (void)in;
  (void)out_size;
  (void)out;
  *out_length = 0U;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Finalizes an operation and optionally generates or verifies a tag.
 * @details Hash and MAC generation use out. AEAD generation uses tag with
 *          op->tag_size bytes. A non-NULL expected_tag requests
 *          authentication using a constant-time comparison. Return
 *          CRY_ERR_AUTH_FAILED on mismatch. The HLD calls abort afterward to
 *          release remaining LLD resources, including on successful
 *          finalization.
 *
 * @param[in,out] op Initialized operation context.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer.
 * @param[out] out_length Actual output length, or zero on failure.
 * @param[out] tag AEAD tag to generate, or NULL.
 * @param[in] expected_tag Tag to verify, or NULL.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_final(cry_operation_t *op, size_t out_size, uint8_t *out,
                    size_t *out_length, uint8_t *tag,
                    const uint8_t *expected_tag) {

  (void)op;
  (void)out_size;
  (void)out;
  (void)tag;
  (void)expected_tag;
  *out_length = 0U;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Cleans up LLD operation state.
 * @details Must handle failed partial begin, failed updates and completed
 *          operations. Finish all accesses to caller memory, release
 *          operation resources and erase intermediate secrets before
 *          returning. The HLD resets only its common fields afterward;
 *          initialization and cleanup of extension fields remain the
 *          responsibility of the LLD.
 *
 * @param[in,out] op Initialized operation context.
 * @notapi
 */
void cry_lld_abort(cry_operation_t *op) {

  (void)op;
}

/**
 * @brief   Executes a single-call public-key or derivation operation.
 * @details Interpret the key identifier and validate suitability for the
 *          selected operation, encoding, scheme parameters and peer point as
 *          applicable. Use suitable randomness when required. Check output
 *          bounds and report actual output length. Agreement returns the
 *          full-width shared secret; HKDF returns exactly job->output_size
 *          bytes. Complete all accesses and clean up all operation resources
 *          before returning, on success or failure. There is no subsequent
 *          stream cleanup hook for single-call operations.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier interpreted by the LLD.
 * @param[in] algorithm Explicit algorithm selector.
 * @param[in] job Operation parameters and buffers.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
msg_t cry_lld_execute(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm, const cry_job_t *job) {

  (void)cryp;
  (void)key;
  (void)algorithm;
  if (job->output_length != NULL) {
    *job->output_length = 0U;
  }

  return CRY_ERR_UNSUPPORTED;
}

#endif /* HAL_USE_CRY == TRUE */
/** @} */
