/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_HUESAT_C_

#include "vpp.h"
#include "vpp_cmu_huesat.h"
#include "vpp_cmu_huesatpriv.h"
#include "vpp_cmu_huesatcfg.h"

static INT VPP_CMU_HUESAT_SetHSInputSignalType(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                              VPP_CMU_INPUT_SIGNAL_TYPE Input);
/*-----------------------------------------------------------------------------
 * Functions
 *-----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 * Description: This function will set the default values for the Huesat registers.
 *
 * Arguments  : Channel - Input channel number.
 *
 * Returns    : On success returns MV_VPP_OK
 *
 * Notes      : None
 *----------------------------------------------------------------------------
 */
INT VPP_CMU_HUESAT_SetDefaults(VPP_OBJ* vpp_obj,
            VPP_CHANNEL_TYPE Channel,
            VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType)
{
    INT RetCode = MV_VPP_OK;

    DbgEnterFunction(("BdrHsSetDefaults"));

    CmuHueSatInputType[Channel] = CmuInputType;

    /* Default values to all the registers */

    RetCode = VPP_CMU_HUESAT_SetHueSatControls(vpp_obj,Channel,
                                        VPP_CMU_HS_ENABLE,TRUE);
    RetCode = VPP_CMU_HUESAT_SetHueSatControls(vpp_obj,Channel,
                                        VPP_CMU_INTELLISAT_ENABLE,TRUE);
    RetCode = VPP_CMU_HUESAT_SetHueSatControls(vpp_obj,Channel,
                        VPP_CMU_FLESH_TAPER_TONE,TRUE);
    RetCode = VPP_CMU_HUESAT_SetHSInputSignalType(vpp_obj,Channel,gCmuHueSatInput);
    RetCode = VPP_CMU_HUESAT_SetHSIcrModeParams(vpp_obj, Channel,
                                   VPP_CMU_GLOBALSAT_CHANGE, 0, gCmuSaturation);
    RetCode = VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj,Channel,gCmuHueSatIcrMode,NULL);

    DbgExitFunction(("BdrHsSetDefaults"));

    return RetCode;
}

