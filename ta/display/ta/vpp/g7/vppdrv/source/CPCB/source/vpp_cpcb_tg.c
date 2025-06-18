/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_CPCB_TG_C_

#include "vpp.h"

#include "vpp_module.h"
#include "vpp_regs.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_be.h"
#include "vpp_cpcb_tg.h"
#include "vpp_cpcb_tg_prv.h"
#include "vpp_cfg.h"

#define DUMMY_TG_SIZE_H_OFF_P      20
#define DUMMY_TG_SIZE_V_OFF_P      6
#define DUMMY_TG_HB_BE             7
#define DUMMY_TG_HB_VOP_OFF        8
#define DUMMY_TG_VB0_VOP_OFF       3
#define DUMMY_TG_VB0_BE            2

#define DUMMY_TG_HB_FP             10
#define DUMMY_TG_VB_FP             2
#define DUMMY_TG_HB_MIN            30
#define DUMMY_TG_VB_MIN            6

#define VTOTAL_HD                  749
#define VTOTAL_FHD                 1124
#define VTOTAL_UHD                 2249

/***************************************************************************************
 * FUNCTION: load the default register values for CPCB TG block
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_LoadDefaultVal(VPP_OBJ *vpp_obj, INT CpcbUnit)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;

    CHECK_CPCB_NUM(CpcbUnit);

    /*load all the default registers*/
    //CPCB0 setting
    if (VPP_CPCB0 == CpcbUnit) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL0;
        vpp_obj->cpcb0_ctrl0.uCTRL0_osdovl_bypass = 0;
        vpp_obj->cpcb0_ctrl0.uCTRL0_MAIN_FIFO_bypass = 0;
        vpp_obj->cpcb0_ctrl0.uCTRL0_GFX1_FIFO_bypass = 0;
        vpp_obj->cpcb0_ctrl0.uCTRL0_GFX2_FIFO_bypass = 0;
        vpp_obj->cpcb0_ctrl0.uCTRL0_CSC_bypass = 0;
        vpp_obj->cpcb0_ctrl0.uCTRL0_osdovl_odat_sel = 0x0;//any is OK
        VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->cpcb0_ctrl0.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL2;
        vpp_obj->cpcb0_ctrl2.uCTRL2_CLKEN_CTRL = 0x0; //disable all planes
        vpp_obj->cpcb0_ctrl2.uCTRL2_cpcb_clken = 0; //doubt???
        vpp_obj->cpcb0_ctrl2.uCTRL2_beFifosts_ctrl = 1;
        vpp_obj->cpcb0_ctrl2.uCTRL2_tgEn = 0; //every time??
        VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->cpcb0_ctrl2.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_CPCB0_PL_EN;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->cpcb0_pl_en.u32);
        vpp_obj->cpcb0_pl_en.u32 = 0x1F;
        VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->cpcb0_pl_en.u32);
    } else if (VPP_CPCB1 == CpcbUnit) {
        //Empty now
    }
    return RetVal;
}

