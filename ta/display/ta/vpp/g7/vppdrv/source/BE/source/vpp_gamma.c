/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_GAMMA_C_

#include "vpp.h"
#include "vpp_gamma.h"
#include "vpp_gammapriv.h"
#include "vpp_gammacfg.h"
#include "galois_io.h"

/*-----------------------------------------------------------------------------
 * Functions
 *-----------------------------------------------------------------------------
 */
/*----------------------------------------------------------------------------
 * Description: This function will set the default values for the Gamma registers.
 *
 * Arguments  : Channel - Input Channel (Main/Pip)
 *
 * Returns    : On success returns MV_VPP_OK
 *
 * Notes      : None
 *----------------------------------------------------------------------------
 */
INT VPP_GAMMA_SetDefaults(VPP_OBJ* vpp_obj,INT Channel)
{
    INT RetCode;
    VPP_GAMMA_PARAMS GammaParams;
    UINT32 regAddr;

    DbgEnterFunction(("BdrGammaSetDefaults"));

    switch (Channel)
    {
    case VPP_VOP:
        regAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_GAMMA + RA_GAMMA_CFG0;
        GA_REG_WORD32_READ(regAddr , &vpp_obj->gamma.stGammaCfg0);
        break;
#ifdef ENABLE_GFX2_VOP2_PATH
    case VPP_VOP2:
        regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2+ RA_VOP2_GAMMA + RA_GAMMA_CFG0;
        GA_REG_WORD32_READ(regAddr , &vpp_obj->gamma.stGammaVop2Cfg0);
        break;
#endif
    default:
        return MV_VPP_ERANGEPARAM;
        break;
    }

    VPP_GAMMA_Enable(vpp_obj,Channel, FALSE);

    GammaParams.BCbGammaEnable = TRUE;
    GammaParams.GYGammaEnable  = TRUE;
    GammaParams.RCrGammaEnable = TRUE;
    RetCode = VPP_GAMMA_SetParams(vpp_obj,Channel,&GammaParams);

    if (RetCode != MV_VPP_OK)
        return RetCode;

    DbgExitFunction(("BdrGammaSetDefaults"));

    return RetCode;
}

