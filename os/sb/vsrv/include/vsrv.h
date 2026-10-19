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
 * @file    sb/vsrv/include/vsrv.h
 * @brief   ARM SandBox VSRV API.
 * @details Virtual SeRVices (VSRV) guest API. A server sandbox publishes
 *          named services, client sandboxes open them and exchange data
 *          through slices of a shared area mapped by the host. Commands
 *          are synchronous messages: a pointer to a command structure in
 *          the client slice or a fixed message, answered with a status
 *          code. Events are notifications. The services and the allowed
 *          sandboxes are defined by the host.
 * @note    No call blocks. Requests, replies, notifications and service
 *          events are signaled with flags on the VRQ chosen by the caller
 *          at publish or open, the VRQ flags are passed to the VRQ
 *          handler, see @p SB_VRQ_HANDLER() in @p sbuser.h and the
 *          @p SB_VSRV_FL_xxx flags in @p sbsysc.h.
 * @note    The host never reads or validates slices and messages, the
 *          receiving side validates them. The server checks that a command
 *          structure, and every pointer in it, is inside the slice of the
 *          requesting client, see @p vsrvIsValidRange(). Slices can change
 *          at any time, copy each field out once and validate the copy.
 * @note    Unless specified otherwise, functions returning @p msg_t return
 *          @p CH_RET_SUCCESS, a @p SB_VSRV_RET_xxx or a @p CH_RET_xxx error
 *          code. @p SB_VSRV_RET_STALE means that the service instance the
 *          handle refers to is gone, the handle can only be closed.
 * @note    This is a prototype of the API, the host side is not
 *          implemented yet, see @p os/sb/note_sb_vsrv.md.
 *
 * @addtogroup ARM_SANDBOX_USER_VSRV_API
 * @{
 */

#ifndef VSRV_H
#define VSRV_H

#include <stddef.h>

#include "sbuser.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Fixed messages
 * @details Request words from @p VSRV_MSG_FIXED_BASE up are fixed messages
 *          with a code in the low byte, never pointers to command
 *          structures: that range is the Cortex-M system area. Codes up to
 *          0x7F are generic, every server answers @p VSRV_MSG_NOP and
 *          @p VSRV_MSG_INFO; codes from 0x80 are defined by the services,
 *          see @p VSRV_MSG_FIXED().
 * @{
 */
/**
 * @brief   First fixed message.
 */
#define VSRV_MSG_FIXED_BASE     0xFFFFFF00U
/**
 * @brief   No operation, replies with @p CH_RET_SUCCESS.
 */
#define VSRV_MSG_NOP            0xFFFFFF00U
/**
 * @brief   Service information, replies with the service protocol version,
 *          see @p VSRV_VERSION().
 */
#define VSRV_MSG_INFO           0xFFFFFF01U
/**
 * @brief   The server drops its state for the slice, the slice layout
 *          restarts from its initial state.
 */
#define VSRV_MSG_RESET          0xFFFFFF02U
/**
 * @brief   First fixed message code defined by services.
 */
#define VSRV_MSG_SERVICE_CODE   0x80U
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a shared memory area descriptor.
 */
typedef struct {
  /**
   * @brief   Base address.
   */
  void                      *base;
  /**
   * @brief   Size in bytes.
   */
  size_t                    size;
} vsrv_area_t;

/**
 * @brief   Type of a request word.
 * @details A fixed message or a pointer to a command structure.
 */
typedef uint32_t vsrv_msg_t;

/**
 * @brief   Type of a command structure header.
 * @details Every command structure starts with this header, followed by
 *          the fields defined by the command.
 */
typedef struct {
  /**
   * @brief   Command, defined by the service.
   */
  uint32_t                  cmd;
  /**
   * @brief   Size of the whole structure, header included.
   */
  uint32_t                  size;
} vsrv_cmd_header_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Builds a fixed message defined by a service.
 *
 * @param[in] code      message code, from @p VSRV_MSG_SERVICE_CODE to 0xFF
 * @return              The fixed message.
 */
#define VSRV_MSG_FIXED(code)                                                \
  (VSRV_MSG_FIXED_BASE | ((uint32_t)(code) & 0xFFU))

/**
 * @brief   Checks if a request word is a fixed message.
 *
 * @param[in] msg       the request word
 * @return              The check result.
 */
#define VSRV_MSG_IS_FIXED(msg)  ((vsrv_msg_t)(msg) >= VSRV_MSG_FIXED_BASE)

/**
 * @brief   Code of a fixed message.
 *
 * @param[in] msg       the fixed message
 * @return              The message code.
 */
#define VSRV_MSG_CODE(msg)      ((uint32_t)(msg) & 0xFFU)

/**
 * @brief   Command structure pointer of a request word.
 * @note    The pointer is not validated.
 *
 * @param[in] msg       the request word, not a fixed message
 * @return              The command structure pointer.
 */
#define VSRV_MSG_CMD(msg)       ((vsrv_cmd_header_t *)(msg))

/**
 * @brief   Builds a service protocol version.
 * @note    The version is a non-negative @p msg_t, it is the reply status
 *          of @p VSRV_MSG_INFO.
 *
 * @param[in] major     major version, incremented on incompatible changes,
 *                      up to 0x7FFF
 * @param[in] minor     minor version
 * @return              The version value.
 */
#define VSRV_VERSION(major, minor)                                          \
  ((((uint32_t)(major) & 0x7FFFU) << 16) | ((uint32_t)(minor) & 0xFFFFU))

/**
 * @brief   Major part of a service protocol version.
 *
 * @param[in] v         the version value
 * @return              The major version.
 */
#define VSRV_VERSION_MAJOR(v)   ((uint32_t)(v) >> 16)

/**
 * @brief   Minor part of a service protocol version.
 *
 * @param[in] v         the version value
 * @return              The minor version.
 */
#define VSRV_VERSION_MINOR(v)   ((uint32_t)(v) & 0xFFFFU)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name    VSRV server API
 * @{
 */
/**
 * @brief   Publishes a service.
 * @details The caller must be the server defined by the host for the
 *          name. The service area is mapped in the caller, requests,
 *          notifications and slice changes are signaled on @p vrq.
 *
 * @param[in] name      service name, at most @p SB_VSRV_NAME_MAX characters
 * @param[in] minsize   minimum slice size required by the service protocol
 * @param[in] vrq       VRQ receiving the server flags
 * @param[out] areap    descriptor of the service area
 * @return              The service identifier or an error code.
 */
static inline msg_t vsrvPublish(const char *name, size_t minsize,
                                uint32_t vrq, vsrv_area_t *areap) {

  __syscall3rrrr(SB_SYSC_VSRV, VSRV_CALL(SB_VSRV_PUBLISH, vrq, 0U),
                 name, minsize);
  (void)r3;
  if (CH_RET_IS_SUCCESS(r0)) {
    areap->base = (void *)r1;
    areap->size = (size_t)r2;
  }
  return (msg_t)r0;
}

/**
 * @brief   Withdraws a service.
 * @details Same effect as the server termination for the service, then the
 *          service area is unmapped from the caller.
 *
 * @param[in] service   service identifier
 * @return              The operation status.
 */
static inline msg_t vsrvWithdraw(uint32_t service) {

  __syscall1r(SB_SYSC_VSRV, VSRV_CALL(SB_VSRV_WITHDRAW, service, 0U));
  return (msg_t)r0;
}

/**
 * @brief   Receives the oldest pending request.
 * @note    The request word is passed as sent by the client, it is not
 *          validated. Check it with @p VSRV_MSG_IS_FIXED() first, then
 *          validate the command structure against the client slice.
 *
 * @param[in] service   service identifier
 * @param[out] msgp     the request word
 * @return              The index of the slice of the requesting client,
 *                      @p SB_VSRV_RET_EMPTY if no request is pending or an
 *                      error code.
 */
static inline msg_t vsrvReceive(uint32_t service, vsrv_msg_t *msgp) {

  __syscall1rr(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_RECEIVE, service, 0U));
  if (CH_RET_IS_SUCCESS(r0)) {
    *msgp = (vsrv_msg_t)r1;
  }
  return (msg_t)r0;
}

/**
 * @brief   Replies to the request received from a slice.
 * @note    If the client went away the reply is discarded and
 *          @p SB_VSRV_RET_STALE is returned.
 *
 * @param[in] service   service identifier
 * @param[in] slice     slice index
 * @param[in] status    reply status, @p CH_RET_SUCCESS, a non-negative
 *                      result defined by the command or a @p CH_RET_xxx
 *                      error code
 * @return              The operation status.
 */
static inline msg_t vsrvReply(uint32_t service, uint32_t slice,
                              msg_t status) {

  __syscall2r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_REPLY, service, slice),
              status);
  return (msg_t)r0;
}