/**************************************************************************************
 * FUNCTION: start the timing generator by set horizontal and vertical total
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                HVtotal - horizontal and vertical total values
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_SetHVTotal(VPP_OBJ *vpp_obj, INT CpcbUnit, INT resID, PVPP_CPCB_TG_HVTOTAL pHVtotal)
{
    UINT32 uiRegAddr;
    T32TG_PRG_Total  stTgTotal;
    T32TG_PRG_Initial stTgInit;
    T32TG_PRG_FT      stTgFt;
    UINT32 mul;

    stTgTotal.u32 = 0;
    stTgInit.u32 = 0;
    stTgFt.u32 = 0;

    CHECK_CPCB_NUM(CpcbUnit);

    mul = (pHVtotal->VTotal == VTOTAL_UHD)? 1: 0;

    if (RES_DSI_CUSTOM == resID)
    {
        stTgTotal.uTotal_vertical   = pHVtotal->Height +
                                      vpp_obj->dsiInfo.dsiTgCalibrationParams.tg_vb_min;
        stTgTotal.uTotal_horizontal = pHVtotal->Width +
                                      vpp_obj->dsiInfo.dsiTgCalibrationParams.tg_hb_min;
    }
    else if ((pHVtotal->VTotal == VTOTAL_UHD) ||
            (pHVtotal->VTotal == VTOTAL_FHD) ||
            (resID == RES_DSI_1920x1080P5994)) {
        //Set Minimum Horizontal and vertical total
        stTgTotal.uTotal_vertical   =    pHVtotal->Height + (DUMMY_TG_VB_MIN<<mul) - 1;
        stTgTotal.uTotal_horizontal =    pHVtotal->Width + (DUMMY_TG_HB_MIN<<mul) - 1;
    } else if ((pHVtotal->VTotal == VTOTAL_HD) ||
        (pHVtotal->VTotal == ((VTOTAL_FHD>>1)-1)) || //VTotal for 1080i resolution
        IS_RES_NEED_FULL_BLANKING(resID)) {
        //Set full Horizontal and vertical total
        stTgTotal.uTotal_vertical   =   pHVtotal->VTotal;
        stTgTotal.uTotal_horizontal =   pHVtotal->HTotal;
    } else {
        //Set Minimum Horizontal and vertical total
        stTgTotal.uTotal_vertical   =   pHVtotal->Height + DUMMY_TG_VB_MIN;
        stTgTotal.uTotal_horizontal =   pHVtotal->Width + DUMMY_TG_HB_MIN;
    }

    if (pHVtotal->HTotal > 8)
        stTgInit.uInitial_xi = stTgTotal.uTotal_horizontal - 8;
    else
        stTgInit.uInitial_xi = 0;

    stTgInit.uInitial_yi = 0;

    stTgFt.uFT_frame = pHVtotal->Width * pHVtotal->Height;

    if (VPP_CPCB0 == CpcbUnit) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CPCB_TG + RA_BETG_TG_PRG;
    } else if (VPP_VOP_TG == CpcbUnit) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_VOPTG + RA_BETG_TG_PRG;
    } else
        return (MV_VPP_EBADPARAM);

    /*set vertical total value field to BCM buffer*/
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_Total, stTgTotal.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_Initial, stTgInit.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_FT, stTgFt.u32);

    return MV_VPP_OK;
}

INT VOP_TG_SetParams(VPP_OBJ *vpp_obj, INT HRes, INT VRes)
{
    INT RetVal = MV_VPP_OK;
#ifdef ENABLE_GFX2_VOP2_PATH
    UINT32 RegAddr, RegVal;
    INT leftCrop, rightCrop, topCrop, bottomCrop;

    leftCrop = 0;
    rightCrop = 0;
    topCrop = 0;
    bottomCrop = 0;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 +  RA_VOP2_TG;

    RegVal = (VRes + DUMMY_TG_SIZE_V_OFF_P) + ((HRes + DUMMY_TG_SIZE_H_OFF_P) << 16);

    /*write TG size to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = HRes + DUMMY_TG_HB_VOP_OFF + ((DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (HRes-rightCrop) + DUMMY_TG_HB_VOP_OFF + (((leftCrop)+DUMMY_TG_HB_BE) << 16);
    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR2), RegVal)))
        return RetVal;
    /*calculate the V blank front edge value*/
    RegVal = VRes + DUMMY_TG_VB0_VOP_OFF + ((DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    RegVal = (VRes-bottomCrop) + DUMMY_TG_VB0_VOP_OFF + ((topCrop+DUMMY_TG_VB0_BE) << 16);
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR2), RegVal)))
        return RetVal;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_CTRL1;
    RegVal = VRes * HRes;
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegVal)))
        return RetVal;

    RegAddr=  vpp_obj->base_addr + RA_Vpp_CTRL0;
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegVal)))
        return RetVal;
