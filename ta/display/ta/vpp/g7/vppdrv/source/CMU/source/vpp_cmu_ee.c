/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifdef ENABLE_EECMU
#define VPP_CMU_EE_C

#if defined(VPP_IN_TRUST_ZONE)
#include "com_type.h"
#endif

#include "vpp.h"
#include "vpp_mp.h"
#include "vpp_regs.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_cmu_ee.h"
#include "vpp_cmu_ee_prv.h"
#include "vpp_cmu.h"

#define VPP_CMU_EE_CopyUserParamToTopCfgFields(stEeTopCfg1, stEeTopCfg2, stEeTopCfg3, stEeTopCfg4, \
                                                                                    pEEParam) \
                        stEeTopCfg1.uCFG1_ftd_inlow = pEEParam->FtdInlow; \
                        stEeTopCfg1.uCFG1_ftd_inhigh = pEEParam->FtdInhigh; \
                        stEeTopCfg1.uCFG1_ftd_outlow = pEEParam->FtdOutlow; \
                        stEeTopCfg2.uCFG2_ftd_outhigh = pEEParam->FtdOuthigh; \
                        stEeTopCfg2.uCFG2_ftd_ylow = pEEParam->FtdYlow; \
                        stEeTopCfg2.uCFG2_ftd_yhigh = pEEParam->FtdYhigh; \
                        stEeTopCfg3.uCFG3_ftd_outchroma_low = pEEParam->FtdOutchromaLow; \
                        stEeTopCfg3.uCFG3_ftd_inchroma_low = pEEParam->FtdInchromaLow; \
                        stEeTopCfg4.uCFG4_ftd_inchroma_high = pEEParam->FtdInchromaHigh; \
                        stEeTopCfg4.uCFG4_ftd_outchroma_high = pEEParam->FtdOutchromaHigh; \


#define VPP_CMU_EE_CopyUserParamToRegFields(stCmuEeCtrl, peECtrlVppObj) \
                            stCmuEeCtrl.field.UpsampleEn = peECtrlVppObj->UpsamplerEn; \
                            stCmuEeCtrl.field.LumaGainEn = peECtrlVppObj->LumaGainEn; \
                            stCmuEeCtrl.field.DetailEn = peECtrlVppObj->DetailEn; \
                            stCmuEeCtrl.field.ChromaGainEn = peECtrlVppObj->ChromaGainEn; \
                            stCmuEeCtrl.field.FtLtiEn =  peECtrlVppObj->FtLtiEn; \
                            stCmuEeCtrl.field.FtDetEn =  peECtrlVppObj->FtDetEn; \
                            stCmuEeCtrl.field.NewLtiAlgoSel =  peECtrlVppObj->NewLtiAlgoSel; \
                            stCmuEeCtrl.field.NewCtiAlgoSel =  peECtrlVppObj->NewCtiAlgoSel;

#define VPP_CMU_EE_CopyRegFieldsToUserParam(stCmuEeCtrl, peECtrlVppObj) \
                            peECtrlVppObj->UpsamplerEn         = stCmuEeCtrl.field.UpsampleEn; \
                            peECtrlVppObj->LumaGainEn          = stCmuEeCtrl.field.LumaGainEn; \
                            peECtrlVppObj->DetailEn            = stCmuEeCtrl.field.DetailEn; \
                            peECtrlVppObj->ChromaGainEn        = stCmuEeCtrl.field.ChromaGainEn; \
                            peECtrlVppObj->FtLtiEn             = stCmuEeCtrl.field.FtLtiEn; \
                            peECtrlVppObj->FtDetEn             = stCmuEeCtrl.field.FtDetEn; \
                            peECtrlVppObj->NewLtiAlgoSel       = stCmuEeCtrl.field.NewLtiAlgoSel; \
                            peECtrlVppObj->NewCtiAlgoSel       = stCmuEeCtrl.field.NewCtiAlgoSel;


#define VPP_CMU_EE_CTRL_INIT_ALL(VALUE, UpsamplerEnVal) \
                                                    .UpsamplerEn            = UpsamplerEnVal, \
                                                    .LumaGainEn             = VALUE, \
                                                    .DetailEn               = VALUE, \
                                                    .ChromaGainEn           = VALUE, \
                                                    .FtLtiEn                = VALUE, \
                                                    .FtDetEn                = VALUE, \
                                                    .NewLtiAlgoSel          = VALUE, \
                                                    .NewCtiAlgoSel          = VALUE,

