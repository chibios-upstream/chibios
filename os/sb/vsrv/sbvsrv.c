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
 * @details Services registry, publishing and attaching, notifications,
 *          synchronous messages, service queries and termination
 *          cleanup.
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
 * @brief   List of the registered services.
 */
static vsrv_service_t *vsrv_list;

/**
 * @brief   Number of registered services, next identifier.
 */
static uint32_t vsrv_nservices;

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
 * Returns true if a name is not used by a registered service.
 */
static bool is_name_unique(const char *name) {
  const vsrv_service_t *svcp;

  for (svcp = vsrv_list; svcp != NULL; svcp = svcp->next) {
    if (strcmp(svcp->def->name, name) == 0) {
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
  vsrv_service_t *svcp;

  for (svcp = vsrv_list; svcp != NULL; svcp = svcp->next) {
    if (strcmp(svcp->def->name, name) == 0) {
      return svcp;
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

  for (svcp = vsrv_list; svcp != NULL; svcp = svcp->next) {
    if (svcp->id == id) {
      if ((svcp->def->server != sbp) || !is_online(svcp)) {
        return NULL;
      }
      return svcp;
    }
  }

  return NULL;
}

/*
 * Returns the handle entry of a sandbox, NULL if not in use.
 */
static vsrv_handle_t *get_handle(sb_class_t *sbp, uint32_t h) {
  const vsrv_conf_t *confp = sbp->vsrvconf;

  if ((h >= (uint32_t)confp->nhandles) || (confp->handles[h].svcp == NULL)) {
    return NULL;
  }

  return &confp->handles[h];
}

/*
 * Returns true if a handle refers to the running instance of its service.
 */
static bool is_handle_current(const vsrv_handle_t *hp) {

  return is_online(hp->svcp) && (hp->generation == hp->svcp->generation);
}

/*
 * Appends slice j to the pending requests queue, called under lock.
 */
static void rq_enqueue(vsrv_service_t *svcp, unsigned j) {
  vsrv_slice_t *slices = svcp->def->slices;

  slices[j].next = VSRV_NO_SLICE;
  if (svcp->rqtail == VSRV_NO_SLICE) {
    svcp->rqhead = (uint8_t)j;
  }
  else {
    slices[svcp->rqtail].next = (uint8_t)j;
  }
  svcp->rqtail = (uint8_t)j;
}

/*
 * Removes the oldest slice from the pending requests queue, called under
 * lock. Returns VSRV_NO_SLICE if the queue is empty.
 */
static unsigned rq_dequeue(vsrv_service_t *svcp) {
  vsrv_slice_t *slices = svcp->def->slices;
  unsigned j = svcp->rqhead;

  if (j != VSRV_NO_SLICE) {
    svcp->rqhead = slices[j].next;
    if (svcp->rqhead == VSRV_NO_SLICE) {
      svcp->rqtail = VSRV_NO_SLICE;
    }
    slices[j].next = VSRV_NO_SLICE;
  }

  return j;
}

/*
 * Removes slice j from the pending requests queue, called under lock.
 */
static void rq_remove(vsrv_service_t *svcp, unsigned j) {
  vsrv_slice_t *slices = svcp->def->slices;
  unsigned prev = VSRV_NO_SLICE;
  unsigned k = svcp->rqhead;

  while ((k != VSRV_NO_SLICE) && (k != j)) {
    prev = k;
    k = slices[k].next;
  }
  if (k == VSRV_NO_SLICE) {
    return;
  }

  if (prev == VSRV_NO_SLICE) {
    svcp->rqhead = slices[j].next;
  }
  else {
    slices[prev].next = slices[j].next;
  }
  if (svcp->rqtail == (uint8_t)j) {
    svcp->rqtail = (uint8_t)prev;
  }
  slices[j].next = VSRV_NO_SLICE;
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

    slp->next = VSRV_NO_SLICE;
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
  if (slp->rqstate == VSRV_RQ_PENDING) {
    rq_remove(svcp, hp->slice);
  }
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
  if (!is_valid_vrq(sbp, vrq)) {
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
  ectxp->r3 = (uint32_t)dp->slicesize;

  return (msg_t)svcp->id;
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
  vsrv_handle_t *hp;

  hp = get_handle(sbp, VSRV_CALL_ID(ectxp->r0));
  if (hp == NULL) {
    return CH_RET_EBADF;
  }

  sbUnmapRegion(sbp, hp->region);
  slice_release(hp);
  hp->svcp = NULL;

  return CH_RET_SUCCESS;
}

static msg_t vsrv_send(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t msg = ectxp->r1;
  const vsrv_handle_t *hp;
  vsrv_service_t *svcp;
  vsrv_slice_t *slp;
  msg_t ret;

  hp = get_handle(sbp, VSRV_CALL_ID(ectxp->r0));
  if (hp == NULL) {
    return CH_RET_EBADF;
  }

  svcp = hp->svcp;
  if (!is_handle_current(hp)) {
    return SB_VSRV_RET_STALE;
  }

  slp = &svcp->def->slices[hp->slice];
  if (slp->rqstate != VSRV_RQ_IDLE) {
    return CH_RET_EBUSY;
  }

  /* A command structure must be in the slice of the sender, aligned, with
     room for its header. The server still validates it.*/
  if (msg < SB_VSRV_MSG_FIXED_BASE) {
    uint32_t base = (uint32_t)get_slice_base(svcp->def, hp->slice);

    if (!MEM_IS_ALIGNED(msg, 4U) || (msg < base) ||
        ((msg - base) > (svcp->def->slicesize - SB_VSRV_CMD_HDR_SIZE))) {
      return CH_RET_EFAULT;
    }
  }

  ret = sbVRQRaisePidI(svcp->def->server, svcp->server_pid, svcp->vrq,
                       SB_VSRV_FL_REQUESTS);
  if (CH_RET_IS_ERROR(ret)) {
    return SB_VSRV_RET_STALE;
  }

  slp->request = msg;
  slp->rqstate = VSRV_RQ_PENDING;
  rq_enqueue(svcp, hp->slice);

  return CH_RET_SUCCESS;
}

static msg_t vsrv_result(sb_class_t *sbp, struct port_extctx *ectxp) {
  const vsrv_handle_t *hp;
  vsrv_slice_t *slp;

  hp = get_handle(sbp, VSRV_CALL_ID(ectxp->r0));
  if (hp == NULL) {
    return CH_RET_EBADF;
  }
  if (!is_handle_current(hp)) {
    return SB_VSRV_RET_STALE;
  }

  slp = &hp->svcp->def->slices[hp->slice];
  switch (slp->rqstate) {
  case VSRV_RQ_REPLIED:
    ectxp->r1 = (uint32_t)slp->status;
    slp->rqstate = VSRV_RQ_IDLE;
    return CH_RET_SUCCESS;
  case VSRV_RQ_PENDING:
  case VSRV_RQ_RECEIVED:
    return SB_VSRV_RET_PENDING;
  default:
    return CH_RET_EINVAL;
  }
}

static msg_t vsrv_cancel(sb_class_t *sbp, struct port_extctx *ectxp) {
  const vsrv_handle_t *hp;
  vsrv_slice_t *slp;

  hp = get_handle(sbp, VSRV_CALL_ID(ectxp->r0));
  if (hp == NULL) {
    return CH_RET_EBADF;
  }
  if (!is_handle_current(hp)) {
    return SB_VSRV_RET_STALE;
  }

  slp = &hp->svcp->def->slices[hp->slice];
  switch (slp->rqstate) {
  case VSRV_RQ_PENDING:
    rq_remove(hp->svcp, hp->slice);
    slp->rqstate = VSRV_RQ_IDLE;
    return CH_RET_SUCCESS;
  case VSRV_RQ_RECEIVED:
    return CH_RET_EBUSY;
  default:
    return CH_RET_EINVAL;
  }
}

static msg_t vsrv_receive(sb_class_t *sbp, struct port_extctx *ectxp) {
  vsrv_service_t *svcp;
  vsrv_slice_t *slp;
  unsigned j;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }

  j = rq_dequeue(svcp);
  if (j == VSRV_NO_SLICE) {
    return SB_VSRV_RET_EMPTY;
  }

  slp = &svcp->def->slices[j];
  slp->rqstate = VSRV_RQ_RECEIVED;
  ectxp->r1 = slp->request;
  ectxp->r2 = (uint32_t)get_client_index(svcp->def, slp->client);
  ectxp->r3 = (uint32_t)slp->client_pid;

  return (msg_t)j;
}

static msg_t vsrv_reply(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t j = VSRV_CALL_SLICE(ectxp->r0);
  vsrv_service_t *svcp;
  vsrv_slice_t *slp;
  msg_t ret;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }
  if (j >= (uint32_t)svcp->def->nslices) {
    return CH_RET_EINVAL;
  }

  /* The client went away, the reply is discarded.*/
  slp = &svcp->def->slices[j];
  if ((slp->state != VSRV_SLICE_ATTACHED) ||
      (slp->generation != svcp->generation)) {
    return SB_VSRV_RET_STALE;
  }
  if (slp->rqstate != VSRV_RQ_RECEIVED) {
    return CH_RET_EINVAL;
  }

  ret = sbVRQRaisePidI(slp->client, slp->client_pid, slp->vrq,
                       SB_VSRV_FL_REPLY);
  if (CH_RET_IS_ERROR(ret)) {
    return SB_VSRV_RET_STALE;
  }

  slp->status  = (msg_t)ectxp->r1;
  slp->rqstate = VSRV_RQ_REPLIED;

  return CH_RET_SUCCESS;
}

static msg_t vsrv_notify(sb_class_t *sbp, struct port_extctx *ectxp) {
  const vsrv_handle_t *hp;
  vsrv_service_t *svcp;
  msg_t ret;

  hp = get_handle(sbp, VSRV_CALL_ID(ectxp->r0));
  if (hp == NULL) {
    return CH_RET_EBADF;
  }

  svcp = hp->svcp;
  if (!is_handle_current(hp)) {
    return SB_VSRV_RET_STALE;
  }

  ret = sbVRQRaisePidI(svcp->def->server, svcp->server_pid, svcp->vrq,
                       SB_VSRV_FL_SLICE(hp->slice));
  if (CH_RET_IS_ERROR(ret)) {
    return SB_VSRV_RET_STALE;
  }

  return CH_RET_SUCCESS;
}

static msg_t vsrv_notify_client(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t j = VSRV_CALL_SLICE(ectxp->r0);
  const vsrv_slice_t *slp;
  vsrv_service_t *svcp;
  msg_t ret;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }
  if (j >= (uint32_t)svcp->def->nslices) {
    return CH_RET_EINVAL;
  }

  slp = &svcp->def->slices[j];
  if ((slp->state != VSRV_SLICE_ATTACHED) ||
      (slp->generation != svcp->generation)) {
    return SB_VSRV_RET_STALE;
  }

  ret = sbVRQRaisePidI(slp->client, slp->client_pid, slp->vrq,
                       SB_VSRV_FL_NOTIFY);
  if (CH_RET_IS_ERROR(ret)) {
    return SB_VSRV_RET_STALE;
  }

  return CH_RET_SUCCESS;
}

static msg_t vsrv_reset_done(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t j = VSRV_CALL_SLICE(ectxp->r0);
  vsrv_service_t *svcp;
  vsrv_slice_t *slp;

  svcp = get_served(sbp, VSRV_CALL_ID(ectxp->r0));
  if (svcp == NULL) {
    return CH_RET_EBADF;
  }
  if (j >= (uint32_t)svcp->def->nslices) {
    return CH_RET_EINVAL;
  }

  slp = &svcp->def->slices[j];
  if (slp->state != VSRV_SLICE_RESET) {
    return CH_RET_EINVAL;
  }
  slp->state = VSRV_SLICE_FREE;

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
 * @brief   VSRV module initialization.
 * @note    Called by @p sbHostInit().
 *
 * @notapi
 */
void __sb_vsrv_init(void) {

  vsrv_list      = NULL;
  vsrv_nservices = 0U;
  chMtxObjectInit(&vsrv_mtx);
}

/**
 * @brief   Initializes a service object.
 * @details Binds the service object to its definition and registers it,
 *          the service is known by name and starts offline.
 * @pre     The static regions of the server and of the clients are set,
 *          they are not started yet.
 *
 * @param[out] svcp     pointer to the @p vsrv_service_t object
 * @param[in] defp      pointer to the service definition
 *
 * @init
 */
void sbVsrvObjectInit(vsrv_service_t *svcp, const vsrv_service_def_t *defp) {
  unsigned j;

  chDbgCheck((svcp != NULL) && (defp != NULL));
  chDbgCheck((defp->name != NULL) && (defp->server != NULL) &&
             (defp->clients != NULL) && (defp->area != NULL) &&
             (defp->slices != NULL));
  chDbgCheck((strlen(defp->name) > (size_t)0) &&
             (strlen(defp->name) <= SB_VSRV_NAME_MAX) &&
             is_name_unique(defp->name));
  chDbgCheck((defp->nslices > 0U) && (defp->nslices <= SB_VSRV_MAX_SLICES));
#if !defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
  chDbgCheck((defp->slicesize >= 32U) &&
             ((defp->slicesize & (defp->slicesize - 1U)) == 0U) &&
             MEM_IS_ALIGNED(defp->area, defp->slicesize));
#else
  chDbgCheck((defp->slicesize >= 32U) &&
             MEM_IS_ALIGNED(defp->slicesize, 32U) &&
             MEM_IS_ALIGNED(defp->area, 32U));
#endif
  chDbgCheck(is_area_isolated(defp));

  svcp->def        = defp;
  svcp->state      = VSRV_SVC_OFFLINE;
  svcp->generation = 0U;
  svcp->server_pid = (sb_pid_t)0;
  svcp->vrq        = (sb_vrqnum_t)0;
  svcp->region     = 0U;
  svcp->rqhead     = VSRV_NO_SLICE;
  svcp->rqtail     = VSRV_NO_SLICE;
  for (j = 0U; j < defp->nslices; j++) {
    memset(&defp->slices[j], 0, sizeof (vsrv_slice_t));
    defp->slices[j].state = VSRV_SLICE_FREE;
    defp->slices[j].next  = VSRV_NO_SLICE;
  }

  /* Registration, the list can be walked by running sandboxes.*/
  chSysLock();
  svcp->id   = vsrv_nservices++;
  svcp->next = vsrv_list;
  vsrv_list  = svcp;
  chSysUnlock();
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
  vsrv_service_t *svcp;
  unsigned i;

  if (sbp->vsrvconf == NULL) {
    return;
  }

  chMtxLock(&vsrv_mtx);

  for (svcp = vsrv_list; svcp != NULL; svcp = svcp->next) {
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

  if (sbp->vsrvconf == NULL) {
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

  if (sbp->vsrvconf == NULL) {
    ectxp->r0 = (uint32_t)CH_RET_EACCES;
    return;
  }

  CH_IRQ_PROLOGUE();
  chSysLockFromISR();
  switch (VSRV_CALL_SUBCODE(ectxp->r0)) {
  case SB_VSRV_SEND:
    ret = vsrv_send(sbp, ectxp);
    break;
  case SB_VSRV_RESULT:
    ret = vsrv_result(sbp, ectxp);
    break;
  case SB_VSRV_CANCEL:
    ret = vsrv_cancel(sbp, ectxp);
    break;
  case SB_VSRV_RECEIVE:
    ret = vsrv_receive(sbp, ectxp);
    break;
  case SB_VSRV_REPLY:
    ret = vsrv_reply(sbp, ectxp);
    break;
  case SB_VSRV_NOTIFY:
    ret = vsrv_notify(sbp, ectxp);
    break;
  case SB_VSRV_NOTIFYC:
    ret = vsrv_notify_client(sbp, ectxp);
    break;
  case SB_VSRV_RESETDONE:
    ret = vsrv_reset_done(sbp, ectxp);
    break;
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