/*--------------------------------------------------------------------------------
 * Description: This function will sets the  Gamma mode
 *
 * Arguments  :
 *                Channel         - Main/Pip
 *                Enable         - TRUE/FALSE
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
 *                On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_GAMMA_Enable(VPP_OBJ* vpp_obj,INT Channel, BOOL Enable)
{
    INT retStatus;
    union tag_GAMMA_CONTROL_REGISTER gammaCtrlReg;
    T32GAMMA_CFG0 stGammaCfg0;
    UINT32 regAddr;

    DbgEnterFunction(("BdrGammaEnable"));

    switch (Channel){
        case VPP_VOP:
            stGammaCfg0.u32 = vpp_obj->gamma.stGammaCfg0;
            regAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_GAMMA + RA_GAMMA_CFG0;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_VOP2:
            stGammaCfg0.u32 = vpp_obj->gamma.stGammaVop2Cfg0;
            regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2+ RA_VOP2_GAMMA + RA_GAMMA_CFG0;
            break;
#endif
        default:
            return MV_VPP_ERANGEPARAM;
            break;
    }
    gammaCtrlReg.Value = stGammaCfg0.uCFG0_GAMMA_CTRL_INT;
    gammaCtrlReg.Bits.GammaMainEnable = Enable;
    stGammaCfg0.uCFG0_GAMMA_CTRL_INT = gammaCtrlReg.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,regAddr , stGammaCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    if (Channel == VPP_VOP)
        vpp_obj->gamma.stGammaCfg0 = stGammaCfg0.u32;
    else
        vpp_obj->gamma.stGammaVop2Cfg0 = stGammaCfg0.u32;

    DbgExitFunction(("BdrGammaEnable"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will set the  Gamma Params
 *
 * Arguments  :
 *                pGammaParams     - Gamma Parameters
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_GAMMA_SetParams(VPP_OBJ* vpp_obj,INT Channel,PVPP_GAMMA_PARAMS pGammaParams)
{

    INT retStatus;
    union tag_GAMMA_CONTROL_REGISTER gammaCtrlReg;
    T32GAMMA_CFG0 stGammaCfg0;
    UINT32 regAddr;

    DbgEnterFunction(("BdrGammaSetParams"));

    switch (Channel)
    {
    case VPP_VOP:
        stGammaCfg0.u32 = vpp_obj->gamma.stGammaCfg0;
        regAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_GAMMA + RA_GAMMA_CFG0;
        break;
#ifdef ENABLE_GFX2_VOP2_PATH
    case VPP_VOP2:
        stGammaCfg0.u32 = vpp_obj->gamma.stGammaVop2Cfg0;
        regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2+ RA_VOP2_GAMMA + RA_GAMMA_CFG0;
        break;
#endif
    default:
        return MV_VPP_ERANGEPARAM;
        break;
    }

    gammaCtrlReg.Value = stGammaCfg0.uCFG0_GAMMA_CTRL_INT;
    gammaCtrlReg.Bits.BCbGammaEnable = pGammaParams->BCbGammaEnable;
    gammaCtrlReg.Bits.GYGammaEnable = pGammaParams->GYGammaEnable;
    gammaCtrlReg.Bits.RCrGammaEnable = pGammaParams->RCrGammaEnable;
    stGammaCfg0.uCFG0_GAMMA_CTRL_INT = gammaCtrlReg.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,regAddr, stGammaCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    if (Channel == VPP_VOP)
        vpp_obj->gamma.stGammaCfg0 = stGammaCfg0.u32;
    else
        vpp_obj->gamma.stGammaVop2Cfg0 = stGammaCfg0.u32;

    DbgExitFunction(("BdrGammaSetParams"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will load the Gamma LUT to gamma BCM buffer
 *
 * Arguments  :
 *                Channel         - Main/Pip
 *                pGammaLUTParams - Pointer to Gamma Params(Mode,Data Size and DisplayFrmt)
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_GAMMA_LoadLUT(VPP_OBJ* vpp_obj,INT Channel,
                            PVPP_GAMMA_LUT_PARAMS  pGammaLUTParams)
{
    UINT16 i;
    UINT32 *GmBcmBufPtr;
    union tag_GAMMA_CONTROL_REGISTER gammaCtrlReg;
    T32GAMMA_CFG0 stGammaCfg0;
    T32GAMMA_GAMMA_HDATA_G stGammaHGdata;
    T32GAMMA_GAMMA_HDATA_B stGammaHBdata;
    T32GAMMA_GAMMA_HDATA_R stGammaHRdata;
    T32tgWrap_CTRL1 vop_gm_ctrl;
    UINT32 regAddr, regaddr_bypass;
    UINT32 regValue;
    VPP_GAMMA_LUT_STRUCT *pstLoadGammaLut;

    ENUM_GAMMA_MODE ModeId = pGammaLUTParams->ModeId;

    DbgEnterFunction(("BdrGammaLoadLUT"));

    if (GM_CUSTOM == ModeId) {
        pstLoadGammaLut = vpp_obj->dv[Channel].gammaLutinfo.pstgammaLut;
    } else {
        pstLoadGammaLut = (VPP_GAMMA_LUT_STRUCT*)stgGammaLut_12bit[ModeId];
    }

    switch (Channel)
    {
    case VPP_VOP:
        stGammaCfg0.u32 = vpp_obj->gamma.stGammaCfg0;
        regAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_GAMMA;
        regaddr_bypass = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_CTRL1;
        GA_REG_WORD32_READ(regaddr_bypass , &vop_gm_ctrl.u32);
        vop_gm_ctrl.uCTRL1_gamma_bypass = 0;
        regValue = vop_gm_ctrl.u32;
        break;
#ifdef ENABLE_GFX2_VOP2_PATH
    case VPP_VOP2:
        stGammaCfg0.u32 = vpp_obj->gamma.stGammaVop2Cfg0;
        regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2+ RA_VOP2_GAMMA;
        vpp_obj->vop2_ctrl0.uCTRL0_gamma_bypass = 0;
        regaddr_bypass = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_CTRL0;
        regValue = vpp_obj->vop2_ctrl0.u32;
        break;
#endif
    default:
        return MV_VPP_ERANGEPARAM;
        break;
    }

    GmBcmBufPtr = vpp_obj->gamma.GmBcmBuf[Channel][vpp_obj->gamma.GmBcmBufWIdx[Channel]];

    if (ModeId == GM_OFF)
    {
        gammaCtrlReg.Value = stGammaCfg0.uCFG0_GAMMA_CTRL_INT;
        gammaCtrlReg.Bits.GammaMainEnable = FALSE;
        stGammaCfg0.uCFG0_GAMMA_CTRL_INT = gammaCtrlReg.Value;

        GAMMA_BCMBUF_WRITE(regAddr+RA_GAMMA_CFG0,stGammaCfg0.u32);
#if defined(VPP_DIAGS)
        /* flush data in D$ */
        CleanDCacheRegion(vpp_obj->gamma.GmBcmBuf[Channel][vpp_obj->gamma.GmBcmBufWIdx[Channel]], 8);
