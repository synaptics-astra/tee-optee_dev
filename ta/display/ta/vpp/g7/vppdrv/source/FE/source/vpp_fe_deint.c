/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_FE_DEINT_C_

#include "com_type.h"
#include "vpp_com.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_regs.h"
#include "vpp_bcmbuf.h"

#include "vpp.h"
#ifndef CONFIG_AMP_TEE_ENABLE
#include <math.h>
#endif
#include "vpp_fe_deint.h"
#include "vpp_fe_deintprv.h"
#include "vpp_fe_deintcfg.h"
/*-----------------------------------------------------------------------------
 * Description  : This function sets the default configuration values to the
 *              : hardware registers
 * Arguments    : None
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetDefaults(VPP_OBJ* vpp_obj)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

#define REFERENCE_CODE 0
#if REFERENCE_CODE //Kept for reference
    T32DINT_CFG2 dint_cfg2;
    T32DINT_CFG3 dint_cfg3;
    T32DINT_CFG4 dint_cfg4;
//    T32DINT_CFG5 dint_cfg5;
    T32DINT_CFG6 dint_cfg6;
    T32DINT_CFG7 dint_cfg7;
    T32DINT_CFG8 dint_cfg8;
    T32DINT_CFG9 dint_cfg9;
    T32DINT_CFG10 dint_cfg10;
    T32DINT_CFG11 dint_cfg11;
    T32DINT_CFG12 dint_cfg12;
    T32DINT_CFG13 dint_cfg13;
    T32DINT_CFG14 dint_cfg14;
    T32DINT_CFG15 dint_cfg15;
    T32DINT_CFG16 dint_cfg16;
    T32DINT_CFG17 dint_cfg17;
    T32DINT_CFG18 dint_cfg18;
    T32DINT_CFG22 dint_cfg22;
    T32DINT_CFG23 dint_cfg23;
    T32DINT_CFG24 dint_cfg24;
    T32DINT_CFG26 dint_cfg26;
    T32DINT_CFG27 dint_cfg27;
    T32DINT_CFG28 dint_cfg28;
    T32DINT_CFG30 dint_cfg30;
    T32DINT_CFG31 dint_cfg31;
    T32DINT_CFG32 dint_cfg32;
    T32DINT_CFG33 dint_cfg33;
#endif

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
#if REFERENCE_CODE //Kept for reference
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG8, &dint_cfg8.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG10, &dint_cfg10.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG11, &dint_cfg11.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG22, &dint_cfg22.u32);
#endif

    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG0, &vpp_obj->dint_cfg0.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG1, &vpp_obj->dint_cfg1.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG19, &vpp_obj->dint_cfg19.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG20, &vpp_obj->dint_cfg20.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG21, &vpp_obj->dint_cfg21.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG25, &vpp_obj->dint_cfg25.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG29, &vpp_obj->dint_cfg29.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG40, &vpp_obj->dint_cfg40.u32);

    /* load default value for VP */
    vpp_obj->dint_cfg40.uCFG40_DEINT_MODE = gVpDefValues.REG_VP_DEINT_MODE;
    vpp_obj->dint_cfg40.uCFG40_FB_CTRL = gVpDefValues.REG_VP_FB_CTRL;

    vpp_obj->fe.VpDeintMode = gVpDefValues.REG_VP_DEINT_MODE;
    vpp_obj->fe.VpCtrlFifo = gVpDefValues.REG_VP_CTRL;
    vpp_obj->fe.VpFbCtrl = gVpDefValues.REG_VP_FB_CTRL;

    /* load default value for De-Interlacer */
    vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL1    = gDeintDefValues.REG_DIN0;
    vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL2    = gDeintDefValues.REG_DIN1;
    vpp_obj->dint_cfg1.uCFG1_DIN_VI_CTRL        = gDeintDefValues.REG_DIN2;
    vpp_obj->dint_cfg1.uCFG1_DIN_VI_HPF_UTH    = gDeintDefValues.REG_DIN3;
    vpp_obj->dint_cfg1.uCFG1_DIN_VI_HPF_LTH    = gDeintDefValues.REG_DIN4;
    vpp_obj->dint_cfg1.uCFG1_DIN_VI_CUED_TH    = gDeintDefValues.REG_DIN5;
#if REFERENCE_CODE //Kept for reference
    dint_cfg2.uCFG2_DIN_EXC_B                = gDeintDefValues.REG_DIN8;
    dint_cfg2.uCFG2_DIN_EXC_T                = gDeintDefValues.REG_DIN9;
    dint_cfg2.uCFG2_DIN_EXC_R                = gDeintDefValues.REG_DIN10;
    dint_cfg2.uCFG2_DIN_EXC_L                = gDeintDefValues.REG_DIN11;
    dint_cfg3.uCFG3_DIN_MCB_CTRL1             = gDeintDefValues.REG_DIN12;
    dint_cfg3.uCFG3_DIN_MCB_CTRL2             = gDeintDefValues.REG_DIN13;
    dint_cfg3.uCFG3_DIN_MCB_CTRL3             = gDeintDefValues.REG_DIN14;
    dint_cfg4.uCFG4_DIN_ME_CTRL1              = gDeintDefValues.REG_DIN15;
    dint_cfg4.uCFG4_DIN_ME_CTRL2              = gDeintDefValues.REG_DIN16;
#endif
    vpp_obj->dint_cfg5.uCFG5_DIN_MAD_VID_THL  = gDeintDefValues.REG_DIN17;
    vpp_obj->dint_cfg5.uCFG5_DIN_MAD_VID_THH  = gDeintDefValues.REG_DIN18;
    vpp_obj->dint_cfg5.uCFG5_DIN_MAD_FLM_TH   = gDeintDefValues.REG_DIN19;
    vpp_obj->dint_cfg5.uCFG5_DIN_MAD2_VID_THL = gDeintDefValues.REG_DIN20;
