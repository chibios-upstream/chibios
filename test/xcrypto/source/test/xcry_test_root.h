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
 * @file    xcry_test_root.h
 * @brief   Test Suite root structures header.
 */

#ifndef XCRY_TEST_ROOT_H
#define XCRY_TEST_ROOT_H

#include "ch_test.h"

#include "xcry_test_sequence_001.h"
#include "xcry_test_sequence_002.h"
#include "xcry_test_sequence_003.h"
#include "xcry_test_sequence_004.h"
#include "xcry_test_sequence_005.h"
#include "xcry_test_sequence_006.h"
#include "xcry_test_sequence_007.h"
#include "xcry_test_sequence_008.h"
#include "xcry_test_sequence_009.h"
#include "xcry_test_sequence_010.h"
#include "xcry_test_sequence_011.h"

#if !defined(__DOXYGEN__)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern const testsuite_t xcry_test_suite;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Shared definitions.                                                       */
/*===========================================================================*/

#include "hal.h"
#include "xcry_vectors.h"

#define TEST_SUITE_NAME "ChibiOS/XHAL Crypto Test Suite"

/* Driver under test, the default is the first LLD instance.*/
#if !defined(XCRY_TEST_DRIVER)
#define XCRY_TEST_DRIVER                    CRYD1
#endif
#define XCRY_DRIVER                         (&XCRY_TEST_DRIVER)

/* Size of each shared buffer, in bytes.*/
#define XCRY_BUFFER_SIZE                    5120U

/* Buffers accessed by DMA, placed in non-cacheable memory where needed.*/
extern uint32_t __nocache_xcry_in[XCRY_BUFFER_SIZE / 4U];
extern uint32_t __nocache_xcry_out[XCRY_BUFFER_SIZE / 4U];
#define xcry_in                             ((uint8_t *)__nocache_xcry_in)
#define xcry_out                            ((uint8_t *)__nocache_xcry_out)

bool xcry_unsupported(cry_algorithm_t algorithm);
bool xcry_key_supported(cry_algorithm_t algorithm, size_t size);
msg_t xcry_load_key(cry_key_type_t type, size_t size, const uint8_t *key);
msg_t xcry_cipher(const xcry_cipher_vector_t *v, cry_direction_t direction,
                  const uint8_t *in, uint8_t *out, size_t frag);
msg_t xcry_aead(const xcry_aead_vector_t *v, cry_direction_t direction,
                const uint8_t *in, uint8_t *out, uint8_t *tag,
                size_t tag_size, size_t frag, bool declare);
bool xcry_aead_optional(const xcry_aead_vector_t *v, msg_t msg);
msg_t xcry_mac(const xcry_mac_vector_t *v, bool verify, uint8_t *tag,
               size_t tag_size, size_t frag);
msg_t xcry_hash(cry_algorithm_t algorithm, const uint8_t *msg, size_t size,
                uint8_t *out, size_t frag);

/* Largest digest or tag size, in bytes.*/
#define XCRY_DIGEST_MAX                     64U

#endif /* !defined(__DOXYGEN__) */

#endif /* XCRY_TEST_ROOT_H */
