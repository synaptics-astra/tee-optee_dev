/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CFG_H_
#define _VPP_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "vpp_api.h"
#include "vpp_module.h"

int video_framerate_num_den(INT resID, int * num, int * den);
RESOLUTION_INFO* VPP_GetResInfo(UINT32 resId);
RESOLUTION_INFO* VPP_GetResInfoFromPlaneID(VPP_OBJ *vpp_obj, ENUM_PLANE_ID planeID);
#ifdef __cplusplus
}
#endif

#endif