#if REFERENCE_CODE //Kept for reference
    dint_cfg6.uCFG6_DIN_MAD2_VID_THH          = gDeintDefValues.REG_DIN21;
    dint_cfg6.uCFG6_DIN_MAD2_FLM_TH           = gDeintDefValues.REG_DIN22;
    dint_cfg6.uCFG6_DIN_MAX_VID_THL           = gDeintDefValues.REG_DIN23;
    dint_cfg6.uCFG6_DIN_MAX_VID_THH           = gDeintDefValues.REG_DIN24;
    dint_cfg7.uCFG7_DIN_MAX_STH1              = gDeintDefValues.REG_DIN25;
    dint_cfg7.uCFG7_DIN_MAX_STH2              = gDeintDefValues.REG_DIN26;
    dint_cfg7.uCFG7_DIN_FLD_THL               = gDeintDefValues.REG_DIN27;
    dint_cfg7.uCFG7_DIN_FLD_THH               = gDeintDefValues.REG_DIN28;
    dint_cfg9.uCFG9_DIN_WVE_TH                = gDeintDefValues.REG_DIN29;
    dint_cfg9.uCFG9_DIN_WVE1_GAIN             = gDeintDefValues.REG_DIN30;
    dint_cfg9.uCFG9_DIN_WVE2_GAIN             = gDeintDefValues.REG_DIN31;
    dint_cfg9.uCFG9_DIN_WVE3_TH1              = gDeintDefValues.REG_DIN32;
    dint_cfg10.uCFG10_DIN_WVE3_TH2            = gDeintDefValues.REG_DIN33;
    dint_cfg10.uCFG10_DIN_WVE3_GAIN           = gDeintDefValues.REG_DIN34;
    dint_cfg11.uCFG11_DIN_WVE_PE_TH1          = gDeintDefValues.REG_DIN37;
    dint_cfg11.uCFG11_DIN_WVE_PE_TH2          = gDeintDefValues.REG_DIN38;
    dint_cfg12.uCFG12_DIN_PE_MASK             = gDeintDefValues.REG_DIN39;
    dint_cfg12.uCFG12_DIN_CHR_THL             = gDeintDefValues.REG_DIN40;
    dint_cfg12.uCFG12_DIN_CHR_THH             = gDeintDefValues.REG_DIN41;
    dint_cfg13.uCFG13_DIN_NC_REG0_THRESH      = (gDeintDefValues.REG_DIN42)|(gDeintDefValues.REG_DIN43<<8)|(gDeintDefValues.REG_DIN44<<16);
    dint_cfg14.uCFG14_DIN_NC_REG1_THRESH      = (gDeintDefValues.REG_DIN46)|(gDeintDefValues.REG_DIN47<<8)|(gDeintDefValues.REG_DIN48<<16);;
    dint_cfg15.uCFG15_DIN_NC_REG2_THRESH      = (gDeintDefValues.REG_DIN50)|(gDeintDefValues.REG_DIN51<<8)|(gDeintDefValues.REG_DIN52<<16);
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH0      = gDeintDefValues.REG_DIN53;
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH1      = gDeintDefValues.REG_DIN54;
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH2      = gDeintDefValues.REG_DIN55;
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH3      = gDeintDefValues.REG_DIN56;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH0      = gDeintDefValues.REG_DIN57;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH1      = gDeintDefValues.REG_DIN58;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH2      = gDeintDefValues.REG_DIN59;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH3      = gDeintDefValues.REG_DIN60;
    dint_cfg18.uCFG18_DIN_PF_MADTH            = gDeintDefValues.REG_DIN61;
    dint_cfg18.uCFG18_DIN_PEXIT_TH            = (gDeintDefValues.REG_DIN62)|(gDeintDefValues.REG_DIN63<<8)|(gDeintDefValues.REG_DIN64<<16);
#endif
    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_CTRL            = gDeintDefValues.REG_DIN65;
    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_EN              = (gDeintDefValues.REG_DIN66)|(gDeintDefValues.REG_DIN67<<8);
    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_PFMODE          = gDeintDefValues.REG_DIN68;
    vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32CTRL0         = gDeintDefValues.REG_DIN72;
    vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32CTRL1         = gDeintDefValues.REG_DIN73;
    vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32NOF0          = gDeintDefValues.REG_DIN74;
    vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32NOF1          = gDeintDefValues.REG_DIN75;
    vpp_obj->dint_cfg21.uCFG21_DIN_FCH_22CTRL          = gDeintDefValues.REG_DIN76;
    vpp_obj->dint_cfg21.uCFG21_DIN_FCH_22NOF0          = gDeintDefValues.REG_DIN77;
    vpp_obj->dint_cfg21.uCFG21_DIN_FCH_22NOF1          = gDeintDefValues.REG_DIN78;
    vpp_obj->dint_cfg21.uCFG21_DIN_FCH_SCNOF0          = gDeintDefValues.REG_DIN79;
#if REFERENCE_CODE //Kept for reference
    dint_cfg22.uCFG22_DIN_FCH_SCNOF1          = gDeintDefValues.REG_DIN80;
    dint_cfg22.uCFG22_DIN_FCH_SCCTRL0         = gDeintDefValues.REG_DIN81;
    dint_cfg22.uCFG22_DIN_FCH_SCCTRL1         = gDeintDefValues.REG_DIN82;
    dint_cfg23.uCFG23_DIN_FCH_SC_PAT0         = (gDeintDefValues.REG_DIN84)|(gDeintDefValues.REG_DIN85<<8)|(gDeintDefValues.REG_DIN86<<16)|(gDeintDefValues.REG_DIN87<<24);
    dint_cfg24.uCFG24_DIN_FCH_SC_PAT1         = (gDeintDefValues.REG_DIN88)|(gDeintDefValues.REG_DIN89<<8)|(gDeintDefValues.REG_DIN90<<16)|(gDeintDefValues.REG_DIN91<<24);;
#endif
    vpp_obj->dint_cfg25.uCFG25_DIN_FCH_SC_LEN          = gDeintDefValues.REG_DIN92;
    vpp_obj->dint_cfg25.uCFG25_DIN_GFRM_GRD_ENT_TH     = gDeintDefValues.REG_DIN93;
    vpp_obj->dint_cfg25.uCFG25_DIN_FRM_FXD_TH          = gDeintDefValues.REG_DIN94;
#if REFERENCE_CODE //Kept for reference
    dint_cfg26.uCFG26_DIN_FLD_ENT_TH0         = gDeintDefValues.REG_DIN95;
    dint_cfg26.uCFG26_DIN_FLD_ENT_TH1         = gDeintDefValues.REG_DIN96;
    dint_cfg26.uCFG26_DIN_FLD_EXT1_TH0        = gDeintDefValues.REG_DIN97;
    dint_cfg26.uCFG26_DIN_FLD_EXT1_TH0A       = gDeintDefValues.REG_DIN98;
    dint_cfg27.uCFG27_DIN_FLD_EXT1_TH1        = gDeintDefValues.REG_DIN99;
    dint_cfg28.uCFG28_DIN_FLD_EXT2_TH0        = (gDeintDefValues.REG_DIN100)|(gDeintDefValues.REG_DIN101<<8);
    dint_cfg28.uCFG28_DIN_FLD_EXT2_TH1        = (gDeintDefValues.REG_DIN102)|(gDeintDefValues.REG_DIN103<<8);
#endif
    vpp_obj->dint_cfg29.uCFG29_DIN_GLB_MOT             = (gDeintDefValues.REG_DIN104)|(gDeintDefValues.REG_DIN105<8);
    vpp_obj->dint_cfg29.uCFG29_DIN_FCH_IND             = gDeintDefValues.REG_DIN106;
#if REFERENCE_CODE //Kept for reference
    dint_cfg30.uCFG30_DIN_STM_SBF0            = gDeintDefValues.REG_DIN107;
    dint_cfg30.uCFG30_DIN_STM_SBF1            = gDeintDefValues.REG_DIN108;
    dint_cfg30.uCFG30_DIN_STM_SBF2            = gDeintDefValues.REG_DIN109;
    dint_cfg30.uCFG30_DIN_STM_SBF3            = gDeintDefValues.REG_DIN110;
    dint_cfg31.uCFG31_DIN_STM_SBF4            = gDeintDefValues.REG_DIN111;
    dint_cfg31.uCFG31_DIN_STM_SBF5            = gDeintDefValues.REG_DIN112;
    dint_cfg31.uCFG31_DIN_STM_SBF6            = gDeintDefValues.REG_DIN113;
    dint_cfg31.uCFG31_DIN_STM_SBF7            = gDeintDefValues.REG_DIN114;
    dint_cfg32.uCFG32_DIN_STM_SBF8            = gDeintDefValues.REG_DIN115;
    dint_cfg32.uCFG32_DIN_STM_SBF9            = gDeintDefValues.REG_DIN116;
    dint_cfg32.uCFG32_DIN_STM_SBF10           = gDeintDefValues.REG_DIN117;
    dint_cfg32.uCFG32_DIN_STM_SBF11           = gDeintDefValues.REG_DIN118;
    dint_cfg33.uCFG33_DIN_STM_SBF12           = gDeintDefValues.REG_DIN119;
    dint_cfg33.uCFG33_DIN_STM_SBF13           = gDeintDefValues.REG_DIN120;
    dint_cfg33.uCFG33_DIN_STM_SBF14           = gDeintDefValues.REG_DIN121;
    dint_cfg33.uCFG33_DIN_STM_SBF15           = gDeintDefValues.REG_DIN122;
