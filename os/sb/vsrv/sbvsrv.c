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
 * @file    sb/vsrv/sbvsrv.c
 * @brief   ARM SandBox host VSRV module code.
 * @details Services registry, publishing and attaching, service queries
 *          and termination cleanup. Notifications and messages are not
 *          implemented yet, their sub-codes return @p CH_RET_ENOSYS.
 *
 * @addtogroup ARM_SANDBOX_VSRV
 * @{
 */

#include <string.h>

#include "sb.h"

#if (SB_CFG_ENABLE_VSRV == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
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

/**
 * @brief   Registered services, @p NULL before @p sbVsrvInit().
 */
static vsrv_service_t *vsrv_services;

/**
 * @brief   Number of registered services.
 */
static unsigned vsrv_nservices;

/**
 * @brief   Mutex serializing the VSRV syscalls and cleanups.
 * @note    The state shared with the fastcalls is also changed under the
 *          kernel lock.
 */
static mutex_t vsrv_mtx;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static size_t get_area_size(const vsrv_service_def_t *dp) {

  return (size_t)dp->nslices * dp->slicesize;
}

static uint8_t *get_slice_base(const vsrv_service_def_t *dp, unsigned j) {

  return dp->area + ((size_t)j * dp->slicesize);
}

/*
 * Returns true if an area overlaps one of the static regions of a sandbox.
 */
static bool is_overlapping_regions(const sb_class_t *sbp,
                                   const uint8_t *base, size_t size) {
  unsigned i;

  for (i = 0U; i < (unsigned)SB_CFG_NUM_REGIONS; i++) {
    const sb_memory_region_t *rp = &sbp->regions[i];

    if (!sb_reg_is_unused(rp) &&
        (base < (rp->area.base + rp->area.size)) &&
        (rp->area.base < (base + size))) {
      return true;
    }
  }

  return false;
}

/*
 * Returns true if the area of a service definition is disjoint from the
 * static regions of its server and of its clients.
 */
static bool is_area_isolated(const vsrv_service_def_t *dp) {
  sb_class_t * const *cpp;

  if (is_overlapping_regions(dp->server, dp->area, get_area_size(dp))) {
    return false;
  }
  for (cpp = dp->clients; *cpp != NULL; cpp++) {
    if (is_overlapping_regions(*cpp, dp->area, get_area_size(dp))) {
      return false;
    }
  }

  return true;
}

/*
 * Returns true if the name of definition i is not used by a previous one.
 */
static bool is_name_unique(const vsrv_service_def_t *defs, unsigned i) {
  unsigned k;

  for (k = 0U; k < i; k++) {
    if (strcmp(defs[k].name, defs[i].name) == 0) {
      return false;
    }
  }

  return true;
}

/*
 * Copies a service name from the sandbox memory, once.
 */
static msg_t copy_name(sb_class_t *sbp, const char *name, char *buf) {
  size_t n;

  n = sb_check_string(sbp, name, SB_VSRV_NAME_MAX + 1U);
  if (n == (size_t)0) {
    return CH_RET_EFAULT;
  }

  memcpy(buf, name, n);
  buf[n - 1U] = '\0';

  return CH_RET_SUCCESS;
}

static vsrv_service_t *find_service(const char *name) {
  unsigned i;

  for (i = 0U; i < vsrv_nservices; i++) {
    if (strcmp(vsrv_services[i].def->name, name) == 0) {
      return &vsrv_services[i];
    }
  }

  return NULL;
}

/*
 * Returns the index of a sandbox in the clients of a service or -1.
 */
static int get_client_index(const vsrv_service_def_t *dp,
                            const sb_class_t *sbp) {
  int i;

  for (i = 0; dp->clients[i] != NULL; i++) {
    if (dp->clients[i] == sbp) {
      return i;
    }
  }

  return -1;
}

static bool is_valid_vrq(const sb_class_t *sbp, uint32_t vrq) {

  return vrq < (uint32_t)(sizeof sbp->vrq.flags / sizeof sbp->vrq.flags[0]);
}

/*
 * Returns true if the service is online with the server execution that
 * published it.
 */
static bool is_online(const vsrv_service_t *svcp) {

  return (svcp->state == VSRV_SVC_ONLINE) &&
         (sbGetPidX(svcp->def->server) == svcp->server_pid);
}

/*
 * Returns the service identified by id if served by this execution of the
 * sandbox.
 */
static vsrv_service_t *get_served(sb_class_t *sbp, uint32_t id) {
  vsrv_service_t *svcp;

  if (id >= (uint32_t)vsrv_nservices) {
    return NULL;
  }

  svcp = &vsrv_services[id];
  if ((svcp->def->server != sbp) || !is_online(svcp)) {
    return NULL;
  }

  return svcp;
}

/*
 * Turns a service offline, the clients of the attached slices get the
 * service lost flag and keep their slices until they close the handles.
 * Called with the VSRV mutex taken.
 */
static void service_offline(vsrv_service_t *svcp) {
  const vsrv_service_def_t *dp = svcp->def;
  unsigned j;

  chSysLock();
  svcp->state  = VSRV_SVC_OFFLINE;
  svcp->rqhead = VSRV_NO_SLICE;
  svcp->rqtail = VSRV_NO_SLICE;
  for (j = 0U; j < dp->nslices; j++) {
    vsrv_slice_t *slp = &dp->slices[j];

    if (slp->state == VSRV_SLICE_ATTACHED) {
      /* Requests in flight are dropped, the results are stale.*/
      slp->rqstate = VSRV_RQ_IDLE;
      (void)sbVRQRaisePidS(slp->client, slp->client_pid, slp->vrq,
                           SB_VSRV_FL_LOST);
    }
    else if (slp->state == VSRV_SLICE_RESET) {
      slp->state = VSRV_SLICE_FREE;
    }
  }
  chSysUnlock();
}

/*
 * Releases the slice of a handle, the slice goes to RESET if the server
 * instance that served it is still running, otherwise to FREE. Called with
 * the VSRV mutex taken.
 */
static void slice_release(const vsrv_handle_t *hp) {
  vsrv_service_t *svcp = hp->svcp;
  vsrv_slice_t *slp = &svcp->def->slices[hp->slice];

  chSysLock();
  slp->rqstate    = VSRV_RQ_IDLE;
  slp->client     = NULL;
  slp->client_pid = (sb_pid_t)0;
  if (is_online(svcp) && (hp->generation == svcp->generation)) {
    slp->state = VSRV_SLICE_RESET;
    (void)sbVRQRaisePidS(svcp->def->server, svcp->server_pid, svcp->vrq,
                         SB_VSRV_FL_SLICES);
  }
  else {
    slp->state = VSRV_SLICE_FREE;
  }
  chSysUnlock();
}

static msg_t vsrv_publish(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t vrq = VSRV_CALL_ID(ectxp->r0);
  char name[SB_VSRV_NAME_MAX + 1U];
  const vsrv_service_def_t *dp;
  vsrv_service_t *svcp;
  msg_t ret;
  unsigned j;

  if ((sbp->vsrvconf->flags & VSRV_CONF_PUBLISH) == 0U) {
    return CH_RET_EACCES;
  }

  ret = copy_name(sbp, (const char *)ectxp->r1, name);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }

  svcp = find_service(name);
  if (svcp == NULL) {
    return CH_RET_ENOENT;
  }
  dp = svcp->def;
  if (dp->server != sbp) {
    return CH_RET_EACCES;
  }
  if (is_online(svcp)) {
    return CH_RET_EBUSY;
  }
  if (!is_valid_vrq(sbp, vrq) || ((size_t)ectxp->r2 > dp->slicesize)) {
    return CH_RET_EINVAL;
  }

  ret = sbMapRegion(sbp, dp->area, get_area_size(dp), SB_REG_IS_DATA);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }

  /* No previous server instance is running, its slices in RESET are
     free.*/
  chSysLock();
  for (j = 0U; j < dp->nslices; j++) {
    if (dp->slices[j].state == VSRV_SLICE_RESET) {
      dp->slices[j].state = VSRV_SLICE_FREE;
    }
  }
  chSysUnlock();

  /* Clearing the blocks of the free slices, attached slices of a previous
     instance keep their content until their clients close them.*/
  for (j = 0U; j < dp->nslices; j++) {
    if (dp->slices[j].state == VSRV_SLICE_FREE) {
      memset(get_slice_base(dp, j), 0, dp->slicesize);
    }
  }

  chSysLock();
  svcp->region     = (unsigned)ret;
  svcp->vrq        = (sb_vrqnum_t)vrq;
  svcp->server_pid = sbGetPidX(sbp);
  svcp->rqhead     = VSRV_NO_SLICE;
  svcp->rqtail     = VSRV_NO_SLICE;
  svcp->generation++;
  svcp->state      = VSRV_SVC_ONLINE;
  chSysUnlock();

  ectxp->r1 = (uint32_t)dp->area;
  ectxp->r2 = (uint32_t)get_area_size(dp);

  return (msg_t)(svcp - vsrv_services);
}

