/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_BE_VOP_C_
#define _VPP_BE_VOP_C_

#include "vpp.h"
#include "com_type.h"
#include "vpp_regs.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_be_vop.h"
#include "vpp_be.h"
#include "vpp.h"
#include "vpp_be_vop_prv.h"
#include "vpp_cfg.h"
#include "vpp_clkGate.h"
#include "vpp_dv_api.h"

static INT VPP_BE_ConfigDns422_420(VPP_OBJ *vpp_obj, UINT32 uiBitDepth, UINT32 uiEnable);

/***************************************************************************************
 * FUNCTION: load all the default values of VOP block to BCM buffer
 * PARAMS: void
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP default values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_LoadDefaultVal(VPP_OBJ *vpp_obj)
{
    INT RetVal = MV_VPP_OK;
#ifdef FIXME_IN_BG4CT
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_CBLANK_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_CBLANK_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_MODE, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_SEL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_ALIGN, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_RND, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_SEL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_ALIGN, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_RND, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_SEL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_ALIGN, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_RND, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_SEL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_MODE, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_CBLANK_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_CBLANK_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_SEL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_CTRL0, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_CTRL1, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_UDATA_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_UDATA_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_MDATA_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_MDATA_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_LDATA_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_3D_HDMI_LDATA_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C2_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C3_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C3_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C4_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C4_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C5_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C5_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C6_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C6_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C7_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C7_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C8_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_C8_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_A0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_A0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_A1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_A1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_A2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_AHD_A2_H, gBeVopDefValues);

    WRITE_REG_DEFAULT_VAL(VOP_ASD_C0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C2_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C3_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C3_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C4_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C4_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C5_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C5_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C6_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C6_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C7_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C7_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C8_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_C8_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_A0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_A0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_A1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_A1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_A2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_ASD_A2_H, gBeVopDefValues);

    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C2_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C3_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C3_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C4_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C4_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C5_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C5_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C6_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C6_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C7_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C7_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C8_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_C8_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_A0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_A0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_A1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_A1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_A2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_HDMI_A2_H, gBeVopDefValues);

    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C2_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C3_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C3_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C4_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C4_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C5_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C5_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C6_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C6_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C7_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C7_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C8_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_C8_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_A0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_A0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_A1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_A1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_A2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_A2_H, gBeVopDefValues);

    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C2_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C3_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C3_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C4_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C4_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C5_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C5_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C6_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C6_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C7_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C7_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C8_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_C8_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_A0_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_A0_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_A1_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_A1_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_A2_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_A2_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_C656_SATURATE, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_CBLANK_L, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_CBLANK_H, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_MODE, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_CLKPOL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_SATURATE, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL30_CTRL, gBeVopDefValues);
    WRITE_REG_DEFAULT_VAL(VOP_TTL24_CTRL, gBeVopDefValues);
    vpp_obj->be.VppBeVopClkPol = gBeVopDefValues.VPP_VOP_CLKPOL;
    vpp_obj->be.VppBeVopHDMIFrmtCtrl = gBeVopDefValues.VPP_VOP_3D_HDMI_CTRL0;
    //GA_REG_WORD32_READ(vpp_obj->base_addr_4k + RA_Vpp_4K_HDMI_VOP + RA_HDMI_VOP_CFG0,&(vpp_obj->hdmi_vop_cfg0.u32));
#endif
    return  RetVal;
};

/***************************************************************************************
 * FUNCTION: set HDMI DNS mode
 * PARAMS: DnsMode - HDMI DNS mode
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP HDMI DNS mode values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiDnsMode(VPP_OBJ *vpp_obj, INT DnsMode)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
//    T32DNS444_422_CFG5 value;
    /*get register address of VOP HDMI MODE*/
    //      RegAddr = vpp_obj->base_addr + RA_Vpp_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_DNS444_422 + RA_DNS444_422_CFG5;
//    GA_REG_WORD32_READ(RegAddr,&(value.u32[0]));

//    value.uCFG5_mode_regs = DnsMode;

    /*set HDMI mode to BCM buffer*/
