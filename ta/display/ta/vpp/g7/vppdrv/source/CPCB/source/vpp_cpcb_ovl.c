/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define VPP_CPCB_OVL_C

#include "vpp_regs.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_cpcb_ovl.h"
#include "vpp.h"
#include "vpp_dv_api.h"

/* Granularity of pixels which can be fetched by client from DRAM (start address per stride) */
static int VIDEO_DATA_X_ALIGN_BYTES[] = {
    16, //00:VPP_UNIQUE_FORMAT_ID_8BIT_420_SP
    64, //01:VPP_UNIQUE_FORMAT_ID_10BIT_420_SP
    8,  //04:VPP_UNIQUE_FORMAT_ID_16BIT_422_YC
    4,  //11:VPP_UNIQUE_FORMAT_ID_32BIT_ARGB
#ifdef TILED_CROPPING_ORG_METHOD
    //Do not use this - since we need to consider dummy byte in case of mode0-10bit
    //which may be make the cropping logic to find start_x address for Y & UV buffer more complex/difficult
    32, //12:VPP_OVL_BYPASS_DATA_FMTER_8BIT_420_SP_TILED_MODE0 - 8x4
    32, //13:VPP_OVL_BYPASS_DATA_FMTER_8BIT_420_SP_TILED_MODE1 - 8x4
    24, //13:VPP_OVL_BYPASS_DATA_FMTER_10BIT_420_SP_TILED_MODE0 - 6x4
    16, //14:VPP_OVL_BYPASS_DATA_FMTER_10BIT_420_SP_TILED_MODE1 - 4x4
#else
    //8x4=32, 6*4=24, 4*4=16 ===> common factor (32,24,16) = 96 i.e 32*3=24*4=16*6
    //Thus logic can be simplified by having the cropping done at alignemnt of 96 pixel
    96,
    96,
    96,
    96,
#endif
    32, //VPP_UNIQUE_FORMAT_ID_20BIT_422_YC
    32, //VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8
    8,  //VPP_UNIQUE_FORMAT_ID_16BIT_RGB565
    4,  //VPP_UNIQUE_FORMAT_ID_16BIT_420_SP,
    12, //VPP_UNIQUE_FORMAT_ID_10BIT_DWA_420_SP,
    12, //VPP_UNIQUE_FORMAT_ID_10BIT_DWA_422_SP
    4,  //VPP_UNIQUE_FORMAT_ID_10BIT_DWA_444,
    8,  //VPP_UNIQUE_FORMAT_ID_16BIT_ARGB4444
    8,  //VPP_UNIQUE_FORMAT_ID_16BIT_ARGB1555
    8,   //VPP_UNIQUE_FORMAT_ID_16BIT_ARGB8332
    4,  //VPP_UNIQUE_FORMAT_ID_24BIT_YUV444
    4,  //VPP_UNIQUE_FORMAT_ID_30BIT_YUV444
    4,  //VPP_UNIQUE_FORMAT_ID_RGB888
};

/*Granularity of pixels in terms of number of rows */
static int VIDEO_DATA_Y_ALIGN_BYTES[] = {
    2, //VPP_UNIQUE_FORMAT_ID_8BIT_420_SP
    2, //VPP_UNIQUE_FORMAT_ID_10BIT_420_SP
    1, //VPP_UNIQUE_FORMAT_ID_16BIT_422_YC
    1, //VPP_UNIQUE_FORMAT_ID_32BIT_ARGB
    //We need to have alignment at 8 since croma is shared between tiles vertically
    8, //12:VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE0
    8, //13:VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE1
    8, //13:VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0
    8, //14:VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE1
    1, //VPP_UNIQUE_FORMAT_ID_20BIT_422_YC
    8, //VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8
    2, //VPP_UNIQUE_FORMAT_ID_16BIT_RGB565
    1,  //VPP_UNIQUE_FORMAT_ID_16BIT_420_SP,
    2,  //VPP_UNIQUE_FORMAT_ID_10BIT_DWA_420_SP,
    1,  //VPP_UNIQUE_FORMAT_ID_10BIT_DWA_422_SP
    1,  //VPP_UNIQUE_FORMAT_ID_10BIT_DWA_444,
    2,  //VPP_UNIQUE_FORMAT_ID_16BIT_ARGB4444 /* should match with VPP_UNIQUE_FORMAT_ID */
    2,  //VPP_UNIQUE_FORMAT_ID_16BIT_ARGB1555
    2,   //VPP_UNIQUE_FORMAT_ID_16BIT_ARGB8332
    1,  //VPP_UNIQUE_FORMAT_ID_24BIT_YUV444
    1,  //VPP_UNIQUE_FORMAT_ID_30BIT_YUV444
    1   //VPP_UNIQUE_FORMAT_ID_RGB888
};