static msg_t vsrv_withdraw(sb_class_t *sbp, struct port_extctx *ectxp) {
  vsrv_service_t *svcp;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }

  service_offline(svcp);
  sbUnmapRegion(sbp, svcp->region);

  return CH_RET_SUCCESS;
}

static msg_t vsrv_open(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t vrq = VSRV_CALL_ID(ectxp->r0);
  const vsrv_conf_t *confp = sbp->vsrvconf;
  char name[SB_VSRV_NAME_MAX + 1U];
  const vsrv_service_def_t *dp;
  vsrv_service_t *svcp;
  vsrv_handle_t *hp;
  vsrv_slice_t *slp;
  uint8_t *base;
  unsigned h, j;
  msg_t ret;

  if ((confp->flags & VSRV_CONF_ATTACH) == 0U) {
    return CH_RET_EACCES;
  }

  ret = copy_name(sbp, (const char *)ectxp->r1, name);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }

  svcp = find_service(name);
  if (svcp == NULL) {
    return CH_RET_ENOENT;
  }
  dp = svcp->def;
  if (get_client_index(dp, sbp) < 0) {
    return CH_RET_EACCES;
  }
  if (!is_online(svcp)) {
    return CH_RET_ENOENT;
  }
  if (!is_valid_vrq(sbp, vrq)) {
    return CH_RET_EINVAL;
  }

  /* One handle per service, also a stale one.*/
  hp = NULL;
  for (h = 0U; h < confp->nhandles; h++) {
    if (confp->handles[h].svcp == svcp) {
      return CH_RET_EEXIST;
    }
    if ((hp == NULL) && (confp->handles[h].svcp == NULL)) {
      hp = &confp->handles[h];
    }
  }
  if (hp == NULL) {
    return CH_RET_EMFILE;
  }

  for (j = 0U; j < dp->nslices; j++) {
    if (dp->slices[j].state == VSRV_SLICE_FREE) {
      break;
    }
  }
  if (j >= dp->nslices) {
    return CH_RET_EBUSY;
  }

  base = get_slice_base(dp, j);
  ret = sbMapRegion(sbp, base, dp->slicesize, SB_REG_IS_DATA);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  memset(base, 0, dp->slicesize);

  hp->generation = svcp->generation;
  hp->slice      = (uint8_t)j;
  hp->region     = (uint8_t)ret;
  hp->svcp       = svcp;

  slp = &dp->slices[j];
  chSysLock();
  slp->client     = sbp;
  slp->client_pid = sbGetPidX(sbp);
  slp->generation = svcp->generation;
  slp->vrq        = (sb_vrqnum_t)vrq;
  slp->rqstate    = VSRV_RQ_IDLE;
  slp->state      = VSRV_SLICE_ATTACHED;
  (void)sbVRQRaisePidS(dp->server, svcp->server_pid, svcp->vrq,
                       SB_VSRV_FL_SLICES);
  chSysUnlock();

  ectxp->r1 = (uint32_t)base;
  ectxp->r2 = (uint32_t)dp->slicesize;

  return (msg_t)(hp - confp->handles);
}

