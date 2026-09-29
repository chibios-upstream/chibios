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
 * @file    CRYPv1/hal_crypto_lld.h
 * @brief   STM32 CRYP/HASH low level driver header.
 * @details AES-ECB and AES-CBC cipher streams on CRYP1 and SHA-256 hash
 *          streams on HASH1. The CRYP and HASH engines are resource units
 *          held by a stream from begin to abort; a second concurrent stream
 *          needing the same engine gets CRY_ERR_BUSY. The only key is the
 *          transient AES key. Caller buffers transferred by DMA must be
 *          DMA-accessible and cache-coherent; buffers in the operation
 *          context are always transferred by the CPU.
 *
 * @addtogroup HAL_CRYPTO
 * @{
 */

#ifndef HAL_CRYPTO_LLD_H
#define HAL_CRYPTO_LLD_H

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    STM32 configuration options
 * @{
 */
/**
 * @brief   CRYP1 enable switch.
 * @details If set to @p TRUE the support for CRYP1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_CRY_USE_CRYP1) || defined(__DOXYGEN__)
#define STM32_CRY_USE_CRYP1                 FALSE
#endif

/**
 * @brief   HASH1 enable switch.
 * @details If set to @p TRUE the support for HASH1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_CRY_USE_HASH1) || defined(__DOXYGEN__)
#define STM32_CRY_USE_HASH1                 FALSE
#endif

/**
 * @brief   CRYP1 DMA interrupt priority level setting.
 */
#if !defined(STM32_CRY_CRYP1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_CRY_CRYP1_IRQ_PRIORITY        9
#endif

/**
 * @brief   HASH1 DMA interrupt priority level setting.
 */
#if !defined(STM32_CRY_HASH1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_CRY_HASH1_IRQ_PRIORITY        9
#endif

/**
 * @brief   CRYP1 IN DMA priority (0..3|lowest..highest).
 */
#if !defined(STM32_CRY_CRYP1_IN_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_CRY_CRYP1_IN_DMA_PRIORITY     0
#endif

/**
 * @brief   CRYP1 OUT DMA priority (0..3|lowest..highest).
 */
#if !defined(STM32_CRY_CRYP1_OUT_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_CRY_CRYP1_OUT_DMA_PRIORITY    1
#endif

/**
 * @brief   HASH1 DMA priority (0..3|lowest..highest).
 */
#if !defined(STM32_CRY_HASH1_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_CRY_HASH1_DMA_PRIORITY        0
#endif

/**
 * @brief   Minimum hash fragment size, in words, for DMA use.
 * @note    If set to zero then DMA is never used.
 * @note    If set to one then DMA is always used.
 */
#if !defined(STM32_CRY_HASH_SIZE_THRESHOLD) || defined(__DOXYGEN__)
#define STM32_CRY_HASH_SIZE_THRESHOLD       1024
#endif

/**
 * @brief   Minimum cipher fragment size, in bytes, for DMA use.
 * @note    If set to zero then DMA is never used.
 * @note    If set to one then DMA is always used.
 */
#if !defined(STM32_CRY_CRYP_SIZE_THRESHOLD) || defined(__DOXYGEN__)
#define STM32_CRY_CRYP_SIZE_THRESHOLD       1024
#endif

/**
 * @brief   HASH DMA error hook.
 * @note    The default action for DMA errors is a system halt because DMA
 *          errors can only happen because of programming errors.
 */
#if !defined(STM32_CRY_HASH_DMA_ERROR_HOOK) || defined(__DOXYGEN__)
#define STM32_CRY_HASH_DMA_ERROR_HOOK(cryp) chSysHalt("DMA failure")
#endif

/**
 * @brief   CRYP DMA error hook.
 * @note    The default action for DMA errors is a system halt because DMA
 *          errors can only happen because of programming errors.
 */
#if !defined(STM32_CRY_CRYP_DMA_ERROR_HOOK) || defined(__DOXYGEN__)
#define STM32_CRY_CRYP_DMA_ERROR_HOOK(cryp) chSysHalt("DMA failure")
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32_HAS_CRYP1)
#define STM32_HAS_CRYP1                     FALSE
#endif

#if !defined(STM32_HAS_HASH1)
#define STM32_HAS_HASH1                     FALSE
#endif

#if STM32_CRY_USE_CRYP1 && !STM32_HAS_CRYP1
#error "CRYP1 not present in the selected device"
#endif

#if STM32_CRY_USE_HASH1 && !STM32_HAS_HASH1
#error "HASH1 not present in the selected device"
#endif

#if !STM32_CRY_USE_CRYP1 && !STM32_CRY_USE_HASH1
#error "CRY driver activated but no CRYP nor HASH peripheral assigned"
#endif

/**
 * @brief   CRYP1 uses DMA.
 */
#if (STM32_CRY_USE_CRYP1 && (STM32_CRY_CRYP_SIZE_THRESHOLD != 0)) ||        \
    defined(__DOXYGEN__)
#define STM32_CRY_CRYP1_USE_DMA             TRUE
#else
#define STM32_CRY_CRYP1_USE_DMA             FALSE
#endif

/**
 * @brief   HASH1 uses DMA.
 */
#if (STM32_CRY_USE_HASH1 && (STM32_CRY_HASH_SIZE_THRESHOLD != 0)) ||        \
    defined(__DOXYGEN__)
#define STM32_CRY_HASH1_USE_DMA             TRUE
#else
#define STM32_CRY_HASH1_USE_DMA             FALSE
#endif

#if STM32_CRY_CRYP_SIZE_THRESHOLD < 0
#error "invalid STM32_CRY_CRYP_SIZE_THRESHOLD value"
#endif

#if STM32_CRY_HASH_SIZE_THRESHOLD < 0
#error "invalid STM32_CRY_HASH_SIZE_THRESHOLD value"
#endif

#if STM32_CRY_CRYP1_USE_DMA
#if !defined(STM32_CRY_CRYP1_IN_DMA_STREAM) ||                              \
    !defined(STM32_CRY_CRYP1_OUT_DMA_STREAM)
#error "CRYP1 DMA streams not defined"
#endif

#if !STM32_DMA_IS_VALID_STREAM(STM32_CRY_CRYP1_IN_DMA_STREAM)
#error "Invalid DMA stream assigned to CRYP1_IN"
#endif

#if !STM32_DMA_IS_VALID_STREAM(STM32_CRY_CRYP1_OUT_DMA_STREAM)
#error "Invalid DMA stream assigned to CRYP1_OUT"
#endif

#if !STM32_DMA_IS_VALID_PRIORITY(STM32_CRY_CRYP1_IN_DMA_PRIORITY)
#error "Invalid DMA priority assigned to CRYP1_IN"
#endif

#if !STM32_DMA_IS_VALID_PRIORITY(STM32_CRY_CRYP1_OUT_DMA_PRIORITY)
#error "Invalid DMA priority assigned to CRYP1_OUT"
#endif

#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_CRY_CRYP1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to CRYP1"
#endif

#if !STM32_DMA_SUPPORTS_DMAMUX
#if !STM32_DMA_IS_VALID_ID(STM32_CRY_CRYP1_IN_DMA_STREAM,                   \
                           STM32_CRYP1_IN_DMA_MSK)