#endif

    //WRITE_VPDEINT_DEFAULT_VAL(DIN143, gDeintDefValues);

    vpp_obj->fe.DinStmCtrl1 = gDeintDefValues.REG_DIN0;
    vpp_obj->fe.DinStmCtrl2 = gDeintDefValues.REG_DIN1;
    vpp_obj->fe.DinViCtrl = gDeintDefValues.REG_DIN2;
    vpp_obj->fe.DinMcbCtrl1 = gDeintDefValues.REG_DIN12;
    vpp_obj->fe.DinMcbCtrl2 = gDeintDefValues.REG_DIN13;
    vpp_obj->fe.DinMcbCtrl3 = gDeintDefValues.REG_DIN14;
    vpp_obj->fe.DinMeCtrl1 = gDeintDefValues.REG_DIN15;
    vpp_obj->fe.DinMeCtrl2 = gDeintDefValues.REG_DIN16;
    vpp_obj->fe.DinFilmCadenceCtrl = gDeintDefValues.REG_DIN65;
    vpp_obj->fe.DinFilmCadenceDetMode1 = gDeintDefValues.REG_DIN66;
    vpp_obj->fe.DinFilmCadenceDetMode2 = gDeintDefValues.REG_DIN67;
    vpp_obj->fe.DinProgFlagCtrl = gDeintDefValues.REG_DIN68;
    vpp_obj->fe.DinFilmCad32Ctrl1 = gDeintDefValues.REG_DIN72;
    vpp_obj->fe.DinFilmCad32Ctrl2 = gDeintDefValues.REG_DIN73;
    vpp_obj->fe.DinFilmCad22Ctrl = gDeintDefValues.REG_DIN76;
    vpp_obj->fe.DinFilmCadSplCadLen = gDeintDefValues.REG_DIN92;
    vpp_obj->fe.EnableIPodSplModeVar = 0;

    //BG_FIX
    VPP_DEINTEXCL_CTRL_PARAMS  InExclParams;
    VPP_DEINTADV_MD_PARAMS AdvParams;

    InExclParams.ExclSel = VPP_DEINTEXCL_PARAM_MODE_NONE;
    InExclParams.LinesFromTop = 0x0;
    InExclParams.LinesFromBottom = 0x0;
    InExclParams.PixelsFromLeft = 0x0;
    InExclParams.PixelsFromRight = 0x0;
    FE_DEINT_SetExclCtrlParams(vpp_obj, &InExclParams);

    // need to export these
    AdvParams.SmallMotFilmExit = TRUE;
    AdvParams.EnablePostWveErrMode32 = FALSE;
    AdvParams.EnablePostWveErrMode22 = FALSE;
    AdvParams.EnablePostWveErrModeOther = FALSE;
    AdvParams.EnableProgFlag = FALSE;
    AdvParams.EnableIPodSplMode = FALSE; // looks like advance viedo spatial mode
    AdvParams.EnableTemporalSplMode = FALSE; // looks like advance video temporal mode
    FE_DEINT_SetAdvancedModeParams(vpp_obj, &AdvParams);

    //Read default registers
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0;
    VPP_REG_WORD32_READ(RegAddr , &vpp_obj->vptop_ctrl_cfg0.u32);