//    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, value.u32[0])))
//        return RetVal;

    /* Set Hdmi mode when Dns is set*/
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG0;
    if(DnsMode)
        vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_mode=1;
    else
        vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_mode=0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->hdmi_vop.hdmi_vop_cfg0.u32);


    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set HDMI parameters
 * PARAMS: pHdmiCtrl - pointer to HDMI control parameters
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP HDMI control values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiParams(VPP_OBJ *vpp_obj, PVPP_VOP_HDMI_CTRL pHdmiCtrl)
{
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP;
    /*set chroma blank*/
    vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_cblank = pHdmiCtrl->CBlank;
    vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_sel = pHdmiCtrl->InputSel;
    vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_align = pHdmiCtrl->OutputAlign;

    /*set HDMI input select to BCM buffer*/
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_HDMI_VOP_CFG0, vpp_obj->hdmi_vop.hdmi_vop_cfg0.u32);

    vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_FifoSts_ctrl1 = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_HDMI_VOP_CFG7, vpp_obj->hdmi_vop.hdmi_vop_cfg7.u32);

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: BE_VOP_ConfigDither
 * PARAMS: stDitherCfg0 - pointer to Dither Cfg0
 *         uiMode, uiBitDepth
 ***************************************************************************************/
void BE_VOP_ConfigDither(T32DITHER_CFG0 *stDitherCfg0, UINT32 uiMode, UINT32 uiBitDepth)
{
    switch(uiMode)
    {
        case VPP_BE_DITHER_TRUNCATE:
            {
                stDitherCfg0->uCFG0_mode = 0;
                stDitherCfg0->uCFG0_ctrl = (uiBitDepth == OUTPUT_BIT_DEPTH_8BIT) ? 1 : 0;
            }
            break;
        case VPP_BE_DITHER_ROUNDING:
        case VPP_BE_DITHER_1D_ERR_DIFFUSION:
            {
                stDitherCfg0->uCFG0_mode = (uiMode == VPP_BE_DITHER_ROUNDING) ? 1 : 2;
                stDitherCfg0->uCFG0_ctrl = (uiBitDepth == OUTPUT_BIT_DEPTH_8BIT) ? 3 : 2;
            }
            break;
        case VPP_BE_DITHER_BYPASS:
        default:
            break;
    }
}

/***************************************************************************************
 * FUNCTION: set AHD parameters
 * PARAMS: pAhdCtrl - pointer to AHD control parameters
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP AHD control values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetAhdParams(VPP_OBJ *vpp_obj, PVPP_VOP_AHD_ASD_CTRL pAhdCtrl)
{
#if !defined(FPGA)
#ifdef TURN_OFF_CPCB3_AHD_TTL
	return MV_VPP_OK;
#endif
	UINT32 RegAddr;
	T32VOP_AHD value;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_AHD;
    GA_REG_WORD32_READ(RegAddr,&(value.u32));
	value.uAHD_sel = pAhdCtrl->InputSel;

    /* set AHD alignment */
//    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (VOP_AHD_ALIGN << 2), pAhdCtrl->OutputAlign);

    /*set AHD input select to BCM buffer*/
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, value.u32);
#endif
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: set ASD parameters
 * PARAMS: pAsdCtrl - pointer to ASD control parameters
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP ASD control values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetAsdParams(VPP_OBJ *vpp_obj, PVPP_VOP_AHD_ASD_CTRL pAsdCtrl)
{
#if !defined(FPGA)
	UINT32 RegAddr;
	T32VOP_ASD value;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_ASD;
    GA_REG_WORD32_READ(RegAddr,&(value.u32));
	value.uASD_sel = pAsdCtrl->InputSel;

    /* set ASD alignment */
 //   VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (VOP_ASD_ALIGN << 2), pAsdCtrl->OutputAlign);

    /*set ASD input select to BCM buffer*/
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, value.u32);
#endif
    return MV_VPP_OK;
}

