/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_BR_C_

#include "vpp.h"

#include "vpp_cmu_br.h"
#include "vpp_cmu_brpriv.h"
#include "vpp_cmu_brcfg.h"

static INT VPP_CMU_BR_SetParams(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                            PVPP_CMU_BR_PARAMS pBrParams);
/*-----------------------------------------------------------------------------
* Functions
*-----------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------
 * Description: This function sets the default values.
 *
 * Returns    : On success returns MV_VPP_OK
 *
 * Notes      : None
 *----------------------------------------------------------------------------
 */
INT VPP_CMU_BR_SetDefaults(VPP_OBJ* vpp_obj,
                         VPP_CHANNEL_TYPE Channel,
                         VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType)
{
    INT RetCode = MV_VPP_OK;

    DbgEnterFunction(("BdrBrSetDefaults"));

    CmuBrInputType[Channel] = CmuInputType;

    RetCode = VPP_CMU_BR_SetParams(vpp_obj,Channel,
                (PVPP_CMU_BR_PARAMS)&CmuBrInputParamsLow);

    if (RetCode != MV_VPP_OK)
        return RetCode;

    DbgExitFunction(("BdrBrSetDefaults"));

    return RetCode;
}

/*------------------------------------------------------------------------
 * Description: This function will set Input Parameters of BR block
 *
 * Arguments  :
 *                pAceParams  - Pointer to params
 *                               structure.
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
static INT VPP_CMU_BR_SetParams(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                            PVPP_CMU_BR_PARAMS pBrParams)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuBrAddr = 0x0;
    CmuBrenreg cmuBrEnReg;
    T32BR_CFG0 stBrCfg0;

    DbgEnterFunction(("BdrBrSetParams"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    DbgAssert(((pBrParams != NULL) , "Invalid Parameter"));
#endif

    if (CmuBrInputType[Channel]== VPP_CMU_RGB_444)
        return retStatus;

    cmuBrEnReg.Bits.en = pBrParams->EnBrightnessLevel;
    cmuBrEnReg.Bits.taper = pBrParams->Taper;
    cmuBrEnReg.Bits.taper_size = pBrParams->TaperSize;

    stBrCfg0.uCFG0_EN = cmuBrEnReg.Value;
    stBrCfg0.uCFG0_TH1 = (pBrParams->BrThreshold1 & 0x3FF);
    stBrCfg0.uCFG0_TH2 = (pBrParams->BrThreshold2 & 0x3FF);

    cmuBrAddr = RA_CMU_BR;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuBrAddr + RA_BR_CFG0, stBrCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stBrCfg0 = stBrCfg0.u32;

    DbgExitFunction(("BdrBrSetParams"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will set Brightness level
 *
 * Arguments  :
 *              Channel  - Main/Pip/Both.
 *              Value    - Set Brightness Value
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_BR_SetBrightnessLevel(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                                VPP_CMU_BR_LEVEL BrLevel)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuBrAddr = 0x0;
    CmuBrenreg cmuBrEnReg;
    T32BR_CFG0 stBrCfg0;

    DbgEnterFunction(("BdrBrSetBrightnessLevel"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((BrLevel >= VPP_CMU_BR_LEVEL0 && BrLevel <= VPP_CMU_BR_MAX_LEVEL )))
        return MV_VPP_EBADPARAM;
#endif

    if (CmuBrInputType[Channel] == VPP_CMU_RGB_444)
        return retStatus;

    stBrCfg0.u32 = vpp_obj->cmu.stBrCfg0;
    cmuBrAddr = RA_CMU_BR;

    cmuBrEnReg.Value = stBrCfg0.uCFG0_EN;
    cmuBrEnReg.Bits.en = BrLevel;
    stBrCfg0.uCFG0_EN = cmuBrEnReg.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuBrAddr + RA_BR_CFG0, stBrCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stBrCfg0 = stBrCfg0.u32;

    DbgExitFunction(("BdrBrSetBrightnessLevel"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will Enable Taper for brightness
 *
 * Arguments  :
 *              Channel   - Main/Pip/Both.
 *              Value     - TRUE/FALSE
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_BR_SetTaperEnable(VPP_OBJ* vpp_obj,
                VPP_CHANNEL_TYPE Channel,BOOL Enable)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuBrAddr = 0x0;
    CmuBrenreg cmuBrEnReg;
    T32BR_CFG0 stBrCfg0;

    DbgEnterFunction(("BdrBrSetTaperEnable"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP)) {
        return MV_VPP_EBADPARAM;
    }
#endif

    stBrCfg0.u32 = vpp_obj->cmu.stBrCfg0;
    cmuBrAddr = RA_CMU_BR;

    cmuBrEnReg.Value = stBrCfg0.uCFG0_EN;
    cmuBrEnReg.Bits.taper = Enable;
    stBrCfg0.uCFG0_EN = cmuBrEnReg.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuBrAddr + RA_BR_CFG0, stBrCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stBrCfg0 = stBrCfg0.u32;
    DbgExitFunction(("BdrBrSetTaperEnable"));

    return retStatus;
}

/*------------------------------------------------------------------------
 * Description: This function will set Taper size for brightness
 *
 * Arguments  :
 *              Channel   - Main/Pip/Both.
 *              Value     - Set Taper size Value
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CMU_BR_SetTaperSize(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                        VPP_CMU_BR_TAPERSIZE TaperSize)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuBrAddr = 0x0;
    CmuBrenreg cmuBrEnReg;
    T32BR_CFG0 stBrCfg0;

    DbgEnterFunction(("BdrBrSetTaperSize"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
    if (!((TaperSize >= VPP_CMU_BR_TAPERSIZE16 && TaperSize <= VPP_CMU_BR_TAPERSIZE512 )))
        return MV_VPP_EBADPARAM;
#endif
    stBrCfg0.u32 = vpp_obj->cmu.stBrCfg0;
    cmuBrAddr = RA_CMU_BR;

    cmuBrEnReg.Value = stBrCfg0.uCFG0_EN;
    cmuBrEnReg.Bits.taper_size = TaperSize;
    stBrCfg0.uCFG0_EN = cmuBrEnReg.Value;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuBrAddr + RA_BR_CFG0, stBrCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stBrCfg0 = stBrCfg0.u32;

    DbgExitFunction(("BdrBrSetTaperSize"));

    return retStatus;
}

/*--------------------------------------------------------------------------------
 * Description: This function will Sett he brightness control threshold
 *
 * Arguments  :
 *                Channel         - Main/Pip
 *                Value         - Threshold1 and Threshold2
 *
 * Returns    : On success returns BDR_ERR_OK or else or one of the
  *
 * Notes      : None
 *--------------------------------------------------------------------------------
 */
INT VPP_CMU_BR_SetThresholds(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                        UINT16 Threshold1, UINT16 Threshold2)
{
    INT retStatus = MV_VPP_OK;
    UINT32 cmuBrAddr = 0x0;
    T32BR_CFG0 stBrCfg0;

    DbgEnterFunction(("BdrBrSetThreshold"));

#ifdef VPP_DEBUG
    if ((Channel != VPP_MAIN) && (Channel != VPP_PIP))
        return MV_VPP_EBADPARAM;
#endif

    stBrCfg0.u32 = vpp_obj->cmu.stBrCfg0;
    cmuBrAddr = RA_CMU_BR;

    stBrCfg0.uCFG0_TH1 = Threshold1;
    stBrCfg0.uCFG0_TH2 = Threshold2;

    retStatus = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CMU + cmuBrAddr + RA_BR_CFG0, stBrCfg0.u32);
    if (MV_VPP_OK != retStatus)
        return retStatus;
    vpp_obj->cmu.stBrCfg0 = stBrCfg0.u32;


    DbgExitFunction(("BdrBrSetThreshold"));

    return retStatus;
}

