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
 * @file    hal_crypto_lld.h
 * @brief   Crypto low-level driver template interface.
 * @addtogroup HAL_CRYPTO
 * @{
 */
#ifndef HAL_CRYPTO_LLD_H
#define HAL_CRYPTO_LLD_H

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

/**
 * @brief   LLD configuration fields.
 * @details Replace this placeholder with the configuration required by the
 *          port.
 */
#define cry_lld_config_fields       uint32_t dummy

/**
 * @brief   LLD driver fields.
 * @details The generation counter is required by the context validity scheme
 *          below. A port adds its resource pools here, for example free maps
 *          or counters for each unit type, and the transient key storage
 *          with its hold count.
 */
#define cry_lld_driver_fields                                               \
  /* Incremented by cry_lld_stop(), invalidating all open contexts.*/       \
  uint32_t                  generation

/* Resource units. Hardware and other resources are accounted by the LLD, the
   HLD keeps no counters. Resource pools live in the driver fields and each
   context records the units it holds (engine, slot, core indexes or masks).
   Units are acquired in a begin function, or for the duration of a single
   call, and released by cry_lld_abort() or before a call returns. Pool
   updates happen inside a system-lock critical section because streams in
   different threads compete for the same units. The configuration hooks
   reject a new configuration while units are in use. Pools must read as
   free while the driver is stopped: initialize them in cry_lld_object_init()
   and restore them in cry_lld_stop(), because the configuration hook runs
   before cry_lld_start(). Exhaustion of a unit type is reported with
   CRY_ERR_BUSY.

   Keys. The driver does not manage keys. CRY_KEY_TRANSIENT (zero) is a
   volatile key that every LLD must be able to load with cry_lld_key_load();
   cry_lld_stop() erases it. All other identifiers are defined by the
   backend, which provides their keys in an unspecified way; the LLD may
   allow loading some of them, with a lifetime it defines, and returns
   CRY_ERR_UNSUPPORTED for the others. Return CRY_ERR_KEY for an identifier
   without a key, including the transient key before any load, and
   CRY_ERR_KEY_MISMATCH for a key unsuitable for the operation. Operations
   using a loadable key hold it: streams from begin to cry_lld_abort(),
   single-call operations for the duration of the call. A hold is one more
   resource unit, a count per loadable key updated under the system lock;
   cry_lld_key_load() checks it under the same lock and returns CRY_ERR_BUSY
   while it is nonzero, so an operation can use a loadable key in place.

   Context validity. Begin records the driver and its current generation in
   the context. cry_lld_stop() reclaims all pools and increments the
   generation, so every open context becomes invalid. An invalidated context
   reads as idle through cry_lld_operation_driver(); cry_lld_abort() erases
   it without releasing anything. A begin function reinitializes it without
   reading its previous content. */

/* Concurrency contract. The HLD does not serialize LLD calls. Callers must
   serialize access to each operation context; the LLD may rely on that
   caller contract but receives no protection from the HLD. Stream and
   single-call entry points may run concurrently from different threads for
   different contexts while the driver stays READY. The LLD owns
   arbitration of shared hardware: it may save and restore per-stream engine
   context, lock internally around engine use, or return CRY_ERR_BUSY when a
   resource cannot be shared. cry_lld_key_load() may run concurrently with
   operations and fails while the key being loaded is held. */

/* Error model. The HLD checks call parameters with chDbgCheck(). The LLD
   owns the stream context and checks its state with chDbgAssert(): wrong
   operation class or direction, AEAD associated data after payload, lengths
   exceeding declared totals, a generation tag size differing from Begin, or
   an output buffer smaller than a size the caller can determine (digest, MAC
   tag, buffered cipher bytes). Return codes report only conditions that
   depend on runtime data or on the backend: authentication failures
   (including received tags or signatures of the wrong size), malformed
   message data, key-dependent output sizes, missing or unsuitable keys, a
   held or non-loadable key on load, unsupported
   features, busy resources and hardware failures. */

/**
 * @brief   Caller-owned stream context.
 * @details The structure is entirely defined by the LLD, the HLD accesses it
 *          only through cry_lld_operation_driver(). It must hold the state
 *          needed to continue the stream on another call, the resource units
 *          the stream holds, and enough information to tell idle, active and
 *          invalidated contexts apart. The template keeps the owning driver,
 *          its generation and the operation class, a port adds unit
 *          references, algorithm state, buffered partial blocks and engine
 *          save areas.
 */
struct cry_operation {
  /**
   * @brief   Driver owning the stream, NULL while idle.
   */
  hal_crypto_driver_c       *driver;
  /**
   * @brief   Driver generation at begin.
   */
  uint32_t                  generation;
  /**
   * @brief   Class of the active stream.
   */
  cry_class_t               cl;
};

/**
 * @brief   Returns the driver owning an operation context.
 * @details Required by the HLD. Returns the driver passed to the begin
 *          function while a stream is active, NULL while the context is idle
 *          or has been invalidated by cry_lld_stop(). May be a macro or a
 *          function. The owning driver object must still exist.
 *
 * @param[in] op        Initialized operation context.
 * @return              The owning driver, or NULL if idle or invalidated.
 * @notapi
 */
#define cry_lld_operation_driver(op)                                        \
  ((((op)->driver != NULL) &&                                               \
    ((op)->generation == (op)->driver->generation)) ? (op)->driver : NULL)

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief   Initializes the Crypto low-level driver.
 * @details Initialize any driver objects provided by the port using
 *          cryObjectInit(). Hardware is started through the driver start
 *          hook.
 *
 * @notapi
 */
  void cry_lld_init(void);

/**
 * @brief   Initializes LLD-specific driver fields.
 * @details Called for every object initialized with cryObjectInit().
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @notapi
 */
  void cry_lld_object_init(hal_crypto_driver_c *cryp);

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
  msg_t cry_lld_start(hal_crypto_driver_c *cryp);

/**
 * @brief   Stops the Crypto driver.
 * @details Called in thread context with no call in progress; streams may be
 *          open. Reclaim all resource pools including the transient key
 *          hold, erase the transient key and other secrets held by the
 *          hardware and the driver, and increment the generation so all
 *          open contexts become invalid.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @notapi
 */
  void cry_lld_stop(hal_crypto_driver_c *cryp);

/**
 * @brief   Selects a Crypto configuration.
 * @details Called during start, with no stream or call active, or through
 *          the base-driver live configuration APIs. Return NULL for
 *          unsupported configurations, or while resource units are in use.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] config Requested configuration.
 * @return  The selected configuration, or NULL.
 * @notapi
 */
  const hal_crypto_config_t *cry_lld_setcfg(hal_crypto_driver_c *cryp,
                                            const hal_crypto_config_t *config);

/**
 * @brief   Selects a configuration by index.
 * @details Same contract as cry_lld_setcfg(). Index zero selects the
 *          default configuration.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] cfgnum Configuration index.
 * @return  The selected configuration, or NULL.
 * @notapi
 */
  const hal_crypto_config_t *cry_lld_selcfg(hal_crypto_driver_c *cryp,
                                            unsigned cfgnum);

/**
 * @brief   Queries algorithm capabilities.
 * @details Report capabilities supported by this driver. Return
 *          CRY_ERR_UNSUPPORTED for algorithms the driver does not implement.
 *          The template advertises none.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] algorithm Explicit algorithm selector.
 * @param[out] caps Algorithm capabilities, cleared by the HLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_get_capabilities(hal_crypto_driver_c *cryp,
                                 cry_algorithm_t algorithm,
                                 cry_capabilities_t *caps);

/**
 * @brief   Loads a key.
 * @details CRY_KEY_TRANSIENT must be supported. Return CRY_ERR_UNSUPPORTED for
 *          identifiers this backend cannot load. Validate material, encoding
 *          and parameters and consume the input before return. Return
 *          CRY_ERR_BUSY, leaving the current key in place, while the key is
 *          held by an operation; check the hold count and install the key
 *          under the system lock, or reserve the key storage under the lock
 *          before a longer import. Wrapped backend formats are unwrapped
 *          inside the backend.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier, CRY_KEY_TRANSIENT or a loadable backend key.
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
                         const uint8_t *data);

/**
 * @brief   Begins a hash stream.
 * @details For the begin functions the context is output only: never read
 *          its previous content, it may be uninitialized storage. Initialize
 *          it completely before any failure path, recording @p cryp, its
 *          current generation and no units, then record each unit as it is
 *          acquired. On success cry_lld_operation_driver() returns @p cryp.
 *          Consume pointer-backed parameters before return. Return
 *          CRY_ERR_BUSY if a required unit type is exhausted. After a failed
 *          begin the HLD calls cry_lld_abort(), releasing the recorded units
 *          and returning the context to idle.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[out] op Operation context, initialized by this call.
 * @param[in] algorithm Hash algorithm.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_hash_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                           cry_algorithm_t algorithm);

/**
 * @brief   Begins a cipher stream.
 * @details Same contract as cry_lld_hash_begin(). Return CRY_ERR_KEY or
 *          CRY_ERR_KEY_MISMATCH, after initializing the context, for a missing
 *          key or one unsuitable for the algorithm and direction. A stream on
 *          a loadable key records a hold on it, released by cry_lld_abort().
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[out] op Operation context, initialized by this call.
 * @param[in] key Key identifier, CRY_KEY_TRANSIENT or a backend key.
 * @param[in] algorithm Cipher algorithm.
 * @param[in] direction Encryption or decryption.
 * @param[in] params Cipher parameters.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_cipher_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                             crykey_t key, cry_algorithm_t algorithm,
                             cry_direction_t direction,
                             const cry_cipher_params_t *params);

/**
 * @brief   Begins a MAC generation or verification stream.
 * @details Same contract as cry_lld_cipher_begin(). Return
 *          CRY_ERR_UNSUPPORTED for unsupported tag sizes.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[out] op Operation context, initialized by this call.
 * @param[in] key Key identifier, CRY_KEY_TRANSIENT or a backend key.
 * @param[in] algorithm MAC algorithm.
 * @param[in] verify True for verification, false for generation.
 * @param[in] tag_size Tag size, in bytes.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_mac_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                          crykey_t key, cry_algorithm_t algorithm,
                          bool verify, size_t tag_size);

/**
 * @brief   Begins an AEAD stream.
 * @details Same contract as cry_lld_cipher_begin(). The LLD keeps the declared
 *          totals and the processed lengths needed to enforce AEAD phase
 *          ordering and to build the final length block. For CCM, return
 *          CRY_ERR_ARGUMENT if the declared payload size does not fit the
 *          length field of 15 - nonce_size bytes; the size is message data.
 *          The HLD performs no runtime check before calling a begin
 *          function, so every begin return leaves the context initialized.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[out] op Operation context, initialized by this call.
 * @param[in] key Key identifier, CRY_KEY_TRANSIENT or a backend key.
 * @param[in] algorithm AEAD algorithm.
 * @param[in] direction Encryption or decryption.
 * @param[in] params AEAD parameters.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_aead_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                           crykey_t key, cry_algorithm_t algorithm,
                           cry_direction_t direction,
                           const cry_aead_params_t *params);

/**
 * @brief   Adds bytes to a hash stream.
 * @details The update functions receive an active context of their class.
 *          Consume input before return and preserve algorithm state and
 *          partial blocks between calls. After a failed update the HLD calls
 *          cry_lld_abort().
 *
 * @param[in,out] op Active hash context.
 * @param[in] size Input length, in bytes.
 * @param[in] in Input buffer.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_hash_update(cry_operation_t *op, size_t size,
                            const uint8_t *in);

/**
 * @brief   Adds bytes to a MAC stream.
 *
 * @param[in,out] op Active MAC context.
 * @param[in] size Input length, in bytes.
 * @param[in] in Input buffer.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_mac_update(cry_operation_t *op, size_t size,
                           const uint8_t *in);

/**
 * @brief   Processes a cipher payload fragment.
 * @details Never exceed out_size; an output buffer too small for the bytes
 *          the caller can predict is a programming error.
 *
 * @param[in,out] op Active cipher context.
 * @param[in] size Input length, in bytes.
 * @param[in] in Input buffer.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer.
 * @param[out] out_length Actual output length, preset to zero by the HLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_cipher_update(cry_operation_t *op, size_t size,
                              const uint8_t *in, size_t out_size,
                              uint8_t *out, size_t *out_length);

/**
 * @brief   Processes an AEAD payload fragment.
 * @details Assert that the payload stays within the declared total and, when
 *          the AAD total is declared, that all AAD has been supplied before
 *          nonempty payload. Decryption output remains provisional until
 *          successful verification.
 *
 * @param[in,out] op Active AEAD context.
 * @param[in] size Input length, in bytes.
 * @param[in] in Input buffer.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer.
 * @param[out] out_length Actual output length, preset to zero by the HLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_aead_update(cry_operation_t *op, size_t size,
                            const uint8_t *in, size_t out_size,
                            uint8_t *out, size_t *out_length);

/**
 * @brief   Adds AEAD associated data.
 * @details Assert that no payload has been processed yet and that the AAD
 *          stays within the declared total.
 *
 * @param[in,out] op Active AEAD context.
 * @param[in] size Associated-data length, in bytes.
 * @param[in] in Associated data.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_aead_update_aad(cry_operation_t *op, size_t size,
                                const uint8_t *in);

/**
 * @brief   Finalizes a hash stream.
 * @details The final functions receive an active context of their class.
 *          The HLD calls cry_lld_abort() afterward, on success or failure, to
 *          release remaining resources and return the context to idle.
 *
 * @param[in,out] op Active hash context.
 * @param[in] out_size Output buffer capacity, at least the digest size.
 * @param[out] out Digest output.
 * @param[out] out_length Digest size, preset to zero by the HLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_hash_final(cry_operation_t *op, size_t out_size,
                           uint8_t *out, size_t *out_length);

/**
 * @brief   Finalizes a cipher stream.
 * @details Unpadded modes assert that no partial block remains where the mode
 *          requires block alignment.
 *
 * @param[in,out] op Active cipher context.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer.
 * @param[out] out_length Actual output length, preset to zero by the HLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_cipher_final(cry_operation_t *op, size_t out_size,
                             uint8_t *out, size_t *out_length);

/**
 * @brief   Finalizes a MAC generation stream.
 * @details Assert a generation context and an output capacity of at least the
 *          tag size selected at Begin.
 *
 * @param[in,out] op Active MAC generation context.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Tag output.
 * @param[out] out_length Tag size, preset to zero by the HLD.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_mac_final(cry_operation_t *op, size_t out_size,
                          uint8_t *out, size_t *out_length);

/**
 * @brief   Verifies a MAC stream.
 * @details Assert a verification context. The received tag is data: return
 *          CRY_ERR_AUTH_FAILED if its size differs from the size selected at
 *          Begin or if it does not match, comparing in constant time.
 *
 * @param[in,out] op Active MAC verification context.
 * @param[in] tag_size Received tag size, in bytes.
 * @param[in] tag Received tag.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_mac_verify(cry_operation_t *op, size_t tag_size,
                           const uint8_t *tag);

/**
 * @brief   Finalizes AEAD encryption and generates the tag.
 * @details Assert an encryption context, declared totals reached and
 *          @p tag_size equal to the size selected at Begin.
 *
 * @param[in,out] op Active AEAD encryption context.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer for deferred payload bytes.
 * @param[out] out_length Actual output length, preset to zero by the HLD.
 * @param[in] tag_size Tag size, in bytes.
 * @param[out] tag Generated tag.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_aead_final(cry_operation_t *op, size_t out_size,
                           uint8_t *out, size_t *out_length,
                           size_t tag_size, uint8_t *tag);

/**
 * @brief   Verifies the AEAD tag.
 * @details Assert a decryption context and declared totals reached. The
 *          received tag is data: return CRY_ERR_AUTH_FAILED if its size
 *          differs from the size selected at Begin or if it does not match,
 *          comparing in constant time.
 *
 * @param[in,out] op Active AEAD decryption context.
 * @param[in] out_size Output buffer capacity, in bytes.
 * @param[out] out Output buffer for deferred payload bytes.
 * @param[out] out_length Actual output length, preset to zero by the HLD.
 * @param[in] tag_size Received tag size, in bytes.
 * @param[in] tag Received tag.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_aead_verify(cry_operation_t *op, size_t out_size,
                            uint8_t *out, size_t *out_length,
                            size_t tag_size, const uint8_t *tag);

/**
 * @brief   Cleans up a stream and returns the context to idle.
 * @details Must handle idle contexts, failed partial begin, failed updates,
 *          completed operations and contexts invalidated by cry_lld_stop().
 *          Release the units recorded in a valid context only; an invalidated
 *          context holds nothing, its units were reclaimed at stop. Finish all
 *          accesses to caller memory and erase intermediate secrets before
 *          returning. On return cry_lld_operation_driver() must return NULL.
 *
 * @param[in,out] op Operation context.
 * @notapi
 */
  void cry_lld_abort(cry_operation_t *op);

/**
 * @brief   Executes a single-call public-key or derivation operation.
 * @details Return CRY_ERR_KEY or CRY_ERR_KEY_MISMATCH for a missing or
 *          unsuitable key, and validate encoding, scheme parameters and peer
 *          point as applicable. Hold any required units, including a
 *          loadable key, only for the duration of the call. Use suitable
 *          randomness when required. Return CRY_ERR_BUFFER if a key-dependent
 *          output does not fit. Agreement returns the full-width shared secret;
 *          HKDF returns exactly job->output_size bytes. Complete all accesses
 *          and clean up all operation resources before returning, on success or
 *          failure.
 *
 * @param[in] cryp Pointer to the Crypto driver.
 * @param[in] key Key identifier, CRY_KEY_TRANSIENT or a backend key.
 * @param[in] algorithm Explicit algorithm selector.
 * @param[in] job Operation parameters and buffers.
 * @return  HAL_RET_SUCCESS or an error code.
 * @notapi
 */
  msg_t cry_lld_execute(hal_crypto_driver_c *cryp, crykey_t key,
                        cry_algorithm_t algorithm, const cry_job_t *job);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CRY == TRUE */
#endif /* HAL_CRYPTO_LLD_H */
/** @} */
