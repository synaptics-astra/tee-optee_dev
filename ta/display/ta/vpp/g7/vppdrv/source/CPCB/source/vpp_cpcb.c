/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_CPCB_C_

#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cfg.h"
#include "vpp_cpcb.h"

#include "vpp_regs.h"
#include "vpp_cpcb_ovl.h"
#include "vpp_cpcb_tg.h"
#include "vpp_cpcb_disptg.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdmitx.h"
#include "vpp_be_vop.h"

              ///////////////////////////////////////////
              //                                       //
              //    CPCB functions for Berlin          //
              //                                       //
              ///////////////////////////////////////////

/*******************************************************************
 * FUNCTION: initialize all VPP CPCB blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_Reset(VPP_OBJ *vpp_obj)
{

    /* reset CPCB video-overlay */
    VPP_OVL_LoadDefaultVal(vpp_obj, VPP_CPCB_OVL0);
    VPP_OVL_LoadDefaultVal(vpp_obj, VPP_CPCB_OVL1);
    VPP_OVL_LoadDefaultVal(vpp_obj, VPP_CPCB_OVL2);

    /* reset CPCB TG */
    CPCB_TG_LoadDefaultVal(vpp_obj, VPP_CPCB0);

    /* reset CPCB DISPLAY TG */
    CPCB_DISPTG_LoadDefaultVal(vpp_obj, VPP_CPCB0);
#ifdef ENABLE_GFX2_VOP2_PATH
    CPCB_DISPTG_LoadDefaultVal(vpp_obj, VPP_CPCB1);
#endif

    return (MV_VPP_OK);
}

/**********************************************************************
 * FUNCTION: Initialize all VPP CPCB blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 *********************************************************************/
int VPP_CPCB_Config(VPP_OBJ *vpp_obj)
{
    VPP_OVL_OSD_CTRL ovl_ctrl;
    int i, cpcbID, chanID;
    int order[7];
    CHAN *chan;

    /* set OSD overlay mode & domain for CPCBs */
    for (cpcbID = CPCB_1; cpcbID <= CPCB_1; cpcbID++) {
        ovl_ctrl.OSdDomain = VPP_OVL_OSD_RGB_DMN; // overlay happen in RGB domain
        //FIXME: BG4CT porting - normal or matte???
        ovl_ctrl.OsdMode = VPP_OVL_OSD_MODE_NORMAL; //  in normal format
        VPP_OVL_SetCtrl(vpp_obj, cpcbID, &ovl_ctrl);
        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE1, VPP_OVL_PLANE_ALPHA_OPAQUE, VPP_PERPIXEL_ALPHA_DISABLE);
        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE2, VPP_OVL_PLANE_ALPHA_OPAQUE, VPP_PERPIXEL_ALPHA_DISABLE);
    }

    /* set z-order of CPCB layers */
    for(cpcbID = CPCB_1; cpcbID < MAX_NUM_CPCBS; cpcbID++) {
        order[0] = order[1] = order[2] = 0;
        for(i = 0; i < vpp_obj->dv[cpcbID].num_of_chans; i++) {
            chanID = vpp_obj->dv[cpcbID].chanID[i];
            if((vpp_obj->chan[chanID].zorder >= 0) && (vpp_obj->chan[chanID].zorder<=6))
                order[vpp_obj->chan[chanID].zorder] = vpp_obj->chan[chanID].dvlayerID;
        }
        VPP_OVL_SetChannelOrder(vpp_obj, cpcbID, order);
    }

    /* reset plane window size */
    for (cpcbID = CPCB_1; cpcbID < MAX_NUM_CPCBS; cpcbID++) {
        for (i = FIRST_CHAN; i < MAX_NUM_CHANS; i++) {
            chan = &(vpp_obj->chan[i]);
            if(chan->dvID == cpcbID) {
                VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
                VPP_CPCB_SetPlaneSourceWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
                VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
                VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 0);
            }
        }
    }

    return (MV_VPP_OK);
}

