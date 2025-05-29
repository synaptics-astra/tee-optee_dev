/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_vmx_api.c
//! \brief      VMX watermark API's used by VPP API/internal functions
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_vmx_apifuncs.h"

int VPP_VMXAPI_StartDataChannelLoader(VPP_OBJ *vpp_obj, INT chanID) {
    return MV_VPP_OK;
}

void VPP_VMXAPI_Notify(VPP_OBJ *vpp_obj, unsigned int eventcode, void *params) {
}

int VPP_VMXAPI_Init(VPP_OBJ *vpp_obj) {
    return MV_VPP_OK;
}

void VPP_VMXAPI_Destroy(VPP_OBJ *vpp_obj) {
}

int VPP_VMXAPI_SetCmd(VPP_OBJ *vpp_obj, int *in_params) {
    return MV_VPP_OK;
}

int VPP_VMXAPI_Config(VPP_OBJ* vpp_obj) {
    return MV_VPP_OK;
}

unsigned int* VPP_VMXAPI_ClkGate_GetReg_IpCtrl(UINT32 RegAddr) {
    return NULL;
}
