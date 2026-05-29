/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_DEBUG_H_
#define _OVP_DEBUG_H_

#include "ovp_module.h"
#include "ovp_common.h"
#include "ErrorCode.h"
#include "ovp_api.h"
#include "ovp_apifuncs.h"
#include "galois_io.h"

#include "vdec_com.h"
#include "ovp_cmd.h"

INT DumpOvpRegs(OVP_OBJ *pstOvpObj);
INT DumpOvpCmdRegs(OVP_OBJ *pstOvpObj);
void OvpFrameUpdateHalfFieldCnt(OVP_OBJ *pstOvpObj, UINT32 u32HalfFieldType);
void OvpFrameUpdateInCnt(OVP_OBJ *pstOvpObj);
void OvpFrameUpdateOutCnt(OVP_OBJ *pstOvpObj);
void OvpFrameUpdate(OVP_OBJ *pstOvpObj, UINT32 addr, UINT32 size, UINT32 width, UINT32 height);
void OvpResetFrameCnt(OVP_OBJ *pstOvpObj);
void OvpResetAllFrameCnt(void);

#endif /* _OVP_DEBUG_H_ */