#endif //ENABLE_GFX2_VOP2_PATH
    return RetVal;
}
  /**************************************************************************************
 * FUNCTION: set the CPCB TG params
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                CpcbTgParams - TG params
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_SetParams(VPP_OBJ *vpp_obj, INT CpcbUnit, INT resID, PVPP_CPCB_TG_PARAMS pTgParams)
{
    UINT32 uiRegAddr = 0;
    T32TG_PRG_CTRL    stTgCtrl;
    T32TG_PRG_CTRL1   stTgCtrl1;
    T32TG_PRG_VX      stTgVx;
    T32TG_PRG_HSYNC   stTgHsync;
    T32TG_PRG_VSYNC   stTgVsync;
    RESOLUTION_INFO *resinfo = VPP_GetResInfo(resID);
    RESOLUTION_INFO *ref_resinfo = VPP_GetResInfo((resinfo->type == TYPE_UHD)?RES_4Kx2K60:RES_1080P60);

    stTgCtrl.u32 = 0;
    stTgCtrl1.u32 = 0;
    stTgVx.u32 = 0;
    stTgHsync.u32 = 0;
    stTgVsync.u32 = 0;

    CHECK_CPCB_NUM(CpcbUnit);

    stTgCtrl.uCTRL_frst     = pTgParams->FrameReset;
    stTgCtrl.uCTRL_mode     = pTgParams->Mode;
    stTgCtrl.uCTRL_lwin     = pTgParams->Lwin;
    stTgCtrl.uCTRL_freeze   = pTgParams->Freeze;

    stTgCtrl1.uCTRL1_sync_ctrl = pTgParams->SyncCtrl;

    if (RES_DSI_CUSTOM == resID) {
        /*Use reduced blanking for Custom Panel ID*/
        stTgHsync.uHSYNC_h_start    = resinfo->active_width +
                                      vpp_obj->dsiInfo.dsiTgCalibrationParams.tg_hb_fp;
        stTgHsync.uHSYNC_h_end      = stTgHsync.uHSYNC_h_start +
                                      vpp_obj->dsiInfo.dsiTgCalibrationParams.tg_hb_fp;
        stTgVsync.uVSYNC_v_start    = resinfo->active_height +
                                      vpp_obj->dsiInfo.dsiTgCalibrationParams.tg_vb_fp;
        stTgVsync.uVSYNC_v_end      = stTgVsync.uVSYNC_v_start +
                                      vpp_obj->dsiInfo.dsiTgCalibrationParams.tg_vb_fp;
        stTgVx.uVX_vx    =  stTgHsync.uHSYNC_h_start;
    } else if (IS_RES_NEED_REDUCED_BLANKING(resID)) {
        /*Use reduced blanking for most of Panel resolutions*/
        stTgHsync.uHSYNC_h_start    =   resinfo->active_width + DUMMY_TG_HB_FP;
        stTgHsync.uHSYNC_h_end      =   stTgHsync.uHSYNC_h_start + DUMMY_TG_HB_FP;
        stTgVsync.uVSYNC_v_start    =   resinfo->active_height + DUMMY_TG_VB_FP;
        stTgVsync.uVSYNC_v_end      =   stTgVsync.uVSYNC_v_start + DUMMY_TG_VB_FP;
        stTgVx.uVX_vx    =  stTgHsync.uHSYNC_h_start;
    } else if (pTgParams->VSyncStartPos ==
            (ref_resinfo->active_height + ref_resinfo->vfrontporch)) {
        /*Use reduced blanking for 2K & 4K resolutions*/
        stTgHsync.uHSYNC_h_start    =   ref_resinfo->active_width + (DUMMY_TG_HB_FP * ref_resinfo->type);
        stTgHsync.uHSYNC_h_end      =   stTgHsync.uHSYNC_h_start + (DUMMY_TG_HB_FP * ref_resinfo->type);
        stTgVsync.uVSYNC_v_start    =   ref_resinfo->active_height + (DUMMY_TG_VB_FP * ref_resinfo->type);
        stTgVsync.uVSYNC_v_end      =   stTgVsync.uVSYNC_v_start + (DUMMY_TG_VB_FP * ref_resinfo->type);
        stTgVx.uVX_vx    =  stTgHsync.uHSYNC_h_start;
    } else {
        stTgHsync.uHSYNC_h_start    =    pTgParams->HSyncStartPos;
        stTgHsync.uHSYNC_h_end      =    pTgParams->HSyncEndPos;
        stTgVsync.uVSYNC_v_start    =    pTgParams->VSyncStartPos;
        stTgVsync.uVSYNC_v_end      =    pTgParams->VSyncEndPos;
        stTgVx.uVX_vx   =  pTgParams->Vsampx;
    }

    if (VPP_CPCB0 == CpcbUnit) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CPCB_TG + RA_BETG_TG_PRG;
    } else if (VPP_VOP_TG == CpcbUnit) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_VOPTG + RA_BETG_TG_PRG;
    } else
        return (MV_VPP_EBADPARAM);


    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_CTRL, stTgCtrl.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_CTRL1, stTgCtrl1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_HSYNC, stTgHsync.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_VSYNC, stTgVsync.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_VX, stTgVx.u32);

    return MV_VPP_OK;
}