static msg_t vsrv_close(sb_class_t *sbp, struct port_extctx *ectxp) {
  const vsrv_conf_t *confp = sbp->vsrvconf;
  uint32_t h = VSRV_CALL_ID(ectxp->r0);
  vsrv_handle_t *hp;

  if ((h >= (uint32_t)confp->nhandles) || (confp->handles[h].svcp == NULL)) {
    return CH_RET_EBADF;
  }

  hp = &confp->handles[h];
  sbUnmapRegion(sbp, hp->region);
  slice_release(hp);
  hp->svcp = NULL;

  return CH_RET_SUCCESS;
}

static msg_t vsrv_slices(sb_class_t *sbp, struct port_extctx *ectxp) {
  const vsrv_service_def_t *dp;
  vsrv_service_t *svcp;
  uint32_t attached, reset;
  unsigned j;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }

  dp = svcp->def;
  attached = 0U;
  reset    = 0U;
  for (j = 0U; j < dp->nslices; j++) {
    const vsrv_slice_t *slp = &dp->slices[j];

    if ((slp->state == VSRV_SLICE_ATTACHED) &&
        (slp->generation == svcp->generation)) {
      attached |= 1U << j;
    }
    else if (slp->state == VSRV_SLICE_RESET) {
      reset |= 1U << j;
    }
  }
  ectxp->r1 = attached;
  ectxp->r2 = reset;

  return CH_RET_SUCCESS;
}