/**
 * @brief   Notifies the client of a slice.
 *
 * @param[in] service   service identifier
 * @param[in] slice     slice index
 * @return              The operation status.
 */
static inline msg_t vsrvNotifyClient(uint32_t service, uint32_t slice) {

  __syscall1r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_NOTIFYC, service, slice));
  return (msg_t)r0;
}

/**
 * @brief   Returns the state of the slices.
 *
 * @param[in] service   service identifier
 * @param[out] attachedp mask of the slices attached to clients
 * @param[out] resetp   mask of the slices waiting for
 *                      @p vsrvResetDone()
 * @return              The operation status.
 */
static inline msg_t vsrvGetSlices(uint32_t service, uint32_t *attachedp,
                                  uint32_t *resetp) {

  __syscall1rrrr(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_SLICES, service, 0U));
  (void)r3;
  if (CH_RET_IS_SUCCESS(r0)) {
    *attachedp = r1;
    *resetp    = r2;
  }
  return (msg_t)r0;
}

/**
 * @brief   Returns the client of a slice and the slice descriptor.
 *
 * @param[in] service   service identifier
 * @param[in] slice     slice index
 * @param[out] slicep   descriptor of the slice
 * @return              The index of the client in the host definition of
 *                      the service or an error code.
 */
static inline msg_t vsrvGetClient(uint32_t service, uint32_t slice,
                                  vsrv_area_t *slicep) {

  __syscall1rrrr(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_CLIENT, service, slice));
  (void)r3;
  if (CH_RET_IS_SUCCESS(r0)) {
    slicep->base = (void *)r1;
    slicep->size = (size_t)r2;
  }
  return (msg_t)r0;
}

