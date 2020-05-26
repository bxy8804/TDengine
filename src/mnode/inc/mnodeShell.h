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

#ifndef TDENGINE_MGMT_SHELL_H
#define TDENGINE_MGMT_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "mnodeDef.h"

int32_t mgmtInitShell();
void    mgmtCleanUpShell();
void    mgmtAddShellMsgHandle(uint8_t msgType, void (*fp)(SMnodeMsg *queuedMsg));

typedef int32_t (*SShowMetaFp)(STableMetaMsg *pMeta, SShowObj *pShow, void *pConn);
typedef int32_t (*SShowRetrieveFp)(SShowObj *pShow, char *data, int32_t rows, void *pConn);
void mnodeAddShowMetaHandle(uint8_t showType, SShowMetaFp fp);
void mnodeAddShowRetrieveHandle(uint8_t showType, SShowRetrieveFp fp);

void mgmtAddToShellQueue(SMnodeMsg *queuedMsg);
void mgmtDealyedAddToShellQueue(SMnodeMsg *queuedMsg);
void mgmtSendSimpleResp(void *thandle, int32_t code);

bool  mgmtCheckQhandle(uint64_t qhandle);
void *mgmtSaveQhandle(void *qhandle, int32_t size);
void  mgmtFreeQhandle(void *qhandle, bool forceRemove);

void *mgmtMallocQueuedMsg(SRpcMsg *rpcMsg);
void *mgmtCloneQueuedMsg(SMnodeMsg *pSrcMsg);
void  mgmtFreeQueuedMsg(SMnodeMsg *pMsg);

#ifdef __cplusplus
}
#endif

#endif