/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_DV_API_C_
#define _VPP_DV_API_C_

#include "vpp_dv_api.h"
#include "vpp_dv_ocsc.h"
#include "vpp_module.h"
#include "com_type.h"
#include "hdmi_edid.h"
#include "vpp_be_hdr.h"

#define VPPDV_EDID_INFO(...) VPP_log(VPP_LOG_DV, VPP_LOG_INFO, "VPPEDID", __VA_ARGS__)

UINT8 IS_DV_CertTest(void)
{
    return 0;
}

/* FE Wrapper API */
INT VPP_DV_FE_DLR_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes)
{
    return MV_VPP_OK;
}

INT VPP_DV_FE_SetInputSize(VPP_OBJ *vpp_obj, INT id, PVPP_FE_DLR_INPUT_RES pDBVIPSize)
{
    return MV_VPP_OK;
}

INT VPP_DV_FE_EnableChannel(VPP_OBJ *vpp_obj, INT Channel, BOOL Enable)
{
    return MV_VPP_OK;
}

INT VPP_DV_FE_ClearChannel(VPP_OBJ *vpp_obj, INT Channel)
{
    return MV_VPP_OK;
}

INT VPP_DV_FE_Config(VPP_OBJ *vpp_obj, INT planeID, UINT32  m_srcfmt, UINT32 m_bits_per_pixel)
{
    return MV_VPP_OK;
}

void VPP_DV_FE_Handler(VPP_OBJ *vpp_obj, PLANE *plane, VBUF_INFO *pinfo)
{
}

void VPP_DV_FE_CompLoadDefaultVal(VPP_OBJ *vpp_obj, INT Channel)
{
}

UINT8 VPP_DV_GetDvMode(VPP_OBJ *vpp_obj)
{
    return 0;
}

UINT8 VPP_DV_GetDvCompMode(VPP_OBJ *vpp_obj)
{
    return 0;
}

void VPP_DV_DataUpdate(VPP_OBJ *vpp_obj, PLANE *plane, VBUF_INFO *pinfo, UINT8 sel)
{
}

void VPP_DV_Updated_Metadata_Addr(VBUF_INFO *pinfo)
{
}

/* BE Wrapper API */
void VPP_DV_BE_Handler(VPP_OBJ *vpp_obj, INT planeID, VBUF_INFO *pinfo, \
    INT VideoFmt, INT reconfig)
{
}

void VPP_DV_ControlPath(VPP_OBJ *vpp_obj, DV *pDV)
{
}

void VPP_DV_PostCompCropHandler(VPP_OBJ *vpp_obj, PLANE *plane, \
    int *leftAdjust, int *rightAdjust, PVPP_FE_DLR_INPUT_RES pTotRes)
{
}

void VPP_DV_SetDelayedPriorityMode(VPP_OBJ *vpp_obj, int frame_count)
{
}

void VPP_DV_Updated_FrameDVMode(BOOL hdrFlag, VBUF_INFO *pinfo)
{
}

/* EDR Wrapper API */
void VPP_DV_EdrHandler(VPP_OBJ *vpp_obj)
{
}

void VPP_DV_EdrReset(VPP_OBJ *vpp_obj)
{
}

/* Dither Wrapper API */
void VPP_DV_DitherCfg(VPP_OBJ *vpp_obj, int color_fmt, int bit_depth)
{
    if((OUTPUT_COLOR_FMT_YCBCR422 == color_fmt) || (OUTPUT_COLOR_FMT_YCBCR444 == color_fmt) || \
            (OUTPUT_COLOR_FMT_RGB888 == color_fmt))
    {
        vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_edr_hdmi_bypass = 1;
        vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_dither_bypass = 1;
        vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_edrhdmi_dither_sel = 1;
        vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_ClampMap_bypass = 1;
        vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_edr_dns_sel = 1;
    }
}

#ifdef CONFIG_AMP_DOLBY_DUMP
void VPP_DV_Dither_SWpatSel(VPP_OBJ *vpp_obj)
{
}
#endif