#ifdef EN_VPP_DYNAMIC_CLKGATE
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_CFG0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->vptop_cfg0.u32);
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */
#endif
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0, 0x30);
    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0,&vpp_obj->vptop_ctrl_cfg0.u32);
    vpp_obj->vptop_ctrl_cfg0.uCFG0_idata_ctrl = 1;
    vpp_obj->vptop_ctrl_cfg0.uCFG0_vp_top_bypass = 1;
    vpp_obj->vptop_ctrl_cfg0.uCFG0_vptop_csc_bypass = 1;
    vpp_obj->vptop_ctrl_cfg0.uCFG0_vptop_dns444_422_bypass = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0, vpp_obj->vptop_ctrl_cfg0.u32);

    return MV_VPP_OK;
}
/*-----------------------------------------------------------------------------
 * Description: Function select the Spatio Temporal Control for Forced 2D
 *     set Luma, Chroma Ctrl for Film and Video pix
 *
 * Arguments  : Forced2DCtrl - Luma Chroma Ctrl Value
 * Returns    :
 *      MV_VPP_OK on success
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetForced2DCtrl(VPP_OBJ* vpp_obj,PVPP_DEINT_FORCED2D_CTRL pForced2DCtrl)
{

#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
    ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.LumaVidCtrl = pForced2DCtrl->LumaCtrlForVideoPix;
    ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaVidCtrl = pForced2DCtrl->ChromaCtrlForVideoPix;
    ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.LumaFlmCtrl = pForced2DCtrl->LumaCtrlForFilmPix;
    ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaFlmCtrl = pForced2DCtrl->ChromaCtrlForFilmPix;
    vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL1 = vpp_obj->fe.DinStmCtrl1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG0, vpp_obj->fe.DinStmCtrl1);
#endif
    return MV_VPP_OK;
}
/*-----------------------------------------------------------------------------
 * Description: Function set the Film Mode Entry Hard and Soft for 32/22 mode
 *
 *
 * Arguments  : pFilmModeEntry -- Film Mode entry mode and Hard/Soft Value
 * Returns    :
 *      MV_VPP_OK on success
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetFilmModeEntry(VPP_OBJ* vpp_obj,PVPP_DEINT_FILM_MODE_ENTRY_PARAM pFilmModeEntry)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
    if(pFilmModeEntry->EntryMode==VPP_DEINT_MODE_32) {
        vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32NOF0 = pFilmModeEntry->NofInHardBld;
        vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32NOF1 = pFilmModeEntry->NofInSoftBld;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG20, vpp_obj->dint_cfg20.u32);
    } else {
        vpp_obj->dint_cfg21.uCFG21_DIN_FCH_22NOF0 = pFilmModeEntry->NofInHardBld;
        vpp_obj->dint_cfg21.uCFG21_DIN_FCH_22NOF1 = pFilmModeEntry->NofInSoftBld;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG21, vpp_obj->dint_cfg21.u32);
    }
#endif

    return MV_VPP_OK;
}
/*-----------------------------------------------------------------------------
 * Description: Function set the OSd CTRL Enable/Disable
 *
 *
 * Arguments  : OSDCtrl -- Enable /Disable OSD Ctrl
 * Returns    :
 *      MV_VPP_OK on success
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetOSDCtrl(VPP_OBJ* vpp_obj,UINT8 OSDCtrl)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
    vpp_obj->dint_cfg29.uCFG29_DIN_FCH_IND = OSDCtrl;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG29, vpp_obj->dint_cfg29.u32);
#endif

    return MV_VPP_OK;
}
/*-----------------------------------------------------------------------------
 * Description: Function selects the desired Preset Mode and sets the registers
 *          based on this mode.
 *
 * Arguments  : DeintPresetMode - Mode based on 2D/3D and Liner/Vector
 *  pMdParams - Params which includes VI mode and Chroma motion.
 * Returns    :
 *      MV_VPP_OK on success
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetPresetMode(VPP_OBJ* vpp_obj, int DeintPresetMode, PVPP_DINT_PRESET_MODE_PARAMS pMdParams)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    UINT16 ViCtrlLocal;

    //VPPAPI_DBG_INFO( "REceived mode is %d, ViMode: %d, ChrMotVidMode: %d, ChrMotFlmMode: 0x%x, FieldPolarity: 0x%x\n",
    //DeintPresetMode, pMdParams->ViMode, pMdParams->ChrMotVidMode, pMdParams->ChrMotFlmMode, pMdParams->FieldPolarity);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;

    switch (DeintPresetMode) {
        case VPP_DEINTPRESET_2D_LINEAR:
        case VPP_DEINTPRESET_2D_VECTOR:
        case VPP_DEINTPRESET_3D_LINEAR:
        case VPP_DEINTPRESET_3D_VECTOR:
        case VPP_DEINTPRESET_3D_MA_LINEAR:
        case VPP_DEINTPRESET_3D_MA_VECTOR:
        case VPP_DEINTPRESET_2D_LINEAR_USE_CURR_FIELD:
        case VPP_DEINTPRESET_2D_VECTOR_USE_CURR_FIELD:
            /* ViMode Settings */
            if (pMdParams->ViMode == VPP_DEINTMODE_OFF)
                ViCtrlLocal = VPP_DEINTVI_NONE;
            else if(pMdParams->ViMode == VPP_DEINTMODE_SAFE)
                ViCtrlLocal = VPP_DEINTVI_ALL; //Safe Mode
            else if(pMdParams->ViMode == VPP_DEINTMODE_AGGRESSIVE)
                ViCtrlLocal = VPP_DEINTVI_AGGRESSIVE;
            else
                ViCtrlLocal = VPP_DEINTVI_ALL;

            /* Program ViCtrl */
            if (ViCtrlLocal & VPP_DEINTVI_REJECT)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnShallowMinRej = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnShallowMinRej = FALSE;

            if (ViCtrlLocal & VPP_DEINTVI_ENABLE_SLOPE)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnSlope = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnSlope = FALSE;

            if (ViCtrlLocal & VPP_DEINTVI_ENABLE_VARIANCE)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnVarMeasure = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnVarMeasure = FALSE;

            if (ViCtrlLocal & VPP_DEINTVI_ENABLE_HPF)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnHighPassChar = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnHighPassChar = FALSE;

            if (ViCtrlLocal & VPP_DEINTVI_ENABLE_DIAG_INTERPOL)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnVectInterpol = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnVectInterpol = FALSE;

            if (ViCtrlLocal & VPP_DEINTVI_CTRL_INTERPOL_ANGLE_BASED_ON_EDGE)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnInterpolLumTrans = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnInterpolLumTrans = FALSE;

            if (ViCtrlLocal & VPP_DEINTVI_SEL_MINIMA_FAR_AWAY_FROM_VERTICAL)
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnShallowMinDistFromVert = TRUE;
            else
                ((DEINT_VI_CTRL_REG_BITS *)(&vpp_obj->fe.DinViCtrl))->Bits.EnShallowMinDistFromVert = FALSE;

            vpp_obj->dint_cfg1.uCFG1_DIN_VI_CTRL = vpp_obj->fe.DinViCtrl;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG1, vpp_obj->dint_cfg1.u32);

            /* Program SpatioTemporalMixer Control */
            if (VppDeintCfgPresetModeParam[DeintPresetMode].pSpatioTemporalMixerCtrl != NULL) {
                ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.LumaVidCtrl = VppDeintCfgPresetModeParam[DeintPresetMode].pSpatioTemporalMixerCtrl->LumaCtrlForVideoPix;
                ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.LumaFlmCtrl = VppDeintCfgPresetModeParam[DeintPresetMode].pSpatioTemporalMixerCtrl->LumaCtrlForFilmPix;
                vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL1 = vpp_obj->fe.DinStmCtrl1;

                //((DEINT_STM_CTRL2_REG_BITS *)(&vpp_obj->fe.DinStmCtrl2))->Bits.LumaChromaYMotionSel = VppDeintCfgPresetModeParam[DeintPresetMode].pSpatioTemporalMixerCtrl->LumaChromaMotSelForY;
                ((DEINT_STM_CTRL2_REG_BITS *)(&vpp_obj->fe.DinStmCtrl2))->Bits.TempTapSelVidMotion = VppDeintCfgPresetModeParam[DeintPresetMode].pSpatioTemporalMixerCtrl->TempTapSel;
                vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL2 = vpp_obj->fe.DinStmCtrl2;

                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG0, vpp_obj->dint_cfg0.u32);
            }

            if ((DeintPresetMode == VPP_DEINTPRESET_3D_MA_LINEAR)
             || (DeintPresetMode == VPP_DEINTPRESET_3D_MA_VECTOR)) {
                switch (pMdParams->ChrMotVidMode) {
                    case VPP_DEINTCHROMA_MOT_VID_SAME_AS_LUMA:
                        ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaVidCtrl = 0x0;//                        ((DEINT_STM_CTRL2_REG_BITS *)(&vpp_obj->fe.DinStmCtrl2))->Bits.LumaChromaCMotionSel = 0x0;
                        break;
                    case VPP_DEINTCHROMA_MOT_FIXED_2D:
                        ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaVidCtrl = 0x1;//                        ((DEINT_STM_CTRL2_REG_BITS *)(&vpp_obj->fe.DinStmCtrl2))->Bits.LumaChromaCMotionSel = 0x0;
                        break;
                    case VPP_DEINTCHROMA_MOT_INDEP_CHROMA_MOT:
                        ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaVidCtrl = 0x0;//                        ((DEINT_STM_CTRL2_REG_BITS *)(&vpp_obj->fe.DinStmCtrl2))->Bits.LumaChromaCMotionSel = 0x4;
                        break;
                    case VPP_DEINTCHROMA_MOT_MAX_OF_CHROMA_AND_LUMA_MOT:
                        ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaVidCtrl = 0x0;//                        ((DEINT_STM_CTRL2_REG_BITS *)(&vpp_obj->fe.DinStmCtrl2))->Bits.LumaChromaCMotionSel = 0x5;
                        break;
                    default:
                        break;
                }

                if (pMdParams->ChrMotFlmMode)
                    ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaFlmCtrl = 0x1;
                else
                    ((DEINT_STM_CTRL1_REG_BITS *)(&vpp_obj->fe.DinStmCtrl1))->Bits.ChromaFlmCtrl = 0x0;

                vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL1 = vpp_obj->fe.DinStmCtrl1;
                vpp_obj->dint_cfg0.uCFG0_DIN_STM_CTRL2 = vpp_obj->fe.DinStmCtrl2;
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG0, vpp_obj->dint_cfg0.u32);
            }

            /* Program DeintMode */
            ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.FiPol = pMdParams->FieldPolarity;

            ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.Mode = VppDeintCfgPresetModeParam[DeintPresetMode].DeintMode;
            vpp_obj->dint_cfg40.uCFG40_DEINT_MODE = vpp_obj->fe.VpDeintMode;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG40, vpp_obj->dint_cfg40.u32);

            break;

        case VPP_DEINTPRESET_BYPASS_AND_FLD_BUFF_DISABLED:
            /* Program DeintMode to Progressive */
            ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.Mode = 0;
            vpp_obj->dint_cfg40.uCFG40_DEINT_MODE = vpp_obj->fe.VpDeintMode;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG40, vpp_obj->dint_cfg40.u32);
            break;
        case VPP_DEINTPRESET_NONE:
        case VPP_DEINTPRESET_BYPASS_AND_FLD_BUFF_ENABLED:
            /* Program DeintMode to Progressive */
            ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.Mode = 0;
            vpp_obj->dint_cfg40.uCFG40_DEINT_MODE = vpp_obj->fe.VpDeintMode;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG40, vpp_obj->dint_cfg40.u32);
            break;
         default:
            break;
    }