static msg_t vsrv_client(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t j = VSRV_CALL_SLICE(ectxp->r0);
  const vsrv_service_def_t *dp;
  const vsrv_slice_t *slp;
  vsrv_service_t *svcp;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }

  dp = svcp->def;
  if (j >= (uint32_t)dp->nslices) {
    return CH_RET_EINVAL;
  }
  slp = &dp->slices[j];
  if ((slp->state != VSRV_SLICE_ATTACHED) ||
      (slp->generation != svcp->generation)) {
    return CH_RET_ENOENT;
  }

  ectxp->r1 = (uint32_t)get_slice_base(dp, (unsigned)j);
  ectxp->r2 = (uint32_t)dp->slicesize;
  ectxp->r3 = (uint32_t)slp->client_pid;

  return (msg_t)get_client_index(dp, slp->client);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   VSRV initialization.
 * @details Binds the service objects to their definitions and makes the
 *          services known by name, all services start offline.
 * @pre     The static regions of the servers and of the clients are set,
 *          no sandbox is started yet.
 *
 * @param[out] svcs     array of @p n service objects
 * @param[in] defs      array of @p n service definitions
 * @param[in] n         number of services
 *
 * @init
 */
void sbVsrvInit(vsrv_service_t *svcs, const vsrv_service_def_t *defs,
                unsigned n) {
  unsigned i, j;

  chDbgCheck((svcs != NULL) && (defs != NULL) && (n > 0U));

  for (i = 0U; i < n; i++) {
    const vsrv_service_def_t *dp = &defs[i];
    vsrv_service_t *svcp = &svcs[i];

    chDbgCheck((dp->name != NULL) && (dp->server != NULL) &&
               (dp->clients != NULL) && (dp->area != NULL) &&
               (dp->slices != NULL));
    chDbgCheck((strlen(dp->name) > (size_t)0) &&
               (strlen(dp->name) <= SB_VSRV_NAME_MAX) &&
               is_name_unique(defs, i));
    chDbgCheck((dp->nslices > 0U) && (dp->nslices <= SB_VSRV_MAX_SLICES));
#if !defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
    chDbgCheck((dp->slicesize >= 32U) &&
               ((dp->slicesize & (dp->slicesize - 1U)) == 0U) &&
               MEM_IS_ALIGNED(dp->area, dp->slicesize));
#else
    chDbgCheck((dp->slicesize >= 32U) &&
               MEM_IS_ALIGNED(dp->slicesize, 32U) &&
               MEM_IS_ALIGNED(dp->area, 32U));
#endif
    chDbgCheck(is_area_isolated(dp));

    svcp->def        = dp;
    svcp->state      = VSRV_SVC_OFFLINE;
    svcp->generation = 0U;
    svcp->server_pid = (sb_pid_t)0;
    svcp->vrq        = (sb_vrqnum_t)0;
    svcp->region     = 0U;
    svcp->rqhead     = VSRV_NO_SLICE;
    svcp->rqtail     = VSRV_NO_SLICE;
    for (j = 0U; j < dp->nslices; j++) {
      memset(&dp->slices[j], 0, sizeof (vsrv_slice_t));
      dp->slices[j].state = VSRV_SLICE_FREE;
      dp->slices[j].next  = VSRV_NO_SLICE;
    }
  }

  chMtxObjectInit(&vsrv_mtx);
  vsrv_services  = svcs;
  vsrv_nservices = n;
}