/***************************************************************************************
 * FUNCTION: load the default register values of CPCB OVL block
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL default values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_LoadDefaultVal(VPP_OBJ *vpp_obj, INT OvlUnit)
{
    UINT32 RegAddr;

    CHECK_OVL_NUM(OvlUnit);

    if (VPP_CPCB_OVL0 == OvlUnit) {
        //FIXME: BG4CT porting
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALPHA_CTRL;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->ovl_alpha_ctrl.u32);
        vpp_obj->ovl_alpha_ctrl.uALPHA_CTRL_pl4_alpctrl0 = 1; //perpixel alpha enable for gfx1
        VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->ovl_alpha_ctrl.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_APM;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->ovl_apm.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALP_UD;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gOvlAlpUd.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALP_UD1;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gOvlAlpUd1.u32);
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_SENSE_CTRL;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gOvlSenseCtrl.u32);
        vpp_obj->cpcb.Cpcb0SrcAlphaPol = vpp_obj->gOvlSenseCtrl.uSENSE_CTRL_sense_ctrl0;
        vpp_obj->cpcb.Cpcb0BgAlphaPol = vpp_obj->gOvlSenseCtrl.uSENSE_CTRL_sense_ctrl1;
    }

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: set the OSD Overlay mode and domain
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               pZOrder - *pZOrder for layer1 channel selection, *(pZOrder+1) for layer2 channel selection.....
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetChannelOrder(VPP_OBJ *vpp_obj, INT OvlUnit, INT *pZOrder) //DONE
{
    UINT32 RegAddr;
    T32OVL_LAY stOvlLay;
    INT RetVal = MV_VPP_OK;

    CHECK_OVL_NUM(OvlUnit);

    if (VPP_CPCB_OVL0 == OvlUnit)
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_LAY;
    else
        return (MV_VPP_EBADPARAM);

    stOvlLay.u32 = 0;
    stOvlLay.uLAY_lay12 =  (*(pZOrder + 0) & 0xf) | ((*(pZOrder + 1) & 0xf) << 4);
    stOvlLay.uLAY_lay34 =  (*(pZOrder + 2) & 0xf);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr,  stOvlLay.u32);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set the OSD Overlay mode and domain
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               OsdCtrl - structure of mode and domain
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetCtrl(VPP_OBJ *vpp_obj, INT OvlUnit, PVPP_OVL_OSD_CTRL pOsdCtrl) //DONE
{
    UINT32 RegAddr;
    UINT32 RegVal;
    T32OVL_CTRL stOvlCtrl;
    INT RetVal = MV_VPP_OK;

    CHECK_OVL_NUM(OvlUnit);

    if (VPP_CPCB_OVL0 == OvlUnit)
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_CTRL;
    else
        return (MV_VPP_EBADPARAM);

    stOvlCtrl.u32 = 0;
    RegVal = (pOsdCtrl->OsdMode & 0x1) | ((pOsdCtrl->OSdDomain&0x01) << 1);
    stOvlCtrl.uCTRL_ctrl = RegVal;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stOvlCtrl.u32);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set the OSD Overlay alpha
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               plane - plane1 or plane2
 *               PlaneAlpha - alpha value
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetPlaneAlpha(VPP_OBJ *vpp_obj, INT OvlUnit, INT plane, UINT32 PlaneAlpha, VPP_PERPIXEL_ALPHA_STATUS enablePerPixelAlpha) //DONE
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;

    CHECK_OVL_NUM(OvlUnit);

    /*get the address of the first register of OSD overlay*/
    if (VPP_CPCB_OVL0 != OvlUnit)
        return (MV_VPP_EBADPARAM);

    if (Vpp_OVL_PLANE1 ==  plane) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALPHA0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, PlaneAlpha);
    } else if (Vpp_OVL_PLANE2 ==  plane) {
        //FIXME: currently hardcoded to plane alpha for 422/420. Need to change config to per pixel alpha for ARGB
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALPHA_CTRL;
        vpp_obj->ovl_alpha_ctrl.uALPHA_CTRL_pl3_alpctrl0 = enablePerPixelAlpha;
        vpp_obj->ovl_alpha_ctrl.uALPHA_CTRL_pl3_alpctrl1 = PlaneAlpha;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->ovl_alpha_ctrl.u32);
        //FIXME: VPP_CT changes - doubts
        // pl3_alpctrl0
        // * 01: Alpha value for pip/gfx0 selected from most significant byte of data input
        // * 10: Alpha value for pip/gfx0 selected from 2nd byte form least significant byte of data input
    } else if (Vpp_OVL_PLANE3 ==  plane) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALPHA_CTRL;
        vpp_obj->ovl_alpha_ctrl.uALPHA_CTRL_pl4_alpctrl0 = enablePerPixelAlpha;
        vpp_obj->ovl_alpha_ctrl.uALPHA_CTRL_pl4_alpctrl1 = PlaneAlpha;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->ovl_alpha_ctrl.u32);
    } else
        return MV_VPP_EBADPARAM;

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set the fixed color associated with each backend plane
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               plane - plane number
 *               Color - plane fix color value
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetPlaneColor(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, UINT32 Color) //DONE
{
    UINT32 RegAddr, RegAddr1;

    CHECK_OVL_NUM(OvlUnit);

    switch (Plane) {
        case Vpp_OVL_PLANE0:
            if (VPP_CPCB_OVL0 == OvlUnit) {
                RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_FIX;
                RegAddr1 = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_FIX0;
            } else
                return (MV_VPP_EBADPARAM);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr,  Color);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr1,  Color);
            break;

        case Vpp_OVL_PLANE1:
            if (VPP_CPCB_OVL0 == OvlUnit)
                RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_FIX1;
            else
                return (MV_VPP_EBADPARAM);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr,  Color);
            break;

        case Vpp_OVL_PLANE2:
            if (VPP_CPCB_OVL0 == OvlUnit)
                RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_FIX2;
            else
                return (MV_VPP_EBADPARAM);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr,  Color);
            break;

        case Vpp_OVL_PLANE3:
            if (VPP_CPCB_OVL0 == OvlUnit)
                RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_FIX3;
            else
                return (MV_VPP_EBADPARAM);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr,  Color);
            break;

        default:
            return MV_VPP_EBADPARAM;
    }

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION: set the polarity of alpha associated with each backend plane
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               plane - plane number
 *               src_pol - source plane alpha polarity, 0: normal, 1: inverse
 *               bg_pol - border plane alpha polarity, 0: normal, 1: inverse
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetPlaneAlphaPolarity(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, UINT32 src_pol, UINT32 bg_pol)
{
    //FIXME: VPP_CT changes - doubts
    UINT32 RegAddr;
    UINT32 *pSrcAlphaPol, *pBgAlphaPol;

    if (VPP_CPCB_OVL0 == OvlUnit) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_SENSE_CTRL;
        pSrcAlphaPol = &vpp_obj->cpcb.Cpcb0SrcAlphaPol;
        pBgAlphaPol = &vpp_obj->cpcb.Cpcb0BgAlphaPol;
    } else
        return (MV_VPP_EBADPARAM);

    switch (Plane) {
        case Vpp_OVL_PLANE1:
            ((VPP_OVL_AL_POL *)pSrcAlphaPol)->field.p1 = src_pol;
            ((VPP_OVL_AL_POL *)pBgAlphaPol)->field.p1 = bg_pol;
            break;
        case Vpp_OVL_PLANE2:
            ((VPP_OVL_AL_POL *)pSrcAlphaPol)->field.p2 = src_pol;
            ((VPP_OVL_AL_POL *)pBgAlphaPol)->field.p2 = bg_pol;
            break;
        case Vpp_OVL_PLANE3:
            ((VPP_OVL_AL_POL *)pSrcAlphaPol)->field.p3 = src_pol;
            ((VPP_OVL_AL_POL *)pBgAlphaPol)->field.p3 = bg_pol;
            break;
        default:
            return (MV_VPP_EBADPARAM);
    }

    vpp_obj->gOvlSenseCtrl.uSENSE_CTRL_sense_ctrl0 = *pSrcAlphaPol;
    vpp_obj->gOvlSenseCtrl.uSENSE_CTRL_sense_ctrl1 = *pBgAlphaPol;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gOvlSenseCtrl.u32);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION: set plane's border alpha  * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *         plane - plane number
 *         alpha - alpha value
 * RETURN:  1 - succeed
 *          others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetBorderAlpha(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, INT Alpha)
{
    UINT32 RegAddr;

    if (OvlUnit != VPP_CPCB_OVL0)
        return (MV_VPP_EBADPARAM);

    switch (Plane) {
        case Vpp_OVL_PLANE1:
            if (VPP_CPCB_OVL0 == OvlUnit)
                vpp_obj->gOvlAlpUd.uALP_UD_pl5_al_ud = Alpha;
            break;

        case Vpp_OVL_PLANE2:
            if (VPP_CPCB_OVL0 == OvlUnit)
                vpp_obj->gOvlAlpUd.uALP_UD_pl6_al_ud = Alpha;
            break;

        case Vpp_OVL_PLANE3:
            if (VPP_CPCB_OVL0 == OvlUnit)
                vpp_obj->gOvlAlpUd1.uALP_UD_pl7_al_ud = Alpha;
            break;

        default:
            return (MV_VPP_EBADPARAM);
    }

    switch (Plane) {
        case Vpp_OVL_PLANE1:
        case Vpp_OVL_PLANE2:
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALP_UD;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gOvlAlpUd.u32);
            break;
        case Vpp_OVL_PLANE3:
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_ALP_UD1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gOvlAlpUd1.u32);
            break;
    }

    return (MV_VPP_OK);
}