/******************************************************************
 * FUNCTION: set z-order for a CPCB
 * PARAMS: *vpp_obj - pointer to VPP obj
 *         cpcbID - CPCB ID
 *         ZOrder_1_4 - packed Z order value of  the first four layers
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetZOrder(VPP_OBJ *vpp_obj, int cpcbID, int ZOrder_1_4)
{
    int planeID, chanID, i;
    CHAN *chan;
    PLANE *plane;
    int order[MAX_NUM_CHANS];
    int ZOrder[MAX_NUM_CHANS];

    for (i=0; i < MAX_NUM_CHANS; i++) {
        ZOrder[i] = -1;
        order[i] = 3;
    }

    /*unpack Z order value of  the first six layers*/
    ZOrder[0] = (ZOrder_1_4 & 0xFF);
    ZOrder[1] = (ZOrder_1_4 & 0xFF00) >> 8;
    ZOrder[2] = (ZOrder_1_4 & 0xFF0000) >> 16;

    for (i=0; i<vpp_obj->dv[cpcbID].num_of_chans; i++) {
        chanID = vpp_obj->dv[cpcbID].chanID[i];
        chan = &(vpp_obj->chan[chanID]);
        planeID = PLANE_OF_CHAN(vpp_obj, chanID);
        plane = &(vpp_obj->plane[planeID]);
        if ((!plane->mute) && (ZOrder[chanID]>=CPCB_ZORDER_1) && (ZOrder[chanID]<=CPCB_ZORDER_4))
            order[ZOrder[chanID]] = chan->dvlayerID; // index from 0 //+1
    }

    VPP_OVL_SetChannelOrder(vpp_obj, cpcbID, order);

    return (MV_VPP_OK);
}

/******************************************************************
 * FUNCTION: set CPCB base plane color
 * PARAMS: *vpp_obj - pointer to VPP obj
 *         cpcbID - CPCB ID
 *         bgcolor - background color value(UYV)
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetBasePlaneColor(VPP_OBJ *vpp_obj, int cpcbID, int bgcolor)
{
    /*set base plane fix color*/
    VPP_OVL_SetPlaneColor(vpp_obj, cpcbID, Vpp_OVL_PLANE0, bgcolor);

    return (MV_VPP_OK);
}

/******************************************************************
 * FUNCTION: set CPCB plane blending factor and background color
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          alpha - alpha of CPCB layer
 *          bgcolor - background color of CPCB layer
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetPlaneAttribute(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int alpha, int bgcolor, ENUM_GLOBAL_ALPHA_FLAG globalAlphaFlag)
{
    VPP_OVL_SetPlaneColor(vpp_obj, cpcbID, layerID+1, bgcolor);

    // set plane source global alpha
    if (layerID == CPCB1_PLANE_1) { /* global alpha only available for CPCB plane-1 */
        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE1, alpha, globalAlphaFlag?VPP_OVL_PLANE_ALPHA_FLAG_ENABLE:VPP_OVL_PLANE_ALPHA_FLAG_DISABLE);

    } else if (layerID == CPCB1_PLANE_2) { /* global alpha only available for CPCB plane-2 */
        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE2, alpha, globalAlphaFlag?VPP_OVL_PLANE_ALPHA_FLAG_ENABLE:VPP_OVL_PLANE_ALPHA_FLAG_DISABLE);

    } else {
        //FIXME: BG4CT porting
#ifdef CONFIG_ENABLE_GFX1_PLANE_ALPHA
        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE3, alpha, globalAlphaFlag?VPP_OVL_PLANE_ALPHA_FLAG_ENABLE:VPP_OVL_PLANE_ALPHA_FLAG_DISABLE);
#endif
    }

    // set plane border global alpha to plane alpha &  make it transperent for per pixel alpha
    VPP_OVL_SetBorderAlpha(vpp_obj, cpcbID, layerID+1, globalAlphaFlag?alpha:VPP_OVL_PLANE_ALPHA_TRANSPERENT);

    return (MV_VPP_OK);
}

/******************************************************************
 * FUNCTION: set CPCB plane background window
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          x - x of layer display window
 *          y - y of layer display window
 *          width - width of layer display window
 *          height - height of layer display window
 * RETURN: MV_VPP_OK
 * NOTE: cropping need to be done before set background window.
 ******************************************************************/