#define VPP_CMU_EE_SKINTONE_DISABLE_ALL() \
                                                    .FtdInlow               = 0, \
                                                    .FtdInhigh              = 0, \
                                                    .FtdOutlow              = 0, \
                                                    .FtdOuthigh             = 0, \
                                                    .FtdYlow                = 0, \
                                                    .FtdYhigh               = 0, \
                                                    .FtdOutchromaLow        = 0, \
                                                    .FtdInchromaLow         = 0, \
                                                    .FtdInchromaHigh        = 0, \
                                                    .FtdOutchromaHigh       = 0,

#define VPP_CMU_EE_SKINTONE_DEFAULT(FtdYlowValue) \
                                                    .FtdInlow               = 110, \
                                                    .FtdInhigh              = 140, \
                                                    .FtdOutlow              = 95, \
                                                    .FtdOuthigh             = 155, \
                                                    .FtdYlow                = FtdYlowValue, \
                                                    .FtdYhigh               = 255, \
                                                    .FtdOutchromaLow        = 100, \
                                                    .FtdInchromaLow         = 1300, \
                                                    .FtdInchromaHigh        = 2300, \
                                                    .FtdOutchromaHigh       = 3500,

// .UpsamplerEn is Reserved
static const VPP_CMU_EE_CTRL EeCtrl[VPP_CMU_EE_MAX_MODE] = {
    {/* default mode */
        VPP_CMU_EE_CTRL_INIT_ALL(0,0)
    },
    {/* soft bypass */
        VPP_CMU_EE_CTRL_INIT_ALL(0,0)
    },
    {/* only UPS enabled */
        VPP_CMU_EE_CTRL_INIT_ALL(0,1)
    },
    {/* only UPS with ringing adapt */
        VPP_CMU_EE_CTRL_INIT_ALL(0,1)
    },
    {/* Old LTI & CTI with all enable */
        .UpsamplerEn            = 1,
        .LumaGainEn             = 1,
        .DetailEn               = 1,
        .ChromaGainEn           = 1,
        .FtLtiEn                = 1,
        .FtDetEn                = 1,
        .NewLtiAlgoSel          = 0,
        .NewCtiAlgoSel          = 0,
    },
    {/* New LTI & CTI wo det ip (if Yin already includes DYdet) */
        .UpsamplerEn            = 1,
        .AdaptiveSamplingEn     = 1,
        .LumaGainEn             = 1,
        .DetailEn               = 0,
        .ChromaGainEn           = 1,
        .FtLtiEn                = 1,
        .FtDetEn                = 1,
        .NewLtiAlgoSel          = 1,
        .NewCtiAlgoSel          = 1,
    },
    {/* All enable with default thresholds */
        VPP_CMU_EE_CTRL_INIT_ALL(1, 1)
    },
    {/* New LTI & CTI with gain control */
        .UpsamplerEn            = 0,
        .LumaGainEn             = 1,
        .DetailEn               = 0,
        .ChromaGainEn           = 1,
        .FtLtiEn                = 0,
        .FtDetEn                = 0,
        .NewLtiAlgoSel          = 1,
        .NewCtiAlgoSel          = 1,
    },
};

static const VPP_CMU_EE_SkinToneRegion SkinToneRegion[VPP_CMU_EE_MAX_MODE]= {
    {
        VPP_CMU_EE_SKINTONE_DEFAULT(0)
    },
    {
        VPP_CMU_EE_SKINTONE_DISABLE_ALL()
    },
    {
        VPP_CMU_EE_SKINTONE_DISABLE_ALL()
    },
    {
        VPP_CMU_EE_SKINTONE_DISABLE_ALL()
    },
    {
        VPP_CMU_EE_SKINTONE_DEFAULT(30)
    },
    {
        VPP_CMU_EE_SKINTONE_DEFAULT(0)
    },
    {
        VPP_CMU_EE_SKINTONE_DEFAULT(0)
    },
    {
        VPP_CMU_EE_SKINTONE_DISABLE_ALL()
    },
};

