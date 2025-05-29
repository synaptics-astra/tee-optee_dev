/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_CPCB_DISP_TG_C_

#include "vpp.h"
#include "vpp_module.h"
#include "vpp_regs.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_cpcb_disptg.h"
#include "vpp_cpcb_tg_prv.h"
#include "vpp_cfg.h"

#define DEFAULT_CPCB_RESOLUTION RES_RESET

#define  VALIDATE_ADDR(addr) {if (addr < vpp_obj->base_addr_gfx_vop) \
                                 return MV_VPP_OK;}
UINT32 CPCB_DISPTG_BaseAddr(VPP_OBJ *vpp_obj, int cpcb)
{
    UINT32 regAddr = 0;

    if (cpcb == CPCB_1) {
        regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_DTG;
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    else if (cpcb == CPCB_2) {
        regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_DTG;
    }
#endif
    else {
        VPP_CPCB_DBG_LOG("CPCB_DISPTG_BaseAddr: Invalid [%d]\r\n", cpcb);
    }
    return regAddr;
}
/***************************************************************************************
 * FUNCTION: load the default register values for CPCB TG block
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_DISPTG_LoadDefaultVal(VPP_OBJ *vpp_obj, int cpcb)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;

    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb) + RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG + RA_TG_PRG_CTRL1;
    VALIDATE_ADDR(RegAddr);
    GA_REG_WORD32_READ(RegAddr, &vpp_obj->disptg_prg_ctrl1[cpcb].u32);
    /*load all the default registers*/
    RESOLUTION_INFO *resinfo = VPP_GetResInfo(DEFAULT_CPCB_RESOLUTION);
    VPP_CPCB_DISPTG_HVTOTAL hvtot;
    VPP_CPCB_DISPTG_PARAMS tg_params;
    VPP_WINDOW window;

    hvtot.HTotal = resinfo->width-1;
    hvtot.VTotal = resinfo->height-1;
    hvtot.Width = resinfo->active_width;
    hvtot.Height = resinfo->active_height;
    CPCB_DISPTG_SetHVTotal(vpp_obj, cpcb, &hvtot, OUTPUT_COLOR_FMT_INVALID);

    tg_params.FrameReset = resinfo->active_height+7;
    if (resinfo->type == TYPE_SD) {
        tg_params.HSyncEndPos = resinfo->active_width + resinfo->hfrontporch;
        tg_params.HSyncStartPos = tg_params.HSyncEndPos + resinfo->hsyncwidth;
        tg_params.VSyncEndPos = resinfo->active_height + resinfo->vfrontporch;
        tg_params.VSyncStartPos = tg_params.VSyncEndPos + resinfo->vsyncwidth;
        tg_params.Vsampx = tg_params.HSyncEndPos/*0 for HDE/VDE */;
    } else {
        tg_params.HSyncStartPos = resinfo->active_width + resinfo->hfrontporch;
        tg_params.HSyncEndPos = tg_params.HSyncStartPos + resinfo->hsyncwidth;
        tg_params.VSyncStartPos = resinfo->active_height + resinfo->vfrontporch/* -1 */;
        tg_params.VSyncEndPos = tg_params.VSyncStartPos + resinfo->vsyncwidth;
        tg_params.Vsampx = tg_params.HSyncStartPos/*0 for HDE/VDE */;
    }
    tg_params.Mode = 0; //3; 0- free running mode 1- sync mode 2- semi sync mode
    tg_params.Lwin = 0xF;
    tg_params.Freeze = 0;
    tg_params.SyncCtrl = 0;
    CPCB_DISPTG_SetParams(vpp_obj, cpcb, &tg_params);

    window.TopLeft.X = 0;
    window.TopLeft.Y = 0;
    window.BottomRight.X = resinfo->active_width;
    window.BottomRight.Y = resinfo->active_height;
    CPCB_DISPTG_SetPlaneWindow(vpp_obj, cpcb, &window);

    return RetVal;
}