INT BE_VOP_SetDsiDitherMode(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiBitDepth)
{
#ifdef ENABLE_GFX2_VOP2_PATH
    T32DITHER_CFG0 stDitherCfg0;
    stDitherCfg0.u32 = 0;
    stDitherCfg0.uCFG0_ycmode = 0; //Input is always RGB

    BE_VOP_ConfigDither(&stDitherCfg0, uiMode, uiBitDepth);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 +             RA_VOP2_DITHER+RA_DITHER_CFG0, stDitherCfg0.u32);
#endif //ENABLE_GFX2_VOP2_PATH
    return 0;
}

int BE_VOP_SetDSIParams(VPP_OBJ *vpp_obj, int cpcbout)
{
#ifdef ENABLE_GFX2_VOP2_PATH
    UINT32 regAddr;
    vpp_obj->vop2_ctrl0.uCTRL0_dp1_inp_en = 0;
    vpp_obj->vop2_ctrl0.uCTRL0_clken_ctrl = 0;

    vpp_obj->vop2_ctrl0.uCTRL0_ififo_sts_ctrl = 0; //FIXME: 1 for fixed color
    vpp_obj->vop2_ctrl0.uCTRL0_ofifo_sts_ctrl = 0;
    vpp_obj->vop2_ctrl0.uCTRL0_emp_sts_ctrl = 1;
    vpp_obj->vop2_ctrl0.uCTRL0_clken_ctrl0 = 0;
    vpp_obj->vop2_ctrl0.uCTRL0_inp_swap_sel = 0x6;

    switch(cpcbout)
    {
        case VPP_VOP_CPCB1_CPCB0:
            regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP;
            vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_FifoSts_ctrl1 = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, regAddr + RA_HDMI_VOP_CFG7, vpp_obj->hdmi_vop.hdmi_vop_cfg7.u32);
            vpp_obj->vop2_ctrl0.uCTRL0_dp1_inp_en = 1;
            vpp_obj->vop2_ctrl0.uCTRL0_ififo_sts_ctrl = 1;
            vpp_obj->vop2_ctrl0.uCTRL0_ofifo_sts_ctrl = 0;
            vpp_obj->vop2_ctrl0.uCTRL0_emp_sts_ctrl = 0;
            vpp_obj->vop2_ctrl0.uCTRL0_clken_ctrl0 = 1;
            vpp_obj->vop2_ctrl0.uCTRL0_inp_swap_sel = 0x24;
            break;
        case VPP_VOP_CPCB1_GFX0:
            vpp_obj->vpp_Ctrl.uCTRL_vop2_inp_sel = 2;
            break;
        case VPP_VOP_CPCB1_GFX1:
            vpp_obj->vpp_Ctrl.uCTRL_vop2_inp_sel = 1;
            break;
        case VPP_VOP_CPCB1_GFX2:
        default:
            vpp_obj->vpp_Ctrl.uCTRL_vop2_inp_sel = 0;
            break;
    }
    regAddr = vpp_obj->base_addr + RA_Vpp_CTRL;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, regAddr , vpp_obj->vpp_Ctrl.u32);

    regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_CTRL0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, regAddr , vpp_obj->vop2_ctrl0.u32);
#endif //ENABLE_GFX2_VOP2_PATH
    VPPAPI_DBG_INFO("BE_VOP_SetDSIParams[%d]: vpp_Ctrl[0x%x] vop2_ctrl0[0x%x]\r\n", cpcbout, vpp_obj->vpp_Ctrl.u32, vpp_obj->vop2_ctrl0.u32);
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: sets output round mode
 * PARAMS: OutputSel - indicates which VOP unit to be loaded(HDMI, AHD, ASD). TTL24 and TTL30 have no round
 *               RndMode - round mode
 * RETURN:  1 - succeed
 *                others - failed to load round register value to BCM buffer or invalid parameters
 ***************************************************************************************/
INT BE_VOP_SetRoundMode(VPP_OBJ *vpp_obj, INT OutputSel, INT RndMode)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    if(OutputSel == VPP_VOP_HDMI){
        RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG0;
        vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_rnd = RndMode;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->hdmi_vop.hdmi_vop_cfg0.u32);
    }

    return RetVal;
}

