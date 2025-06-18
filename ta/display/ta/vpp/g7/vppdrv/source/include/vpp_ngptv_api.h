
/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_ngptv_api.h
//! \brief      Top level NGPTV watermark API's
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef __VPP_NGPTV_API_H__
#define __VPP_NGPTV_API_H__

#include "vpp_module.h"

#ifndef CONFIG_OPTEE
#include "tee_internal_core_mem_api.h"
#endif
#include "tee_internal_api.h"
#include "tee_ta_osal.h"

int MV_VPPOBJ_NxgTaCommandHandler(TEE_Param params[4]);

int MV_VPPOBJ_StartChannelDataLoader_Ngptv(VPP_OBJ *vpp_obj, UINT32 uiPlaneId);

int VPP_SetNgptvCmd(VPP_OBJ *vpp_obj, int *in_params);

int VPP_IsNgptvServiceEnabled(VPP_OBJ *vpp_obj, UINT32 uiPlaneId);

int VPP_NGPTV_Create(VPP_OBJ *vpp_obj);

int VPP_NGPTV_Monitor(VPP_OBJ *vpp_obj, UINT32 uiPlaneId);

int VPP_NGPTV_Config(VPP_OBJ* vpp_obj);

int VPP_NGPTV_Reset(VPP_OBJ* vpp_obj);
#endif //__VPP_NGPTV_API_H__
