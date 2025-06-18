/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_BE_TA_H_
#define _VPP_BE_TA_H_

#ifndef CONFIG_OPTEE
#include "tee_ta.h"
#endif
#include "com_type.h"

int VPP_HDMICmdHandler(int handle);
int VPP_GetHDMICmdSem(int handle, UINT64 *addr);

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
int VPP_RegCecCallback(int handle, UINT32 eventCode,
                UINT64 funcAddr, UINT64 contextAddr);
int VPP_UnRegCecCallback(int handle);
int VPP_RegHdmiCpcb0Callback(int handle, UINT64 funcAddr);
int VPP_UnRegHdmiCpcb0Callback(int handle);
int VPP_GetCECEvtInfo(int handle, void *pshmbuf);

#ifdef CONFIG_AMP_HDMI_HDCP_2X
int VPP_HDCP2XHandler(int handle);
int VPP_CreateHdcp2XTask(void);
int VPP_DestroyHdcp2XTask(void);
#endif //CONFIG_AMP_HDMI_HDCP_2X
#endif /* _VPP_TA_H_ */