INT BE_VOP_SetPixTot(VPP_OBJ *vpp_obj, INT OutputSel, INT Width, INT Height)
{
    UINT32 pixtot = 0, uiRegAddr = 0;

    pixtot = Width * Height;
    uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMI_VOP_CFG6, pixtot);

    return 0;
}
/***************************************************************************************
 * FUNCTION: load CSC coeffs and offsets to VOP
 * PARAMS: OutputSel - indicates which VOP unit to be loaded(HDMI, AHD, ASD, TTL24, TTL30)
 *               ColorFmt - Color Format
 * RETURN:  1 - succeed
 *                others - failed to load CSC coeffs and offsets to BCM buffer
 ***************************************************************************************/
INT BE_VOP_LoadCscCoeff(VPP_OBJ *vpp_obj, INT OutputSel, INT ColorFmt)
{
    RESOLUTION_INFO  *pResinfo;

    if((OutputSel < VPP_VOP_AHD) || (OutputSel >= VPP_VOP_OUTPUT_MAX))
        return MV_VPP_EBADPARAM;

    if(OutputSel != VPP_VOP_HDMI)
        return MV_VPP_EBADPARAM;

    pResinfo = VPP_GetResInfo(VPP_GetResId(CPCB_1));

    VPP_DV_BE_VopLoadCscCoeff( vpp_obj, ColorFmt, pResinfo->active_width);
    return MV_VPP_OK;
}

/**************************************************************************************
 * FUNCTION: set HDMI/HD/SD encoder clock dividor
 * PARAMS: OutputSel - VOP output ID(VPP_VOP_HDMI, VPP_VOP_AHD, VPP_VOP_ASD)
 *         ClkSel - clock selector
 * RETURN:  1 - succeed
 *          others - failed
 ***************************************************************************************/
INT BE_VOP_ClkSelect(VPP_OBJ *vpp_obj, INT OutputSel, INT ClkSel)
{
    UINT32 RegAddr;
    INT RegValue;

    if((ClkSel < VPP_VOP_CLK_DISABLE) ||(ClkSel >= VPP_VOP_CLK_MAX))
        return MV_VPP_EBADPARAM;

    if(OutputSel == VPP_VOP_HDMI)
    {
        RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG0;

        if(VPP_VOP_CLK_DISABLE == ClkSel){
            RegValue = 0x00;
        }else if(VPP_VOP_CLK_D3 == ClkSel){
            RegValue = 0x07;
        }else if(VPP_VOP_CLK_D1 == ClkSel){
            RegValue = 0x04;
        }else{ //x2, x4, x6, x8, x12
            RegValue = (ClkSel << 3) | 0x05;
        }
        vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_clkx_ctrl = RegValue;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->hdmi_vop.hdmi_vop_cfg0.u32);
    }
    else{
        T32HDMI_VOP_CFG1 value;
        /*get register address of VOP clock dividor */
        switch(OutputSel){
            case VPP_VOP_ASD:
                RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG1;
                break;
            default:
                return MV_VPP_EBADPARAM;
        }

        if(VPP_VOP_CLK_DISABLE == ClkSel){
            RegValue = 0x00;
        }else if(VPP_VOP_CLK_D3 == ClkSel){
            RegValue = 0x07;
        }else if(VPP_VOP_CLK_D1 == ClkSel){
            RegValue = 0x04;
        }else{ //x2, x4, x6, x8, x12
            RegValue = (ClkSel << 3) | 0x05;
        }
        GA_REG_WORD32_READ(RegAddr,&value.u32);
        value.uCFG1_asd_clkx_ctrl = RegValue;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, value.u32);
    }
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Enable or disable HDMI formatter
 * PARAMS: Enable - 1: Enable  0: Disable
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtEnable(VPP_OBJ *vpp_obj, UINT8 Enable)
{
#ifdef FIXME_IN_BG4CT
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.Enable = Enable;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (VOP_3D_HDMI_CTRL0 << 2), vpp_obj->be.VppBeVopHDMIFrmtCtrl);
#endif
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Set HDMI formatter packing mode.
 * PARAMS: Mode - 0: Progressive Frame Packing
 *                2: Field Alternative Packing
 *                3: Field Frame Packing
 *         Enable - 0: disable Active Space insert
 *                  1: enable Active Space insert
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtPackingMode(VPP_OBJ *vpp_obj, UINT8 Mode, UINT8 Enable)
{
#ifdef FIXME_IN_BG4CT
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.PackMode = Mode;
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.ASInsert = Enable;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (VOP_3D_HDMI_CTRL0 << 2), vpp_obj->be.VppBeVopHDMIFrmtCtrl);
#endif
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Set HDMI formatter to one time L/R configuration mode, and indicate veiw ID for next frame during VblackL+VactiveL time.
 * PARAMS: ViewID - 0: Left view  1: Right view
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtViewIDOneTime(VPP_OBJ *vpp_obj, UINT8 ViewID)
{
#ifdef FIXME_IN_BG4CT
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.Mode = 0;
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.ViewID = ViewID;
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.Load = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (VOP_3D_HDMI_CTRL0 << 2), vpp_obj->be.VppBeVopHDMIFrmtCtrl);
#endif
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Set HDMI formatter to continuous L/R configuration mode, and indicate veiw ID for next frame during VblackL+VactiveL time.
 * PARAMS: ViewID - 0: Left view  1: Right view
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtViewIDContinuous(VPP_OBJ *vpp_obj, UINT8 ViewID)
{
#ifdef FIXME_IN_BG4CT
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.Mode = 1;
    ((VPP_VOP_HDMI_FRMT_CTRL0 *)(&(vpp_obj->be.VppBeVopHDMIFrmtCtrl)))->field.ViewID = ViewID;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (VOP_3D_HDMI_CTRL0 << 2), vpp_obj->be.VppBeVopHDMIFrmtCtrl);