/**************************************************************************************
 * FUNCTION: set the the window position for one plane in the backend. can be used to disable a plane by setting the window to 0
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *               CpCbPlane - CPCB plane number
 *               WindowPos - plane window position
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_SetPlaneWindow(VPP_OBJ *vpp_obj, INT CpcbUnit, INT CpCbPlane, PVPP_WINDOW pWindowPos)
{
    T32TG_PL_X stTgPlaneX;
    T32TG_PL_Y stTgPlaneY;
    UINT32 PlaneAddr = 0, uiTgAddr = 0;

    stTgPlaneX.u32 = 0;
    stTgPlaneY.u32 = 0;

    CHECK_CPCB_NUM(CpcbUnit);

    if (VPP_CPCB0 == CpcbUnit) {
        uiTgAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CPCB_TG;
    } else if (VPP_VOP_TG) {
        uiTgAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_VOPTG;
    } else {
        return (MV_VPP_EBADPARAM);
    }

    switch (CpCbPlane) {
        case TG_PLANE0_BASE:
            PlaneAddr = uiTgAddr + RA_BETG_PL0;
            break;
        case TG_PLANE1_MAIN:
            PlaneAddr = uiTgAddr + RA_BETG_PL1;
            break;
        case TG_PLANE1_MAIN_CROP:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL1_CR;
            break;
        case TG_PLANE2_PIP:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL2;
            break;
        case TG_PLANE2_PIP_CROP:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL2_CR;
            break;
        case TG_PLANE3_GFX0:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL3;
            break;
        case TG_PLANE3_GFX0_CROP:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL3_CR;
            break;
        case TG_PLANE4_BASE_BORDER:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL4;
            break;
        case TG_PLANE5_MAIN_BORDER:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL5;
            break;
        case TG_PLANE6_PIP_BORDER:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL6;
            break;
        case TG_PLANE7_GFX0_BORDER:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL7;
            break;
        case TG_PLANE8_OVL:
            PlaneAddr = uiTgAddr +  + RA_BETG_PL8;
            break;
        default:
            return (MV_VPP_EBADPARAM);
    }

    stTgPlaneX.uX_start     = pWindowPos->TopLeft.X;
    stTgPlaneX.uX_end       = pWindowPos->BottomRight.X;
    stTgPlaneY.uY_start     = pWindowPos->TopLeft.Y;
    stTgPlaneY.uY_end       = pWindowPos->BottomRight.Y;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, PlaneAddr + RA_TG_PL_X, stTgPlaneX.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, PlaneAddr + RA_TG_PL_Y, stTgPlaneY.u32);

    return MV_VPP_OK;
}

/**************************************************************************************
 * FUNCTION: set CPCB TG actual clock
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *               ClkSel - CPCB clock selector
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG clock selection to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_ClkSelect(VPP_OBJ *vpp_obj, INT CpcbUnit, INT ClkSel)
{
    UINT32 RegAddr;
    INT RegValue;

    CHECK_CPCB_NUM(CpcbUnit);

    if ((ClkSel < VPP_CPCB_CLK_DISABLE) ||(ClkSel >= VPP_CPCB_CLK_MAX))
        return MV_VPP_EBADPARAM;

    if (VPP_CPCB0 == CpcbUnit)
        RegAddr = vpp_obj->base_addr + RA_Vpp_cpcb0Clk;
    else
        return (MV_VPP_EBADPARAM);

    if (VPP_CPCB_CLK_DISABLE == ClkSel) {
        RegValue = 0x00;
    } else if (VPP_CPCB_CLK_D3 == ClkSel) {
        RegValue = 0x07;
    } else if (VPP_CPCB_CLK_D1 == ClkSel) {
        RegValue = 0x04;
    } else { //x2, x4, x6, x8, x12
        RegValue = (ClkSel << 3) | 0x05;
    }

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegValue);

    return MV_VPP_OK;
}

#ifdef VPP_UNDERFLOW_FIX
INT VOP_TG_Enable(VPP_OBJ *vpp_obj, int cpcbID, int Enable)
{
    UINT32 RegAddr;
    T32HDMI_VOP_CFG5 sHdmiVopCfg5;

    if (cpcbID != CPCB_1)
        return MV_VPP_OK;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP+ RA_HDMI_VOP_CFG5;
    GA_REG_WORD32_READ(RegAddr, &sHdmiVopCfg5.u32);
    //Clear TG
    if (0 == Enable) {
        sHdmiVopCfg5.uCFG5_clear = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, sHdmiVopCfg5.u32);
        sHdmiVopCfg5.uCFG5_clear = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, sHdmiVopCfg5.u32);
    } else {
        //Enable TG
        sHdmiVopCfg5.uCFG5_start = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, sHdmiVopCfg5.u32);
        sHdmiVopCfg5.uCFG5_start = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, sHdmiVopCfg5.u32);
    }

    return MV_VPP_OK;
}
#else
INT VOP_TG_Enable(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;
    T32HDMI_VOP_CFG5 sHdmiVopCfg5;

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP+ RA_HDMI_VOP_CFG5;
    GA_REG_WORD32_READ(RegAddr, &sHdmiVopCfg5.u32);

    sHdmiVopCfg5.uCFG5_start = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, sHdmiVopCfg5.u32);
    sHdmiVopCfg5.uCFG5_start = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, sHdmiVopCfg5.u32);

    return MV_VPP_OK;
}
#endif

INT CPCB_TG_Enable(VPP_OBJ *vpp_obj, INT CpcbUnit, INT Enable)
{
    UINT32 RegAddr;

    if (VPP_CPCB0 == CpcbUnit)
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL2;
    else
        return (MV_VPP_EBADPARAM);

    vpp_obj->cpcb0_ctrl2.uCTRL2_tgEn = Enable;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cpcb0_ctrl2.u32);
    vpp_obj->cpcb0_ctrl2.uCTRL2_tgEn = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cpcb0_ctrl2.u32);

    return MV_VPP_OK;
}

INT VOP2_TG_Start(VPP_OBJ *vpp_obj, INT CpcbUnit,INT start)
{
#ifdef ENABLE_GFX2_VOP2_PATH
    UINT32 RegAddr;
    /*VOP2 TG only during DP1->DP2 and other DP2 cases*/
    if(IS_CPCB_VOUT_DISP_MODE(CpcbUnit, SINGLE_MODE_SEC) || (CpcbUnit == CPCB_2))
    {
        //VOP2 DTG -> VOP2 ASSYNC FIFO ->VOP2 TG -> VOP TG -> CPCB TG
        RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 +  RA_VOP2_TG_CTRL;
        if (start == 0) {
            vpp_obj->vop2_tg_ctl.uTG_CTRL_clear = 1;
            if (CpcbUnit != CPCB_2)
                vpp_obj->vop2_tg_ctl.uTG_CTRL_dp1Afifo_clear = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vop2_tg_ctl.u32);

            vpp_obj->vop2_tg_ctl.uTG_CTRL_clear = 0;
            if (CpcbUnit != CPCB_2)
                vpp_obj->vop2_tg_ctl.uTG_CTRL_dp1Afifo_clear = 0;
            VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vop2_tg_ctl.u32);
        } else {
            vpp_obj->vop2_tg_ctl.uTG_CTRL_start = 1;
            if (CpcbUnit != CPCB_2)
                vpp_obj->vop2_tg_ctl.uTG_CTRL_dp1Afifo_start = 1;

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->vop2_tg_ctl.u32);

            vpp_obj->vop2_tg_ctl.uTG_CTRL_start = 0;
            if (CpcbUnit != CPCB_2)
                vpp_obj->vop2_tg_ctl.uTG_CTRL_dp1Afifo_start = 0;

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->vop2_tg_ctl.u32);
        }
    }
#endif //ENABLE_GFX2_VOP2_PATH
    return MV_VPP_OK;
}

INT CPCB_TG_PlaneEnable(VPP_OBJ *vpp_obj, INT CpcbUnit, INT CpCbPlane, INT Enable)
{
    UINT32 RegAddr;
    INT bit_pos = 0;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL2;
    if (VPP_CPCB0 == CpcbUnit) {
        if (CpCbPlane==TG_PLANE1_MAIN)
            bit_pos = 0;
        else if (CpCbPlane==TG_PLANE2_PIP)
            bit_pos = 2;
        else if (CpCbPlane==TG_PLANE3_GFX0)
            bit_pos = 3;
    } else
        return (MV_VPP_EBADPARAM);

    if (Enable)
        vpp_obj->cpcb0_ctrl2.uCTRL2_CLKEN_CTRL |= 1 << bit_pos;
    else
        vpp_obj->cpcb0_ctrl2.uCTRL2_CLKEN_CTRL &= ~(1 << bit_pos);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cpcb0_ctrl2.u32);

    return MV_VPP_OK;
}