INT VPP_OVL_EnablePreMultipliedAlpha(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, INT Enable)
{
    UINT32 RegAddr;
    UINT8 ovlfmt = 0;

    if (OvlUnit != VPP_CPCB_OVL0)
        return (MV_VPP_EBADPARAM);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_APM;

    switch (Plane) {
        case Vpp_OVL_PLANE1:
            if (VPP_CPCB_OVL0 == OvlUnit)
                vpp_obj->ovl_apm.uAPM_plane15_en = Enable;
            break;

        case Vpp_OVL_PLANE2:
            if (VPP_CPCB_OVL0 == OvlUnit)
                vpp_obj->ovl_apm.uAPM_plane26_en = Enable;
            break;

        case Vpp_OVL_PLANE3:
            if (VPP_CPCB_OVL0 == OvlUnit)
                vpp_obj->ovl_apm.uAPM_plane37_en = Enable;
            break;

        default:
            return (MV_VPP_EBADPARAM);
    }

    ovlfmt = VPP_DV_GetCapability(vpp_obj, DV_CAP_STATUS); //ARGB32 PM un-supported for Dolby

    if (!ovlfmt)
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->ovl_apm.u32);

    return (MV_VPP_OK);
}

INT VPP_OVL_SetARGB32PreAlpha(VPP_OBJ *vpp_obj, UINT32 planeEn)
{
    UINT32 RegAddr;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_OVL1 + RA_OVL_APM;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, planeEn);

    return (MV_VPP_OK);
}
/***************************************************************************************
 * FUNCTION: Get bits per pixel with given inputs.
 * PARAMS:  iBitsPerPixel   - Number of bits per pixel
 *          iSrcFmt         - Format of the input buffer
 *          iSrcOrder       - Order of the input data
 *
 * RETURN:  xx - Total number of bits per pixel (succeed)
 *          0 - Bad paramters.
 ***************************************************************************************/