/**
 * @brief   Checks that a range is inside a slice.
 * @note    Alignment is not checked.
 *
 * @param[in] slicep    descriptor of the slice
 * @param[in] p         start of the range
 * @param[in] size      size of the range
 * @return              The check result.
 * @retval true         if the range is inside the slice.
 * @retval false        if the range is not inside the slice.
 */
static inline bool vsrvIsValidRange(const vsrv_area_t *slicep,
                                    const void *p, size_t size) {
  uintptr_t base = (uintptr_t)slicep->base;
  uintptr_t addr = (uintptr_t)p;

  return (addr >= base) && (size <= slicep->size) &&
         ((addr - base) <= (slicep->size - size));
}

/**
 * @brief   Releases a slice left by its client.
 * @details The server no longer uses the slice, it can be given to another
 *          client.
 *
 * @param[in] service   service identifier
 * @param[in] slice     slice index
 * @return              The operation status.
 */
static inline msg_t vsrvResetDone(uint32_t service, uint32_t slice) {

  __syscall1r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_RESETDONE, service, slice));
  return (msg_t)r0;
}
/** @} */

/**
 * @name    VSRV client API
 * @{
 */
/**
 * @brief   Opens a service.
 * @details The caller must be a client allowed by the host for the name.
 *          A slice is allocated in the service area and mapped in the
 *          caller, notifications, replies and the service loss are
 *          signaled on @p vrq.
 * @note    The final size can be larger than requested because of the MPU
 *          alignment constraints, do not assume a rounding rule.
 *
 * @param[in] name      service name, at most @p SB_VSRV_NAME_MAX characters
 * @param[in] size      requested slice size, zero for the service minimum
 * @param[in] vrq       VRQ receiving the client flags
 * @param[out] slicep   descriptor of the slice, with its final size
 * @return              The handle or an error code.
 */
