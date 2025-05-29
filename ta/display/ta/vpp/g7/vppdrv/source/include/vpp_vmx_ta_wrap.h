/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_vmx_ta_wrap.h
//! \brief      Top level VMX API's for TA interface(wrapper) layer
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_VMX_TA_WRAP_H_
#define _VPP_VMX_TA_WRAP_H_

#include "vpp_module.h"

void VPP_VMXAPI_InitTaAccessVoilation(void);

void VPP_VMXAPI_SetTaAccessVoilation(void);

void VPP_VMXAPI_CheckTaAccessVoilation(void);

int VPP_VMXAPI_VmxTaCommandHandler(TEE_Param params[4]);

#endif //_VPP_VMX_TA_WRAP_H_