INT VPP_OVL_GetBpp(VPP_OBJ *vpp_obj, INT32 planeID, INT32 iBitsPerPixel, INT32 iSrcFmt)
{
    PLANE *plane;
    plane = &vpp_obj->plane[planeID];
    UINT8 uchDataFmt = 0;   //Total number of bits per pixel

    //check if total bits per pixel is sent instead of bit depth
    //TBD: Remove this step when VDEC give bit depth in m_bits_per_pixel
    if (iBitsPerPixel > 12) {
        //Convert total bits per pixel to bit depth
        int mult_fact = 1000;
        int bit_frac = mult_fact;
        switch (iSrcFmt) {
            case SRCFMT_YUV420SP:
            case SRCFMT_YUV420SP_TILED_MODE0:
            case SRCFMT_YUV420SP_TILED_MODE1:
#ifdef CONFIG_AMP_VPP_ROTATION
            case SRCFMT_YUV420SP_TILED_V4H8:
#endif
                bit_frac = mult_fact*iBitsPerPixel/8;
                break;
            case SRCFMT_ARGB32:
            case SRCFMT_ARGB32_PM:
            case SRCFMT_XRGB32:
            case SRCFMT_IPT444:
            case SRCFMT_YUV444:
                bit_frac = mult_fact*iBitsPerPixel/32;
                break;
            case SRCFMT_YUV420SP_DWA:
            case SRCFMT_YUV422SP_DWA:
            case SRCFMT_YUV444_DWA:
                bit_frac = 1250;
                break;
            case SRCFMT_RGB888:
                 bit_frac = mult_fact*iBitsPerPixel/24;
                break;
            case SRCFMT_RGB565:
            case SRCFMT_YUV422:
            default:
                //Assume default to 422
                bit_frac = mult_fact*iBitsPerPixel/16;
                break;
        }
        if (bit_frac==1000) //1*mult_fact
            iBitsPerPixel = 8;
        else if (bit_frac==1250) //1*mult_fact
            iBitsPerPixel = 10;
        else {
            uchDataFmt = 16;
            iBitsPerPixel = 16; //420SP 16-bit
        }

    }
    if (8 == iBitsPerPixel) {
        switch (iSrcFmt) {
            case SRCFMT_YUV420SP:
                uchDataFmt = 8;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP;
                break;
            case SRCFMT_YUV422:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_422_YC;
                break;
            case SRCFMT_ARGB32:
            case SRCFMT_ARGB32_PM:
            case SRCFMT_XRGB32:
            case SRCFMT_IPT444:
                uchDataFmt = 32;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_32BIT_ARGB;
                break;
            case SRCFMT_RGB888:
                uchDataFmt = 24;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_RGB888;
                break;
            case SRCFMT_RGB565:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_RGB565;
                break;
            case SRCFMT_YUV444:
                uchDataFmt = 24;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_24BIT_YUV444;
                break;
            case SRCFMT_YUV420SP_TILED_MODE0:
                uchDataFmt = 8;
                plane->tile.width = 8;
                plane->tile.height = 4;
                plane->tile.bytesPerTile = plane->tile.width*plane->tile.height;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE0;
                break;
            case SRCFMT_YUV420SP_TILED_MODE1:
                uchDataFmt = 8;
                plane->tile.width = 4;
                plane->tile.height = 4;
                plane->tile.bytesPerTile = plane->tile.width*plane->tile.height;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE1;
                break;
#ifdef CONFIG_AMP_VPP_ROTATION
            case SRCFMT_YUV420SP_TILED_V4H8:
                uchDataFmt = 8;
                plane->tile.width = 8;
                plane->tile.height = 4;
                plane->tile.bytesPerTile = plane->tile.width*plane->tile.height;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8;
                break;
#endif

            default:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_422_YC;
                break;
        }
    } else if (10 == iBitsPerPixel) {
        switch (iSrcFmt) {
            case SRCFMT_YUV420SP:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_420_SP;
                break;
            case SRCFMT_YUV422:
                uchDataFmt = 20;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_20BIT_422_YC;
                break;
            case SRCFMT_YUV444:
                uchDataFmt = 30;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_30BIT_YUV444;
                break;
            case SRCFMT_RGB888:
                uchDataFmt = 30;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_RGB888;
                break;
            case SRCFMT_YUV420SP_TILED_MODE0:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0;
                break;
            case SRCFMT_YUV420SP_TILED_MODE1:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE1;
                break;
            case SRCFMT_YUV420SP_DWA:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_DWA_420_SP;
                break;
            case SRCFMT_YUV422SP_DWA:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_DWA_422_SP;
                break;
            case SRCFMT_YUV444_DWA:
                uchDataFmt = 32;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_DWA_444;
                break;
            default:
                uchDataFmt = 20;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_20BIT_422_YC;
                break;
        }
    } else if(12 == iBitsPerPixel) {
       switch(iSrcFmt) {
            case SRCFMT_YUV420SP:
                uchDataFmt = 24;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_20BIT_422_YC;
                break;
            default:
                uchDataFmt = 24;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_20BIT_422_YC;
                break;
        }
    } else {
        VDB_print("iBitsPerPixel is wrong : %d \n",iBitsPerPixel);
    }

    VDB_print("plane(%d)->uchDataFmt,uchClientMode : %d, %d ;  alignment(x,y) : (%d,%d)\n",planeID, uchDataFmt, plane->uchClientMode,
            VPP_OVL_getDataAlignBytes_X(plane->uchClientMode, 0), VPP_OVL_getDataAlignBytes_Y(plane->uchClientMode, 0));
    return uchDataFmt;
}