#endif
    return MV_VPP_OK;
}

INT FE_DEINT_ChangeMode(VPP_OBJ* vpp_obj, UINT32 uiMode, UINT32 uiScanType)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;

    if(DEINT_2D == uiMode) {
        ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.Mode = VPP_DEINTMODE_SPATIAL_NEXT;
    } else if(DEINT_3D == uiMode) {
        ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.Mode = VPP_DEINTMODE_MA_3D;
    }

    ((VP_DEINT_MODE_BITS *)(&vpp_obj->fe.VpDeintMode))->Bits.FiPol = uiScanType;

    vpp_obj->dint_cfg40.uCFG40_DEINT_MODE = vpp_obj->fe.VpDeintMode;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG40, vpp_obj->dint_cfg40.u32);
#endif
    return MV_VPP_OK;
}


/*-----------------------------------------------------------------------------
 * Description: This API is used for setting the IPOD settings.
 * Arguments  : pAdvMdParams - Pointer to structure containing weave error modes,
 *                      IpodSpecialMode, Progressive Flag etc.
 * Returns    :
 *              MV_VPP_OK on success
 *              Error code on failure
 * Note       :
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetAdvancedModeParams(VPP_OBJ* vpp_obj, PVPP_DEINTADV_MD_PARAMS pAdvMdParams)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;

    /* Enabling Post exit for 32 mode */
    if(pAdvMdParams->SmallMotFilmExit)
        ((DEINT_FLM_CAD_32CTRL1_REG_BITS *)(&vpp_obj->fe.DinFilmCad32Ctrl1))->Bits.EnPostExitFor32 = 1; //Enable Post Exit for 32
    else
        ((DEINT_FLM_CAD_32CTRL1_REG_BITS *)(&vpp_obj->fe.DinFilmCad32Ctrl1))->Bits.EnPostExitFor32 = 0; //Disable Post Exit for 32

     vpp_obj->dint_cfg20.uCFG20_DIN_FCH_32CTRL0 = vpp_obj->fe.DinFilmCad32Ctrl1;
     VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG20, vpp_obj->dint_cfg20.u32);

    if(pAdvMdParams->EnablePostWveErrMode22)
        ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnPixelExcl22 = 1;
    else
        ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnPixelExcl22 = 0;

    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_CTRL = vpp_obj->fe.DinFilmCadenceCtrl;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG19, vpp_obj->dint_cfg19.u32);

    /* PF Control Registers */
    if (pAdvMdParams->EnableProgFlag) {
        //Set value = 0x3
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnDinPixFrameMADThresh = 1;
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnFxdGblMotionThresh = 1;
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnPixelExclusion = 0;         ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnSmallFxdWin = 0;

    } else {
        //Set value = 0x0
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnDinPixFrameMADThresh = 0;
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnFxdGblMotionThresh = 0;
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnPixelExclusion = 0;
        ((DEINT_FLM_CAD_PROG_FLAG_REG_BITS *)(&vpp_obj->fe.DinProgFlagCtrl))->Bits.EnSmallFxdWin = 0;
    }
    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_PFMODE = vpp_obj->fe.DinProgFlagCtrl;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG19, vpp_obj->dint_cfg19.u32);

    /* Enable32SMFE; TBD */
    vpp_obj->fe.EnableIPodSplModeVar = pAdvMdParams->EnableIPodSplMode;

    // Temporal
    if (pAdvMdParams->EnableTemporalSplMode)
        vpp_obj->dint_cfg5.uCFG5_DIN_MAD_VID_THH = 0x22;
    else
        vpp_obj->dint_cfg5.uCFG5_DIN_MAD_VID_THH = 0x09;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG5, vpp_obj->dint_cfg5.u32);
#endif

    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description: This API enables the special Cadence detection based
 *          on the user input.
 *
 * Arguments  : pUserCadence - Takes Enable, Length and Pattern as input
 * Returns    :
 *  MV_VPP_OK on success
 * Note:
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_ConfigUserCadence(VPP_OBJ* vpp_obj, int Cadence, PVPP_DEINTUSER_CAD_PARAMS pUserCadence)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;

    if (Cadence == VPP_DEINTUSER_CAD_MODE_1) {
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.EnPrgCad1Det = pUserCadence->Enable;
        vpp_obj->dint_cfg19.uCFG19_DIN_FCH_EN = (vpp_obj->fe.DinFilmCadenceDetMode1) | (vpp_obj->fe.DinFilmCadenceDetMode2 << 8);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG19, vpp_obj->dint_cfg19.u32);

        ((DEINT_FLM_CAD_SPL_CAD_LEN_REG_BITS *)(&vpp_obj->fe.DinFilmCadSplCadLen))->Bits.LenOfProgCadPat0 = pUserCadence->Length;
        vpp_obj->dint_cfg25.uCFG25_DIN_FCH_SC_LEN = vpp_obj->fe.DinFilmCadSplCadLen;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG25, vpp_obj->dint_cfg25.u32);

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG23, pUserCadence->Pattern);
    } else if (Cadence == VPP_DEINTUSER_CAD_MODE_2) {

        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.EnPrgCad2Det = pUserCadence->Enable;
        vpp_obj->dint_cfg19.uCFG19_DIN_FCH_EN = (vpp_obj->fe.DinFilmCadenceDetMode1) | (vpp_obj->fe.DinFilmCadenceDetMode2 << 8);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG19, vpp_obj->dint_cfg19.u32);

        ((DEINT_FLM_CAD_SPL_CAD_LEN_REG_BITS *)(&vpp_obj->fe.DinFilmCadSplCadLen))->Bits.LenOfProgCadPat1 = pUserCadence->Length;
        vpp_obj->dint_cfg25.uCFG25_DIN_FCH_SC_LEN = vpp_obj->fe.DinFilmCadSplCadLen;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG25, vpp_obj->dint_cfg25.u32);

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG24, pUserCadence->Pattern);
    }