int VPP_CPCB_SetPlaneBGWindow(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int x, int y, int width, int height)
{
    VPP_WINDOW bgwin;

    bgwin.TopLeft.X = x;
    bgwin.TopLeft.Y = y;
    bgwin.BottomRight.X = x + width;
    bgwin.BottomRight.Y = y + height;

    switch (cpcbID) {
        case CPCB_1:
            switch (layerID) {
                case CPCB1_PLANE_1: /* CPCB plane 1 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE5_MAIN_BORDER, &bgwin);
                    break;
                case CPCB1_PLANE_2: /* CPCB plane 2 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE6_PIP_BORDER, &bgwin);
                    break;
                case CPCB1_PLANE_3: /* CPCB plane 3 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE7_GFX0_BORDER, &bgwin);
                    break;
                default:
                    break;
            }
            break;
    }

    return (MV_VPP_OK);
}


/*************************************************************************
 * FUNCTION: set CPCB plane source window
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          x - x of layer source window
 *          y - y of layer source window
 *          width - width of layer source window
 *          height - height of layer source window
 * RETURN: MV_VPP_OK
 * NOTE: for CPCB source window, we do chopping in upper layer function
 *       because DataLoader and SCL also need source window chopping
 *       information.
 ************************************************************************/
int VPP_CPCB_SetPlaneSourceWindow(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int x, int y, int width, int height)
{
    VPP_WINDOW window;
    UINT32 uiRegAddr = 0;
    int planeID;

    window.TopLeft.X = x;
    window.TopLeft.Y = y;
    window.BottomRight.X = x + width;
    window.BottomRight.Y = y + height;

    switch (cpcbID) {
        case CPCB_1:
            switch (layerID) {
                case CPCB1_PLANE_1: /* CPCB plane 1 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE1_MAIN, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE1_MAIN_CROP, &window);
                    uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL3;
                    planeID = PLANE_MAIN;
                    break;
                case CPCB1_PLANE_2: /* CPCB plane 2 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE2_PIP, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE2_PIP_CROP, &window);
                    uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL5;
                    planeID = PLANE_PIP;
                    break;
                case CPCB1_PLANE_3: /* CPCB plane 3 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE3_GFX0, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE3_GFX0_CROP, &window);
                    uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL6;
                    planeID = PLANE_GFX1;
                    break;
                default:
                    break;
            }
            break;
    }
    if (uiRegAddr) {
#ifdef VPP_UNDERFLOW_FIX
        if (0 == width || 0 == height) {
            //Program default value
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, 0x64);
            if (vpp_obj->plane[planeID].source_win_valid) {
                VPPAPI_DBG_INFO("Programming pixtot as 0x64 for planeID = %d  - size:%dx%d\n", \
                        layerID, width, height);
                vpp_obj->plane[planeID].source_win_valid = 0;
            }
        } else
#endif
        {
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, width * height);
            vpp_obj->plane[planeID].source_win_valid = 1;
        }
    }

    return (MV_VPP_OK);
}

/*************************************************************************
 * FUNCTION: set CPCB plane alpha polarity
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          src_pol - source alpha polarity
 *          bg_pol - border alpha polarity
 * RETURN: MV_VPP_OK
 ************************************************************************/
int VPP_CPCB_SetPlaneAlphaPolarity(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int src_pol, int bg_pol)
{
    switch (cpcbID) {
        case CPCB_1:
            switch (layerID){
                case CPCB1_PLANE_1: /* CPCB plane 1 */
                    VPP_OVL_SetPlaneAlphaPolarity(vpp_obj, cpcbID, Vpp_OVL_PLANE1, src_pol, bg_pol);
                    break;
                case CPCB1_PLANE_2: /* CPCB plane 2 */
                    VPP_OVL_SetPlaneAlphaPolarity(vpp_obj, cpcbID, Vpp_OVL_PLANE2, src_pol, bg_pol);
                    break;
                case CPCB1_PLANE_3: /* CPCB plane 3 */
                    VPP_OVL_SetPlaneAlphaPolarity(vpp_obj, cpcbID, Vpp_OVL_PLANE3, src_pol, bg_pol);
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
    return (MV_VPP_OK);
}

/*************************************************************************
 * FUNCTION: set CPCB plane crop window
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          x - x of layer crop window
 *          y - y of layer crop window
 *          width - width of layer crop window
 *          height - height of layer crop window
 * RETURN: MV_VPP_OK
 * NOTE: for CPCB source window, we do chopping in upper layer function
 *       because DataLoader and SCL also need source window chopping
 *       information.
 ************************************************************************/
int VPP_CPCB_SetPlaneCropWindow(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int x, int y, int width, int height)
{
    #if DISABLE_CROP_FOR_ALL_PLANE
        return (MV_VPP_EUNSUPPORT);
    #endif
    VPP_WINDOW window;
    window.TopLeft.X = x;
    window.TopLeft.Y = y;
    window.BottomRight.X = x + width;
    window.BottomRight.Y = y + height;

    switch (cpcbID) {
        case CPCB_1:
            switch (layerID) {
                case CPCB1_PLANE_1: /* CPCB plane 1 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE1_MAIN_CROP, &window);
                    break;
                case CPCB1_PLANE_2: /* CPCB plane 2 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE2_PIP_CROP, &window);
                    break;
                case CPCB1_PLANE_3: /* CPCB plane 3 */
                    CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE3_GFX0_CROP, &window);
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set VPP CPCB output resolution
 * PARAMS:   *vpp_obj - pointer to VPP object
 *         cpcbID - CPCB ID
 *         resID - resolution ID
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetOutputResolution(VPP_OBJ *vpp_obj, int cpcbID, int resID, int color_fmt)
{
    RESOLUTION_INFO *resinfo = VPP_GetResInfo(resID);
    RESOLUTION_INFO *prog_resinfo = resinfo;
    RESOLUTION_INFO *intlr_resinfo = resinfo;
    VPP_CPCB_TG_HVTOTAL hvtot;
    VPP_CPCB_TG_PARAMS tg_params;
    VPP_CPCB_DISPTG_HVTOTAL disphvtot;
    VPP_CPCB_DISPTG_PARAMS disptg_params;
    VPP_WINDOW window;
    int clk_ctrl, i;

    /* TODO: set TG clock according to CPCB resolution */
    if ((resID != RES_RESET)&& (cpcbID == VPP_CPCB0)) {
#if !defined(FPGA)
        clk_ctrl = VPP_CPCB_CLK_D1;
#else
#if defined (VPP_ON_VELOCE)
        if (veloce_cpcb_clk_div < 0) {
            if (resinfo->freq >= 593000)
                clk_ctrl = VPP_CPCB_CLK_D1;
            else if (resinfo->freq >= 296000)
                clk_ctrl = VPP_CPCB_CLK_D2;
            else if (resinfo->freq >= 148000) {
                if (resinfo->scan == SCAN_PROGRESSIVE)
                    clk_ctrl = VPP_CPCB_CLK_D4;
                else
                    clk_ctrl = VPP_CPCB_CLK_D8; //to be updated
            } else if (resinfo->freq >= 74000)
                clk_ctrl = VPP_CPCB_CLK_D8;
            else
                clk_ctrl = VPP_CPCB_CLK_D12;
        } else
            clk_ctrl = veloce_cpcb_clk_div;
#else
        if (resinfo->freq >= 593000)
            clk_ctrl = VPP_CPCB_CLK_D1;
        else if (resinfo->freq >= 296000)
            clk_ctrl = VPP_CPCB_CLK_D2;
        else if (resinfo->freq >= 148000) {
            if (resinfo->scan == SCAN_PROGRESSIVE)
                clk_ctrl = VPP_CPCB_CLK_D4;
            else
                clk_ctrl = VPP_CPCB_CLK_D8; //FIXME
        } else
            clk_ctrl = VPP_CPCB_CLK_D8;//FIXME
#endif
#endif
        CPCB_TG_ClkSelect(vpp_obj, cpcbID, clk_ctrl);
    }

    /* set CPCB end display resolution */
    hvtot.HTotal = resinfo->width-1;
    hvtot.Width = resinfo->active_width; //FIXME: bg4ct porting
    if (resinfo->scan == SCAN_INTERLACED) {
        hvtot.VTotal = (resinfo->height >> 1)-1;
        hvtot.Height = (resinfo->active_height >> 1);
    } else {
        hvtot.VTotal = resinfo->height-1;
        hvtot.Height = resinfo->active_height;
    }
    CPCB_TG_SetHVTotal(vpp_obj, cpcbID, resID, &hvtot);

    if (VPP_CPCB0 == cpcbID) {
        //Program VOP TG
        CPCB_TG_SetHVTotal(vpp_obj, VPP_VOP_TG, resID, &hvtot);
    }

    if (color_fmt == OUTPUT_COLOR_FMT_YCBCR420) {   //Divide horizontal values by 2 for 420
        disphvtot.HTotal = (resinfo->width/2)-1;
        disphvtot.Width = resinfo->active_width / 2;
    } else {
        disphvtot.HTotal = (resinfo->width)-1;
        disphvtot.Width = resinfo->active_width; //FIXME: bg4ct porting
    }
    if (resinfo->scan == SCAN_INTERLACED) {
        disphvtot.VTotal = (resinfo->height >> 1)-1;
        disphvtot.Height = (resinfo->active_height >> 1);
    } else {
        disphvtot.VTotal = resinfo->height-1;
        disphvtot.Height = resinfo->active_height;
    }
    CPCB_DISPTG_SetHVTotal(vpp_obj, cpcbID, &disphvtot, color_fmt);

    if (resinfo->scan == SCAN_INTERLACED) {
        CPCB_DISPTG_EnableIntl(vpp_obj, cpcbID, 1);
        tg_params.FrameReset = (intlr_resinfo->active_height >> 1) + VPP_CPCB_FRST_POSITION;
        if (resinfo->type == TYPE_SD) {
            tg_params.HSyncEndPos = intlr_resinfo->active_width + intlr_resinfo->hfrontporch;
            tg_params.HSyncStartPos = tg_params.HSyncEndPos + intlr_resinfo->hsyncwidth;
            if ((resID == RES_VGA_480P5994) || (resID == RES_VGA_480P60)) {
                tg_params.VSyncStartPos = (intlr_resinfo->active_height >> 1) +
                    (intlr_resinfo->vfrontporch >> 1) - 1;
                tg_params.VSyncEndPos = tg_params.VSyncStartPos +
                    (intlr_resinfo->vsyncwidth >> 1);
            } else {
                tg_params.VSyncEndPos = (intlr_resinfo->active_height >> 1) +
                    intlr_resinfo->vfrontporch - 1;
                tg_params.VSyncStartPos = tg_params.VSyncEndPos + intlr_resinfo->vsyncwidth;
            }
            tg_params.Vsampx = tg_params.HSyncEndPos/*0 for HDE/VDE */;
        } else {
            tg_params.HSyncStartPos = intlr_resinfo->active_width + intlr_resinfo->hfrontporch;
            tg_params.HSyncEndPos = tg_params.HSyncStartPos + intlr_resinfo->hsyncwidth;
            if (((resID <= RES_720P50) && (resID >= RES_720P30)) ||
                    ((resID <= RES_1080P2398) && (resID >= RES_1080P30))) {
                tg_params.VSyncStartPos = (intlr_resinfo->active_height >> 1) +
                    (intlr_resinfo->vfrontporch >> 1) - 1;
                tg_params.VSyncEndPos = tg_params.VSyncStartPos + (intlr_resinfo->vsyncwidth >> 1);
            } else {
                tg_params.VSyncStartPos = (intlr_resinfo->active_height >> 1) +
                    intlr_resinfo->vfrontporch - 1;
                tg_params.VSyncEndPos = tg_params.VSyncStartPos + intlr_resinfo->vsyncwidth;
            }
            tg_params.Vsampx = tg_params.HSyncStartPos/*0 for HDE/VDE */;
        }
    } else {
        CPCB_DISPTG_EnableIntl(vpp_obj, cpcbID, 0);
        tg_params.FrameReset = prog_resinfo->active_height + VPP_CPCB_FRST_POSITION;
        if (resinfo->type == TYPE_SD) {
            tg_params.HSyncEndPos = prog_resinfo->active_width + prog_resinfo->hfrontporch;
            tg_params.HSyncStartPos = tg_params.HSyncEndPos + prog_resinfo->hsyncwidth;
            tg_params.VSyncEndPos = prog_resinfo->active_height + prog_resinfo->vfrontporch;
            tg_params.VSyncStartPos = tg_params.VSyncEndPos + prog_resinfo->vsyncwidth;
            tg_params.Vsampx = tg_params.HSyncEndPos/*0 for HDE/VDE */;
        } else {
            tg_params.HSyncStartPos = prog_resinfo->active_width + prog_resinfo->hfrontporch;
            tg_params.HSyncEndPos = tg_params.HSyncStartPos + prog_resinfo->hsyncwidth;
            tg_params.VSyncStartPos = prog_resinfo->active_height + prog_resinfo->vfrontporch/* -1 */;
            tg_params.VSyncEndPos = tg_params.VSyncStartPos + prog_resinfo->vsyncwidth;
            tg_params.Vsampx = tg_params.HSyncStartPos/*0 for HDE/VDE */;
        }
    }

    tg_params.Mode = 0; //3; 0- free running mode 1- sync mode 2- semi sync mode
    tg_params.Lwin = 0xF;
    tg_params.Freeze = 0;
    tg_params.SyncCtrl = 0;

    if (VPP_CPCB0 == cpcbID) {
        //Program VOP TG
        CPCB_TG_SetParams(vpp_obj, cpcbID, resID, &tg_params);
        CPCB_TG_SetParams(vpp_obj, VPP_VOP_TG, resID, &tg_params);
    } else if (VPP_CPCB1 == cpcbID) {
        VOP_TG_SetParams(vpp_obj,hvtot.Width, hvtot.Height);
    }

    if (color_fmt == OUTPUT_COLOR_FMT_YCBCR420) {   //Divide horizontal values by 2 for 420
        tg_params.HSyncStartPos = prog_resinfo->active_width/2 + prog_resinfo->hfrontporch/2;
        tg_params.HSyncEndPos = tg_params.HSyncStartPos + prog_resinfo->hsyncwidth/2;
        tg_params.Vsampx = tg_params.HSyncStartPos;
    }

    disptg_params.FrameReset = tg_params.FrameReset;
    disptg_params.HSyncEndPos = tg_params.HSyncEndPos;
    disptg_params.HSyncStartPos = tg_params.HSyncStartPos;
    disptg_params.VSyncEndPos = tg_params.VSyncEndPos;
    disptg_params.VSyncStartPos = tg_params.VSyncStartPos;
    disptg_params.Vsampx = tg_params.Vsampx;
    disptg_params.Mode = tg_params.Mode;
    disptg_params.Lwin = tg_params.Lwin;
    disptg_params.Freeze = tg_params.Freeze;
    disptg_params.SyncCtrl = tg_params.SyncCtrl;
    if (resinfo->scan == SCAN_INTERLACED) {
        disptg_params.VSStartPos = (intlr_resinfo->width - intlr_resinfo->hbackporch
                 - intlr_resinfo->hsyncwidth) - (intlr_resinfo->width >> 1);
        disptg_params.VSEndPos = (intlr_resinfo->width - intlr_resinfo->hbackporch
                 - intlr_resinfo->hsyncwidth) - (intlr_resinfo->width >> 1);
    } else {
        disptg_params.VSStartPos = disptg_params.Vsampx;
        disptg_params.VSEndPos = disptg_params.Vsampx;
    }
    CPCB_DISPTG_SetParams(vpp_obj, cpcbID, &disptg_params);

    /* set CPCB base plane */
    window.TopLeft.X = 0;
    window.TopLeft.Y = 0;
    window.BottomRight.X = prog_resinfo->active_width;
    if (resinfo->scan == SCAN_INTERLACED)
        window.BottomRight.Y = (prog_resinfo->active_height >> 1);
    else
        window.BottomRight.Y = prog_resinfo->active_height;

    if (VPP_CPCB0 == cpcbID) {
        CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE0_BASE, &window);
        CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE4_BASE_BORDER, &window);
        CPCB_TG_SetPlaneWindow(vpp_obj, cpcbID, TG_PLANE8_OVL, &window);
    }
    if (color_fmt == OUTPUT_COLOR_FMT_YCBCR420) {
        window.BottomRight.X = prog_resinfo->active_width/2;
        CPCB_DISPTG_SetPlaneWindow(vpp_obj, cpcbID, &window);
        window.BottomRight.X = prog_resinfo->active_width;
    } else {
        CPCB_DISPTG_SetPlaneWindow(vpp_obj, cpcbID, &window);
    }

    if (VPP_CPCB0 == cpcbID) {
        for(i = 0; i < vpp_obj->dv[cpcbID].num_of_chans; i++) {
            switch(vpp_obj->dv[cpcbID].chanID[i])
            {
                //Programm VOP TG same as CPCB TG.
                case CHAN_MAIN:
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE0_BASE, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE1_MAIN, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE1_MAIN_CROP, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE5_MAIN_BORDER, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE4_BASE_BORDER, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE8_OVL, &window);
                    break;
                case CHAN_PIP:
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE2_PIP, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE2_PIP_CROP, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE6_PIP_BORDER, &window);
                    break;
                case CHAN_GFX1:
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE3_GFX0, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE3_GFX0_CROP, &window);
                    CPCB_TG_SetPlaneWindow(vpp_obj, VPP_VOP_TG, TG_PLANE7_GFX0_BORDER, &window);
                    break;
                default:
                    break;
            }
        }
#ifdef VPP_UNDERFLOW_FIX
        //Program HDMI VOP PixTOT
        BE_VOP_SetPixTot(vpp_obj, 0, window.BottomRight.X,  window.BottomRight.Y);
#endif
    }
    vpp_obj->uiResID[cpcbID] = resID;
    return (MV_VPP_OK);
}

int VPP_CPCB_EnablePlane(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int enable)
{
    switch (cpcbID) {
        case CPCB_1:
            switch (layerID) {
                case CPCB1_PLANE_1: /* CPCB plane 1 */
                    CPCB_TG_PlaneEnable(vpp_obj, cpcbID, TG_PLANE1_MAIN, enable);
                    break;
                case CPCB1_PLANE_2: /* CPCB plane 2 */
                    CPCB_TG_PlaneEnable(vpp_obj, cpcbID, TG_PLANE2_PIP, enable);
                    break;
                case CPCB1_PLANE_3: /* CPCB plane 3 */
                    CPCB_TG_PlaneEnable(vpp_obj, cpcbID, TG_PLANE3_GFX0, enable);
                    break;
            }
            break;
    }

    return (MV_VPP_OK);
}

VOID VPP_CPCB_ResetPlane(VPP_OBJ *vpp_obj, INT chanID)
{
    CHAN *chan;
    PLANE *plane;
    INT planeID, cpcbID;

    if(MAX_NUM_CHANS != chanID)
    {
        cpcbID = CPCB_OF_CHAN(vpp_obj, chanID);
        chan = &vpp_obj->chan[chanID];
        planeID = PLANE_OF_CHAN(vpp_obj, chanID);
        plane = &vpp_obj->plane[planeID];

        VPPAPI_DBG_INFO("chanID: %d planeID: %d\r\n", chanID, planeID);
        /* if inline frame is not provided, no inline DMA needed */
        /* reset plane->actv_win */
        plane->actv_win.width = 0;
        plane->actv_win.height = 0;

        /* reset plane->crop_actv_win */
        plane->crop_actv_win.width = 0;
        plane->crop_actv_win.height = 0;

        /* reset plane->crop_scl_actv_win */
        plane->crop_scl_actv_win.width = 0;
        plane->crop_scl_actv_win.height = 0;

        /* reset frame_win */
        plane->frame_win.x = 0;
        plane->frame_win.y = 0;
        plane->frame_win.width = 0;
        plane->frame_win.height = 0;

        /* close CPCB video source/background plane if opened previously */
        VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        VPP_CPCB_SetPlaneSourceWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
#ifndef VPP_UNDERFLOW_FIX
        VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 0);
#endif
    }
}