static inline msg_t vsrvOpen(const char *name, size_t size,
                             uint32_t vrq, vsrv_area_t *slicep) {

  __syscall3rrrr(SB_SYSC_VSRV, VSRV_CALL(SB_VSRV_OPEN, vrq, 0U),
                 name, size);
  (void)r3;
  if (CH_RET_IS_SUCCESS(r0)) {
    slicep->base = (void *)r1;
    slicep->size = (size_t)r2;
  }
  return (msg_t)r0;
}

/**
 * @brief   Closes a handle.
 * @details The slice is unmapped from the caller and released, also for
 *          stale handles.
 *
 * @param[in] handle    handle returned by @p vsrvOpen()
 * @return              The operation status.
 */
static inline msg_t vsrvClose(uint32_t handle) {

  __syscall1r(SB_SYSC_VSRV, VSRV_CALL(SB_VSRV_CLOSE, handle, 0U));
  return (msg_t)r0;
}

/**
 * @brief   Sends a request.
 * @details The command structure, and the data it points to, must be in
 *          the slice of the handle; they belong to the server until the
 *          reply. The reply is signaled by @p SB_VSRV_FL_REPLY and
 *          retrieved with @p vsrvResult().
 *
 * @param[in] handle    handle returned by @p vsrvOpen()
 * @param[in] cmdp      pointer to the command structure
 * @return              The operation status.
 * @retval CH_RET_EBUSY if a request is already in flight on the handle.
 */
static inline msg_t vsrvSend(uint32_t handle, vsrv_cmd_header_t *cmdp) {

  __syscall2r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_SEND, handle, 0U), cmdp);
  return (msg_t)r0;
}

/**
 * @brief   Sends a fixed message.
 * @details The reply is signaled by @p SB_VSRV_FL_REPLY and retrieved with
 *          @p vsrvResult().
 *
 * @param[in] handle    handle returned by @p vsrvOpen()
 * @param[in] msg       a @p VSRV_MSG_xxx message or a service message built
 *                      with @p VSRV_MSG_FIXED()
 * @return              The operation status.
 * @retval CH_RET_EBUSY if a request is already in flight on the handle.
 */
static inline msg_t vsrvSendFixed(uint32_t handle, vsrv_msg_t msg) {

  __syscall2r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_SEND, handle, 0U), msg);
  return (msg_t)r0;
}

/**
 * @brief   Retrieves the reply to the request in flight.
 * @note    Results are read from the command structure, after validation.
 *
 * @param[in] handle    handle returned by @p vsrvOpen()
 * @param[out] statusp  the status replied by the server
 * @return              The operation status.
 * @retval CH_RET_SUCCESS if the reply has been retrieved, the handle can
 *                      send again.
 * @retval SB_VSRV_RET_PENDING if the reply is not available yet.
 */
static inline msg_t vsrvResult(uint32_t handle, msg_t *statusp) {

  __syscall1rr(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_RESULT, handle, 0U));
  if ((msg_t)r0 == CH_RET_SUCCESS) {
    *statusp = (msg_t)r1;
  }
  return (msg_t)r0;
}

/**
 * @brief   Cancels the request in flight.
 * @note    Only a request not yet received by the server can be cancelled.
 *
 * @param[in] handle    handle returned by @p vsrvOpen()
 * @return              The operation status.
 * @retval CH_RET_EBUSY if the server already received the request.
 */
static inline msg_t vsrvCancel(uint32_t handle) {

  __syscall1r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_CANCEL, handle, 0U));
  return (msg_t)r0;
}

/**
 * @brief   Notifies the server.
 *
 * @param[in] handle    handle returned by @p vsrvOpen()
 * @return              The operation status.
 */
static inline msg_t vsrvNotify(uint32_t handle) {

  __syscall1r(SB_FASTC_VSRV, VSRV_CALL(SB_VSRV_NOTIFY, handle, 0U));
  return (msg_t)r0;
}
/** @} */

#endif /* VSRV_H */

/** @} */
