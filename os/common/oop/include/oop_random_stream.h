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
 * @file        oop_random_stream.h
 * @brief       Generated Random Stream Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  OOP_RANDOM_STREAM
 * @brief       Random data streams interface.
 * @{
 */

#ifndef OOP_RANDOM_STREAM_H
#define OOP_RANDOM_STREAM_H

#include "oop_random_stream.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Seek modes
 * @{
 */
/**
 * @brief       The file offset is set to offset bytes.
 */
#define RSTM_SEEK_SET                       0

/**
 * @brief       The file offset is set to its current location plus offset
 *              bytes.
 */
#define RSTM_SEEK_CUR                       1

/**
 * @brief       The file offset is set to the size of the file plus offset
 *              bytes.
 */
#define RSTM_SEEK_END                       2
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
 * @interface   random_stream_i
 * @extends     sequential_stream_i
 *
 * @brief       Random data streams interface.
 * @details     This module define an interface for generic random data
 *              streams.
 *              This interface is meant to be implemented in classes requiring
 *              file-like capability.
 *
 * @name        Interface @p random_stream_i structures
 * @{
 */

/**
 * @brief       Type of a random stream interface.
 */
typedef struct random_stream random_stream_i;

/**
 * @brief       Interface @p random_stream_i virtual methods table.
 */
struct random_stream_vmt {
  /* Memory offset between this interface structure and begin of
     the implementing class structure.*/
  size_t instance_offset;
  /* From base_interface_i.*/
  /* From sequential_stream_i.*/
  size_t (*write)(void *ip, const uint8_t *bp, size_t n);
  size_t (*read)(void *ip, uint8_t *bp, size_t n);
  int (*put)(void *ip, uint8_t b);
  int (*get)(void *ip);
  int (*unget)(void *ip, int b);
  /* From random_stream_i.*/
  int32_t (*seek)(void *ip, int32_t offset, int whence);
  int32_t (*getsize)(void *ip);
};

/**
 * @brief       Structure representing a random stream interface.
 */
struct random_stream {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct random_stream_vmt *vmt;
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Virtual methods of random_stream_i
 * @{
 */
/**
 * @brief       Random Stream seek.
 * @details     This function changes the stream read/write offset. On failure
 *              the offset is not changed.
 *
 * @param[in,out] ip            Pointer to a @p random_stream_i instance.
 * @param[in]     offset        The stream offset, relative to the position
 *                              selected by @p whence.
 * @param[in]     whence        Seek mode.
 * @return                      The resulting offset, in bytes from the
 *                              beginning of the stream, or a negative error
 *                              code.
 */
CC_FORCE_INLINE
static inline int32_t rstmSeek(void *ip, int32_t offset, int whence) {
  random_stream_i *self = (random_stream_i *)ip;

  return self->vmt->seek(ip, offset, whence);
}

/**
 * @brief       Random Stream size.
 *
 * @param[in,out] ip            Pointer to a @p random_stream_i instance.
 * @return                      The stream size in bytes or a negative error
 *                              code.
 */
CC_FORCE_INLINE
static inline int32_t rstmGetSize(void *ip) {
  random_stream_i *self = (random_stream_i *)ip;

  return self->vmt->getsize(ip);
}
/** @} */

#endif /* OOP_RANDOM_STREAM_H */

/** @} */
