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

/*------------------------------------------------------------------------*/
/* Sample code of OS dependent controls for FatFs                         */
/* (C)ChaN, 2014                                                          */
/*------------------------------------------------------------------------*/

#include "hal.h"
#include "ff.h"

#if FF_FS_REENTRANT
/* Synchronization objects, one for each volume plus the system one used
   with FF_FS_LOCK.*/
static semaphore_t ff_sem[FF_VOLUMES + 1];

/**
 * @brief   Creates the synchronization object of a volume.
 *
 * @param[in] vol       volume number, @p FF_VOLUMES for the system object
 * @return              Always 1, the object is created.
 */
int ff_mutex_create(int vol) {

  chSemObjectInit(&ff_sem[vol], 1);
  return 1;
}

/**
 * @brief   Deletes the synchronization object of a volume.
 *
 * @param[in] vol       volume number, @p FF_VOLUMES for the system object
 */
void ff_mutex_delete(int vol) {

  chSemReset(&ff_sem[vol], 0);
}

/**
 * @brief   Requests the grant to access a volume.
 *
 * @param[in] vol       volume number, @p FF_VOLUMES for the system object
 * @return              The operation result.
 * @retval 1            if the grant was obtained.
 * @retval 0            if the timeout expired.
 */
int ff_mutex_take(int vol) {

  msg_t msg = chSemWaitTimeout(&ff_sem[vol], (sysinterval_t)FF_FS_TIMEOUT);
  return msg == MSG_OK;
}

/**
 * @brief   Releases the grant to access a volume.
 *
 * @param[in] vol       volume number, @p FF_VOLUMES for the system object
 */
void ff_mutex_give(int vol) {

  chSemSignal(&ff_sem[vol]);
}
#endif /* FF_FS_REENTRANT */

#if FF_USE_LFN == 3	/* LFN with a working buffer on the heap */
/**
 * @brief   Allocates a memory block for the LFN working buffer.
 *
 * @param[in] size      size of the block
 * @return              Pointer to the block, @p NULL if not enough memory.
 */
void *ff_memalloc(UINT size) {

  return chHeapAlloc(NULL, size);
}

/**
 * @brief   Frees a memory block allocated by @p ff_memalloc().
 *
 * @param[in] mblock    pointer to the block
 */
void ff_memfree(void *mblock) {

  chHeapFree(mblock);
}
#endif /* FF_USE_LFN == 3 */
