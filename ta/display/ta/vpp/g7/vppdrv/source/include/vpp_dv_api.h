/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_DV_API_H_
#define _VPP_DV_API_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_module.h"
#include "vpp_fe_dlr.h"
#include "hdmi_edid.h"

typedef enum DV_CAP_SEL_T
{
    DV_CAP_EDRCAP = 0,
    DV_CAP_EDREN,
    DV_CAP_STDDV,
    DV_CAP_LL,
    DV_CAP_STATUS
}DV_CAP_SEL;

typedef enum DV_DATA_SEL_T
{
    DV_DATA_PMODE = 0,
    DV_DATA_REFWIN,
    DV_DATA_DISPWIN
}DV_DATA_SEL;

#define VPP_DOVI_COMP_MD_SIZE  1800
#define DV_DISABLED  0
#define DV_ENABLED   1
#define DV_CLKG_LMT  3
#define VPP_BASE_ADDR_OCSC    (vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_OCSC)


#define INT_TO_SIGN16_CONVERT(x)    ((x<0)?((x*(-1)) + (1<<15)):x)
#define SIGN16_TO_SHORT_INT_CONVERT(OcscCoeff)  \
                                    OcscCoeff & 0x8000 ? (OcscCoeff & 0x7fff) * -1 : OcscCoeff


UINT8 IS_DV_CertTest(void);

/* Composer Wrapper API */
INT VPP_DV_FE_DLR_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes);
INT VPP_DV_FE_SetInputSize(VPP_OBJ *vpp_obj, INT id, PVPP_FE_DLR_INPUT_RES pDBVIPSize);
INT VPP_DV_FE_EnableChannel(VPP_OBJ *vpp_obj, INT Channel, BOOL Enable);
INT VPP_DV_FE_ClearChannel(VPP_OBJ *vpp_obj, INT Channel);
INT VPP_DV_FE_Config(VPP_OBJ *vpp_obj, INT planeID, UINT32  m_srcfmt, UINT32 m_bits_per_pixel);
void VPP_DV_FE_Handler(VPP_OBJ *vpp_obj, PLANE *plane, VBUF_INFO *pinfo);

void VPP_DV_Updated_Metadata_Addr(VBUF_INFO *pinfo);

/* DM Wrapper API */
void VPP_DV_BE_Handler(VPP_OBJ *vpp_obj, INT planeID, VBUF_INFO *pinfo, \
    INT VideoFmt, INT reconfig);
void VPP_DV_PostCompCropHandler(VPP_OBJ *vpp_obj, PLANE *plane, \
    int *leftAdjust, int *rightAdjust, PVPP_FE_DLR_INPUT_RES pTotRes);
void VPP_DV_SetDelayedPriorityMode(VPP_OBJ *vpp_obj, int frame_count);
void VPP_DV_Updated_FrameDVMode(BOOL hdrFlag, VBUF_INFO *pinfo);
void VPP_DV_ControlPath(VPP_OBJ *vpp_obj, DV *pDV);

/* EDR Wrapper API */
void VPP_DV_EdrHandler(VPP_OBJ *vpp_obj);
void VPP_DV_EdrReset(VPP_OBJ *vpp_obj);

/* Dither Wrapper API */
void VPP_DV_DitherCfg(VPP_OBJ *vpp_obj, int color_fmt, int bit_depth);
#ifdef CONFIG_AMP_DOLBY_DUMP
void VPP_DV_Dither_SWpatSel(VPP_OBJ *vpp_obj);
#endif

/* OCSC Wrapper API */
INT VPP_DV_BE_VopLoadCscCoeff(VPP_OBJ *vpp_obj, INT ColorFmt, INT ActWidth);

/* HDMI EDID API */
int VPP_DV_GetEdidInfo(VPP_OBJ *vpp_obj, UINT8 *raw_edid);
EDID_RETURNS VPP_DV_ParseEdid(unsigned char *pDataBuf, unsigned char BlkLen, PEDID_DOLBYVISION_CAP pDolbyVisionVSVDB);
int VPP_DV_HDMItx_SetSignalType(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
int VPP_DV_HDMItx_GetL11MdStatus(VPP_OBJ *vpp_obj);
int VPP_DV_HDMItx_UpdateVSIF(VPP_OBJ *vpp_obj, BOOL dolbyvision_en);
void VPP_DV_HDMItx_PackVSInfoFrame(VPP_OBJ *vpp_obj, \
    VPP_HDMI_PKT_VNDRSPEC_INFOFRM *pVndrSpecInfoFrm, \
    UINT8 *pDataBuf);

void VPP_DVOBJ_Reset(VPP_OBJ *vpp_obj);

/* Data isolation API */
void VPP_DV_FE_CompLoadDefaultVal(VPP_OBJ *vpp_obj, INT Channel);
UINT8 VPP_DV_GetDvMode(VPP_OBJ *vpp_obj);
UINT8 VPP_DV_GetDvCompMode(VPP_OBJ *vpp_obj);
void VPP_DV_DataUpdate(VPP_OBJ *vpp_obj, PLANE *plane, VBUF_INFO *pinfo, UINT8 sel);
void VPP_DV_BE_GfxHandler(VPP_OBJ *vpp_obj, DV *pDV);
UINT8 VPP_DV_CfgEdrSupport(VPP_OBJ *vpp_obj, VPP_HDMI_DOLBYVISION_CAP *dolbycap, \
    int color_fmt, int bit_depth, int resID);
UINT8 VPP_DV_GetCapability(VPP_OBJ *vpp_obj, UINT8 sel);
void VPP_DV_HandleHPD(VPP_OBJ *vpp_obj, UINT8 rxC, UINT8 bt2020);
int VPP_DVOBJ_Init(VPP_OBJ *vpp_obj);
int VPP_DVOBJ_Destroy(VPP_OBJ *vpp_obj);
void VPP_DV_InitClkGate(VPP_OBJ *vpp_obj, UINT32 *pvalue[DV_CLKG_LMT]);

/* User Config API */
int VPP_DVAPI_setHDRmode(VPP_OBJ *vpp_obj, int *in_params);
int VPP_DVAPI_setHDMImode(VPP_OBJ *vpp_obj, UINT32 mode);
int VPP_DVAPI_HDR_SetConfig(VPP_OBJ *vpp_obj, VPP_HDR_CONFIG_PARAM *param);

int MV_VPPOBJ_LoadVSDB(int handle, unsigned char *vsdb_buf);
int MV_VPPOBJ_EnableLowLatency(int handle, int enable);
int MV_VPPOBJ_SetPriorityMode(int handle, int priority);
int MV_VPPOBJ_SetDovi2HdrMap(int handle, int enable);
int MV_VPPOBJ_SetHdrFeature(int handle, VPP_HDR_FEATURE feature);
int MV_VPPOBJ_SetGfxInfo(int handle, int type, int gfx_min, int gfx_max);

#ifdef __cplusplus
}
#endif

#endif

