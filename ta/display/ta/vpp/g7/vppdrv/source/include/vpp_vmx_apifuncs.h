/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_vmx_apifuncs.h
//! \brief      VMX watermark API's used by VPP API/internal functions
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_VMX_APIFUNCS_H_
#define _VPP_VMX_APIFUNCS_H_

#include "vpp_module.h"

int VPP_VMXAPI_StartDataChannelLoader(VPP_OBJ *vpp_obj, INT chanID);

void VPP_VMXAPI_Notify(VPP_OBJ *vpp_obj, unsigned int eventcode, void *params);

int VPP_VMXAPI_Init(VPP_OBJ *vpp_obj);

void VPP_VMXAPI_Destroy(VPP_OBJ *vpp_obj);

int VPP_VMXAPI_SetCmd(VPP_OBJ *vpp_obj, int *in_params);

int VPP_VMXAPI_Config(VPP_OBJ* vpp_obj);

unsigned int* VPP_VMXAPI_ClkGate_GetReg_IpCtrl(UINT32 RegAddr);

#endif //_VPP_VMX_APIFUNCS_H_