#endif

        /*update bcm buffer index*/
        vpp_obj->gamma.GmBcmBufRIdx[Channel] = vpp_obj->gamma.GmBcmBufWIdx[Channel];
        vpp_obj->gamma.GmBcmBufWIdx[Channel] = (vpp_obj->gamma.GmBcmBufWIdx[Channel] + 1) & 0x01;

        return (MV_VPP_OK);
    }

    GAMMA_BCMBUF_WRITE(regaddr_bypass,regValue);

    /*set gamma control register value & address to gamma bcm buffer*/
    gammaCtrlReg.Bits.GammaMainEnable = TRUE;
    gammaCtrlReg.Bits.BCbGammaEnable = TRUE;
    gammaCtrlReg.Bits.GYGammaEnable = TRUE;
    gammaCtrlReg.Bits.RCrGammaEnable = TRUE;

    stGammaCfg0.uCFG0_GAMMA_CTRL_INT = gammaCtrlReg.Value;
    stGammaCfg0.uCFG0_GAMMA_HCTRL_INT = 0;
    GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_CFG0, stGammaCfg0.u32);

    /*set gamma host control register value & address to gamma bcm buffer*/
    for(i = VPP_MAIN_GAMMA_TABLE_START;i < VPP_MAIN_GAMMA_TABLE_INDEX_END; i=i+2)
    {
        /*set gamma host address register value & address to gamma bcm buffer*/
        stGammaCfg0.uCFG0_GAMMA_HADDR = ((i/2) & 0x7FF);
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_CFG0, stGammaCfg0.u32);

        if (GM_INVERSE == ModeId)
        {
            stGammaHRdata.uGAMMA_HDATA_R_HDATA_R0 = 4095 - i*4;
            stGammaHGdata.uGAMMA_HDATA_G_HDATA_G0 = 4095 - i*4;
            stGammaHBdata.uGAMMA_HDATA_B_HDATA_B0 = 4095 - i*4;

            stGammaHRdata.uGAMMA_HDATA_R_HDATA_R1 = 4095 - (i+1)*4;
            stGammaHGdata.uGAMMA_HDATA_G_HDATA_G1 = 4095 - (i+1)*4;
            stGammaHBdata.uGAMMA_HDATA_B_HDATA_B1 = 4095 - (i+1)*4;
        }
        else
        {
            stGammaHRdata.uGAMMA_HDATA_R_HDATA_R0 = pstLoadGammaLut[i].ui16BData;
            stGammaHGdata.uGAMMA_HDATA_G_HDATA_G0 = pstLoadGammaLut[i].ui16RData;
            stGammaHBdata.uGAMMA_HDATA_B_HDATA_B0 = pstLoadGammaLut[i].ui16GData;

            stGammaHRdata.uGAMMA_HDATA_R_HDATA_R1 = pstLoadGammaLut[i+1].ui16BData;
            stGammaHGdata.uGAMMA_HDATA_G_HDATA_G1 = pstLoadGammaLut[i+1].ui16RData;
            stGammaHBdata.uGAMMA_HDATA_B_HDATA_B1 = pstLoadGammaLut[i+1].ui16GData;
        }
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_GAMMA_HDATA_B , stGammaHBdata.u32);
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_GAMMA_HDATA_G , stGammaHGdata.u32);
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_GAMMA_HDATA_R , stGammaHRdata.u32);
    }
    /* TBA : 1025th Data*/
    {
        /*set gamma host address register value & address to gamma bcm buffer*/
        stGammaCfg0.uCFG0_GAMMA_HADDR = (0x3FF); //FIXED for 1025
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_CFG0, stGammaCfg0.u32);

        if (GM_INVERSE == ModeId)
        {
            stGammaHRdata.uGAMMA_HDATA_R_HDATA_R0 = 0;
            stGammaHGdata.uGAMMA_HDATA_G_HDATA_G0 = 0;
            stGammaHBdata.uGAMMA_HDATA_B_HDATA_B0 = 0;
        }
        else
        {
            stGammaHRdata.uGAMMA_HDATA_R_HDATA_R0 = pstLoadGammaLut[VPP_MAIN_GAMMA_TABLE_INDEX_END].ui16BData;
            stGammaHGdata.uGAMMA_HDATA_G_HDATA_G0 = pstLoadGammaLut[VPP_MAIN_GAMMA_TABLE_INDEX_END].ui16RData;
            stGammaHBdata.uGAMMA_HDATA_B_HDATA_B0 = pstLoadGammaLut[VPP_MAIN_GAMMA_TABLE_INDEX_END].ui16GData;
        }
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_GAMMA_HDATA_B , stGammaHBdata.u32);
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_GAMMA_HDATA_G , stGammaHGdata.u32);
        GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_GAMMA_HDATA_R , stGammaHRdata.u32);
    }
    /*set gamma host control register value & address to gamma bcm buffer*/
    stGammaCfg0.uCFG0_GAMMA_HCTRL_INT = 1;
    GAMMA_BCMBUF_WRITE(regAddr + RA_GAMMA_CFG0, stGammaCfg0.u32);

    if (Channel == VPP_VOP)
        vpp_obj->gamma.stGammaCfg0 = stGammaCfg0.u32;
    else
        vpp_obj->gamma.stGammaVop2Cfg0 = stGammaCfg0.u32;

