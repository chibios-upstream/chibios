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
 * @file    sb/vsrv/sbvsrv.h
 * @brief   ARM SandBox host VSRV module macros and structures.
 * @details Virtual SeRVices (VSRV) host side. The host application defines
 *          the services, binding names to a server sandbox, the allowed
 *          client sandboxes and a shared area, and grants each sandbox the
 *          right to publish and to attach with its VSRV settings.
 * @note    This is a prototype of the interface, the module is not
 *          implemented yet, see @p os/sb/note_sb_vsrv.md.
 *
 * @addtogroup ARM_SANDBOX_VSRV
 * @{
 */

#ifndef SBVSRV_H
#define SBVSRV_H

#if (SB_CFG_ENABLE_VSRV == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Sandbox VSRV settings flags
 * @{
 */
/**
 * @brief   The sandbox can publish the services defined for it.
 */
#define VSRV_CONF_PUBLISH       (1U << 0)
/**
 * @brief   The sandbox can open the services allowing it.
 */
#define VSRV_CONF_ATTACH        (1U << 1)
/** @} */

/**
 * @brief   No slice, end of the pending requests queue.
 */
#define VSRV_NO_SLICE           0xFFU

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if SB_CFG_ENABLE_VRQ != TRUE
#error "VSRV requires SB_CFG_ENABLE_VRQ == TRUE"
#endif

#if CH_CFG_USE_MUTEXES != TRUE
#error "VSRV requires CH_CFG_USE_MUTEXES == TRUE"
#endif

#if PORT_SWITCHED_REGIONS_NUMBER == 0
#error "VSRV requires PORT_SWITCHED_REGIONS_NUMBER > 0"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Service states.
 */
typedef enum {
  VSRV_SVC_OFFLINE = 0,                 /**< Not published.                 */
  VSRV_SVC_ONLINE                       /**< Published by its server.       */
} vsrv_svc_state_t;

/**
 * @brief   Slice states.
 */
typedef enum {
  VSRV_SLICE_FREE = 0,                  /**< No block, can be attached.     */
  VSRV_SLICE_ATTACHED,                  /**< Attached to a client.          */
  VSRV_SLICE_RESET                      /**< Released, the server can still
                                             be using it.                   */
} vsrv_slice_state_t;

/**
 * @brief   Request states.
 */
typedef enum {
  VSRV_RQ_IDLE = 0,                     /**< No request in flight.          */
  VSRV_RQ_PENDING,                      /**< Queued for the server.         */
  VSRV_RQ_RECEIVED,                     /**< Taken by the server.           */
  VSRV_RQ_REPLIED                       /**< Replied, waiting for result.   */
} vsrv_rq_state_t;

/**
 * @brief   Type of a service object.
 */
typedef struct vsrv_service vsrv_service_t;

/**
 * @brief   Type of a slice object.
 * @note    The host reads and writes these fields, the application only
 *          provides the storage, see @p vsrv_service_def_t.
 */
typedef struct {
  /**
   * @brief   Slice state.
   */
  vsrv_slice_state_t            state;
  /**
   * @brief   Request state.
   */
  vsrv_rq_state_t               rqstate;
  /**
   * @brief   Attached client.
   */
  sb_class_t                    *client;
  /**
   * @brief   PID of the attached client execution.
   */
  sb_pid_t                      client_pid;
  /**
   * @brief   Service generation at attach time.
   */
  uint32_t                      generation;
  /**
   * @brief   Client VRQ.
   */
  sb_vrqnum_t                   vrq;
  /**
   * @brief   Request word, a fixed message or a command structure pointer.
   */
  uint32_t                      request;
  /**
   * @brief   Reply status.
   */
  msg_t                         status;
  /**
   * @brief   Next slice in the pending requests queue.
   */
  uint8_t                       next;
} vsrv_slice_t;

/**
 * @brief   Type of a service definition.
 * @details The area is made of @p nslices blocks of @p slicesize bytes,
 *          slice j always uses block j, there is no allocator.
 * @note    On ARMv7-M the slice size is a power of two, at least 32, and the
 *          area is aligned to it; on ARMv8-M the slice size is a multiple of
 *          32 and the area is aligned to 32 bytes. The area must be
 *          representable as one MPU region of the server and disjoint from
 *          every other area, from the private regions of all sandboxes and
 *          from host data.
 */
typedef struct {
  /**
   * @brief   Service name, at most @p SB_VSRV_NAME_MAX characters.
   */
  const char                    *name;
  /**
   * @brief   Server sandbox.
   */
  sb_class_t                    *server;
  /**
   * @brief   Allowed client sandboxes, @p NULL terminated.
   * @note    The index of a client in this array is its identity as seen
   *          by the server.
   */
  sb_class_t * const            *clients;
  /**
   * @brief   Shared area, @p nslices blocks of @p slicesize bytes.
   */
  uint8_t                       *area;
  /**
   * @brief   Slice size.
   */
  size_t                        slicesize;
  /**
   * @brief   Slice objects storage, one per client attached at the same
   *          time.
   */
  vsrv_slice_t                  *slices;
  /**
   * @brief   Number of slices, at most @p SB_VSRV_MAX_SLICES.
   */
  unsigned                      nslices;
} vsrv_service_def_t;

/**
 * @brief   Structure representing a service object.
 */
struct vsrv_service {
  /**
   * @brief   Next registered service.
   */
  vsrv_service_t                *next;
  /**
   * @brief   Service identifier, assigned at registration.
   */
  uint32_t                      id;
  /**
   * @brief   Service definition.
   */
  const vsrv_service_def_t      *def;
  /**
   * @brief   Service state.
   */
  vsrv_svc_state_t              state;
  /**
   * @brief   Generation, incremented when the service goes online.
   */
  uint32_t                      generation;
  /**
   * @brief   PID of the publishing server execution.
   */
  sb_pid_t                      server_pid;
  /**
   * @brief   Server VRQ.
   */
  sb_vrqnum_t                   vrq;
  /**
   * @brief   Server switched region holding the area mapping.
   */
  unsigned                      region;
  /**
   * @brief   Pending requests queue head, a slice index.
   */
  uint8_t                       rqhead;
  /**
   * @brief   Pending requests queue tail, a slice index.
   */
  uint8_t                       rqtail;
};

/**
 * @brief   Type of a handle table entry.
 * @note    The host reads and writes these fields, the application only
 *          provides the storage, see @p vsrv_conf_t.
 */
typedef struct {
  /**
   * @brief   Service, @p NULL if the entry is free.
   */
  vsrv_service_t                *svcp;
  /**
   * @brief   Service generation at open time.
   */
  uint32_t                      generation;
  /**
   * @brief   Attached slice index.
   */
  uint8_t                       slice;
  /**
   * @brief   Switched region holding the slice mapping.
   */
  uint8_t                       region;
} vsrv_handle_t;

/**
 * @brief   Type of a sandbox VSRV settings structure.
 */
typedef struct vsrv_conf {
  /**
   * @brief   Rights of the sandbox, @p VSRV_CONF_xxx flags.
   */
  uint32_t                      flags;
  /**
   * @brief   Handle table storage.
   */
  vsrv_handle_t                 *handles;
  /**
   * @brief   Number of handles, open at the same time.
   */
  unsigned                      nhandles;
} vsrv_conf_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void __sb_vsrv_init(void);
  void sbVsrvObjectInit(vsrv_service_t *svcp, const vsrv_service_def_t *defp);
  void __sb_vsrv_cleanup(sb_class_t *sbp);
  void sb_sysc_vsrv(sb_class_t *sbp, struct port_extctx *ectxp);
  void sb_fastc_vsrv(sb_class_t *sbp, struct port_extctx *ectxp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SB_CFG_ENABLE_VSRV == TRUE */

#endif /* SBVSRV_H */

/** @} */