#error "invalid DMA stream associated to CRYP1_IN"
#endif

#if !STM32_DMA_IS_VALID_ID(STM32_CRY_CRYP1_OUT_DMA_STREAM,                  \
                           STM32_CRYP1_OUT_DMA_MSK)
#error "invalid DMA stream associated to CRYP1_OUT"
#endif
#endif /* !STM32_DMA_SUPPORTS_DMAMUX */
#endif /* STM32_CRY_CRYP1_USE_DMA */

#if STM32_CRY_HASH1_USE_DMA
#if !defined(STM32_CRY_HASH1_DMA_STREAM)
#error "HASH1 DMA stream not defined"
#endif

#if !STM32_DMA_IS_VALID_STREAM(STM32_CRY_HASH1_DMA_STREAM)
#error "Invalid DMA stream assigned to HASH1"
#endif

#if !STM32_DMA_IS_VALID_PRIORITY(STM32_CRY_HASH1_DMA_PRIORITY)
#error "Invalid DMA priority assigned to HASH1"
#endif

#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_CRY_HASH1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to HASH1"
#endif

#if !STM32_DMA_SUPPORTS_DMAMUX
#if !STM32_DMA_IS_VALID_ID(STM32_CRY_HASH1_DMA_STREAM, STM32_HASH1_DMA_MSK)
#error "invalid DMA stream associated to HASH1"
#endif
#endif /* !STM32_DMA_SUPPORTS_DMAMUX */
#endif /* STM32_CRY_HASH1_USE_DMA */