#endif
    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description: This API sets the Exclusion params for film mode.
 *
 * Arguments  : pExclParams - Takes Lines and Pixel values
 *          (Top, Bottom, Left and Right)
 * Returns    :
 *  MV_VPP_OK on success
 * Note       :
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetExclCtrlParams(VPP_OBJ* vpp_obj, PVPP_DEINTEXCL_CTRL_PARAMS pExclParams)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    T32DINT_CFG2 dint_cfg2;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG2, &dint_cfg2.u32);

    switch(pExclParams->ExclSel) {
        case VPP_DEINTEXCL_PARAM_MODE_NONE:                //0x0
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnSplitZoneDet32 = 0;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnSplitZoneDet22 = 0;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnTopZoneDet32 = 0;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnTopZoneDet22 = 0;
            break;

        case VPP_DEINTEXCL_PARAM_EXCLUSION_SEL:            //0xF
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnSplitZoneDet32 = 1;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnSplitZoneDet22 = 1;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnTopZoneDet32 = 1;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnTopZoneDet22 = 1;
            break;

        case VPP_DEINTEXCL_PARAM_EXCLUSION_SEL_2ZONE:  //0x3
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnSplitZoneDet32 = 1;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnSplitZoneDet22 = 1;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnTopZoneDet32 = 0;
            ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnTopZoneDet22 = 0;
            break;

        default:
            break;
    }

    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_CTRL = vpp_obj->fe.DinFilmCadenceCtrl;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG19, vpp_obj->dint_cfg19.u32);

    dint_cfg2.uCFG2_DIN_EXC_B = pExclParams->LinesFromBottom;
    dint_cfg2.uCFG2_DIN_EXC_T = pExclParams->LinesFromTop;
    dint_cfg2.uCFG2_DIN_EXC_R = pExclParams->PixelsFromRight;
    dint_cfg2.uCFG2_DIN_EXC_L = pExclParams->PixelsFromLeft;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG2, dint_cfg2.u32);
#endif

    return MV_VPP_OK;
}

/**********************************************************************
 * FUNCTION: This API sets the Vp Read/Write client clock settings
 *
 * PARAMS: pVpClientClk - Pointer to VP client clock settings
 * RETURN: 1 - succeed
 *         others - failed to load lookup table to BCM buffer
 **********************************************************************/
INT FE_DEINT_SetVpDeintClientClk(VPP_OBJ* vpp_obj, PVPP_VP_CLIENT_CLK pVpClientClk)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

    if (pVpClientClk->field.WriteEnable)
        ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.WrMask = 0;
    else
        ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.WrMask = 1;

    if (pVpClientClk->field.Read0Enable)
        ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.RdMaskFldBufLow = 0;
    else
        ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.RdMaskFldBufLow = 1;

    if (pVpClientClk->field.Read1Enable)
        ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.RdMaskFldBufHigh = 0;
    else
        ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.RdMaskFldBufHigh = 1;


    if(pVpClientClk->field.ClkEnable) {
        vpp_obj->vptop_ctrl_cfg0.uCFG0_vp_top_bypass = 0;
        vpp_obj->vptop_ctrl_cfg0.uCFG0_idata_ctrl = 0;
    } else {
        vpp_obj->vptop_ctrl_cfg0.uCFG0_vp_top_bypass = 1;
        vpp_obj->vptop_ctrl_cfg0.uCFG0_idata_ctrl = 1;
    }
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->vptop_ctrl_cfg0.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT + RA_DINT_CFG40;
    vpp_obj->dint_cfg40.uCFG40_FB_CTRL = vpp_obj->fe.VpFbCtrl;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->dint_cfg40.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CLKEN_CTRL;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, pVpClientClk->data);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VP_CLKEN_CTRL;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, 0/*pVpClientClk->field.ClkEnable*/);

#endif
    return MV_VPP_OK;
}


/**********************************************************************
 * FUNCTION: This API sets the Vp Read/Write client pixel/DMA size
 *
 * PARAMS: pVpClientSize - Pointer to VP client pixel/DMA size
 * RETURN: 1 - succeed
 *         others - failed to load lookup table to BCM buffer
 **********************************************************************/
INT FE_DEINT_SetVpDeintClientSize(VPP_OBJ* vpp_obj, PVPP_VP_CLIENT_SIZE pVpClientSize, int training)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    UINT32 TotalInpPix;
    UINT32 TotPixForWriteClient, TotPixForReadClient;
    UINT32 FbBufMode;
    T32ReadClient_NonStdRes stNonStdRes;

    TotalInpPix = pVpClientSize->ivRes * pVpClientSize->ihRes;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, TotalInpPix);

    TotPixForWriteClient = (pVpClientSize->ivRes + 1) * pVpClientSize->ihRes;

    FbBufMode = ((VP_FB_CTRL_BITS *)(&vpp_obj->fe.VpFbCtrl))->Bits.DeintClientMode ? 26 : 20;
    //TotPixForReadClient = (TotPixForWriteClient * FbBufMode + 127) / 128;
    TotPixForReadClient = (pVpClientSize->ihRes * FbBufMode + 127) / 128 * (pVpClientSize->ivRes + 1);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_pix;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, TotPixForWriteClient);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_pack;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, 0x0);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Word;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, TotPixForReadClient);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Word;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, TotPixForReadClient);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_pack;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, 0x0);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_pack;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, 0x0);

    if(pVpClientSize->ihRes * 20 % 128) {
        stNonStdRes.uNonStdRes_enable = 1;
        stNonStdRes.uNonStdRes_pixlineTot = pVpClientSize->ihRes;
        stNonStdRes.uNonStdRes_flushCnt = 5;
    } else {
        stNonStdRes.uNonStdRes_enable = 0;
    }

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_NonStdRes, stNonStdRes.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_NonStdRes, stNonStdRes.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_NonStdRes, stNonStdRes.u32);

#endif
    return MV_VPP_OK;
}

