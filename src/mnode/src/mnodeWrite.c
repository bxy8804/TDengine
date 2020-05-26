/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include "os.h"
#include "taosdef.h"
#include "tsched.h"
#include "tbalance.h"
#include "tgrant.h"
#include "ttimer.h"
#include "tglobal.h"
#include "dnode.h"
#include "mnodeDef.h"
#include "mnodeInt.h"
#include "mnodeServer.h"
#include "mnodeAcct.h"
#include "mnodeDnode.h"
#include "mnodeMnode.h"
#include "mnodeDb.h"
#include "mnodeSdb.h"
#include "mnodeVgroup.h"
#include "mnodeUser.h"
#include "mnodeTable.h"
#include "mnodeShell.h"

static void (*tsMnodeProcessWriteMsgFp[TSDB_MSG_TYPE_MAX])(SMnodeMsg *);

void mnodeAddWriteMsgHandle(uint8_t msgType, void (*fp)(SMnodeMsg *pMsg)) {
  tsMnodeProcessWriteMsgFp[msgType] = fp;
}

int32_t mnodeProcessWrite(SMnodeMsg *pMsg) {
  SRpcMsg *rpcMsg = &pMsg->rpcMsg;
  if (rpcMsg->pCont == NULL) {
    mError("%p, msg:%s content is null", rpcMsg->ahandle, taosMsg[rpcMsg->msgType]);
    return TSDB_CODE_INVALID_MSG_LEN;
  }

  if (!sdbIsMaster()) {
    SMnodeRsp *rpcRsp = &pMsg->rpcRsp;
    SRpcIpSet *ipSet = rpcMallocCont(sizeof(SRpcIpSet));
    mgmtGetMnodeIpSetForShell(ipSet);
    rpcRsp->rsp = ipSet;
    rpcRsp->len = sizeof(SRpcIpSet);

    mTrace("%p, msg:%s will be redireced, inUse:%d", rpcMsg->ahandle, taosMsg[rpcMsg->msgType], ipSet->inUse);
    for (int32_t i = 0; i < ipSet->numOfIps; ++i) {
      mTrace("mnode index:%d ip:%s:%d", i, ipSet->fqdn[i], htons(ipSet->port[i]));
    }

    return TSDB_CODE_REDIRECT;
  }

   if (grantCheck(TSDB_GRANT_TIME) != TSDB_CODE_SUCCESS) {
    mError("%p, msg:%s not processed, grant time expired", rpcMsg->ahandle, taosMsg[rpcMsg->msgType]);
    return TSDB_CODE_GRANT_EXPIRED;
  }

  if (tsMnodeProcessReadMsgFp[rpcMsg->msgType] == NULL) {
    mError("%p, msg:%s not processed, no handle exist", rpcMsg->ahandle, taosMsg[rpcMsg->msgType]);
    return TSDB_CODE_MSG_NOT_PROCESSED;
  }

  if (!mnodeInitMsg(pMsg)) {
    mError("%p, msg:%s not processed, reason:%s", rpcMsg->ahandle, taosMsg[rpcMsg->msgType], tstrerror(terrno));
    return terrno;
  }

  if (!pMsg->pUser->writeAuth) {
    mError("%p, msg:%s not processed, no rights", rpcMsg->ahandle, taosMsg[rpcMsg->msgType]);
    return TSDB_CODE_NO_RIGHTS;
  }

  return (*tsMnodeProcessWriteMsgFp[rpcMsg->msgType])(pMsg);
}

static void mgmtDoDealyedAddToShellQueue(void *param, void *tmrId) {
  mgmtAddToShellQueue(param);
}

void mgmtDealyedAddToShellQueue(SMnodeMsg *queuedMsg) {
  void *unUsed = NULL;
  taosTmrReset(mgmtDoDealyedAddToShellQueue, 300, queuedMsg, tsMgmtTmr, &unUsed);
}