/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_FE_DCM_H_
#define _VPP_FE_DCM_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_AMP_DCM

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

/**
  Input mode of DCM from transfer characterstics
 **/

typedef enum VPP_FE_DCM_IN_MODE_t
{
    VPP_FE_DCM_IN_MODE_MIN = 0,
    VPP_FE_DCM_IN_MODE_DISABLE = 0,
    VPP_FE_DCM_IN_MODE_SDR_709,
    VPP_FE_DCM_IN_MODE_HLG,
    VPP_FE_DCM_IN_MODE_HDR10,
    VPP_FE_DCM_IN_MODE_MAX
}VPP_FE_DCM_IN_MODE;

/**
  Output mode of DCM from Capability
 **/

typedef enum VPP_FE_DCM_OUT_MODE_t
{
    VPP_FE_DCM_OUT_MODE_MIN = 0,
    VPP_FE_DCM_OUT_MODE_DISABLE = 0,
    VPP_FE_DCM_OUT_MODE_SDR_709,
    VPP_FE_DCM_OUT_MODE_HLG,
    VPP_FE_DCM_OUT_MODE_HDR10,
    VPP_FE_DCM_OUT_MODE_MAX
}VPP_FE_DCM_OUT_MODE;

typedef enum {
    DCM_CSC_TYPE_MIN = 0,
    DCM_CSC_TYPE_NORMAL = 0,
    DCM_CSC_TYPE_CMAP = 1,
    DCM_CSC_TYPE_MAX
}DCM_CSC_TYPE;

//#define ENABLE_DCM_DEBUG

#ifdef ENABLE_DCM_DEBUG
#define VPP_FE_DCM_DBG_PRINTF   VPPAPI_DBG_INFO
#define VPP_DCM_INFO VPPAPI_DBG_INFO
#else
#define VPP_FE_DCM_DBG_PRINTF(...)
#define VPP_DCM_INFO(...)
#endif

#define DCM_REG(planeID, name) (planeID? RA_GDCM_##name : RA_VDCM_##name)

void dcmHandler(VPP_OBJ *vpp_obj, INT uiPlaneId, VBUF_INFO *pstVbufInfo);
INT VPP_FE_DCM_Init(VPP_OBJ *vpp_obj);
INT VPP_FE_DCM_GDCM_Init(VPP_OBJ *vpp_obj, INT uiPlaneId);
INT SetDcmPQValues(VPP_OBJ *vpp_obj, INT uiPlaneId, INT Hue, INT Sat, INT Lght);
unsigned int DCM_CSC_C15O18F12_BASE(VPP_OBJ *vpp_obj, int planeID, int csc_type);
unsigned int DCM_CSC_C13O18F12_BASE(VPP_OBJ *vpp_obj, int planeID, int csc_type);
unsigned int DCM_LUT_BASE(VPP_OBJ *vpp_obj, int planeID);
unsigned int DCM_REG_BASE(VPP_OBJ *vpp_obj, int planeID);
#endif

#ifdef __cplusplus
        }
#endif

#endif