/* OCSC Wrapper API */
INT VPP_DV_BE_VopLoadCscCoeff(VPP_OBJ *vpp_obj, INT ColorFmt, INT ActWidth)
{
    return VPP_BE_VOP_LoadCscCoeff(vpp_obj, ColorFmt, ActWidth);
}

/* HDMI EDID API */
int VPP_DV_GetEdidInfo(VPP_OBJ *vpp_obj, UINT8 *raw_edid)
{
    return EDID_ERR_OK;
}

EDID_RETURNS VPP_DV_ParseEdid(unsigned char *pDataBuf, unsigned char BlkLen, PEDID_DOLBYVISION_CAP pDolbyVisionVSVDB)
{
    VPPDV_EDID_INFO("EDID_CEA_EXTENDED_DOLBY_BLK_TAG parsing un-supported\n");
    return EDID_ERR_OK;
}

int VPP_DV_HDMItx_SetSignalType(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
   return 0;
}

int VPP_DV_HDMItx_GetL11MdStatus(VPP_OBJ *vpp_obj)
{
    return 0;
}

int VPP_DV_HDMItx_UpdateVSIF(VPP_OBJ *vpp_obj, BOOL dolbyvision_en)
{
    return DV_DISABLED;
}

void VPP_DV_HDMItx_PackVSInfoFrame(VPP_OBJ *vpp_obj, \
    VPP_HDMI_PKT_VNDRSPEC_INFOFRM *pVndrSpecInfoFrm, \
    UINT8 *pDataBuf)
{

}


void VPP_DVOBJ_Reset(VPP_OBJ *vpp_obj)
{
}

/* Data Isolation API */
void VPP_DV_BE_GfxHandler(VPP_OBJ *vpp_obj, DV *pDV)
{
}

UINT8 VPP_DV_CfgEdrSupport(VPP_OBJ *vpp_obj, VPP_HDMI_DOLBYVISION_CAP *dolbycap, \
    int color_fmt, int bit_depth, int resID)
{
    return 0;
}

UINT8 VPP_DV_GetCapability(VPP_OBJ *vpp_obj, UINT8 sel)
{
    return 0;
}

int VPP_DVOBJ_Init(VPP_OBJ *vpp_obj)
{
    return MV_VPP_OK;
}

int VPP_DVOBJ_Destroy(VPP_OBJ *vpp_obj)
{
    return MV_VPP_OK;
}

void VPP_DV_HandleHPD(VPP_OBJ *vpp_obj, UINT8 rxC, UINT8 bt2020)
{
}

void VPP_DV_InitClkGate(VPP_OBJ *vpp_obj, UINT32 *pvalue[DV_CLKG_LMT])
{
}

/* User API */
int VPP_DVAPI_setHDRmode(VPP_OBJ *vpp_obj, int *in_params)
{
    if(in_params[0] == VPP_HDR_MODE_AUTO)
    {
        vpp_obj->m_HdrMode = VPP_HDR_MODE_AUTO;
        vpp_obj->m_outformat = VPP_OUTFORMAT_SDR;
        vpp_obj->videofmtUpdated = 1;
        return (MV_VPP_OK);
    }
    else
    {
        /*Forced output modes unsupported with Dolby vision IP disabled*/
        return (MV_VPP_EHWSTATEWRONG);
    }
}

int VPP_DVAPI_setHDMImode(VPP_OBJ *vpp_obj, UINT32 mode)
{
    return (MV_VPP_EUNSUPPORT);
}

int VPP_DVAPI_HDR_SetConfig(VPP_OBJ *vpp_obj, VPP_HDR_CONFIG_PARAM *param)
{
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_LoadVSDB(int handle, unsigned char *vsdb_buf)
{
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_EnableLowLatency(int handle, int enable)
{
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_SetPriorityMode(int handle, int priority)
{
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_SetDovi2HdrMap(int handle, int enable)
{
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_SetHdrFeature(int handle, VPP_HDR_FEATURE feature)
{
    return (MV_VPP_EUNSUPPORT);
}

int MV_VPPOBJ_SetGfxInfo(int handle, int type, int gfx_min, int gfx_max)
{
    return (MV_VPP_EUNSUPPORT);
}

#endif
