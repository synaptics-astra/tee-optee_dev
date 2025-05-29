/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_ngptv_api.c
//! \brief      Top level NGPTV watermark API's
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_module.h"
#include "vpp_ngptv_api.h"

int MV_VPPOBJ_NxgTaCommandHandler(TEE_Param params[4])
{
    return MV_VPP_OK;
}

int MV_VPPOBJ_StartChannelDataLoader_Ngptv(VPP_OBJ *vpp_obj, UINT32 uiPlaneId)
{
    return 0;
}

int VPP_SetNgptvCmd(VPP_OBJ *vpp_obj, int *in_params)
{
    return 0;
}

int VPP_IsNgptvServiceEnabled(VPP_OBJ *vpp_obj, UINT32 uiPlaneId) {
    return 0;
}

int VPP_NGPTV_Monitor(VPP_OBJ *vpp_obj, UINT32 uiPlaneId) {
    return 0;
}

int VPP_NGPTV_Create(VPP_OBJ *vpp_obj) {
    return 0;
}

int VPP_NGPTV_Config(VPP_OBJ* vpp_obj) {
    return 0;
}

int VPP_NGPTV_Reset(VPP_OBJ* vpp_obj) {
    return 0;
}