INT FE_DEINT_SetVpTG(VPP_OBJ *vpp_obj, PVPP_VP_CLIENT_RES pVpClientRes, int progressive) //DONE
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
    UINT32 RegVal;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_vptg;

    if (progressive) {
        RegVal = (pVpClientRes->VRes + VPP_FE_VP_TG_SIZE_V_OFF_P) + ((pVpClientRes->HRes + VPP_FE_VP_TG_SIZE_H_OFF_P) << 16);
    } else {
        RegVal = (pVpClientRes->VRes + VPP_FE_VP_TG_SIZE_V_OFF_I) + ((pVpClientRes->HRes + VPP_FE_VP_TG_SIZE_H_OFF_I) << 16);
    }

    /*write TG size to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pVpClientRes->HRes + VPP_FE_VP_TG_HB_FE_OFF + (VPP_FE_VP_TG_HB_BE << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pVpClientRes->VRes + VPP_FE_VP_TG_VB0_FE_OFF + (VPP_FE_VP_TG_VB0_BE << 16);

    /*writer the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    return RetVal;
}

INT FE_DEINT_SetVpDMX(VPP_OBJ *vpp_obj, PVPP_VP_CLIENT_RES pVpClientRes, int progressive)
{
    INT RetVal = MV_VPP_OK;
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    T32DINTDMX_CFG0 dintdmx_cfg0;
    T32DINTDMX_CFG1 dintdmx_cfg1;
    T32DINTDMX_CFG2 dintdmx_cfg2;
    T32DINTDMX_CFG3 dintdmx_cfg3;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINTDMX;
    VPP_REG_WORD32_READ(RegAddr + RA_DINTDMX_CFG0, &dintdmx_cfg0.u32);

    if (progressive) {
        dintdmx_cfg1.uCFG1_HRES = pVpClientRes->HRes-1; //doubt on progressive programmin
        dintdmx_cfg1.uCFG1_HT = pVpClientRes->HRes + VPP_FE_VP_TG_SIZE_H_OFF_P - 1;
        dintdmx_cfg2.uCFG2_VRES = (pVpClientRes->VRes/2)-1;
        dintdmx_cfg2.uCFG2_VT = pVpClientRes->VRes + VPP_FE_VP_TG_SIZE_V_OFF_P - 1;
        dintdmx_cfg3.uCFG3_IVT = pVpClientRes->VRes + VPP_FE_VP_TG_SIZE_V_OFF_P - 1;
    } else {
        dintdmx_cfg1.uCFG1_HRES = pVpClientRes->HRes-1;
        dintdmx_cfg1.uCFG1_HT = pVpClientRes->HRes + VPP_FE_VP_TG_SIZE_H_OFF_I - 1;
        dintdmx_cfg2.uCFG2_VRES = pVpClientRes->VRes-1;
        dintdmx_cfg2.uCFG2_VT = 2*(pVpClientRes->VRes + VPP_FE_VP_TG_SIZE_V_OFF_I) - 1;
        dintdmx_cfg3.uCFG3_IVT = pVpClientRes->VRes + VPP_FE_VP_TG_SIZE_V_OFF_I - 1;
    }

    if(progressive)
        dintdmx_cfg0.uCFG0_CTRL = 0x64;
    else
        dintdmx_cfg0.uCFG0_CTRL = 0xe4;

    dintdmx_cfg0.uCFG0_PROSCAN = 1;
    dintdmx_cfg0.uCFG0_INIT_EN = 1;
    dintdmx_cfg0.uCFG0_MEMSIZE = 0x77F;
    dintdmx_cfg0.uCFG0_SW_SEL = 1; // ignore hardware calibration

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_DINTDMX_CFG1), dintdmx_cfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_DINTDMX_CFG2), dintdmx_cfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_DINTDMX_CFG3), dintdmx_cfg3.u32);
    //toggle on resolution change
    dintdmx_cfg0.uCFG0_INIT = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_DINTDMX_CFG0), dintdmx_cfg0.u32);
    dintdmx_cfg0.uCFG0_INIT = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_DINTDMX_CFG0), dintdmx_cfg0.u32);

#endif
    return RetVal;
}

INT FE_DEINT_SetField(VPP_OBJ *vpp_obj, UINT8 is_top) {
    INT RetVal = MV_VPP_OK;
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0;

    if(is_top)
        vpp_obj->vptop_ctrl_cfg0.uCFG0_fld = 1;
    else
        vpp_obj->vptop_ctrl_cfg0.uCFG0_fld = 0;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->vptop_ctrl_cfg0.u32);
#endif

    return RetVal;
}

INT FE_DEINT_VpTG_Enable(VPP_OBJ *vpp_obj, UINT8 enable) {
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
    T32VPTOP_CTRL_vptg_ctrl vptop_ctrl_vptg_ctrl;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_vptg_ctrl;
    VPP_REG_WORD32_READ(RegAddr , &vptop_ctrl_vptg_ctrl.u32);

    if(0 == enable) {
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
    } else {
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);

        vptop_ctrl_vptg_ctrl.uvptg_ctrl_start = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_start = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
    }
    return RetVal;
}

INT FE_DEINT_VpClient_Clear(VPP_OBJ *vpp_obj, VPP_VP_CLIENT_TYPE client_id) {
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
    T32ReadClient_Rd readclient_rd;
#endif
    T32WriteClient_Wr writeclient_wr;

    if(client_id==VPP_VP_CLIENT_WRITE) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_Wr;
        VPP_REG_WORD32_READ(RegAddr, &writeclient_wr.u32);
        writeclient_wr.uWr_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, writeclient_wr.u32);
        writeclient_wr.uWr_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, writeclient_wr.u32);
    }else if(client_id==VPP_VP_CLIENT_READ0 || client_id==VPP_VP_CLIENT_READ1){
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
        if(client_id==VPP_VP_CLIENT_READ0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Rd;
        else
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Rd;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
#endif
    }

    return RetVal;
}


INT FE_DEINT_VpClient_Enable(VPP_OBJ *vpp_obj, VPP_VP_CLIENT_TYPE client_id, UINT8 Enable) {
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
    T32ReadClient_Rd readclient_rd;
#endif
    T32WriteClient_Wr writeclient_wr;

    if(client_id==VPP_VP_CLIENT_WRITE) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_Wr;
        VPP_REG_WORD32_READ(RegAddr, &writeclient_wr.u32);
        writeclient_wr.uWr_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, writeclient_wr.u32);
        writeclient_wr.uWr_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, writeclient_wr.u32);
    }else if(client_id==VPP_VP_CLIENT_READ0 || client_id==VPP_VP_CLIENT_READ1){
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
        if(client_id==VPP_VP_CLIENT_READ0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Rd;
        else
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Rd;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
#endif
    }

    return RetVal;
}

/*-----------------------------------------------------------------------------
 * Description: This API sets the registers related to Cadence detection,
 *              thresholds (based on type of input), VP Client settings.
 *
 * Arguments  : pDeintParams - Pointer to deint params structure
 * Returns    :
 *      MV_VPP_OK on success
 * Note       : None
 *-----------------------------------------------------------------------------
 */
INT FE_DEINT_SetInputParams(VPP_OBJ *vpp_obj, VPP_DEINT_INPUT_TYPE DeintInpType,
                            PVPP_DEINT_THRES_PARAMS pThresSettings)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;
    T32DINT_CFG3 dint_cfg3;
    T32DINT_CFG22 dint_cfg22;
    PVPP_DEINT_THRES_PARAMS     pDeintThParams;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG3, &dint_cfg3.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG22, &dint_cfg22.u32);

    // Sets the Noise calibration and Weave error threshold registers
    if (pThresSettings != NULL) {
        pDeintThParams = pThresSettings;
    } else {
        // Set thresholds based on the input
        if (DeintInpType == VPP_DEINT_INPUT_HD) {
            pDeintThParams = &VppDeintCfgThresParams[VPP_DEINT_INPUT_HD];
        } else if (DeintInpType == VPP_DEINT_INPUT_SD_PAL) {
            pDeintThParams = &VppDeintCfgThresParams[VPP_DEINT_INPUT_SD_PAL];
        } else {
            pDeintThParams = &VppDeintCfgThresParams[VPP_DEINT_INPUT_SD_NTSC];
        }
    }
    DeintSetThresholds(vpp_obj,pDeintThParams);

    if (DeintInpType == VPP_DEINT_INPUT_HD) {
        ((DEINT_FLM_CAD_32CTRL1_REG_BITS *)(&vpp_obj->fe.DinFilmCad32Ctrl1))->Bits.EnWveErrSig2For32Exit = 0;
        // 0x30c 56 for HD - line crossing issue
        ((DEINT_MCB_CTRL_REG1_BITS *)(&vpp_obj->fe.DinMcbCtrl1))->Bits.FrmMotionSelPrevFld = 1;
        ((DEINT_MCB_CTRL_REG1_BITS *)(&vpp_obj->fe.DinMcbCtrl1))->Bits.CombFrmMotionSelCurrAndPrev = 1;
        // 0x30d 0x3f for HD
        ((DEINT_MCB_CTRL_REG2_BITS *)(&vpp_obj->fe.DinMcbCtrl2))->Bits.WegtAvForMotionRecCirCtrl = 0xf;

        ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnPixelExcl32 = 1;
        ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnPixelExclAuxCad = 1;
        dint_cfg22.uCFG22_DIN_FCH_SCCTRL1 = 0xEA;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG22, dint_cfg22.u32);
