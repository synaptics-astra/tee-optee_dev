/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_vmx_ta_wrap.c
//! \brief      Top level VMX API's for TA interface(wrapper) layer
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_vmx_ta_wrap.h"

void VPP_VMXAPI_InitTaAccessVoilation(void) {
}

void VPP_VMXAPI_SetTaAccessVoilation(void) {
}

void VPP_VMXAPI_CheckTaAccessVoilation(void) {
}

int VPP_VMXAPI_VmxTaCommandHandler(TEE_Param params[4]) {
    return MV_VPP_OK;
}