#if (STM32_CRY_CRYP1_USE_DMA || STM32_CRY_HASH1_USE_DMA) &&                \
    !defined(STM32_DMA_REQUIRED)
#define STM32_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Caller-owned stream context.
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
   * @brief   Class of the stream.
   */
  cry_class_t               cl;
  /**
   * @brief   The stream holds the CRYP engine.
   */
  bool                      cryp_held;
  /**
   * @brief   The stream holds the HASH engine.
   */
  bool                      hash_held;
  /**
   * @brief   The stream holds the transient key.
   */
  bool                      tkey_held;
  /**
   * @brief   Size of the buffered partial block or word, in bytes.
   */
  size_t                    partial_size;
  /**
   * @brief   Buffered partial AES block, or deferred hash bytes.
   */
  uint8_t                   partial[16];
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   LLD configuration fields.
 */
#define cry_lld_config_fields                                               \
  /* No configuration needed.*/                                             \
  uint32_t                  dummy

#if (STM32_CRY_USE_CRYP1 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   CRYP related driver fields.
 */
#define cry_lld_cryp_fields                                                 \
  /* CRYP engine unit, true when free.*/                                    \
  bool                      cryp_free;                                      \
  /* Transient key loaded.*/                                                \
  bool                      tkey_loaded;                                    \
  /* Operations holding the transient key.*/                                \
  unsigned                  tkey_holds;                                     \
  /* KEYSIZE field for the transient key.*/                                 \
  uint32_t                  tkey_ksize;                                     \
  /* Transient key in key registers order, right-aligned.*/                 \
  uint32_t                  tkey[8];                                        \
  /* Thread waiting for CRYP DMA completion.*/                              \
  thread_reference_t        cryp_tr;                                        \
  /* CRYP DMA streams, NULL when DMA is not used.*/                         \
  const stm32_dma_stream_t  *cryp_dma_in;                                   \
  const stm32_dma_stream_t  *cryp_dma_out;
#else
#define cry_lld_cryp_fields
#endif

#if (STM32_CRY_USE_HASH1 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   HASH related driver fields.
 */
#define cry_lld_hash_fields                                                 \
  /* HASH engine unit, true when free.*/                                    \
  bool                      hash_free;                                      \
  /* Thread waiting for HASH DMA completion.*/                              \
  thread_reference_t        hash_tr;                                        \
  /* HASH DMA stream, NULL when DMA is not used.*/                          \
  const stm32_dma_stream_t  *hash_dma;
#else
#define cry_lld_hash_fields
#endif

/**
 * @brief   LLD driver fields.
 */
#define cry_lld_driver_fields                                               \
  cry_lld_cryp_fields                                                       \
  cry_lld_hash_fields                                                       \
  /* Incremented by cry_lld_stop(), invalidating all open contexts.*/       \
  uint32_t                  generation