#endif
    return MV_VPP_OK;
}

static INT VPP_BE_ConfigDns422_420(VPP_OBJ *vpp_obj, UINT32 uiBitDepth, UINT32 uiEnable)
{
    T32DNS422_420_CFG0 stDnsCfg0;

    memset(&stDnsCfg0, 0, sizeof(T32DNS422_420_CFG0));

    if(0 == uiEnable) {
        stDnsCfg0.uCFG0_dns422_420_en = 0;
    }
    else {
        stDnsCfg0.uCFG0_dns422_420_en = 1;
    }

    stDnsCfg0.uCFG0_dns422_420_msb_swap = 0;
    stDnsCfg0.uCFG0_sp_en = 0;
    stDnsCfg0.uCFG0_dns422_420_auto_pixcnt = 1;
    stDnsCfg0.uCFG0_outdata_mode = uiBitDepth;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_DNS422_420 + RA_DNS422_420_CFG0, stDnsCfg0.u32);

    return 0;
}

INT BE_VOP_SetOutputFmt_420(VPP_OBJ *vpp_obj, BOOL Enable, UINT32 bitdepth)
{
    /* 420 VOP Changes for Dolphin */

    INT32 RegAddr;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP+ RA_HDMI_VOP_CFG7;
    if(Enable)
    {
        vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_mode = 0x03;
        vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_420_inp = 0x01;
        if(bitdepth == OUTPUT_BIT_DEPTH_8BIT)
            vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_clr_depth = 0x04;
        else if(bitdepth == OUTPUT_BIT_DEPTH_10BIT)
            vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_clr_depth = 0x05;
        else
            vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_clr_depth = 0x06;
    }
    else
    {
        vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_mode = 0x02;  //default
        vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_clr_depth = 0x6;  //default
        vpp_obj->hdmi_vop.hdmi_vop_cfg7.uCFG7_hdmitx_420_inp = 0x00;   //default
    }
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->hdmi_vop.hdmi_vop_cfg7.u32);

    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_HDMI_VOP_DNS422_420,
                            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns422_420_bypass?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);
    VPP_BE_ConfigDns422_420(vpp_obj, bitdepth, Enable);
    return MV_VPP_OK;
}
#endif


