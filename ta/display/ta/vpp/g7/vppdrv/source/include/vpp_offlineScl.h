/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_OFFLINE_SCL_H_
#define _VPP_OFFLINE_SCL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_module.h"

#define VPP_OFFLINESCL_USING_VIP
#define VPP_OFFLINE_DHUB_SIZE_MULTI (16)
#define VPP_OFFLINE_DHUB_SIZE_MULTI_PIXELS (8)

#define VPP_OFFLINESCL_CONTENT_MAX_H (1920)
#define VPP_OFFLINESCL_CONTENT_MAX_V (1080)

/* Enable VPP_OFFLINESCL_4K_IN_LIMIT_SUPPORT to reduce max content size in VPP_SCL_MODE_INLINE_OFFLINE when
 * MAIN SCL is in small up scale and VIP SCL is in small down scaling for 4K input at 4K display resolution*/
#define VPP_OFFLINESCL_4K_IN_LIMIT_SUPPORT
#define VPP_OFFLINESCL_4K_IN_CONTENT_MAX_H (1600)
#define VPP_OFFLINESCL_4K_IN_CONTENT_MAX_V (900)

#ifdef VPP_OFFLINESCL_4K_IN_LIMIT_SUPPORT
#define VPP_OFFLINESCL_4K_IN_MIN_OVRES (1081)
#define VPP_OFFLINESCL_4K_IN_MAX_OVRES (1255)
#define VPP_OFFLINESCL_4K_IN_MIN_OHRES (1921)
#define VPP_OFFLINESCL_IS_4K_IN_LIMIT(sclMode, OHRes, OVRes) ((VPP_SCL_MODE_INLINE_OFFLINE == sclMode)\
        && (VPP_OFFLINESCL_4K_IN_MAX_OVRES >= OVRes)\
        && (OVRes >= VPP_OFFLINESCL_4K_IN_MIN_OVRES)\
        && (OHRes >= VPP_OFFLINESCL_4K_IN_MIN_OHRES))
#else
#define VPP_OFFLINESCL_IS_4K_IN_LIMIT(sclMode, OVRes) 0
#endif

#define VPP_OFFLINESCL_DEFAULT_BITS_PER_PIXEL (24)

#define VPP_OFFLINESCL_BITS_PER_BYTES (8)

#define VPP_OFFLINESCL_MAX(x,y) ((x>y) ? x : y)
#define VPP_OFFLINESCL_IS_FRAMECROP_UPDATED(currCrop, newCrop) ( \
        (((currCrop)->cropAfterSclLeft    != (newCrop)->leftCrop) || \
            ((currCrop)->cropAfterSclRight   != (newCrop)->rightCrop) || \
            ((currCrop)->cropAfterSclTop     != (newCrop)->topCrop) || \
            ((currCrop)->cropAfterSclBottom  != (newCrop)->bottomCrop)))\

#define VPP_OFFLINESCL_IS_LINEDROP_ONLY(IVRes, OVRes) \
        ((IVRes > OVRes * 3) && (OVRes < 140))

/* Load Default values for VIP path SCL */
INT  VPP_OfflineScl_LoadDefaultVal(VPP_OBJ *vpp_obj);

/* This funcion is used to configure offline scaling based on sclaing requirement */
int VPP_OfflineScl_Config(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, INT chanID);


/* This funcion is used to start client and dHub for Offline Scaling */
int VPP_OfflineScl_ClientStart(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, INT chanID);

/* Return the vppSysClk used in the current resolution */
int VPP_GetVppSysClk_KHz(VPP_OBJ *vpp_obj, UNSG32 *pVppSysClk,INT chanID);

INT VPP_OfflineScl_ConfigLineDrop(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, INT chanID);

INT FE_OfflineRd_Config(VPP_OBJ *vpp_obj, INT ldr_id);

INT FE_OfflineRd_LoadDefaultVal(VPP_OBJ *vpp_obj, INT Channel);

#ifdef __cplusplus
}
#endif

#endif