/*------------------------------------------------------------------------
 * Description: This function will set Parameters of this block
 *
 * Arguments  : pHueSatParams  - Pointer to params
 *                                       structure.
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_HUESAT_SetHueSatAdjustParams(VPP_OBJ* vpp_obj,
                       VPP_CHANNEL_TYPE Channel,
                       PVPP_CMU_HUESAT_PARAMS pHueSatParams)
{
    INT retStatus = MV_VPP_OK;
    UINT8 i;
    VPP_CMU_ICR_USER_MODE UsrMode;
    UINT32 cmuHsAddr = 0x0;
    union tagCMU_HS_ENABLE_REGISTER enableReg;
    T32HS_CFG0 stHsCfg0;
    T32HS_CFG10 stHsCfg10;

    DbgEnterFunction(("BdrHueSatSetHueSatAdjustParams"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (pHueSatParams == NULL)
        return MV_VPP_EBADPARAM;
#endif

    stHsCfg0.u32 = vpp_obj->cmu.stHsCfg0;
    stHsCfg10.u32 = vpp_obj->cmu.stHsCfg10;
    cmuHsAddr = RA_CMU_HS;

    enableReg.Bits.HSEnable = pHueSatParams->SatEnable;
    enableReg.Bits.IntelliSatEnable = pHueSatParams->IntelliSatEnable;
    enableReg.Bits.FleshToneTaperEnable = pHueSatParams->FleshToneTaperEnable;

    stHsCfg0.uCFG0_EN = enableReg.Value;
    stHsCfg10.uCFG10_GL_SAT = pHueSatParams->GlobalSaturation;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG0, stHsCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg0 = stHsCfg0.u32;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG10, stHsCfg10.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg10 = stHsCfg10.u32;

    for (i = 0; i < 14; i++) {
        UsrMode.Axis[i] =  gCmuModeTable[0][0][0][i];
        UsrMode.Hue[i]  =  gCmuModeTable[0][0][1][i];
        UsrMode.Sat[i]  =  gCmuModeTable[0][0][2][i];
    }

    VPP_CMU_HUESAT_SetHSIcrMode(vpp_obj,Channel,pHueSatParams->IcrMode,&UsrMode);

    DbgExitFunction(("BdrHueSatSetHueSatAdjustParams"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will set Parameters like Huesat,Intellisat,
 *              Fleshtone and GlobalSaturation
 * Arguments  :
 *              pHSCtrls  - variable for calling the enum values.
 *              Enable    - TRUE/FALSE
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
 INT VPP_CMU_HUESAT_SetHueSatControls(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                                  VPP_CMU_HUESAT_CTRL_PARAMS HSCtrls, BOOL Enable)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuHsAddr = 0x0;
    union tagCMU_HS_ENABLE_REGISTER enableReg;
    T32HS_CFG0 stHsCfg0;

    DbgEnterFunction(("BdrHueSatSetHueSatControls"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((HSCtrls >= 0) && (HSCtrls <= VPP_CMU_CTRLS_END)))
        return MV_VPP_EBADPARAM;
#endif

    if (CmuHueSatInputType[Channel] == VPP_CMU_RGB_444)
        return retStatus;

    stHsCfg0.u32 = vpp_obj->cmu.stHsCfg0;
    cmuHsAddr = RA_CMU_HS;
    enableReg.Value = stHsCfg0.uCFG0_EN;
    if (HSCtrls == VPP_CMU_HS_ENABLE) {
        enableReg.Bits.HSEnable = Enable;
    } else if (HSCtrls == VPP_CMU_INTELLISAT_ENABLE) {
        enableReg.Bits.IntelliSatEnable = Enable;
    } else if (HSCtrls == VPP_CMU_FLESH_TAPER_TONE) {
        enableReg.Bits.FleshToneTaperEnable = Enable;
    }
    stHsCfg0.uCFG0_EN = enableReg.Value;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG0, stHsCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg0 = stHsCfg0.u32;

    DbgExitFunction(("BdrHueSatSetHueSatControls"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will set the input type for the CMU block.
 *
 * Arguments  :
 *              Channel      - Main/Pip
 *              Input        - VPP_CMU_INPUT_SIGNAL_TYPE
 *
 * Returns    : On success returns BDR_ERR_OK .
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
static INT VPP_CMU_HUESAT_SetHSInputSignalType(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                              VPP_CMU_INPUT_SIGNAL_TYPE Input)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuHsAddr = 0x0;
    T32HS_CFG11 stHsCfg11;
    T32HS_CFG12 stHsCfg12;
    T32HS_CFG13 stHsCfg13;
    T32HS_CFG14 stHsCfg14;
    T32HS_CFG15 stHsCfg15;

    DbgEnterFunction(("BdrHueSatSetInputSignalType"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!(Input == VPP_CMU_YUV_601 || Input == VPP_CMU_YUV_709))
        return MV_VPP_EBADPARAM;
#endif

    cmuHsAddr = RA_CMU_HS;
    stHsCfg11.uCFG11_MSAT_RGBY = gHueSatMaxSat[Input][0];
    stHsCfg11.uCFG11_MSAT_RCR = gHueSatMaxSat[Input][1];
    stHsCfg12.uCFG12_MSAT_RCB = gHueSatMaxSat[Input][2];
    stHsCfg12.uCFG12_MSAT_GCR = gHueSatMaxSat[Input][3];
    stHsCfg13.uCFG13_MSAT_GCB = gHueSatMaxSat[Input][4];
    stHsCfg13.uCFG13_MSAT_BCR = gHueSatMaxSat[Input][5];
    stHsCfg14.uCFG14_MSAT_BCB = gHueSatMaxSat[Input][6];
    stHsCfg14.uCFG14_MSAT_ROFF = gHueSatOff[Input][0];
    stHsCfg15.uCFG15_MSAT_GOFF = gHueSatOff[Input][1];
    stHsCfg15.uCFG15_MSAT_BOFF = gHueSatOff[Input][2];

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG11, stHsCfg11.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG12, stHsCfg12.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG13, stHsCfg13.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG14, stHsCfg14.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG15, stHsCfg15.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    DbgExitFunction(("BdrHueSatSetInputSignalType"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will set all the ICR mode params (Axis, Hue and Sat)
 *
 * Arguments  :
 *              Channel      - Main/Pip
 *              pMode            - Pointer to Mode.PVPP_CMU_ICR_MODE
 *              pUsrMode         - Pointer to User Mode.
 *                                 PVPP_CMU_ICR_USER_MODE
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
 INT VPP_CMU_HUESAT_SetHSIcrMode(VPP_OBJ* vpp_obj,
                                 VPP_CHANNEL_TYPE Channel,
                                 VPP_CMU_ICR_MODE Mode,
                                 PVPP_CMU_ICR_USER_MODE pMode)

{
    UCHAR i;
    INT retStatus = MV_VPP_OK;
    UINT32 cmuHsAddr = 0x0;
    T32HS_CFG0 stHsCfg0;
    T32HS_CFG1 stHsCfg1;
    T32HS_CFG2 stHsCfg2;
    T32HS_CFG3 stHsCfg3;
    T32HS_CFG4 stHsCfg4;
    T32HS_CFG5 stHsCfg5;
    T32HS_CFG6 stHsCfg6;
    T32HS_CFG7 stHsCfg7;
    T32HS_CFG8 stHsCfg8;
    T32HS_CFG9 stHsCfg9;
    T32HS_CFG10 stHsCfg10;

    DbgEnterFunction(("BdrHueSatSetIcrMode"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (pMode != NULL)
        return MV_VPP_EBADPARAM;
    if (!((Mode >= VPP_CMU_ICR_MODE1 && Mode <= VPP_CMU_ICR_USER_DEFINED)))
        return MV_VPP_EBADPARAM;
#endif

    stHsCfg0.u32 = vpp_obj->cmu.stHsCfg0;
    stHsCfg1.u32 = vpp_obj->cmu.stHsCfg1;
    stHsCfg2.u32 = vpp_obj->cmu.stHsCfg2;
    stHsCfg3.u32 = vpp_obj->cmu.stHsCfg3;
    stHsCfg4.u32 = vpp_obj->cmu.stHsCfg4;
    stHsCfg5.u32 = vpp_obj->cmu.stHsCfg5;
    stHsCfg6.u32 = vpp_obj->cmu.stHsCfg6;
    stHsCfg7.u32 = vpp_obj->cmu.stHsCfg7;
    stHsCfg8.u32 = vpp_obj->cmu.stHsCfg8;
    stHsCfg9.u32 = vpp_obj->cmu.stHsCfg9;
    stHsCfg10.u32 = vpp_obj->cmu.stHsCfg10;
    cmuHsAddr = RA_CMU_HS;

    if (Mode == VPP_CMU_ICR_USER_DEFINED) {
        DbgAssert(((pMode!=NULL),"Invalid ICR Mode"));
        for (i = 0; i < BDR_CMU_MAX_ICR_HS_AXES; i++) {
            GlobalCmuHSSetHue (i, pMode->Axis[i]);
            GlobalCmuHSSetHueChange (i, pMode->Hue[i]);
            GlobalCmuHSSetSatChange (i,  pMode->Sat[i]);
        }
    } else if (Mode == VPP_CMU_ICR_RECOVER_AXES) {
        for (i = 0; i < BDR_CMU_MAX_ICR_HS_AXES; i++) {
            GlobalCmuHSSetHue (i, MULTIPLICATIONFACTOR_HUE_AXIS(gCmuModeTable[HueSatInputSignalPip][VPP_CMU_ICR_MODE1][0][i]));
        }
    } else {
        for (i = 0; i < BDR_CMU_MAX_ICR_HS_AXES; i++) {
            GlobalCmuHSSetHue (i, MULTIPLICATIONFACTOR_HUE_AXIS(gCmuModeTable[HueSatInputSignalPip][Mode][0][i]));
            GlobalCmuHSSetHueChange (i, gCmuModeTable[HueSatInputSignalPip][Mode][1][i]);
            GlobalCmuHSSetSatChange (i, gCmuModeTable[HueSatInputSignalPip][Mode][2][i]);
        }
    }

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG0, stHsCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg0 = stHsCfg0.u32;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG1, stHsCfg1.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg1 = stHsCfg1.u32;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG2, stHsCfg2.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg2 = stHsCfg2.u32;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG3, stHsCfg3.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg3 = stHsCfg3.u32;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG4, stHsCfg4.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg4 = stHsCfg4.u32;

    if (Mode != VPP_CMU_ICR_RECOVER_AXES) {
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG5, stHsCfg5.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        vpp_obj->cmu.stHsCfg5 = stHsCfg5.u32;
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG6, stHsCfg6.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        vpp_obj->cmu.stHsCfg6 = stHsCfg6.u32;
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG7, stHsCfg7.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        vpp_obj->cmu.stHsCfg7 = stHsCfg7.u32;
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG8, stHsCfg8.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        vpp_obj->cmu.stHsCfg8 = stHsCfg8.u32;
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG9, stHsCfg9.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        vpp_obj->cmu.stHsCfg9 = stHsCfg9.u32;
        retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG10, stHsCfg10.u32);
        if (MV_VPP_OK != retStatus)
            return retStatus;
        vpp_obj->cmu.stHsCfg10 = stHsCfg10.u32;
    }

    DbgExitFunction(("BdrHueSatSetIcrMode"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will set all the ICR mode params individually (Axis,Hue and Sat)
 *
 * Arguments  :
 *              Channel      - Main/Pip
 *              pIcrParams   - Variable for selecting the param
 *              Axis         - 14 different Axis values
 *              Value        - Unsigned integer value for setting these params
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
 *              On error BDR_ERR_INVALID_INSTANCE will be returned.
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
 INT VPP_CMU_HUESAT_SetHSIcrModeParams(VPP_OBJ* vpp_obj,
                                       VPP_CHANNEL_TYPE Channel,
                                       VPP_CMU_ICR_PARAMS pIcrParams,
                                       VPP_CMU_HS_AXIS Axis,UINT16 Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuHsAddr = 0x0;
    UINT32 cmuHsCfgAddr = 0x0;
    UINT32 cmuHsCfgVal = 0x0;
    T32HS_CFG0 stHsCfg0;
    T32HS_CFG1 stHsCfg1;
    T32HS_CFG2 stHsCfg2;
    T32HS_CFG3 stHsCfg3;
    T32HS_CFG4 stHsCfg4;
    T32HS_CFG5 stHsCfg5;
    T32HS_CFG6 stHsCfg6;
    T32HS_CFG7 stHsCfg7;
    T32HS_CFG8 stHsCfg8;
    T32HS_CFG9 stHsCfg9;
    T32HS_CFG10 stHsCfg10;

    DbgEnterFunction(("BdrHueSatSetIcrModeParams"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((Axis >= 0) && (Axis <= VPP_CMU_HS_MAX_AXES)))
        return MV_VPP_EBADPARAM;
    if (!((pIcrParams >= 0) && (pIcrParams <= VPP_CMU_HUESAT_END)))
        return MV_VPP_EBADPARAM;
#endif

    stHsCfg0.u32 = vpp_obj->cmu.stHsCfg0;
    stHsCfg1.u32 = vpp_obj->cmu.stHsCfg1;
    stHsCfg2.u32 = vpp_obj->cmu.stHsCfg2;
    stHsCfg3.u32 = vpp_obj->cmu.stHsCfg3;
    stHsCfg4.u32 = vpp_obj->cmu.stHsCfg4;
    stHsCfg5.u32 = vpp_obj->cmu.stHsCfg5;
    stHsCfg6.u32 = vpp_obj->cmu.stHsCfg6;
    stHsCfg7.u32 = vpp_obj->cmu.stHsCfg7;
    stHsCfg8.u32 = vpp_obj->cmu.stHsCfg8;
    stHsCfg9.u32 = vpp_obj->cmu.stHsCfg9;
    stHsCfg10.u32 = vpp_obj->cmu.stHsCfg10;
    cmuHsAddr = RA_CMU_HS;

    if (pIcrParams == VPP_CMU_HUESAT_HUE) {
        GlobalCmuHSSetHue (Axis, Value);
        CmuHSHueCfgRegAddr (Axis, cmuHsCfgAddr, cmuHsCfgVal);
    } else if (pIcrParams == VPP_CMU_HUESAT_HUECHANGE) {
        GlobalCmuHSSetHueChange (Axis, Value);
        CmuHSHueChangeCfgRegAddr (Axis, cmuHsCfgAddr, cmuHsCfgVal);
    } else if (pIcrParams == VPP_CMU_HUESAT_SATCHANGE) {
        GlobalCmuHSSetSatChange (Axis, Value);
        CmuHSSatChangeCfgRegAddr (Axis, cmuHsCfgAddr, cmuHsCfgVal);
    } else if (pIcrParams == VPP_CMU_GLOBALSAT_CHANGE) {
        stHsCfg10.uCFG10_GL_SAT = Value;
        cmuHsCfgAddr = RA_HS_CFG10;
        cmuHsCfgVal = stHsCfg10.u32;
    }
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + cmuHsCfgAddr, cmuHsCfgVal);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stHsCfg0 = stHsCfg0.u32;
    vpp_obj->cmu.stHsCfg1 = stHsCfg1.u32;
    vpp_obj->cmu.stHsCfg2 = stHsCfg2.u32;
    vpp_obj->cmu.stHsCfg3 = stHsCfg3.u32;
    vpp_obj->cmu.stHsCfg4 = stHsCfg4.u32;
    vpp_obj->cmu.stHsCfg5 = stHsCfg5.u32;
    vpp_obj->cmu.stHsCfg6 = stHsCfg6.u32;
    vpp_obj->cmu.stHsCfg7 = stHsCfg7.u32;
    vpp_obj->cmu.stHsCfg8 = stHsCfg8.u32;
    vpp_obj->cmu.stHsCfg9 = stHsCfg9.u32;
    vpp_obj->cmu.stHsCfg10 = stHsCfg10.u32;

    DbgExitFunction(("BdrHueSatGetIcrModeParams"));
    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will set Manual brightness correction values
 *
 * Arguments  :
 *              Channel      - Main/Pip
 *              pIcrParams   - Variable for selecting the param
 *              BRValues     - Brightness correction values
 *
 * Returns    : On success returns BDR_ERR_OK or else one of the error code
 *
 *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_HUESAT_SetBRCorrection(VPP_OBJ* vpp_obj,
                                   VPP_CHANNEL_TYPE Channel,
                                   UINT16 *Value)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuHsAddr = 0x0;
    UINT32 count = 0x0;

    T32HS_CFG16 stHsCfg16;
    T32HS_CFG17 stHsCfg17;
    T32HS_CFG18 stHsCfg18;
    T32HS_CFG19 stHsCfg19;
    T32HS_CFG20 stHsCfg20;

    DbgEnterFunction(("BdrHueSatSetBRCorrection"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (NULL == Value)
        return MV_VPP_EBADPARAM;
#endif

    cmuHsAddr = RA_CMU_HS;

    stHsCfg16.uCFG16_BRG1  = Value[count++];
    stHsCfg16.uCFG16_BRG2  = Value[count++];
    stHsCfg16.uCFG16_BRG3  = Value[count++];
    stHsCfg17.uCFG17_BRG4  = Value[count++];
    stHsCfg17.uCFG17_BRG5  = Value[count++];
    stHsCfg17.uCFG17_BRG6  = Value[count++];
    stHsCfg18.uCFG18_BRG7  = Value[count++];
    stHsCfg18.uCFG18_BRG8  = Value[count++];
    stHsCfg18.uCFG18_BRG9  = Value[count++];
    stHsCfg19.uCFG19_BRG10 = Value[count++];
    stHsCfg19.uCFG19_BRG11 = Value[count++];
    stHsCfg19.uCFG19_BRG12 = Value[count++];
    stHsCfg20.uCFG20_BRG13 = Value[count++];

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG16, stHsCfg16.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG17, stHsCfg17.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG18, stHsCfg18.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG19, stHsCfg19.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuHsAddr + RA_HS_CFG20, stHsCfg20.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;

    DbgExitFunction(("BdrHueSetBRCorrection"));
    return retStatus;
}

