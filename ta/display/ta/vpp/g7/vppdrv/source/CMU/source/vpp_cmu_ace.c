/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_ACE_

#include "vpp.h"
#include "vpp_mp.h"

#include "vpp_cmu_ace.h"
#include "vpp_cmu_acepriv.h"
#include "vpp_cmu_acecfg.h"

/*----------------------------------------------------------------------------
 * Description: This function will set the default values for the Ace registers.
 *
 * Returns    : On success returns MV_VPP_OK
 *
 * Notes      : None
 *----------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetDefaults(VPP_OBJ* vpp_obj,
                            VPP_CHANNEL_TYPE Channel,
                            VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType)
{
    INT RetCode = MV_VPP_OK;

    DbgEnterFunction(("BdrAceSetDefaults"));

    CmuAceInputType[Channel] = CmuInputType;

    RetCode = VPP_CMU_ACE_SetParams(vpp_obj,Channel,0,
                  VPP_CMU_ACE_RANGE_16_236,0,TRUE);

    if (RetCode != MV_VPP_OK)
        return RetCode;

    DbgExitFunction(("BdrAceSetDefaults"));

    return RetCode;
}

/*--------------------------------------------------------------------------------
 * Description: This function will enable/disable the ace module
 *
 * Arguments  :
 *              Channel - Main/Pip/Both
 *              bool - True/False
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_Enable(VPP_OBJ *vpp_obj,VPP_CHANNEL_TYPE Channel, BOOL Enable)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    AceEnVal instEnableAce;
    T32ACE_CFG0 stAceCfg0;
    T32ACE_CFG1 stAceCfg1;

    DbgEnterFunction(("BdrAceSetColorCompensationLevel"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
#endif

    cmuAceAddr = RA_CMU_ACE;
    stAceCfg0.uCFG0_WFG1 = vpp_obj->cmu.ACEParam.Params.WeightingFactor1;
    stAceCfg0.uCFG0_WFG2 = vpp_obj->cmu.ACEParam.Params.WeightingFactor2;
    stAceCfg0.uCFG0_WFG3 = vpp_obj->cmu.ACEParam.Params.WeightingFactor3;
    stAceCfg1.uCFG1_GB2  = vpp_obj->cmu.ACEParam.Params.GuardBand2;
    instEnableAce.EnAce.en = vpp_obj->cmu.ACEParam.Params.Enable;
    instEnableAce.EnAce.cc = vpp_obj->cmu.ACEParam.Params.ColorCompensationLevel;
    instEnableAce.EnAce.crush =  vpp_obj->cmu.ACEParam.Params.Crush;

    instEnableAce.EnAce.en = Enable;
    stAceCfg0.uCFG0_EN = instEnableAce.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG0, stAceCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG1, stAceCfg1.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    vpp_obj->cmu.ACEParam.Params.Enable = instEnableAce.EnAce.en;

    DbgExitFunction(("BdrAceSetColorCompensationLevel"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will set Parameters of Ace block
 *
 * Arguments  :
 *              pAceParams  - Pointer to params structure
 *                     This structure can be NULL if DefaultVal setting is TRUE
 *              Range/Mode - This range/mode is defined when DefaultVal is TRUE.
 *                     In other case this will be -1/Max.
 *              DefautVal - Boolean value for setting Default values
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetParams(VPP_OBJ *vpp_obj, VPP_CHANNEL_TYPE Channel,
                          PVPP_CMU_ACE_PARAMS pAceParams,
                          VPP_CMU_ACE_RANGE Range,
                          VPP_CMU_ACE_MODE Mode, BOOL DefautVal)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    AceEnVal instEnableAce;
    T32ACE_CFG0 stAceCfg0;
    T32ACE_CFG1 stAceCfg1;
    T32ACE_CFG2 stAceCfg2;
    T32ACE_CFG3 stAceCfg3;
    T32ACE_CFG4 stAceCfg4;
    T32ACE_CFG5 stAceCfg5;
    T32ACE_CFG6 stAceCfg6;

    DbgEnterFunction(("BdrAceSetParams"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
#endif

    if (CmuAceInputType[Channel] == VPP_CMU_RGB_444)
        return retStatus;

    if (Range < 0 || Range >= VPP_CMU_ACE_MAX_RANGE)
        return MV_VPP_EBADPARAM;

    if (Mode < 0 || Mode >= VPP_CMU_ACE_MAX_MODE)
        return MV_VPP_EBADPARAM;

    if (DefautVal) {
        pAceParams = (PVPP_CMU_ACE_PARAMS)&gCmuAceInputParams[Range][Mode];
    }

    cmuAceAddr = RA_CMU_ACE;

    instEnableAce.EnAce.en = pAceParams->Enable;
    instEnableAce.EnAce.cc = pAceParams->ColorCompensationLevel;
    instEnableAce.EnAce.crush =  pAceParams->Crush;

    stAceCfg0.uCFG0_EN = instEnableAce.Value;
    stAceCfg0.uCFG0_WFG1 = pAceParams->WeightingFactor1;
    stAceCfg0.uCFG0_WFG2 = pAceParams->WeightingFactor2;
    stAceCfg0.uCFG0_WFG3 = pAceParams->WeightingFactor3;

    stAceCfg1.uCFG1_GB2 = pAceParams->GuardBand2;
    stAceCfg1.uCFG1_GB3 = pAceParams->GuardBand3;

    stAceCfg2.uCFG2_MS2 = pAceParams->MaximumSlope2;
    stAceCfg2.uCFG2_MS3 = pAceParams->MaximumSlope3;
    stAceCfg2.uCFG2_MS1 = pAceParams->MaximumSlope1;

    stAceCfg3.uCFG3_TH0 = pAceParams->ThresHold0;
    stAceCfg3.uCFG3_TH1 = pAceParams->ThresHold1;

    stAceCfg4.uCFG4_TH2 = pAceParams->ThresHold2;
    stAceCfg4.uCFG4_TH3 = pAceParams->ThresHold3;

    stAceCfg5.uCFG5_TH4 = pAceParams->ThresHold4;
    stAceCfg5.uCFG5_TH5 = pAceParams->ThresHold5;

    stAceCfg6.uCFG6_OP0 = pAceParams->OutPut0;
    stAceCfg6.uCFG6_OP5 = pAceParams->OutPut5;
    /* uCFG4_MODE  will set correctly while switching the CMU position */
    stAceCfg6.uCFG6_MODE = vpp_obj->cmu.ACEParam.Params.Mode;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG0, stAceCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG1, stAceCfg1.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG2, stAceCfg2.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG3, stAceCfg3.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG4, stAceCfg4.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG5, stAceCfg5.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG6, stAceCfg6.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    DbgExitFunction(("BdrAceSetParams"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will set the color compensation level
 *
 * Arguments  :
 *              Channel - Main/Pip
 *              Value - Level value
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetColorCompensationLevel(VPP_OBJ *vpp_obj,
                                          VPP_CHANNEL_TYPE Channel, UCHAR Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    AceEnVal instEnableAce;
    T32ACE_CFG0 stAceCfg0;

    DbgEnterFunction(("BdrAceSetColorCompensationLevel"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
#endif

    cmuAceAddr = RA_CMU_ACE;

    stAceCfg0.uCFG0_WFG1 = vpp_obj->cmu.ACEParam.Params.WeightingFactor1;
    stAceCfg0.uCFG0_WFG2 = vpp_obj->cmu.ACEParam.Params.WeightingFactor2;
    stAceCfg0.uCFG0_WFG3 = vpp_obj->cmu.ACEParam.Params.WeightingFactor3;
    instEnableAce.EnAce.en = vpp_obj->cmu.ACEParam.Params.Enable;
    instEnableAce.EnAce.cc = vpp_obj->cmu.ACEParam.Params.ColorCompensationLevel;
    instEnableAce.EnAce.crush =  vpp_obj->cmu.ACEParam.Params.Crush;

    instEnableAce.EnAce.cc = Value;
    stAceCfg0.uCFG0_EN = instEnableAce.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG0, stAceCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.ACEParam.Params.ColorCompensationLevel = instEnableAce.EnAce.cc;

    DbgExitFunction(("BdrAceSetColorCompensationLevel"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will Set the weighting factors for the ACE in CMU
 *
 * Arguments  :
 *              Channel - Main/Pip
 *              Region - ACE region (1/2/3)
 *              Value - WF
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetWeightingFactor(VPP_OBJ *vpp_obj,
                                   VPP_CHANNEL_TYPE Channel,
                                   VPP_CMU_ACE_REGION Region,
                                   UCHAR Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    AceEnVal instEnableAce;
    T32ACE_CFG0 stAceCfg0;
    T32ACE_CFG1 stAceCfg1;

    DbgEnterFunction(("BdrAceSetWeightingFactor"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((Region >= VPP_CMU_ACE_REGION1 && Region <= VPP_CMU_ACE_REGION3)))
        return MV_VPP_EBADPARAM;
#endif
    cmuAceAddr = RA_CMU_ACE;
    stAceCfg0.uCFG0_WFG1 = vpp_obj->cmu.ACEParam.Params.WeightingFactor1;
    stAceCfg0.uCFG0_WFG2 = vpp_obj->cmu.ACEParam.Params.WeightingFactor2;
    stAceCfg0.uCFG0_WFG3 = vpp_obj->cmu.ACEParam.Params.WeightingFactor3;
    stAceCfg1.uCFG1_GB2  = vpp_obj->cmu.ACEParam.Params.GuardBand2;
    instEnableAce.EnAce.en = vpp_obj->cmu.ACEParam.Params.Enable;
    instEnableAce.EnAce.cc = vpp_obj->cmu.ACEParam.Params.ColorCompensationLevel;
    instEnableAce.EnAce.crush =  vpp_obj->cmu.ACEParam.Params.Crush;
    stAceCfg0.uCFG0_EN = instEnableAce.Value;

    switch (Region) {
        case VPP_CMU_ACE_REGION1:
            stAceCfg0.uCFG0_WFG1 = Value;
            break;
        case VPP_CMU_ACE_REGION2:
            stAceCfg0.uCFG0_WFG2 = Value;
            break;
        case VPP_CMU_ACE_REGION3:
            stAceCfg0.uCFG0_WFG3 = Value;
            break;
        default:
            break;
    }

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG0, stAceCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG1, stAceCfg1.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    switch (Region) {
        case VPP_CMU_ACE_REGION1:
            vpp_obj->cmu.ACEParam.Params.WeightingFactor1 = Value;
            break;
        case VPP_CMU_ACE_REGION2:
            vpp_obj->cmu.ACEParam.Params.WeightingFactor2 = Value;
            break;
        case VPP_CMU_ACE_REGION3:
            vpp_obj->cmu.ACEParam.Params.WeightingFactor3 = Value;
            break;
        default:
            break;
    }

    DbgExitFunction(("BdrAceSetWeightingFactor"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will Set the Threshold values for the ACE in CMU
 *
 * Arguments  :
 *              Channel - Main/Pip
 *              ThLevel - Threshold levels (0 to 5)
 *              Value - Threshold value
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */

INT VPP_CMU_ACE_SetThreshHold(VPP_OBJ *vpp_obj,VPP_CHANNEL_TYPE Channel,
                           VPP_CMU_ACE_THOLD ThLevel, UCHAR Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    T32ACE_CFG4 stAceCfg4;
    T32ACE_CFG3 stAceCfg3;
    T32ACE_CFG5 stAceCfg5;

    DbgEnterFunction(("BdrAceSetThreshHold"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((ThLevel >= VPP_CMU_ACE_THOLD0 && ThLevel <= VPP_CMU_ACE_THOLD5)))
        return MV_VPP_EBADPARAM;
#endif
    cmuAceAddr = RA_CMU_ACE;

    if (ThLevel <= VPP_CMU_ACE_THOLD3) {
        stAceCfg3.uCFG3_TH0 = vpp_obj->cmu.ACEParam.Params.ThresHold0;
        stAceCfg3.uCFG3_TH1 = vpp_obj->cmu.ACEParam.Params.ThresHold1;
        stAceCfg4.uCFG4_TH2 = vpp_obj->cmu.ACEParam.Params.ThresHold2;
        stAceCfg4.uCFG4_TH3 = vpp_obj->cmu.ACEParam.Params.ThresHold3;
        switch (ThLevel) {
            case VPP_CMU_ACE_THOLD0:
                stAceCfg3.uCFG3_TH0 = Value;
                break;
            case VPP_CMU_ACE_THOLD1:
                stAceCfg3.uCFG3_TH1 = Value;
                break;
            case VPP_CMU_ACE_THOLD2:
                stAceCfg4.uCFG4_TH2 = Value;
                break;
            case VPP_CMU_ACE_THOLD3:
                stAceCfg4.uCFG4_TH3 = Value;
                break;
            default:
                break;
        }
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG3, stAceCfg3.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG4, stAceCfg4.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        switch (ThLevel) {
            case VPP_CMU_ACE_THOLD0:
                vpp_obj->cmu.ACEParam.Params.ThresHold0 = Value;
                break;
            case VPP_CMU_ACE_THOLD1:
                vpp_obj->cmu.ACEParam.Params.ThresHold1 = Value;
                break;
            case VPP_CMU_ACE_THOLD2:
                vpp_obj->cmu.ACEParam.Params.ThresHold2 = Value;
                break;
            case VPP_CMU_ACE_THOLD3:
                vpp_obj->cmu.ACEParam.Params.ThresHold3 = Value;
                break;
            default:
                break;
        }
    } else {
        stAceCfg5.uCFG5_TH4 = vpp_obj->cmu.ACEParam.Params.ThresHold4;
        stAceCfg5.uCFG5_TH5 = vpp_obj->cmu.ACEParam.Params.ThresHold5;
        switch (ThLevel) {
            case VPP_CMU_ACE_THOLD4:
                stAceCfg5.uCFG5_TH4 = Value;
                break;
            case VPP_CMU_ACE_THOLD5:
                stAceCfg5.uCFG5_TH5 = Value;
                break;
            default:
                break;
        }
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG5, stAceCfg5.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        switch (ThLevel) {
            case VPP_CMU_ACE_THOLD4:
                vpp_obj->cmu.ACEParam.Params.ThresHold4 = Value;
                break;
            case VPP_CMU_ACE_THOLD5:
                vpp_obj->cmu.ACEParam.Params.ThresHold5 = Value;
                break;
            default:
                break;
        }
    }

    DbgExitFunction(("BdrAceSetThreshHold"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will Set the Output configuration values for the ACE
 *
 * Arguments  :
 *              Channel - Main/Pip
 *              Output - Type of Output (Op0/Op5)
 *              Value - Output values
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetOutput(VPP_OBJ *vpp_obj,
                          VPP_CHANNEL_TYPE Channel,
                          VPP_CMU_ACE_OUTPUT OutPut,
                          USHORT Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    T32ACE_CFG6 stAceCfg6;

    DbgEnterFunction(("BdrAceSetOutput"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((OutPut >= VPP_CMU_ACE_OUTPUT0 && OutPut <= VPP_CMU_ACE_OUTPUT5)))
        return MV_VPP_EBADPARAM;
#endif

    cmuAceAddr = RA_CMU_ACE;
    stAceCfg6.uCFG6_OP0  = vpp_obj->cmu.ACEParam.Params.OutPut0;
    stAceCfg6.uCFG6_OP5  = vpp_obj->cmu.ACEParam.Params.OutPut5;
    stAceCfg6.uCFG6_MODE = vpp_obj->cmu.ACEParam.Params.Mode;
    switch (OutPut) {
        case VPP_CMU_ACE_OUTPUT0:
            stAceCfg6.uCFG6_OP0 = Value;
            break;
        case VPP_CMU_ACE_OUTPUT5:
            stAceCfg6.uCFG6_OP5 = Value;
            break;
        default:
            break;
    }
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG6, stAceCfg6.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    switch (OutPut) {
        case VPP_CMU_ACE_OUTPUT0:
            vpp_obj->cmu.ACEParam.Params.OutPut0 = Value;
            break;
        case VPP_CMU_ACE_OUTPUT5:
            vpp_obj->cmu.ACEParam.Params.OutPut5 = Value;
            break;
        default:
            break;
    }

    DbgExitFunction(("BdrAceSetOutput"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will Set the Guard Band values for the ACE in CMU
 *
 * Arguments  :
 *              Channel - Main/Pip
 *              GuardBand - Gaurd Band regions
 *              Value - Guard band Value
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetGuardBand(VPP_OBJ *vpp_obj,
                             VPP_CHANNEL_TYPE Channel,
                             VPP_CMU_ACE_GUARDBAND GuardBand,
                             UCHAR Value)
{
    INT retStatus = MV_VPP_OK;
    AceEnVal instEnableAce;
    UINT32 cmuAceAddr = 0x0;
    T32ACE_CFG0 stAceCfg0;
    T32ACE_CFG1 stAceCfg1;

    DbgEnterFunction(("BdrAceSetGuardBand"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((GuardBand >= VPP_CMU_ACE_GUARDBAND2 && GuardBand <= VPP_CMU_ACE_GUARDBAND3)))
        return MV_VPP_EBADPARAM;
#endif

    cmuAceAddr = RA_CMU_ACE;
    switch (GuardBand) {
        case VPP_CMU_ACE_GUARDBAND2:
            stAceCfg0.uCFG0_WFG1 = vpp_obj->cmu.ACEParam.Params.WeightingFactor1;
            stAceCfg0.uCFG0_WFG2 = vpp_obj->cmu.ACEParam.Params.WeightingFactor2;
            stAceCfg0.uCFG0_WFG3 = vpp_obj->cmu.ACEParam.Params.WeightingFactor3;
            instEnableAce.EnAce.en = vpp_obj->cmu.ACEParam.Params.Enable;
            instEnableAce.EnAce.cc = vpp_obj->cmu.ACEParam.Params.ColorCompensationLevel;
            instEnableAce.EnAce.crush =  vpp_obj->cmu.ACEParam.Params.Crush;
            stAceCfg0.uCFG0_EN = instEnableAce.Value;
            stAceCfg1.uCFG1_GB2  = Value;
            stAceCfg1.uCFG1_GB3  = vpp_obj->cmu.ACEParam.Params.GuardBand3;
            retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG0, stAceCfg0.u32);
            if (MV_VPP_OK != retStatus)
                return retStatus;
            retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG1, stAceCfg1.u32);
            if (MV_VPP_OK != retStatus)
                return retStatus;
            vpp_obj->cmu.ACEParam.Params.GuardBand2 = Value;
            break;
        case VPP_CMU_ACE_GUARDBAND3:
            stAceCfg1.uCFG1_GB3 = Value;
            stAceCfg1.uCFG1_GB2  = vpp_obj->cmu.ACEParam.Params.GuardBand2;
            retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG1, stAceCfg1.u32);
            if (MV_VPP_OK != retStatus)
                return retStatus;
            vpp_obj->cmu.ACEParam.Params.GuardBand3 = Value;
            break;
        default:
            break;
    }

    DbgExitFunction(("BdrAceSetGuardBand"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will Set the Maximum slope values for the ACE in CMU
 *
 * Arguments  :
 *              Channel - Main/Pip
 *              Maxslope - Maximum Slope levels (1/2/3)
 *              Value - Maximum Slope Value
 *
 * Returns    : On success returns MV_VPP_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetMaxSlope(VPP_OBJ *vpp_obj,
                            VPP_CHANNEL_TYPE Channel,
                            VPP_CMU_ACE_MAXSLOPE Maxslope,
                            UCHAR Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuAceAddr = 0x0;
    T32ACE_CFG2 stAceCfg2;

    DbgEnterFunction(("BdrAceSetMaxSlope"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((Maxslope >= VPP_CMU_ACE_MAXSLOPE1 && Maxslope <= VPP_CMU_ACE_MAXSLOPE3)))
        return MV_VPP_EBADPARAM;
#endif

    cmuAceAddr = RA_CMU_ACE;
    stAceCfg2.uCFG2_MS1 = vpp_obj->cmu.ACEParam.Params.MaximumSlope1;
    stAceCfg2.uCFG2_MS2 = vpp_obj->cmu.ACEParam.Params.MaximumSlope2;
    stAceCfg2.uCFG2_MS3 = vpp_obj->cmu.ACEParam.Params.MaximumSlope3;
    switch (Maxslope) {
        case VPP_CMU_ACE_MAXSLOPE1:
            stAceCfg2.uCFG2_MS1 = Value;
            break;
        case VPP_CMU_ACE_MAXSLOPE2:
            stAceCfg2.uCFG2_MS2 = Value;
            break;
        case VPP_CMU_ACE_MAXSLOPE3:
            stAceCfg2.uCFG2_MS3 = Value;
            break;
        default:
            break;
    }
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuAceAddr + RA_ACE_CFG2, stAceCfg2.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    switch (Maxslope) {
        case VPP_CMU_ACE_MAXSLOPE1:
            vpp_obj->cmu.ACEParam.Params.MaximumSlope1 = Value;
            break;
        case VPP_CMU_ACE_MAXSLOPE2:
            vpp_obj->cmu.ACEParam.Params.MaximumSlope2 = Value;
            break;
        case VPP_CMU_ACE_MAXSLOPE3:
            vpp_obj->cmu.ACEParam.Params.MaximumSlope3 = Value;
            break;
        default:
            break;
    }

    DbgExitFunction(("BdrAceSetMaxSlope"));

    return retStatus;
}

