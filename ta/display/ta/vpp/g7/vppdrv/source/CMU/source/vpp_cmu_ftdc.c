/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_FTDC_C_

#include "vpp.h"

#include "vpp_cmu_ftdc.h"
#include "vpp_cmu_ftdcpriv.h"
#include "vpp_cmu_ftdccfg.h"

static int LoadFleshToneCoefficents(VPP_OBJ* vpp_obj,VPP_FTDC_MODE Mode,VPP_CMU_INPUT_SIGNAL_TYPE Input);
static int LoadFleshToneOffsets(VPP_OBJ* vpp_obj,VPP_FTDC_MODE Mode,VPP_CMU_INPUT_SIGNAL_TYPE Input);

/*-----------------------------------------------------------------------------
 * Functions
 *-----------------------------------------------------------------------------
 */
/*------------------------------------------------------------------------
 * Description: This API sets the default values for FTDC
 *
 * Arguments  : Channel: Input channel type
 *                       CmuInputType: Input type (SD/HD)
 *
 * Returns    : On success returns MV_VPP_OK
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_FTDC_SetDefaults(VPP_OBJ* vpp_obj,
            VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType)
{
    INT retStatus = MV_VPP_OK;
    VPP_CMU_FTDC_PARAMS FtdcParams;

    DbgEnterFunction(("BdrFtdcClose"));

    gCmuFtdcInputType[0] = CmuInputType;

    VPP_CMU_FTDC_Enable(vpp_obj, VPP_MAIN, FALSE);
    VPP_CMU_FTDC_Enable(vpp_obj, VPP_PIP, FALSE);

    FtdcParams.OutChromaLow = gCmuFtdcOutChromaLow[CmuInputType];
    FtdcParams.InChromaLow = gCmuFtdcInChromaLow[CmuInputType];
    FtdcParams.OutChromaHigh = gCmuFtdcOutChromaHigh[CmuInputType];
    FtdcParams.InChromaHigh = gCmuFtdcInChromaHigh[CmuInputType];
    FtdcParams.HueInLow = gCmuFtdcInLow[CmuInputType];
    FtdcParams.HueInHigh = gCmuFtdcInHigh[CmuInputType];
    FtdcParams.HueOutLow = gCmuFtdcOutLow[CmuInputType];
    FtdcParams.HueOutHigh = gCmuFtdcOutHigh[CmuInputType];
    FtdcParams.LumaLow = gCmuFtdcYthLow[CmuInputType];
    FtdcParams.LumaHigh = gCmuFtdcYthHigh[CmuInputType];
    FtdcParams.HueWidth = gCmuFtdcHueWidth[CmuInputType];
    FtdcParams.ChromaWidth = gCmuFtdcChromaWidth[CmuInputType];
    VPP_CMU_FTDC_SetParams(vpp_obj, &FtdcParams);

    DbgExitFunction(("BdrFtdcClose"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will Enable FTDC
 *
 * Arguments  :
 *              Enable (Bool)  - TRUE/FALSE
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_FTDC_Enable(VPP_OBJ* vpp_obj,
            VPP_CHANNEL_TYPE Channel, BOOL Enable)
{
    INT retStatus = MV_VPP_OK;
    T32FTDC_CFG0 stFtdcCfg0;

    DbgEnterFunction(("BdrFtdcEnable"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
#endif

    if (gCmuFtdcInputType[Channel] == VPP_CMU_RGB_444)
        return retStatus;

    stFtdcCfg0.u32 = 0;
    /* Load current values */
    stFtdcCfg0.uCFG0_INLOW = vpp_obj->cmu.FTDCParam.InChromaLow;
    stFtdcCfg0.uCFG0_OUTLOW = vpp_obj->cmu.FTDCParam.OutChromaLow;
    stFtdcCfg0.uCFG0_INHIGH = vpp_obj->cmu.FTDCParam.InChromaHigh;
    stFtdcCfg0.uCFG0_M_EN = vpp_obj->cmu.FTDCParam.FtdcEnable;
    stFtdcCfg0.uCFG0_P_EN = vpp_obj->cmu.FTDCParam.FtdcPipEnable;
    /* Updated the new value */
    switch (Channel) {
        case VPP_MAIN:
            stFtdcCfg0.uCFG0_M_EN = Enable;
            break;
        case VPP_PIP:
            stFtdcCfg0.uCFG0_P_EN = Enable;
            break;
        default:
            break;
    }
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG0, stFtdcCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    DbgExitFunction(("BdrFtdcEnable"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will set Parameters of FTDC
 *
 * Arguments  :
 *              pFtdcParams  - Pointer to params
 *                             structure.
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_FTDC_SetParams(VPP_OBJ* vpp_obj, PVPP_CMU_FTDC_PARAMS pFtdcParams)
{
    INT retStatus = MV_VPP_OK;
    T32FTDC_CFG0 stFtdcCfg0;
    T32FTDC_CFG1 stFtdcCfg1;
    T32FTDC_CFG2 stFtdcCfg2;
    T32FTDC_CFG3 stFtdcCfg3;
    T32FTDC_CFG4 stFtdcCfg4;

    DbgEnterFunction(("BdrFtdcSetParams"));

    GA_REG_WORD32_READ(vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG1, &stFtdcCfg1.u32);

    /* Load current values */
    stFtdcCfg0.uCFG0_M_EN = vpp_obj->cmu.FTDCParam.FtdcEnable;
    stFtdcCfg0.uCFG0_P_EN = vpp_obj->cmu.FTDCParam.FtdcPipEnable;
    /* Updated the new value */
    stFtdcCfg0.uCFG0_INLOW = pFtdcParams->HueInLow;
    stFtdcCfg0.uCFG0_OUTLOW = pFtdcParams->HueOutLow;
    stFtdcCfg0.uCFG0_INHIGH = pFtdcParams->HueInHigh;
    stFtdcCfg1.uCFG1_OUTHIGH = pFtdcParams->HueOutHigh;
    stFtdcCfg1.uCFG1_YTH_LOW = pFtdcParams->LumaLow;
    stFtdcCfg1.uCFG1_YTH_HIGH = pFtdcParams->LumaHigh;

    stFtdcCfg2.uCFG2_OUTCHROMA_LOW = pFtdcParams->OutChromaLow;
    stFtdcCfg2.uCFG2_INCHROMA_LOW = pFtdcParams->InChromaLow;
    stFtdcCfg3.uCFG3_OUTCHROMA_HIGH = pFtdcParams->OutChromaHigh;
    stFtdcCfg3.uCFG3_INCHROMA_HIGH = pFtdcParams->InChromaHigh;

    stFtdcCfg4.uCFG4_HUE_WIDTH     = pFtdcParams->HueWidth;
    stFtdcCfg4.uCFG4_CHROMA_WIDTH  = pFtdcParams->ChromaWidth;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG0, stFtdcCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG1, stFtdcCfg1.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG2, stFtdcCfg2.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG3, stFtdcCfg3.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG4, stFtdcCfg4.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    DbgExitFunction(("BdrFtdcSetParams"));

    return retStatus;
}


/*--------------------------------------------------------------------------------
 * Description: This function will set the FTDC correction mode.
 *
 * Arguments  :
 *              Mode - VPP_FTDC_MODE
 *
 * Returns    : On success returns BDR_ERR_OK .
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_FTDC_SetMode(VPP_OBJ* vpp_obj,VPP_FTDC_MODE Mode)
{
    INT retStatus = MV_VPP_OK;

    DbgEnterFunction(("BdrFtdcSetMode"));

    /*
    * Flesh Tone correction Coefficents and Offsets
    */
    LoadFleshToneCoefficents(vpp_obj,Mode,gCmuFtdcInputType[0]);
    LoadFleshToneOffsets(vpp_obj,Mode,gCmuFtdcInputType[0]);

    FtdcMode = Mode;

    DbgExitFunction(("BdrFtdcSetMode"));

    return retStatus;
}

/*------------------------------------------------------------------------------*/
static int LoadFleshToneCoefficents(VPP_OBJ* vpp_obj,VPP_FTDC_MODE Mode,VPP_CMU_INPUT_SIGNAL_TYPE Input)
{
    UCHAR i = 0;
    INT retStatus = MV_VPP_OK;

    T32FTDC_CFG5 stFtdcCfg5;

    stFtdcCfg5.uCFG5_COEFF00 = gCmuFtdcCoeffRegion[Mode][Input][i++];
    stFtdcCfg5.uCFG5_COEFF01 = gCmuFtdcCoeffRegion[Mode][Input][i++];
    stFtdcCfg5.uCFG5_COEFF10 = gCmuFtdcCoeffRegion[Mode][Input][i++];
    stFtdcCfg5.uCFG5_COEFF11 = gCmuFtdcCoeffRegion[Mode][Input][i++];
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG5, stFtdcCfg5.u32);

    return retStatus;
}

/*------------------------------------------------------------------------------*/

static int LoadFleshToneOffsets(VPP_OBJ* vpp_obj,VPP_FTDC_MODE Mode,VPP_CMU_INPUT_SIGNAL_TYPE Input)
{
    UCHAR i = 0;
    INT retStatus = MV_VPP_OK;

    T32FTDC_CFG6 stFtdcCfg6;

    stFtdcCfg6.uCFG6_OFF0 = gCmuFtdcOffsetRegion[Mode][Input][i++];
    stFtdcCfg6.uCFG6_OFF1 = gCmuFtdcOffsetRegion[Mode][Input][i++];
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + RA_CMU_FTDC + RA_FTDC_CFG6, stFtdcCfg6.u32);

    return retStatus;
}