/**************************************************************************************
 * FUNCTION: start the timing generator by set horizontal and vertical total
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                HVtotal - horizontal and vertical total values
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_DISPTG_SetHVTotal(VPP_OBJ *vpp_obj, INT cpcb, PVPP_CPCB_DISPTG_HVTOTAL pHVtotal, int color_fmt)
{
    UINT32 uiRegAddr;
    T32TG_PRG_Total  stTgTotal;
    T32TG_PRG_Initial stTgInit;
    T32TG_PRG_FT      stTgFt;
    UINT8 val = 8;

    stTgTotal.u32 = 0;
    stTgInit.u32 = 0;
    stTgFt.u32 = 0;

    //Set Horizontal and vertical total
    stTgTotal.uTotal_vertical   =   pHVtotal->VTotal;
    stTgTotal.uTotal_horizontal =   pHVtotal->HTotal;

    if (color_fmt == OUTPUT_COLOR_FMT_YCBCR420) {
        val = 4;
    }

    if (pHVtotal->HTotal > val)
        stTgInit.uInitial_xi = stTgTotal.uTotal_horizontal - val;
    else
        stTgInit.uInitial_xi = 0;

    stTgInit.uInitial_yi = 0;

    stTgFt.uFT_frame = pHVtotal->Width * pHVtotal->Height;

    uiRegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb)+ RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG;
    VALIDATE_ADDR(uiRegAddr);
    /*set vertical total value field to BCM buffer*/
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_Total, stTgTotal.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_Initial, stTgInit.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_FT, stTgFt.u32);
    return MV_VPP_OK;
}

