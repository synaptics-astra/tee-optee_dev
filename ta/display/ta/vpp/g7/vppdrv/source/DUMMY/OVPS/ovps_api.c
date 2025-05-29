/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       ovps_api.c
//! \brief      dummy implementation of ovpslib
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#define _OVPS_API_C_
#include "vpp_module.h"
#include "ovps_api.h"

int MV_OVPS_Create(void)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_Open_Client(INT m_uchClientId)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_Close_Client(INT m_uchClientId)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_PushRequest(INT m_uchClientId, OVPS_CLIENT_REQUEST *pstOvpsRequest)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_ISR_TaskHandler(UINT32 uieventSts)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_PopRequest(OVPS_CLIENT_REQUEST **pstOvpsRequest)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_GetNoOfRequestsWaiting(INT m_uchClientId, UINT32 *puiRequestsWaiting)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_ReleaseRequest(INT m_uchClientId,OVPS_CLIENT_REQUEST **pstOvpsRequest)
{
    return (MV_VPP_OK);
}

int MV_OVPS_Destroy(void)
{
    return (MV_VPP_OK);
}

int OVPS_RegEventCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr)
{
    return (MV_VPP_OK);
}

INT MV_OVPS_Suspend(BOOL optimize)
{
    return MV_VPP_OK;
}

INT MV_OVPS_EnableClkGate(void)
{
    return MV_VPP_OK;
}

int MV_OVPS_Resume(BOOL optimize)
{
    return MV_VPP_OK;
}
int MV_OVPS_TimerStop(void)
{
    return MV_VPP_OK;
}
