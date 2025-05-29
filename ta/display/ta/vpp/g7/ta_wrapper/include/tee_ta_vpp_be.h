/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_BE_TA_H_
#define _VPP_BE_TA_H_

#include "tee_ta.h"
#include "com_type.h"

int VPP_HDMICmdHandler(int handle);

HRESULT VPP_DeleteHdmiTask(void);
HRESULT VPP_DeleteCECTask(void);
HRESULT VPP_CreateHdmiTask(void);
HRESULT VPP_CreateCECTask(void);
int VPP_RegHdcpCallback(int handle, UINT32 eventCode,
                UINT64 funcAddr, UINT64 contextAddr);
int VPP_UnRegHdcpCallback(int handle);
int VPP_RegHdmiCallback(int handle, UINT32 eventCode,
                UINT64 funcAddr, UINT64 contextAddr);
int VPP_UnRegHdmiCallback(int handle);
int VPP_RegEarcCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr);
int VPP_UnRegEarcCallback(int handle);
int VPP_RegCecCallback(int handle, UINT32 eventCode,
                UINT64 funcAddr, UINT64 contextAddr);
int VPP_UnRegCecCallback(int handle);
int VPP_RegHdmiCpcb0Callback(int handle, UINT64 funcAddr);
int VPP_UnRegHdmiCpcb0Callback(int handle);
int VPP_GetCECEvtInfo(int handle, void *pshmbuf);
int VPP_GetEarcEvtInfo(int handle, void *pbuf);
int VPP_SetHdmi5vState(int state);
int VPP_HDMI_HPDTaskHandler(int handle);
int VPP_CECHandler(int handle);
HRESULT VPP_CreateHdmiHpdTask(void);
HRESULT VPP_DeleteHdmiHpdTask(void);
int VPP_UpdateDynamicList(UINT8* mnfrId, UINT8* prodId);

#endif /* _VPP_TA_H_ */