/*
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN16 << 2)),0xCB); // change by PANI from 0x10
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN17 << 2)),0x01); // change by PANI from 0x02
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN18 << 2)),0x09); // change by PANI from 0x12
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN23 << 2)),0x01); // change by PANI from 0x04
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN24 << 2)),0x07); // change by PANI from 0x22
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN25 << 2)),0x06);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN26 << 2)),0x03); */
    } else {
        ((DEINT_FLM_CAD_32CTRL1_REG_BITS *)(&vpp_obj->fe.DinFilmCad32Ctrl1))->Bits.EnWveErrSig2For32Exit = 0;
        // 0x30c 66 for SD - line crossing issue
        ((DEINT_MCB_CTRL_REG1_BITS *)(&vpp_obj->fe.DinMcbCtrl1))->Bits.FrmMotionSelPrevFld = 1; // PANI - VIJAYKR
        ((DEINT_MCB_CTRL_REG1_BITS *)(&vpp_obj->fe.DinMcbCtrl1))->Bits.CombFrmMotionSelCurrAndPrev = 1; // PANI  - VIJAYKR
        // 0x30d 0x7f for SD
        ((DEINT_MCB_CTRL_REG2_BITS *)(&vpp_obj->fe.DinMcbCtrl2))->Bits.WegtAvForMotionRecCirCtrl = 0xf; // PANI

        ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnPixelExcl32 = 0;
        ((DEINT_FLM_CAD_CTRL_REG_BITS *)(&vpp_obj->fe.DinFilmCadenceCtrl))->Bits.EnPixelExclAuxCad = 0;
        dint_cfg22.uCFG22_DIN_FCH_SCCTRL1 = 0xA8;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG22, dint_cfg22.u32);
/*
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN16 << 2)),0xCB); // PANI wants to change it for SD
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN17 << 2)),0x01); // PANI - 0x02 VIJAYKR
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN18 << 2)),0x09); // PANI enabletemporalsplmode 0x22  - 0x15 VIJAYKR
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN23 << 2)),0x01); // PANI
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN24 << 2)),0x07); // PANI
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN25 << 2)),0x06); // VIJAYKR
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + (DIN26 << 2)),0x03); // VIJAYKR */
    }

    dint_cfg3.uCFG3_DIN_MCB_CTRL1 = vpp_obj->fe.DinMcbCtrl1;
    dint_cfg3.uCFG3_DIN_MCB_CTRL2 = vpp_obj->fe.DinMcbCtrl2;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG3, dint_cfg3.u32);
    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_CTRL = vpp_obj->fe.DinFilmCadenceCtrl;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG19, vpp_obj->dint_cfg19.u32);
#endif

    return MV_VPP_OK;
}

INT FE_DEINT_SetFilmCadenceCtrl(VPP_OBJ* vpp_obj,UINT16 FilmCadence)
{
#ifdef CONFIG_DEINT_ENABLE
    UINT32 RegAddr;

    /* Film Cadence Handling registers */
    if (FilmCadence & VPP_DEINT32_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG2_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode2))->Bits.En32CadDet = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG2_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode2))->Bits.En32CadDet = FALSE;

    if (FilmCadence & VPP_DEINT22_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG2_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode2))->Bits.En22CadDet = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG2_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode2))->Bits.En22CadDet = FALSE;

    if (FilmCadence & VPP_DEINT2224_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En2224Det = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En2224Det = FALSE;

    if (FilmCadence & VPP_DEINT2332_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En2332Det = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En2332Det = FALSE;

    if (FilmCadence & VPP_DEINT32322_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En32322Det = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En32322Det = FALSE;

    if (FilmCadence & VPP_DEINT55_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En55Det = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En55Det = FALSE;

    if (FilmCadence & VPP_DEINT64_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En64Det = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En64Det = FALSE;

    if (FilmCadence & VPP_DEINT87_FLM_CAD)
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En87Det = TRUE;
    else
        ((DEINT_FLM_CAD_DET_MODE_REG1_BITS *)(&vpp_obj->fe.DinFilmCadenceDetMode1))->Bits.En87Det = FALSE;

    vpp_obj->dint_cfg19.uCFG19_DIN_FCH_EN = (vpp_obj->fe.DinFilmCadenceDetMode2 << 8) | vpp_obj->fe.DinFilmCadenceDetMode1;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT + RA_DINT_CFG19;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->dint_cfg19.u32);
#endif

    return MV_VPP_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////LOCAL STATIC FUNCTIONS/////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_DEINT_ENABLE
static void DeintSetThresholds(VPP_OBJ* vpp_obj,PVPP_DEINT_THRES_PARAMS pDeintThParams)
{
    UINT32 RegAddr;
    T32DINT_CFG9 dint_cfg9;
    T32DINT_CFG10 dint_cfg10;
    T32DINT_CFG16 dint_cfg16;
    T32DINT_CFG17 dint_cfg17;
    T32DINT_CFG18 dint_cfg18;
    T32DINT_CFG26 dint_cfg26;
    T32DINT_CFG27 dint_cfg27;
    T32DINT_CFG28 dint_cfg28;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_DINT;
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG9, &dint_cfg9.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG10, &dint_cfg10.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG16, &dint_cfg16.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG17, &dint_cfg17.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG18, &dint_cfg18.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG26, &dint_cfg26.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG27, &dint_cfg27.u32);
    VPP_REG_WORD32_READ(RegAddr + RA_DINT_CFG28, &dint_cfg28.u32);

    // Weave error thresholds
    dint_cfg9.uCFG9_DIN_WVE_TH = pDeintThParams->InterFldDiffThresWveErrDet;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG9, dint_cfg9.u32);

    // Noise Calibration thresholds
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG13, pDeintThParams->NoiseCalibThresSADRegion0);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG14, pDeintThParams->NoiseCalibThresSADRegion1);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG15, pDeintThParams->NoiseCalibThresSADRegion2);

    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH0 = pDeintThParams->NoiseCalibFramePixMotThresBelowReg0;
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH1 = pDeintThParams->NoiseCalibFramePixMotThresBwReg0AndReg1;
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH2 = pDeintThParams->NoiseCalibFramePixMotThresBwReg1AndReg2;
    dint_cfg16.uCFG16_DIN_NC_PIX_MAD_TH3 = pDeintThParams->NoiseCalibFramePixMotThresAboveReg2;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG16, dint_cfg16.u32);

    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH0 = pDeintThParams->NoiseCalibFrameGblMotThresBelowReg0;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH1 = pDeintThParams->NoiseCalibFrameGblMotThresBwReg0AndReg1;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH2 = pDeintThParams->NoiseCalibFrameGblMotThresBwReg1AndReg2;
    dint_cfg17.uCFG17_DIN_NC_GLB_MAD_TH3 = pDeintThParams->NoiseCalibFrameGblMotThresAboveReg2;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG17, dint_cfg17.u32);

    dint_cfg18.uCFG18_DIN_PF_MADTH = pDeintThParams->PixelThresFor32Det;
    dint_cfg18.uCFG18_DIN_PEXIT_TH = (pDeintThParams->PostExitCoarseThresFor32 << 16) | pDeintThParams->PostExitFineThresFor32;

    vpp_obj->dint_cfg29.uCFG29_DIN_GLB_MOT = pDeintThParams->GblMotThresToCtrlFldMotion;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_DINT_CFG29, vpp_obj->dint_cfg29.u32);
}
#endif