/**************************************************************************************
 * FUNCTION: set the CPCB TG params
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                CpcbTgParams - TG params
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_DISPTG_SetParams(VPP_OBJ *vpp_obj, INT cpcb, PVPP_CPCB_DISPTG_PARAMS pTgParams)
{
    UINT32 uiRegAddr = 0;

    T32TG_PRG_CTRL    stTgCtrl;
    T32TG_PRG_VX      stTgVx;
    T32TG_PRG_HSYNC   stTgHsync;
    T32TG_PRG_VSYNC   stTgVsync;
    T32TG_PRG_VS      stTgVs;

    stTgCtrl.u32 = 0;
    stTgVx.u32 = 0;
    stTgHsync.u32 = 0;
    stTgVsync.u32 = 0;
    stTgVs.u32 = 0;

    stTgCtrl.uCTRL_frst     = pTgParams->FrameReset;
    stTgCtrl.uCTRL_mode     = pTgParams->Mode;
    stTgCtrl.uCTRL_lwin     = pTgParams->Lwin;
    stTgCtrl.uCTRL_freeze   = pTgParams->Freeze;

    vpp_obj->disptg_prg_ctrl1[cpcb].uCTRL1_sync_ctrl = pTgParams->SyncCtrl;

    stTgHsync.uHSYNC_h_start    =    pTgParams->HSyncStartPos;
    stTgHsync.uHSYNC_h_end      =    pTgParams->HSyncEndPos;
    stTgVsync.uVSYNC_v_start    =    pTgParams->VSyncStartPos;
    stTgVsync.uVSYNC_v_end      =    pTgParams->VSyncEndPos;
    stTgVs.uVS_h_start          =    pTgParams->VSStartPos;
    stTgVs.uVS_h_end            =    pTgParams->VSEndPos;

    stTgVx.uVX_vx   =  pTgParams->Vsampx;

    uiRegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb)+ RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG;
    VALIDATE_ADDR(uiRegAddr);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_CTRL, stTgCtrl.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_CTRL1, vpp_obj->disptg_prg_ctrl1[cpcb].u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_HSYNC, stTgHsync.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_VSYNC, stTgVsync.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_VX, stTgVx.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_PRG_VS, stTgVs.u32);
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
INT CPCB_DISPTG_SetPlaneWindow(VPP_OBJ *vpp_obj, INT cpcb, PVPP_WINDOW pWindowPos)
{
    T32TG_PL_X stTgPlaneX;
    T32TG_PL_Y stTgPlaneY;
    UINT32 PlaneAddr;

    stTgPlaneX.u32 = 0;
    stTgPlaneY.u32 = 0;

    PlaneAddr = CPCB_DISPTG_BaseAddr(vpp_obj,cpcb) + RA_DTG_DISPLAY_TG + RA_BETG_PL0;
    VALIDATE_ADDR(PlaneAddr);

    stTgPlaneX.uX_start     = pWindowPos->TopLeft.X;
    stTgPlaneX.uX_end       = pWindowPos->BottomRight.X;
    stTgPlaneY.uY_start     = pWindowPos->TopLeft.Y;
    stTgPlaneY.uY_end       = pWindowPos->BottomRight.Y;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, PlaneAddr + RA_TG_PL_X, stTgPlaneX.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, PlaneAddr + RA_TG_PL_Y, stTgPlaneY.u32);
    return MV_VPP_OK;
}

INT CPCB_DISPTG_EnableIntl(VPP_OBJ *vpp_obj, INT cpcb, INT Enable)
{
    UINT32 RegAddr;

    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj,cpcb) + RA_DTG_CTRL1;
    VALIDATE_ADDR(RegAddr);

    vpp_obj->dtg_ctrl1[cpcb].uCTRL1_fld_en = 0; //FIXME: rewuired????
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->dtg_ctrl1[cpcb].u32);

    if (Enable) {
        vpp_obj->dtg_ctrl1[cpcb].uCTRL1_fld_en = 1;
        vpp_obj->disptg_prg_ctrl1[cpcb].uCTRL1_res_change_en = 0;//programmed after first interrupt in Toggle field
        VPPAPI_DBG_INFO("vppdrv: fld enable for interlaced input\n");
    }else {
        vpp_obj->dtg_ctrl1[cpcb].uCTRL1_fld_en = 0;
        vpp_obj->disptg_prg_ctrl1[cpcb].uCTRL1_res_change_en = 0;
    }

    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj,cpcb) + RA_DTG_CTRL1;
    VALIDATE_ADDR(RegAddr);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->dtg_ctrl1[cpcb].u32);
    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb) + RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG+RA_TG_PRG_CTRL1;
    VALIDATE_ADDR(RegAddr);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->disptg_prg_ctrl1[cpcb].u32);

    vpp_obj->disptg_intl_count = 0; // the field enable will be programmed in nex interrupt

    return MV_VPP_OK;
}

INT CPCB_DISPTG_ToggleField(VPP_OBJ *vpp_obj, INT cpcb) {
    UINT32 RegAddr;
    T32DTG_status dtg_status;
    T32DTG_CTRL1 dtg_ctrl1;
    T32TG_PRG_VSYNC   stTgVsync;
    T32TG_PRG_VS      stTgVs;
    T32TG_PRG_Total  stTgTotal;

    RESOLUTION_INFO *resinfo = VPP_GetResInfo(vpp_obj->uiResID[cpcb]);
    VPP_CPCB_DISPTG_HVTOTAL disphvtot;

    if (resinfo->scan != SCAN_INTERLACED)
        return MV_VPP_OK;

    vpp_obj->disptg_intl_count++;

    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj,cpcb) + RA_DTG_CTRL1;
    VALIDATE_ADDR(RegAddr);
    GA_REG_WORD32_READ(RegAddr, &dtg_ctrl1.u32);// need to read

    if (vpp_obj->disptg_intl_count == 1 && dtg_ctrl1.uCTRL1_fld_en) { //confirm fld_en is programmed
        RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj,cpcb) + RA_DTG_status;
        VALIDATE_ADDR(RegAddr);
        GA_REG_WORD32_READ(RegAddr, &dtg_status.u32);
        if (dtg_status.ustatus_ofld) {
            vpp_obj->dtg_ctrl1[cpcb].uCTRL1_fld_ctrl = 1;
        } else {
            vpp_obj->dtg_ctrl1[cpcb].uCTRL1_fld_ctrl = 0;
        }
        RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj,cpcb) + RA_DTG_CTRL1;
        VALIDATE_ADDR(RegAddr);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->dtg_ctrl1[cpcb].u32);

        vpp_obj->disptg_prg_ctrl1[cpcb].uCTRL1_res_change_en = 0x1FF;
        RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb)+ RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG + RA_TG_PRG_CTRL1;
        VALIDATE_ADDR(RegAddr);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->disptg_prg_ctrl1[cpcb].u32);
    } else if (vpp_obj->disptg_intl_count == 1) {
        vpp_obj->disptg_intl_count = 0; //reset counter
    }
    //FIXME: need to program scl polarity bit here.

    disphvtot.HTotal = resinfo->width-1;
    disphvtot.Width = resinfo->active_width;
    if (vpp_obj->disptg_intl_count%2 == 1) //for next interrupt
        disphvtot.VTotal = (resinfo->height >> 1);
    else
        disphvtot.VTotal = (resinfo->height >> 1)-1;
    disphvtot.Height = (resinfo->active_height >> 1);

    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb)+ RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG;
    VALIDATE_ADDR(RegAddr);
    stTgTotal.uTotal_vertical   =   disphvtot.VTotal;
    stTgTotal.uTotal_horizontal =   disphvtot.HTotal;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_TG_PRG_Total, stTgTotal.u32);

    RegAddr = CPCB_DISPTG_BaseAddr(vpp_obj, cpcb)+ RA_DTG_DISPLAY_TG + RA_BETG_TG_PRG;
    VALIDATE_ADDR(RegAddr);
    if (vpp_obj->disptg_intl_count%2 == 1) { //for next interrupt
        //TODO: VPP_SW_BRINGUP: Check of this is required
        stTgVsync.uVSYNC_v_start    =    (disphvtot.VTotal + 1) - resinfo->vbackporch - resinfo->vsyncwidth;
        stTgVsync.uVSYNC_v_end      =    (disphvtot.VTotal + 1) - resinfo->vbackporch;
        stTgVs.uVS_h_start          =    (resinfo->width - resinfo->hbackporch - resinfo->hsyncwidth) - (resinfo->width >> 1);
        stTgVs.uVS_h_end            =    (resinfo->width - resinfo->hbackporch - resinfo->hsyncwidth) - (resinfo->width >> 1);
    } else {
        stTgVsync.uVSYNC_v_start    =    (disphvtot.VTotal + 1) - resinfo->vbackporch - resinfo->vsyncwidth;
        stTgVsync.uVSYNC_v_end      =    (disphvtot.VTotal + 1) - resinfo->vbackporch;
        stTgVs.uVS_h_start          =    (resinfo->width - resinfo->hbackporch - resinfo->hsyncwidth);
        stTgVs.uVS_h_end            =    (resinfo->width - resinfo->hbackporch - resinfo->hsyncwidth);
    }

    if (resinfo->type == TYPE_SD) {
        stTgVsync.uVSYNC_v_start    =    (disphvtot.VTotal + 1) - resinfo->vbackporch;
        stTgVsync.uVSYNC_v_end      =    (disphvtot.VTotal + 1) - resinfo->vbackporch - resinfo->vsyncwidth;
    } else {
        stTgVsync.uVSYNC_v_start    =    (disphvtot.VTotal + 1) - resinfo->vbackporch - resinfo->vsyncwidth;
        stTgVsync.uVSYNC_v_end      =    (disphvtot.VTotal + 1) - resinfo->vbackporch;
    }

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_TG_PRG_VSYNC, stTgVsync.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_TG_PRG_VS, stTgVs.u32);

    return MV_VPP_OK;
}