/**
 * @brief   VSRV cleanup of a terminating sandbox.
 * @details The services published by the terminating execution go
 *          offline, its handles are released. Its mappings go with its
 *          regions table.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 *
 * @notapi
 */
void __sb_vsrv_cleanup(sb_class_t *sbp) {
  unsigned i;

  if ((vsrv_services == NULL) || (sbp->vsrvconf == NULL)) {
    return;
  }

  chMtxLock(&vsrv_mtx);

  for (i = 0U; i < vsrv_nservices; i++) {
    vsrv_service_t *svcp = &vsrv_services[i];

    if ((svcp->def->server == sbp) && (svcp->state == VSRV_SVC_ONLINE) &&
        (svcp->server_pid == sbGetPidX(sbp))) {
      service_offline(svcp);
    }
  }

  for (i = 0U; i < sbp->vsrvconf->nhandles; i++) {
    vsrv_handle_t *hp = &sbp->vsrvconf->handles[i];

    if (hp->svcp != NULL) {
      slice_release(hp);
      hp->svcp = NULL;
    }
  }

  chMtxUnlock(&vsrv_mtx);
}

/**
 * @brief   VSRV syscall handler.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] ectxp     pointer to the sandbox exception context
 *
 * @notapi
 */
void sb_sysc_vsrv(sb_class_t *sbp, struct port_extctx *ectxp) {
  msg_t ret;

  if ((vsrv_services == NULL) || (sbp->vsrvconf == NULL)) {
    ectxp->r0 = (uint32_t)CH_RET_EACCES;
    return;
  }

  chMtxLock(&vsrv_mtx);
  switch (VSRV_CALL_SUBCODE(ectxp->r0)) {
  case SB_VSRV_PUBLISH:
    ret = vsrv_publish(sbp, ectxp);
    break;
  case SB_VSRV_WITHDRAW:
    ret = vsrv_withdraw(sbp, ectxp);
    break;
  case SB_VSRV_OPEN:
    ret = vsrv_open(sbp, ectxp);
    break;
  case SB_VSRV_CLOSE:
    ret = vsrv_close(sbp, ectxp);
    break;
  default:
    ret = CH_RET_ENOSYS;
    break;
  }
  chMtxUnlock(&vsrv_mtx);

  ectxp->r0 = (uint32_t)ret;
}

/**
 * @brief   VSRV fastcall handler.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] ectxp     pointer to the sandbox exception context
 *
 * @notapi
 */
void sb_fastc_vsrv(sb_class_t *sbp, struct port_extctx *ectxp) {
  msg_t ret;

  if ((vsrv_services == NULL) || (sbp->vsrvconf == NULL)) {
    ectxp->r0 = (uint32_t)CH_RET_EACCES;
    return;
  }

  CH_IRQ_PROLOGUE();
  chSysLockFromISR();
  switch (VSRV_CALL_SUBCODE(ectxp->r0)) {
  case SB_VSRV_SLICES:
    ret = vsrv_slices(sbp, ectxp);
    break;
  case SB_VSRV_CLIENT:
    ret = vsrv_client(sbp, ectxp);
    break;
  default:
    ret = CH_RET_ENOSYS;
    break;
  }
  chSysUnlockFromISR();
  CH_IRQ_EPILOGUE();

  ectxp->r0 = (uint32_t)ret;
}

SB_SVC_HANDLER(SB_SYSC_VSRV, sb_sysc_vsrv);
SB_SVC_HANDLER(SB_FASTC_VSRV, sb_fastc_vsrv);

#endif /* SB_CFG_ENABLE_VSRV == TRUE */

/** @} */