#if defined(VPP_DIAGS)
    CleanDCacheRegion(vpp_obj->gamma.GmBcmBuf[Channel][vpp_obj->gamma.GmBcmBufWIdx[Channel]], GM_BCM_BUFFER_SIZE);
#endif

    /*update bcm buffer index*/
    vpp_obj->gamma.GmBcmBufRIdx[Channel] = vpp_obj->gamma.GmBcmBufWIdx[Channel];
    vpp_obj->gamma.GmBcmBufWIdx[Channel] = (vpp_obj->gamma.GmBcmBufWIdx[Channel] + 1) & 0x01;

    DbgExitFunction(("BdrGammaLoadLUT"));

    return MV_VPP_OK;
}

/*--------------------------------------------------------------------------------
 * Description: This function will load the Gamma LUT to hardware
 *
 * Arguments  :
 *                Channel         - Main/Pip
 *                GmMode - gamma mode
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_Gamma_LoadRegister(VPP_OBJ* vpp_obj,INT Channel, INT GmMode)
{
    if (GmMode != GM_OFF)
        VPP_BCMBUF_WriteBlock(vpp_obj->pVbiBcmBuf,vpp_obj->gamma.GmBcmBuf[Channel][vpp_obj->gamma.GmBcmBufRIdx[Channel]], GM_BCM_BUFFER_SIZE);
    else
        VPP_BCMBUF_WriteBlock(vpp_obj->pVbiBcmBuf,vpp_obj->gamma.GmBcmBuf[Channel][vpp_obj->gamma.GmBcmBufRIdx[Channel]], 8);

    return  MV_VPP_OK;
}


/*--------------------------------------------------------------------------------
 * Description: This function will read the Gamma table
 *
 * Arguments  :
 *                Channel         - Main/Pip
 *                location - read location
 *                gammaVal[] - output value
 *
 * Returns    : On success returns MV_VPP_OK else error
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_Gamma_ReadValue(VPP_OBJ* vpp_obj,INT Channel, INT location, INT16 gammaVal[])
{
    T32GAMMA_CFG0 stGammaCfg0;
    UINT32 regAddr;
    T32GAMMA_GAMMA_CDATA_G stGammaCGdata;
    T32GAMMA_GAMMA_CDATA_B stGammaCBdata;
    T32GAMMA_GAMMA_CDATA_R stGammaCRdata;

    DbgEnterFunction(("BdrGammaLoadLUT"));

    switch (Channel)
    {
    case VPP_VOP:
        stGammaCfg0.u32 = vpp_obj->gamma.stGammaCfg0;
        regAddr = vpp_obj->base_addr + RA_Vpp_tgWrap0 + RA_tgWrap_GAMMA;
        break;
#ifdef ENABLE_GFX2_VOP2_PATH
    case VPP_VOP2:
        stGammaCfg0.u32 = vpp_obj->gamma.stGammaVop2Cfg0;
        regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2+ RA_VOP2_GAMMA;
        break;
#endif
    default:
        return MV_VPP_ERANGEPARAM;
        break;
    }
    /*Gamma Register for R/G/B has two Gamma data, So dividing location as did in write*/
    stGammaCfg0.uCFG0_GAMMA_HADDR = ((location / 2) & 0x7FF);
    stGammaCfg0.uCFG0_GAMMA_HCTRL_INT = 0;

    GA_REG_WORD32_WRITE(regAddr + RA_GAMMA_CFG0, stGammaCfg0.u32);

    GA_REG_WORD32_READ(regAddr + RA_GAMMA_GAMMA_HDATA_B,&stGammaCBdata.u32);

    GA_REG_WORD32_READ(regAddr + RA_GAMMA_GAMMA_CDATA_B,&stGammaCBdata.u32);
    GA_REG_WORD32_READ(regAddr + RA_GAMMA_GAMMA_CDATA_G,&stGammaCGdata.u32);
    GA_REG_WORD32_READ(regAddr + RA_GAMMA_GAMMA_CDATA_R,&stGammaCRdata.u32);

    gammaVal[0] = stGammaCBdata.uGAMMA_CDATA_B_CDATA_B0;
    gammaVal[1] = stGammaCBdata.uGAMMA_CDATA_B_CDATA_B1;
    gammaVal[2] = stGammaCGdata.uGAMMA_CDATA_G_CDATA_G0;
    gammaVal[3] = stGammaCGdata.uGAMMA_CDATA_G_CDATA_G1;
    gammaVal[4] = stGammaCRdata.uGAMMA_CDATA_R_CDATA_R0;
    gammaVal[5] = stGammaCRdata.uGAMMA_CDATA_R_CDATA_R1;

    stGammaCfg0.uCFG0_GAMMA_HCTRL_INT = 1;
    GA_REG_WORD32_WRITE(regAddr + RA_GAMMA_CFG0, stGammaCfg0.u32);

    DbgExitFunction(("BdrGammaReadValue"));

    return  MV_VPP_OK;
}