static INT CMU_EE_SetCtrl(VPP_OBJ *vpp_obj, INT EeUnit,
                 PVPP_CMU_EE_CTRL pEeCtrl);
static INT CMU_EE_SetLtiGain(VPP_OBJ *vpp_obj, INT EeUnit, INT LtiGain);
static INT CMU_EE_SetCtiGain(VPP_OBJ *vpp_obj, INT EeUnit, INT CtiGain);
static INT CMU_EE_SetSkinToneRegion(VPP_OBJ *vpp_obj, INT EeUnit,
                 PVPP_CMU_EE_SkinToneRegion pSkinToneRegion);

/***************************************************************************************
 * FUNCTION: internal fuction to get the start address of an EE instance by unit number
 * PARAMS: CMU - EE unit number(EE0, EE1)
 * RETURN:  the start address
 ***************************************************************************************/
UINT32 INTERNAL_EE_GetRegStartAddr(VPP_OBJ *vpp_obj, INT EeUnit)
{
    return (vpp_obj->base_addr_mp + RA_VPP_MP_EECMU +RA_EECMU_EETOP );
}

/***************************************************************************************
 * FUNCTION: load the default register values of FE detail EE and CMU EE block
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CMU EE default values to BCM buffer
 ***************************************************************************************/
INT CMU_EE_LoadDefaultVal(VPP_OBJ *vpp_obj)
{
    INT RetVal = MV_VPP_OK;
    VPP_CMU_EE_CTRL eeCtrl;

    /* EE default settings */
    memset(&eeCtrl, 0, sizeof(VPP_CMU_EE_CTRL));

    if ( 1 == gCmuEeDefValues.VPP_CMU_EE_CTRL)
        eeCtrl.UpsamplerEn = 1;
    else
        return MV_VPP_EBADPARAM;/* Recommented to enable ups alone by default */

    CMU_EE_SetCtrl(vpp_obj, 0, &eeCtrl);
    CMU_EE_SetLtiGain(vpp_obj, 0, gCmuEeDefValues.VPP_CMU_EE_LTIGAIN);
    CMU_EE_SetCtiGain(vpp_obj, 0, gCmuEeDefValues.VPP_CMU_EE_CTIGAIN);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of EE  based on mode
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB GDet values to BCM buffer
 ***************************************************************************************/

INT CMU_EE_SetEEParamsMode(VPP_OBJ *vpp_obj, INT mode)
{
    INT RetVal = MV_VPP_OK;
    INT EeUnit = 0;
    INT LtiGain[VPP_CMU_EE_MAX_MODE] = \
                {VPP_DEFAULT_LTI_GAIN, 0, 0, 0, VPP_DEFAULT_LTI_GAIN, \
                VPP_DEFAULT_LTI_GAIN, VPP_DEFAULT_LTI_GAIN, vpp_obj->cmu.lti_gain};
    INT CtiGain[VPP_CMU_EE_MAX_MODE] = \
                {VPP_DEFAULT_LTI_GAIN, 0, 0, 0, VPP_DEFAULT_LTI_GAIN, \
                VPP_DEFAULT_LTI_GAIN, VPP_DEFAULT_LTI_GAIN, vpp_obj->cmu.cti_gain};

    if (VPP_CMU_EE_MAX_MODE <= mode)
        return -1;

    if (vpp_obj->cmu.cmu_enabled == 0) {
        VPP_CMU_Enable(vpp_obj,TRUE);
    }

    RetVal = CMU_EE_SetCtrl(vpp_obj, EeUnit, (PVPP_CMU_EE_CTRL)&EeCtrl[mode]);
    RetVal = CMU_EE_SetLtiGain(vpp_obj, EeUnit, LtiGain[mode]);
    RetVal = CMU_EE_SetCtiGain(vpp_obj, EeUnit, CtiGain[mode]);
    RetVal = CMU_EE_SetSkinToneRegion(vpp_obj, EeUnit, (PVPP_CMU_EE_SkinToneRegion)&SkinToneRegion[mode]);
    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Initialize detail EE and CMU EE according to configuration
 * PARAMS: DetThSel - Threshold configuration selection.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - invalid input parameters.
 *                MV_VPP_EBCMBUFFULL - BCM buffer is full.
 ***************************************************************************************/
INT CMU_EE_Config(VPP_OBJ *vpp_obj, INT DetThSel)
{
    /*check input parameters*/
    if ((VPP_FE_DET_TH_LOW > DetThSel) || (VPP_FE_DET_TH_HIGH < DetThSel))
        return MV_VPP_EBADPARAM;

    CMU_EE_SetLtiGain(vpp_obj, 0, gEeLtiCtiGain[DetThSel][0]);
    CMU_EE_SetCtiGain(vpp_obj, 0, gEeLtiCtiGain[DetThSel][1]);

    return  MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: sets CMU EE control.
 * PARAMS: EeUnit - EE unit number(EE0, EE1)
 *             pEeCtrl - pointer to the struct of EE control.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL - BCM buffer is full.
 ***************************************************************************************/
static INT CMU_EE_SetCtrl(VPP_OBJ *vpp_obj, INT EeUnit, PVPP_CMU_EE_CTRL pEeCtrl)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    T32EE_TOP_CFG0 stEeTopCfg0;
    VPP_CMU_EE_PRI_CTRL stCmuEeCtrl;
    PVPP_CMU_EE_CTRL peECtrlVppObj;

    stEeTopCfg0.uCFG0_lti_gain = vpp_obj->cmu.lti_gain;
    stEeTopCfg0.uCFG0_cti_gain = vpp_obj->cmu.cti_gain;
    peECtrlVppObj = &vpp_obj->cmu.EECtrl;

    VPP_CMU_EE_CopyUserParamToRegFields(stCmuEeCtrl, pEeCtrl);
    stEeTopCfg0.uCFG0_ctrl = stCmuEeCtrl.data;

    RegAddr = INTERNAL_EE_GetRegStartAddr(vpp_obj, EeUnit);
    /*get register value from local block buffer and write it to BCM bufer*/

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG0, stEeTopCfg0.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    VPP_CMU_EE_CopyRegFieldsToUserParam(stCmuEeCtrl, peECtrlVppObj)

    return  RetVal;
}

/***************************************************************************************
 * FUNCTION: sets CMU EE LTI gain.
 * PARAMS: EeUnit - EE unit number(EE0, EE1).
 *             LtiGain - EE LTI gain value.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL - BCM buffer is full.
 ***************************************************************************************/
static INT CMU_EE_SetLtiGain(VPP_OBJ *vpp_obj, INT EeUnit, INT LtiGain)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    T32EE_TOP_CFG0 stEeTopCfg0;
    VPP_CMU_EE_PRI_CTRL stCmuEeCtrl;
    PVPP_CMU_EE_CTRL peECtrlVppObj;

    RegAddr = INTERNAL_EE_GetRegStartAddr(vpp_obj, EeUnit);
    stEeTopCfg0.uCFG0_cti_gain = vpp_obj->cmu.cti_gain;
    peECtrlVppObj = &vpp_obj->cmu.EECtrl;

    VPP_CMU_EE_CopyUserParamToRegFields(stCmuEeCtrl, peECtrlVppObj);
    stEeTopCfg0.uCFG0_ctrl = stCmuEeCtrl.data;

    stEeTopCfg0.uCFG0_lti_gain = LtiGain;

    /*write CMU_EE_LTIGAIN register value to BCM bufer*/
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG0, stEeTopCfg0.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    vpp_obj->cmu.lti_gain = stEeTopCfg0.uCFG0_lti_gain;

    return  RetVal;
}

/***************************************************************************************
 * FUNCTION: sets CMU EE control parameters.
 * PARAMS: EeUnit - EE unit number(EE0, EE1).
 *             CtiGain - EE CTI gain value.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL - BCM buffer is full.
 ***************************************************************************************/
static INT CMU_EE_SetCtiGain(VPP_OBJ *vpp_obj, INT EeUnit, INT CtiGain)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    T32EE_TOP_CFG0 stEeTopCfg0;
    VPP_CMU_EE_PRI_CTRL stCmuEeCtrl;
    PVPP_CMU_EE_CTRL peECtrlVppObj;

    RegAddr = INTERNAL_EE_GetRegStartAddr(vpp_obj, EeUnit);
    stEeTopCfg0.uCFG0_lti_gain = vpp_obj->cmu.lti_gain;
    peECtrlVppObj = &vpp_obj->cmu.EECtrl;

    VPP_CMU_EE_CopyUserParamToRegFields(stCmuEeCtrl, peECtrlVppObj);
    stEeTopCfg0.uCFG0_ctrl = stCmuEeCtrl.data;

    stEeTopCfg0.uCFG0_cti_gain = CtiGain;

    /*write CMU_EE_CTIGAIN register value to BCM bufer*/
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG0, stEeTopCfg0.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    vpp_obj->cmu.cti_gain = stEeTopCfg0.uCFG0_cti_gain;

    return  RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of EE params
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CMU EE values to BCM buffer
 ***************************************************************************************/
INT CMU_EE_SetEEParams(VPP_OBJ *vpp_obj, INT EeUnit, PVPP_CMU_EE_PARAMS pEEParam)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
    INT diff = 0;
    T32EE_TOP_CFG1 stEeTopCfg1;
    T32EE_TOP_CFG2 stEeTopCfg2;
    T32EE_TOP_CFG3 stEeTopCfg3;
    T32EE_TOP_CFG4 stEeTopCfg4;
    T32EE_TOP_CFG5 stEeTopCfg5;

    RegAddr = INTERNAL_EE_GetRegStartAddr(vpp_obj, EeUnit);

    /* Skin Tone Region params */
    VPP_CMU_EE_CopyUserParamToTopCfgFields(stEeTopCfg1, stEeTopCfg2, stEeTopCfg3, stEeTopCfg4, \
                                                                                pEEParam)

    diff = (stEeTopCfg1.uCFG1_ftd_inlow - stEeTopCfg1.uCFG1_ftd_outlow);
    diff = diff ? diff : 1;
    stEeTopCfg5.uCFG5_ftd_huewidth = 4095 / diff;

    diff = (stEeTopCfg3.uCFG3_ftd_inchroma_low - stEeTopCfg3.uCFG3_ftd_outchroma_low);
    diff = diff ? diff : 1;
    stEeTopCfg5.uCFG5_ftd_chroma_width = 32767 / diff;

    /*write register value to BCM bufer*/
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG1, stEeTopCfg1.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG2, stEeTopCfg2.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG3, stEeTopCfg3.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG4, stEeTopCfg4.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG5, stEeTopCfg5.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: sets CMU EE skin tone region
 * PARAMS: EeUnit - EE unit number(EE0, EE1).
 *             pSkinToneRegion - FTDC region.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL - BCM buffer is full.
 ***************************************************************************************/
static INT CMU_EE_SetSkinToneRegion(VPP_OBJ *vpp_obj, INT EeUnit, PVPP_CMU_EE_SkinToneRegion pSkinToneRegion)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    INT diff = 0;
    T32EE_TOP_CFG1 stEeTopCfg1;
    T32EE_TOP_CFG2 stEeTopCfg2;
    T32EE_TOP_CFG3 stEeTopCfg3;
    T32EE_TOP_CFG4 stEeTopCfg4;
    T32EE_TOP_CFG5 stEeTopCfg5;

    RegAddr = INTERNAL_EE_GetRegStartAddr(vpp_obj, EeUnit);

    VPP_CMU_EE_CopyUserParamToTopCfgFields(stEeTopCfg1, stEeTopCfg2, stEeTopCfg3, stEeTopCfg4, \
                                                                                pSkinToneRegion)

    diff = (stEeTopCfg1.uCFG1_ftd_inlow - stEeTopCfg1.uCFG1_ftd_outlow);
    diff = diff ? diff : 1;
    stEeTopCfg5.uCFG5_ftd_huewidth = 4095 / diff;

    diff = (stEeTopCfg3.uCFG3_ftd_inchroma_low - stEeTopCfg3.uCFG3_ftd_outchroma_low);
    diff = diff ? diff : 1;
    stEeTopCfg5.uCFG5_ftd_chroma_width = 32767 / diff;

    /*write register value to BCM bufer*/
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG1, stEeTopCfg1.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG2, stEeTopCfg2.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG3, stEeTopCfg3.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG4, stEeTopCfg4.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_EE_TOP_CFG5, stEeTopCfg5.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    return  RetVal;
}
#endif /* ENABLE_EECMU */