INT VPP_OVL_getDataAlignBytes_X(INT clientMode, INT maxBufferWidth) {
    INT dataAlign_x = VIDEO_DATA_X_ALIGN_BYTES[clientMode];
#ifdef ENABLE_FE_FULLCROP
    switch (clientMode) {
        //For tiled mode let us read complete stride/buffer and crop unwanted data at FETG
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP:
        case VPP_UNIQUE_FORMAT_ID_10BIT_420_SP:
        case VPP_UNIQUE_FORMAT_ID_16BIT_420_SP:
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE0:
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE1:
        case VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0:
        case VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE1:
#ifdef CONFIG_AMP_VPP_ROTATION
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8:
#endif
            dataAlign_x = maxBufferWidth;
            break;
    }
#endif //ENABLE_FE_FULLCROP
    return dataAlign_x;
}
INT VPP_OVL_getDataAlignBytes_Y(INT clientMode, INT maxBufferHeight) {
    INT dataAlign_y = VIDEO_DATA_Y_ALIGN_BYTES[clientMode];
    return dataAlign_y;
}

INT VPP_OVL_getTileDetails(INT clientMode, INT resWidth, INT resHeight, PVPP_TILE pVppTile) {
    /*Number of extra bytes per Stride*/
    pVppTile->tileStrideFrac = 0;

    switch (clientMode) {
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE0:
            pVppTile->width = 8;
            pVppTile->height = 4;
            pVppTile->bytesPerTile = pVppTile->width*pVppTile->height;
            break;
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE1:
            pVppTile->width = 4;
            pVppTile->height = 4;
            pVppTile->bytesPerTile = pVppTile->width*pVppTile->height;
            break;
        case VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0:
            pVppTile->width = 6;
            pVppTile->height = 4;
            //15th and 31st bytes are not used & dropped by depVppTile block
            pVppTile->bytesPerTile = (pVppTile->width*pVppTile->height+2)*1.25;
            //number of tiles per stride * 2 bytes per tile
            pVppTile->tileStrideFrac = (resWidth / (pVppTile->width*pVppTile->height)) * 2;
            break;
        case VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE1:
            pVppTile->width = 4;
            pVppTile->height = 4;
            pVppTile->bytesPerTile = pVppTile->width*pVppTile->height*1.25;
            break;
#ifdef CONFIG_AMP_VPP_ROTATION
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8:
            pVppTile->width = 8;
            pVppTile->height = 4;
            pVppTile->bytesPerTile = pVppTile->width*pVppTile->height;
            break;
#endif

        default:
            pVppTile->width = 1;
            pVppTile->height = 1;
            pVppTile->bytesPerTile = 1;
            break;
    }

    pVppTile->nTileX = (resWidth + (pVppTile->width-1))/pVppTile->width; /*Number of pVppTiles in X direction*/
    pVppTile->nTileY = (resHeight + (pVppTile->height-1))/pVppTile->height; /*Number of pVppTiles in Y direction*/

    return (MV_VPP_OK);
}
