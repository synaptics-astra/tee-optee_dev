/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_BE_HDR_H__
#define __VPP_BE_HDR_H__

#define isHDR(pinfo) ((pinfo->transfer_characteristics == VPP_TRANSFER_CHAR_SMPTE_ST_2048) || \
     (pinfo->transfer_characteristics == VPP_TRANSFER_CHAR_SMPTE_ST_428_I))

#define isLDR(pinfo) ((pinfo->transfer_characteristics == VPP_TRANSFER_CHAR_BT2020_1) || \
        (pinfo->transfer_characteristics == VPP_TRANSFER_CHAR_BT2020_2))

#define isHLG(pinfo) ((pinfo->transfer_characteristics == VPP_TRANSFER_CHAR_ARIB_B67) || \
     (pinfo->alt_transfer_characteristics == VPP_TRANSFER_CHAR_ARIB_B67))

#define isHDR10PLUS(pinfo) (pinfo->m_hdr10plus_present_mode == 1)

#define isDOVI(pinfo) (pinfo->m_dv_present_mode & (VPP_DOLBY_VISION_RPU | VPP_DOLBY_VISION_LL))

#define isSDR2020(pinfo) (pinfo->m_colorprimaries == VPP_COLOR_PRIM_BT2020)

#define isBT2020HDMIOutputSupported(color_fmt, bit_depth) ((OUTPUT_COLOR_FMT_RGB888 != color_fmt) && (OUTPUT_BIT_DEPTH_8BIT != bit_depth) && (vpp_obj->BT2020_capability & VPP_COLOR_STD_BT2020YCC))

#define IS_HDR_ENABLED() ((vpp_obj->HDR_enable != FALSE) || (vpp_obj->HLG_enable != FALSE) || (vpp_obj->HDR10PLUS_enable != FALSE))

typedef enum
{
    HDR_MODULE_DCM = 0,
    HDR_MODULE_DOLBYVISION = 1,
    HDR_MODULE_MAX = 2,
}HDR_MODULE_TYPE;

typedef enum {
    VPP_HDR_AUTOMODE_0, //switch based on builtin frame
    VPP_HDR_AUTOMODE_1, //do not use builtin frame for switching
}VPP_HDR_AUTOMODE_TYPE;

typedef struct VPP_HDR_CONFIG_T
{
    UINT8 u8ConfigEn;
    UINT8 u8AutoModeType;
    UINT8 u8SwitchingDelay[VPP_OUTFORMAT_MAX][VPP_OUTFORMAT_MAX];
    HDR_MODULE_TYPE HDR_MODULE_SELECTION[VPP_OUTFORMAT_MAX];
}VPP_HDR_CONFIG;

int MV_VPPOBJ_HDREnable (int handle, INT Enable);
INT VPP_BE_HDR_Init(VPP_OBJ *vpp_obj);
void VPP_BE_HDR_Process(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
void VPP_BE_HDR_GetMetadata(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
void VPP_BE_HDR_SetOutputMode(VPP_OBJ *vpp_obj, DV *pDV);
void VPP_BE_HDR_SetCapability(VPP_OBJ *vpp_obj);
int VPP_BE_HDR_BT2020Process(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
BOOL VPP_BE_HDR_SetHDMIEarlySignaling(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
void VPP_BE_HDR_SetConfig(VPP_OBJ *vpp_obj, VPP_HDR_CONFIG_PARAM *param);
INT VPP_BE_HDR_Destroy(VPP_OBJ *vpp_obj);
void VPP_BE_SelectHDRModule(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
#endif