/**
 * @brief   Returns the driver owning an operation context.
 *
 * @param[in] op        initialized operation context
 * @return              The owning driver, or NULL if idle or invalidated.
 *
 * @notapi
 */
#define cry_lld_operation_driver(op)                                        \
  ((((op)->driver != NULL) &&                                               \
    ((op)->generation == (op)->driver->generation)) ? (op)->driver : NULL)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern hal_crypto_driver_c CRYD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void cry_lld_init(void);
  void cry_lld_object_init(hal_crypto_driver_c *cryp);
  msg_t cry_lld_start(hal_crypto_driver_c *cryp);
  void cry_lld_stop(hal_crypto_driver_c *cryp);
  const hal_crypto_config_t *cry_lld_setcfg(hal_crypto_driver_c *cryp,
                                            const hal_crypto_config_t *config);
  const hal_crypto_config_t *cry_lld_selcfg(hal_crypto_driver_c *cryp,
                                            unsigned cfgnum);
  msg_t cry_lld_get_capabilities(hal_crypto_driver_c *cryp,
                                 cry_algorithm_t algorithm,
                                 cry_capabilities_t *caps);
  msg_t cry_lld_key_load(hal_crypto_driver_c *cryp, crykey_t key,
                         const cry_key_params_t *params,
                         cry_key_format_t format, size_t size,
                         const uint8_t *data);
  msg_t cry_lld_hash_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                           cry_algorithm_t algorithm);
  msg_t cry_lld_cipher_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                             crykey_t key, cry_algorithm_t algorithm,
                             cry_direction_t direction,
                             const cry_cipher_params_t *params);
  msg_t cry_lld_mac_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                          crykey_t key, cry_algorithm_t algorithm,
                          bool verify, size_t tag_size);
  msg_t cry_lld_aead_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                           crykey_t key, cry_algorithm_t algorithm,
                           cry_direction_t direction,
                           const cry_aead_params_t *params);
  msg_t cry_lld_hash_update(cry_operation_t *op, size_t size,
                            const uint8_t *in);
  msg_t cry_lld_mac_update(cry_operation_t *op, size_t size,
                           const uint8_t *in);
  msg_t cry_lld_cipher_update(cry_operation_t *op, size_t size,
                              const uint8_t *in, size_t out_size,
                              uint8_t *out, size_t *out_length);
  msg_t cry_lld_aead_update(cry_operation_t *op, size_t size,
                            const uint8_t *in, size_t out_size,
                            uint8_t *out, size_t *out_length);
  msg_t cry_lld_aead_update_aad(cry_operation_t *op, size_t size,
                                const uint8_t *in);
  msg_t cry_lld_hash_final(cry_operation_t *op, size_t out_size,
                           uint8_t *out, size_t *out_length);
  msg_t cry_lld_cipher_final(cry_operation_t *op, size_t out_size,
                             uint8_t *out, size_t *out_length);
  msg_t cry_lld_mac_final(cry_operation_t *op, size_t out_size,
                          uint8_t *out, size_t *out_length);
  msg_t cry_lld_mac_verify(cry_operation_t *op, size_t tag_size,
                           const uint8_t *tag);
  msg_t cry_lld_aead_final(cry_operation_t *op, size_t out_size,
                           uint8_t *out, size_t *out_length,
                           size_t tag_size, uint8_t *tag);
  msg_t cry_lld_aead_verify(cry_operation_t *op, size_t out_size,
                            uint8_t *out, size_t *out_length,
                            size_t tag_size, const uint8_t *tag);
  void cry_lld_abort(cry_operation_t *op);
  msg_t cry_lld_execute(hal_crypto_driver_c *cryp, crykey_t key,
                        cry_algorithm_t algorithm, const cry_job_t *job);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CRY == TRUE */

#endif /* HAL_CRYPTO_LLD_H */

/** @} */
