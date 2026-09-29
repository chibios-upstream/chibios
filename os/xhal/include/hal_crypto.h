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
 * @file        hal_crypto.h
 * @brief       Generated Crypto Driver header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_CRYPTO
 * @brief       Cryptographic operation-class APIs and driver lifecycle.
 * @details     All cryptographic operations use unlocked thread context and
 *              may block. The HLD checks call parameters and manages the
 *              driver lifecycle. Stream contexts are defined and managed by
 *              the LLD, which also checks context state. Parameter and
 *              context-state violations are programming errors caught by debug
 *              checks and assertions; return codes report only conditions that
 *              depend on runtime data or on the backend. The driver does not
 *              manage keys: key zero is a volatile transient key and the only
 *              key every driver can load with cryKeyLoad(). Other identifiers
 *              refer to keys the backend provides in an unspecified way, for
 *              example provisioned into an HSM; whether some of them can also
 *              be loaded, and with which lifetime, is backend-defined. The LLD
 *              rejects identifiers without a key and keys unsuitable for an
 *              operation. Operations using a loadable key hold it, and loading
 *              that key fails while it is held; callers sharing a driver
 *              serialize loading and the begin that uses the key, for example
 *              with drvLock(). Callers must serialize access to each operation
 *              context. Different contexts may be used concurrently from
 *              different threads; the driver remains in HAL_DRV_STATE_READY
 *              while streams are open. Hardware and other resources are
 *              accounted by the LLD, which reports CRY_ERR_BUSY when a
 *              resource is exhausted and may reject live reconfiguration while
 *              resources are in use. drvStop() requires that no call is in
 *              progress; it reclaims all resources and invalidates every open
 *              stream, and invalidated contexts behave as idle. Begin
 *              functions initialize the context they receive, which must not
 *              hold an active stream. Before disposal, abort or stop using all
 *              contexts. LLD extension fields and APIs may implement
 *              additional behavior without changing the common operation
 *              interface. Caller buffers must remain valid for each
 *              synchronous call. This API is not source or binary compatible
 *              with the legacy HAL Crypto API.
 * @{
 */

#ifndef HAL_CRYPTO_H
#define HAL_CRYPTO_H

#include "hal_base_driver.h"

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Crypto result codes
 * @{
 */
/**
 * @brief       Unsupported algorithm, capability or key operation.
 */
#define CRY_ERR_UNSUPPORTED                 ((msg_t)-64)

/**
 * @brief       Invalid runtime data such as key parameters, message lengths or
 *              encodings.
 */
#define CRY_ERR_ARGUMENT                    ((msg_t)-65)

/**
 * @brief       No key with this identifier, or the transient key is not
 *              loaded.
 */
#define CRY_ERR_KEY                         ((msg_t)-66)

/**
 * @brief       Key is unsuitable for the requested operation.
 */
#define CRY_ERR_KEY_MISMATCH                ((msg_t)-67)

/**
 * @brief       Authentication or signature verification failed.
 */
#define CRY_ERR_AUTH_FAILED                 ((msg_t)-68)

/**
 * @brief       Insufficient capacity for a key- or data-dependent output.
 */
#define CRY_ERR_BUFFER                      ((msg_t)-69)

/**
 * @brief       Random generator failure.
 */
#define CRY_ERR_ENTROPY                     ((msg_t)-70)

/**
 * @brief       Requested resource or operation is busy.
 */
#define CRY_ERR_BUSY                        ((msg_t)-71)

/**
 * @brief       Backend operation failure.
 */
#define CRY_ERR_FAILURE                     ((msg_t)-72)
/** @} */

/**
 * @name    Crypto constants
 * @{
 */
/**
 * @brief       Transient key, the only identifier every driver can load.
 */
#define CRY_KEY_TRANSIENT                   0U

/**
 * @brief       Unknown AEAD total length where permitted.
 */
#define CRY_LENGTH_UNKNOWN                  SIZE_MAX

/**
 * @brief       Backend supports single-call operations.
 */
#define CRY_CAP_ONESHOT                     1U

/**
 * @brief       Backend supports multipart operations.
 */
#define CRY_CAP_STREAM                      2U
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief       Type of structure representing a Crypto driver.
 */
typedef struct hal_crypto_driver hal_crypto_driver_c;

/**
 * @brief       Type of structure representing a Crypto configuration.
 */
typedef struct hal_crypto_config hal_crypto_config_t;

/**
 * @brief       Alias for the Crypto driver type.
 */
typedef hal_crypto_driver_c CRYDriver;

/**
 * @brief       Alias for the Crypto configuration type.
 */
typedef hal_crypto_config_t CRYConfig;

/**
 * @brief       Key identifier.
 * @details     CRY_KEY_TRANSIENT is a volatile key, erased by drvStop(), that
 *              every driver can load with cryKeyLoad(). Other identifiers are
 *              defined by the backend, which provides their keys in an
 *              unspecified way; loading some of them may be possible, with a
 *              backend-defined lifetime. The driver never generates, exports
 *              or deletes keys. The HLD passes identifiers to the LLD
 *              unchecked.
 */
typedef uint32_t crykey_t;

/**
 * @brief       Operation class.
 */
typedef enum {
  /**
   * @brief       No active or recognized operation class.
   */
  CRY_CLASS_NONE,
  /**
   * @brief       Unpadded symmetric encryption or decryption.
   */
  CRY_CLASS_CIPHER,
  /**
   * @brief       Authenticated encryption with associated data.
   */
  CRY_CLASS_AEAD,
  /**
   * @brief       Message authentication code generation or verification.
   */
  CRY_CLASS_MAC,
  /**
   * @brief       Unkeyed message digest computation.
   */
  CRY_CLASS_HASH,
  /**
   * @brief       Public-key signature generation or verification.
   */
  CRY_CLASS_SIGNATURE,
  /**
   * @brief       Asymmetric encryption or decryption.
   */
  CRY_CLASS_ASYMMETRIC,
  /**
   * @brief       Key agreement computing a shared secret.
   */
  CRY_CLASS_AGREEMENT,
  /**
   * @brief       Key derivation producing output bytes.
   */
  CRY_CLASS_DERIVATION
} cry_class_t;

/**
 * @brief       Explicit algorithm selectors; values are not a wire protocol.
 */
typedef enum {
  /**
   * @brief       No algorithm selected.
   */
  CRY_ALG_NONE = 0,
  /**
   * @brief       AES in electronic codebook mode.
   */
  CRY_ALG_AES_ECB,
  /**
   * @brief       AES in cipher block chaining mode.
   */
  CRY_ALG_AES_CBC,
  /**
   * @brief       AES in 128-bit cipher feedback mode.
   */
  CRY_ALG_AES_CFB128,
  /**
   * @brief       AES in counter mode.
   */
  CRY_ALG_AES_CTR,
  /**
   * @brief       AES Galois/counter authenticated encryption.
   */
  CRY_ALG_AES_GCM,
  /**
   * @brief       AES counter with CBC-MAC authenticated encryption.
   */
  CRY_ALG_AES_CCM,
  /**
   * @brief       AES cipher-based message authentication code.
   */
  CRY_ALG_AES_CMAC,
  /**
   * @brief       HMAC using SHA-256.
   */
  CRY_ALG_HMAC_SHA256,
  /**
   * @brief       HMAC using SHA-384.
   */
  CRY_ALG_HMAC_SHA384,
  /**
   * @brief       HMAC using SHA-512.
   */
  CRY_ALG_HMAC_SHA512,
  /**
   * @brief       SHA-1 message digest.
   */
  CRY_ALG_SHA1,
  /**
   * @brief       SHA-224 message digest.
   */
  CRY_ALG_SHA224,
  /**
   * @brief       SHA-256 message digest.
   */
  CRY_ALG_SHA256,
  /**
   * @brief       SHA-384 message digest.
   */
  CRY_ALG_SHA384,
  /**
   * @brief       SHA-512 message digest.
   */
  CRY_ALG_SHA512,
  /**
   * @brief       RSASSA-PSS using SHA-256.
   */
  CRY_ALG_RSA_PSS_SHA256,
  /**
   * @brief       RSASSA-PSS using SHA-384.
   */
  CRY_ALG_RSA_PSS_SHA384,
  /**
   * @brief       RSASSA-PSS using SHA-512.
   */
  CRY_ALG_RSA_PSS_SHA512,
  /**
   * @brief       RSASSA-PKCS1-v1_5 using SHA-256.
   */
  CRY_ALG_RSA_PKCS1_SHA256,
  /**
   * @brief       RSASSA-PKCS1-v1_5 using SHA-384.
   */
  CRY_ALG_RSA_PKCS1_SHA384,
  /**
   * @brief       RSASSA-PKCS1-v1_5 using SHA-512.
   */
  CRY_ALG_RSA_PKCS1_SHA512,
  /**
   * @brief       ECDSA using SHA-256.
   */
  CRY_ALG_ECDSA_SHA256,
  /**
   * @brief       ECDSA using SHA-384.
   */
  CRY_ALG_ECDSA_SHA384,
  /**
   * @brief       ECDSA using SHA-512.
   */
  CRY_ALG_ECDSA_SHA512,
  /**
   * @brief       RSAES-OAEP using SHA-256.
   */
  CRY_ALG_RSA_OAEP_SHA256,
  /**
   * @brief       RSAES-OAEP using SHA-384.
   */
  CRY_ALG_RSA_OAEP_SHA384,
  /**
   * @brief       RSAES-OAEP using SHA-512.
   */
  CRY_ALG_RSA_OAEP_SHA512,
  /**
   * @brief       Elliptic-curve Diffie-Hellman key agreement.
   */
  CRY_ALG_ECDH,
  /**
   * @brief       HKDF extract and expand using SHA-256.
   */
  CRY_ALG_HKDF_SHA256,
  /**
   * @brief       HKDF extract and expand using SHA-384.
   */
  CRY_ALG_HKDF_SHA384,
  /**
   * @brief       HKDF extract and expand using SHA-512.
   */
  CRY_ALG_HKDF_SHA512
} cry_algorithm_t;

/**
 * @brief       Key representation and mathematical type.
 */
typedef enum {
  /**
   * @brief       AES symmetric key material.
   */
  CRY_KEY_AES,
  /**
   * @brief       HMAC symmetric key material.
   */
  CRY_KEY_HMAC,
  /**
   * @brief       RSA public key.
   */
  CRY_KEY_RSA_PUBLIC,
  /**
   * @brief       RSA public and private key pair.
   */
  CRY_KEY_RSA_PAIR,
  /**
   * @brief       Elliptic-curve public key.
   */
  CRY_KEY_ECC_PUBLIC,
  /**
   * @brief       Elliptic-curve public and private key pair.
   */
  CRY_KEY_ECC_PAIR,
  /**
   * @brief       Secret input keying material for derivation.
   */
  CRY_KEY_DERIVATION
} cry_key_type_t;

/**
 * @brief       Supported curve identifiers; support is backend-dependent.
 */
typedef enum {
  /**
   * @brief       No elliptic curve applies to this key.
   */
  CRY_CURVE_NONE,
  /**
   * @brief       NIST P-256 elliptic curve.
   */
  CRY_CURVE_P256,
  /**
   * @brief       NIST P-384 elliptic curve.
   */
  CRY_CURVE_P384,
  /**
   * @brief       NIST P-521 elliptic curve.
   */
  CRY_CURVE_P521
} cry_curve_t;

/**
 * @brief       Import formats.
 * @details     RAW is symmetric/derivation bytes or a fixed-width big-endian
 *              ECC scalar. SEC1 is an uncompressed public point. PKCS1_DER is
 *              an RSA key. BACKEND is an opaque representation defined and
 *              validated by the selected LLD. These identifiers do not imply
 *              backend support for a format.
 */
typedef enum {
  /**
   * @brief       Raw symmetric bytes or a fixed-width big-endian ECC scalar.
   */
  CRY_KEY_FORMAT_RAW,
  /**
   * @brief       SEC1 uncompressed ECC public point.
   */
  CRY_KEY_FORMAT_SEC1,
  /**
   * @brief       DER-encoded PKCS #1 RSA key.
   */
  CRY_KEY_FORMAT_PKCS1_DER,
  /**
   * @brief       Opaque representation defined and validated by the backend.
   */
  CRY_KEY_FORMAT_BACKEND
} cry_key_format_t;

/**
 * @brief       Cipher/AEAD direction.
 */
typedef enum {
  /**
   * @brief       Encrypt the input data.
   */
  CRY_ENCRYPT,
  /**
   * @brief       Decrypt the input data.
   */
  CRY_DECRYPT
} cry_direction_t;

/**
 * @brief       Single-call operation kind.
 */
typedef enum {
  /**
   * @brief       Generate a signature over a precomputed digest.
   */
  CRY_JOB_SIGN_DIGEST,
  /**
   * @brief       Verify a signature over a precomputed digest.
   */
  CRY_JOB_VERIFY_DIGEST,
  /**
   * @brief       Hash and sign a message.
   */
  CRY_JOB_SIGN_MESSAGE,
  /**
   * @brief       Hash a message and verify its signature.
   */
  CRY_JOB_VERIFY_MESSAGE,
  /**
   * @brief       Encrypt using an asymmetric scheme.
   */
  CRY_JOB_ASYM_ENCRYPT,
  /**
   * @brief       Decrypt using an asymmetric scheme.
   */
  CRY_JOB_ASYM_DECRYPT,
  /**
   * @brief       Compute a shared secret using key agreement.
   */
  CRY_JOB_AGREEMENT,
  /**
   * @brief       Derive output bytes using HKDF.
   */
  CRY_JOB_DERIVE
} cry_job_kind_t;

/**
 * @brief       Mathematical key parameters for loading a key.
 * @details     These parameters describe key material, not permissions or a
 *              fixed operation algorithm. The LLD checks compatibility with
 *              each requested operation.
 */
typedef struct {
  /**
   * @brief       Material type.
   */
  cry_key_type_t            type;
  /**
   * @brief       Key/modulus size in bits.
   */
  size_t                    bits;
  /**
   * @brief       ECC curve, otherwise NONE.
   */
  cry_curve_t               curve;
} cry_key_params_t;

/**
 * @brief       Per-algorithm backend capabilities; zero means unsupported.
 */
typedef struct {
  /**
   * @brief       CRY_CAP_* flags.
   */
  uint32_t                  flags;
  /**
   * @brief       Minimum supported key size.
   */
  size_t                    min_key_bits;
  /**
   * @brief       Maximum supported key size.
   */
  size_t                    max_key_bits;
} cry_capabilities_t;

/**
 * @brief       Cipher parameters; no implicit padding is performed.
 */
typedef struct {
  /**
   * @brief       IV/counter; consumed during Begin.
   */
  const uint8_t             *iv;
  /**
   * @brief       Zero for ECB, 16 for other AES modes.
   */
  size_t                    iv_size;
} cry_cipher_params_t;

/**
 * @brief       AEAD parameters, copied/consumed by the backend during Begin.
 */
typedef struct {
  /**
   * @brief       Per-message nonce.
   */
  const uint8_t             *nonce;
  /**
   * @brief       Nonce length in bytes.
   */
  size_t                    nonce_size;
  /**
   * @brief       Authentication tag length.
   */
  size_t                    tag_size;
  /**
   * @brief       Total AAD size, or CRY_LENGTH_UNKNOWN.
   */
  size_t                    aad_size;
  /**
   * @brief       Total payload size, or CRY_LENGTH_UNKNOWN.
   */
  size_t                    data_size;
} cry_aead_params_t;

/**
 * @brief       Signature scheme parameters. Hash is fixed by the algorithm.
 */
typedef struct {
  /**
   * @brief       PSS salt size; zero for other schemes.
   */
  size_t                    salt_size;
} cry_signature_params_t;

/**
 * @brief       RSA-OAEP parameters; MGF1 uses the selected algorithm's hash.
 */
typedef struct {
  /**
   * @brief       Optional OAEP label.
   */
  const uint8_t             *label;
  /**
   * @brief       Label length in bytes.
   */
  size_t                    label_size;
} cry_asymmetric_params_t;

/**
 * @brief       HKDF inputs; source key is the input keying material.
 */
typedef struct {
  /**
   * @brief       Optional HKDF salt, consumed before return.
   */
  const uint8_t             *salt;
  /**
   * @brief       Salt length in bytes.
   */
  size_t                    salt_size;
  /**
   * @brief       Optional HKDF context information, consumed before return.
   */
  const uint8_t             *info;
  /**
   * @brief       Context information length in bytes.
   */
  size_t                    info_size;
} cry_derivation_params_t;

/**
 * @brief       Caller-owned stream context, defined by the LLD.
 * @details     The LLD provides the complete structure definition in its
 *              header, so the size is known and contexts can be allocated
 *              statically or on the stack. Fields are private to the LLD; the
 *              HLD does not inspect them. A begin function initializes the
 *              context; its previous content is ignored, so fresh storage
 *              needs no initialization. Every return from a begin function,
 *              including failures, leaves the context initialized. Beginning
 *              on a context that holds an active stream is a programming error
 *              that leaks the resources of that stream. Do not copy a live
 *              context and do not access it concurrently.
 */
typedef struct cry_operation cry_operation_t;

/**
 * @brief       Backend single-call descriptor.
 * @details     Only the parameter member selected by kind may be inspected.
 *              Input pointers are borrowed until the call returns. Output
 *              capacity and actual length are separate. Signature encoding is
 *              RSA modulus-sized big-endian bytes, or ECDSA fixed-width
 *              big-endian r followed by s. Agreement and derivation return
 *              bytes to the supplied output buffer.
 */
typedef struct {
  /**
   * @brief       Operation kind selecting the valid parameter member.
   */
  cry_job_kind_t            kind;
  /**
   * @brief       Borrowed message, digest, ciphertext or ECDH peer public
   *              point.
   */
  const uint8_t             *input;
  /**
   * @brief       Input length in bytes.
   */
  size_t                    input_size;
  /**
   * @brief       Borrowed signature to verify; unused by other operation
   *              kinds.
   */
  const uint8_t             *signature;
  /**
   * @brief       Signature length in bytes.
   */
  size_t                    signature_size;
  /**
   * @brief       Caller-owned signature or asymmetric encryption/decryption
   *              output.
   */
  uint8_t                   *output;
  /**
   * @brief       Output buffer capacity in bytes.
   */
  size_t                    output_size;
  /**
   * @brief       Receives the actual output length in bytes.
   */
  size_t                    *output_length;
  /**
   * @brief       Parameters selected by the operation kind.
   */
  union {
    /**
     * @brief       Parameters for signature generation or verification.
     */
    cry_signature_params_t  signature;
    /**
     * @brief       Parameters for asymmetric encryption or decryption.
     */
    cry_asymmetric_params_t asymmetric;
    /**
     * @brief       HKDF salt and context information.
     */
    cry_derivation_params_t derivation;
  } params;
} cry_job_t;

#include "hal_crypto_lld.h"

/**
 * @brief       Crypto driver configuration.
 */
struct hal_crypto_config {
  /**
   * @brief       LLD-specific configuration fields.
   */
  cry_lld_config_fields;
};

/**
 * @class       hal_crypto_driver_c
 * @extends     hal_base_driver_c
 *
 * @brief       Crypto driver with independent operation contexts.
 *
 * @name        Class @p hal_crypto_driver_c structures
 * @{
 */

/**
 * @brief       Type of a Crypto driver class.
 */
typedef struct hal_crypto_driver hal_crypto_driver_c;

/**
 * @brief       Class @p hal_crypto_driver_c virtual methods table.
 */
struct hal_crypto_driver_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From hal_base_driver_c.*/
  msg_t (*start)(void *ip, const void *config);
  void (*stop)(void *ip);
  const void * (*setcfg)(void *ip, const void *config);
  const void * (*selcfg)(void *ip, unsigned cfgnum);
  /* From hal_crypto_driver_c.*/
};

/**
 * @brief       Structure representing a Crypto driver class.
 */
struct hal_crypto_driver {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_crypto_driver_vmt *vmt;
  /**
   * @brief       Driver state.
   */
  driver_state_t            state;
  /**
   * @brief       Associated configuration structure.
   */
  const void                *config;
  /**
   * @brief       Driver argument.
   */
  void                      *arg;
#if (HAL_USE_MUTUAL_EXCLUSION == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Driver mutual exclusion object.
   */
  driver_mutex_t            mutex;
#endif /* HAL_USE_MUTUAL_EXCLUSION == TRUE */
#if (HAL_USE_REGISTRY == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Driver identifier.
   */
  unsigned int              id;
  /**
   * @brief       Driver name.
   */
  const char                *name;
  /**
   * @brief       Registry link structure.
   */
  hal_regent_t              regent;
#endif /* HAL_USE_REGISTRY == TRUE */
#if (defined(cry_lld_driver_fields)) || defined (__DOXYGEN__)
  /**
   * @brief       Optional LLD-specific driver fields.
   */
  cry_lld_driver_fields;
#endif /* defined(cry_lld_driver_fields) */
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of hal_crypto_driver_c.*/
  void *__cry_objinit_impl(void *ip, const void *vmt);
  void __cry_dispose_impl(void *ip);
  msg_t __cry_start_impl(void *ip, const void *config);
  void __cry_stop_impl(void *ip);
  const void *__cry_setcfg_impl(void *ip, const void *config);
  const void *__cry_selcfg_impl(void *ip, unsigned cfgnum);
  /* Regular functions.*/
  void cryInit(void);
  msg_t cryGetCapabilities(hal_crypto_driver_c *cryp,
                           cry_algorithm_t algorithm, cry_capabilities_t *caps);
  msg_t cryKeyLoad(hal_crypto_driver_c *cryp, crykey_t key,
                   const cry_key_params_t *params, cry_key_format_t format,
                   size_t size, const uint8_t *data);
  msg_t cryHashBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                     cry_algorithm_t algorithm);
  msg_t cryCipherBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                       crykey_t key, cry_algorithm_t algorithm,
                       cry_direction_t direction,
                       const cry_cipher_params_t *params);
  msg_t cryMacBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                    crykey_t key, cry_algorithm_t algorithm, bool verify,
                    size_t tag_size);
  msg_t cryAeadBegin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                     crykey_t key, cry_algorithm_t algorithm,
                     cry_direction_t direction,
                     const cry_aead_params_t *params);
  msg_t cryHashUpdate(cry_operation_t *op, size_t size, const uint8_t *in);
  msg_t cryMacUpdate(cry_operation_t *op, size_t size, const uint8_t *in);
  msg_t cryCipherUpdate(cry_operation_t *op, size_t size, const uint8_t *in,
                        size_t out_size, uint8_t *out, size_t *out_length);
  msg_t cryAeadUpdate(cry_operation_t *op, size_t size, const uint8_t *in,
                      size_t out_size, uint8_t *out, size_t *out_length);
  msg_t cryAeadUpdateAAD(cry_operation_t *op, size_t size, const uint8_t *in);
  msg_t cryHashFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                     size_t *out_length);
  msg_t cryCipherFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                       size_t *out_length);
  msg_t cryMacFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                    size_t *out_length);
  msg_t cryMacVerify(cry_operation_t *op, size_t tag_size, const uint8_t *tag);
  msg_t cryAeadFinal(cry_operation_t *op, size_t out_size, uint8_t *out,
                     size_t *out_length, size_t tag_size, uint8_t *tag);
  msg_t cryAeadVerify(cry_operation_t *op, size_t out_size, uint8_t *out,
                      size_t *out_length, size_t tag_size, const uint8_t *tag);
  void cryOperationAbort(cry_operation_t *op);
  msg_t crySignDigest(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm,
                      const cry_signature_params_t *params, size_t size,
                      const uint8_t *in, size_t out_size, uint8_t *out,
                      size_t *out_length);
  msg_t crySignMessage(hal_crypto_driver_c *cryp, crykey_t key,
                       cry_algorithm_t algorithm,
                       const cry_signature_params_t *params, size_t size,
                       const uint8_t *in, size_t out_size, uint8_t *out,
                       size_t *out_length);
  msg_t cryVerifyDigest(hal_crypto_driver_c *cryp, crykey_t key,
                        cry_algorithm_t algorithm,
                        const cry_signature_params_t *params, size_t size,
                        const uint8_t *in, size_t signature_size,
                        const uint8_t *signature);
  msg_t cryVerifyMessage(hal_crypto_driver_c *cryp, crykey_t key,
                         cry_algorithm_t algorithm,
                         const cry_signature_params_t *params, size_t size,
                         const uint8_t *in, size_t signature_size,
                         const uint8_t *signature);
  msg_t cryAsymEncrypt(hal_crypto_driver_c *cryp, crykey_t key,
                       cry_algorithm_t algorithm,
                       const cry_asymmetric_params_t *params, size_t size,
                       const uint8_t *in, size_t out_size, uint8_t *out,
                       size_t *out_length);
  msg_t cryAsymDecrypt(hal_crypto_driver_c *cryp, crykey_t key,
                       cry_algorithm_t algorithm,
                       const cry_asymmetric_params_t *params, size_t size,
                       const uint8_t *in, size_t out_size, uint8_t *out,
                       size_t *out_length);
  msg_t cryKeyAgreement(hal_crypto_driver_c *cryp, crykey_t key,
                        cry_algorithm_t algorithm, size_t peer_size,
                        const uint8_t *peer, size_t out_size, uint8_t *out,
                        size_t *out_length);
  msg_t cryDeriveKey(hal_crypto_driver_c *cryp, crykey_t key,
                     cry_algorithm_t algorithm,
                     const cry_derivation_params_t *params, size_t size,
                     uint8_t *out);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Default constructor of hal_crypto_driver_c
 * @{
 */
/**
 * @brief       Default initialization function of @p hal_crypto_driver_c.
 *
 * @param[out]    self          Pointer to a @p hal_crypto_driver_c instance to
 *                              be initialized.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline hal_crypto_driver_c *cryObjectInit(hal_crypto_driver_c *self) {
  extern const struct hal_crypto_driver_vmt __hal_crypto_driver_vmt;

  return __cry_objinit_impl(self, &__hal_crypto_driver_vmt);
}
/** @} */

#endif /* HAL_USE_CRY == TRUE */

#endif /* HAL_CRYPTO_H */

/** @} */
