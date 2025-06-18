/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_PTM_WRAPPER_H_
#define _VPP_PTM_WRAPPER_H_

#include "vpp_module.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_VPP_PTM_REF_UNREF_ENABLE
TEE_Result VPP_PtmInit(VPP_PTM_OBJ *ptm_obj);
void VPP_PtmDeinit(VPP_PTM_OBJ *ptm_obj);
void VPP_PtmHandleSessionIncrement(void);
void VPP_PtmHandleSessionDecrement(void);

TEE_Result VPP_PtmRef(VPP_PTM_OBJ *ptm_obj, int planeID, VDEC_VID_BUF_DESC *frame);
TEE_Result VPP_PtmUnref(VPP_PTM_OBJ *ptm_obj, int planeID, VDEC_VID_BUF_DESC *frame);
TEE_Result VPP_PtmUpdateRef(VPP_PTM_OBJ *ptm_obj, int planeID, VDEC_VID_BUF_DESC *frame_new, VDEC_VID_BUF_DESC *frame_prev);
#else
#define VPP_PtmInit(ptm_obj)          TEE_SUCCESS
#define VPP_PtmDeinit(ptm_obj)        TEE_SUCCESS
#define VPP_PtmHandleSessionIncrement()
#define VPP_PtmHandleSessionDecrement()
#define VPP_PtmRef(ptm_obj, planeID, frame)
#define VPP_PtmUnref(ptm_obj, planeID, frame)
#define VPP_PtmUpdateRef(ptm_obj, planeID, frame_new, frame_prev) TEE_SUCCESS
#endif //CONFIG_VPP_PTM_REF_UNREF_ENABLE

#ifdef __cplusplus
}
#endif

#endif //_VPP_PTM_WRAPPER_H_

