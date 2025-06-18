/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_FE_DCM_C_
#define _VPP_FE_DCM_C_

#ifdef CONFIG_AMP_DCM
#include "Galois_memmap.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_module.h"
#include "vpp_fe_dcm_prv.h"
#include "vpp_fe_dcm_common.h"
#include "vpp_be_prv.h"
#include "vpp_gfx_vop.h"
#include "vpp_mp.h"
#include "vpp_be.h"
#include "vpp_be_hdr.h"
#include "vpp_csc_icsc.h"
#include "vpp_dv_api.h"

static INT VPP_FE_DCM_Load_Y2R_Csc(VPP_OBJ *vpp_obj, INT planeID, INT iCscType, INT iCscMode);
static INT VPP_FE_DCM_Load_R2Y_Csc(VPP_OBJ *vpp_obj, INT planeID, INT iCscType, INT iCscMode);
static INT VPP_FE_DCM_Load_LSH_2D_LUT(VPP_OBJ *vpp_obj, INT planeID, INT LutType);
static INT VPP_FE_DCM_Load_Y_S_GAIN_THR_LUT(VPP_OBJ *vpp_obj, INT planeID, UINT32 uiMode);

static INT VPP_FE_DCM_Load_Y2R_Csc(VPP_OBJ *vpp_obj, INT planeID, INT iCscType, INT iCscMode)
{
    T32CSC_C15O18F12_CFG0 stCfg0;
    T32CSC_C15O18F12_CFG1 stCfg1;
    T32CSC_C15O18F12_CFG2 stCfg2;
    T32CSC_C15O18F12_CFG3 stCfg3;
    T32CSC_C15O18F12_CFG4 stCfg4;
    T32CSC_C15O18F12_CFG5 stCfg5;
    T32CSC_C15O18F12_CFG6 stCfg6;
    T32CSC_C15O18F12_CFG7 stCfg7;
    INT iSignMagBitLocCoeff = 0, iSignMagBitLocOff = 0;
    UINT32 uiRegAddr = 0;

    INT *psiCscCoeff = NULL, *psiCscOffset = NULL;

    stCfg0.u32 = stCfg1.u32 = stCfg2.u32 = stCfg3.u32 = stCfg4.u32 = stCfg5.u32 = stCfg6.u32 = stCfg7.u32 = 0;


    VPP_DCM_INFO("%s: type %d mode %d\r\n", __FUNCTION__,iCscType, iCscMode);

    uiRegAddr = DCM_CSC_C15O18F12_BASE(vpp_obj, planeID, iCscType);
    if(!uiRegAddr){
        VPP_DCM_INFO("CSC_C15O18F12 not exists for plane: %d\r\n", planeID);
        return 0;
    }

    iSignMagBitLocCoeff = 14;
    iSignMagBitLocOff  = 17;
    if(DCM_CSC_TYPE_NORMAL == iCscType){
        psiCscCoeff = giInputYUV2RGB_CscCoeff[iCscMode];
        psiCscOffset = giInputYUV2RGB_CscOffset[iCscMode];
    } else if(DCM_CSC_TYPE_CMAP == iCscType){
        psiCscCoeff = giCMAP_OutputYUV2RGB_CscCoeff[iCscMode];
        psiCscOffset = giCMAP_OutputYUV2RGB_CscOffset[iCscMode];
    }

    stCfg0.mCFG0_C0 = SIGN_MAG(psiCscCoeff[0], iSignMagBitLocCoeff);
    stCfg0.mCFG0_C1 = SIGN_MAG(psiCscCoeff[1], iSignMagBitLocCoeff);
    stCfg1.mCFG1_C2 = SIGN_MAG(psiCscCoeff[2], iSignMagBitLocCoeff);
    stCfg1.mCFG1_C3 = SIGN_MAG(psiCscCoeff[3], iSignMagBitLocCoeff);
    stCfg2.mCFG2_C4 = SIGN_MAG(psiCscCoeff[4], iSignMagBitLocCoeff);
    stCfg2.mCFG2_C5 = SIGN_MAG(psiCscCoeff[5], iSignMagBitLocCoeff);
    stCfg3.mCFG3_C6 = SIGN_MAG(psiCscCoeff[6], iSignMagBitLocCoeff);
    stCfg3.mCFG3_C7 = SIGN_MAG(psiCscCoeff[7], iSignMagBitLocCoeff);
    stCfg4.mCFG4_C8 = SIGN_MAG(psiCscCoeff[8], iSignMagBitLocCoeff);

    stCfg5.mCFG5_OFF1 = SIGN_MAG(psiCscOffset[0], iSignMagBitLocOff);
    stCfg6.mCFG6_OFF2 = SIGN_MAG(psiCscOffset[1], iSignMagBitLocOff);
    stCfg7.mCFG7_OFF3 = SIGN_MAG(psiCscOffset[2], iSignMagBitLocOff);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG0, stCfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG1, stCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG2, stCfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG3, stCfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG4, stCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG5, stCfg5.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG6, stCfg6.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C15O18F12_CFG7, stCfg7.u32);

#ifdef ENABLE_DCM_DEBUG
    UINT uiCount = 0;
    for(uiCount = 0; uiCount < 9; uiCount++){
        VPP_FE_DCM_DBG_PRINTF("Coeff[%d] = %d, In Sign magnitude format = 0x%x\n", uiCount, psiCscCoeff[uiCount], SIGN_MAG(psiCscCoeff[uiCount], iSignMagBitLocCoeff));
    }
    for(uiCount = 0; uiCount < 3; uiCount++){
        VPP_FE_DCM_DBG_PRINTF("Offset[%d] = %d, In Sign magnitude format = 0x%x\n", uiCount, psiCscOffset[uiCount], SIGN_MAG(psiCscOffset[uiCount], iSignMagBitLocOff));
    }
#endif
    return 0;
}

static INT VPP_FE_DCM_Load_R2Y_Csc(VPP_OBJ *vpp_obj, INT uiPlaneId, INT iCscType, INT iCscMode)
{
    T32CSC_C13O18F12_CFG0 stCfg0;
    T32CSC_C13O18F12_CFG1 stCfg1;
    T32CSC_C13O18F12_CFG2 stCfg2;
    T32CSC_C13O18F12_CFG3 stCfg3;
    T32CSC_C13O18F12_CFG4 stCfg4;
    T32CSC_C13O18F12_CFG5 stCfg5;
    T32CSC_C13O18F12_CFG6 stCfg6;
    T32CSC_C13O18F12_CFG7 stCfg7;

    INT iSignMagBitLocCoeff = 0, iSignMagBitLocOff = 0;
    UINT32 uiRegAddr = 0;
    INT *psiCscCoeff = NULL, *psiCscOffset = NULL;

    VPP_DCM_INFO("%s: type %d mode %d\r\n", __FUNCTION__,iCscType, iCscMode);
    stCfg0.u32 = stCfg1.u32 = stCfg2.u32 = stCfg3.u32 = stCfg4.u32 = stCfg5.u32 = stCfg6.u32 = stCfg7.u32 = 0;

    uiRegAddr = DCM_CSC_C13O18F12_BASE(vpp_obj, uiPlaneId, iCscType);
    if(!uiRegAddr) {
        VPP_DCM_INFO("CSC_C13O18F12 not exists for planeID: %d\r\n", uiPlaneId);
    }

    iSignMagBitLocCoeff = 12;
    iSignMagBitLocOff  = 17;
    if(DCM_CSC_TYPE_NORMAL == iCscType){
        if(uiPlaneId != PLANE_MAIN)
            psiCscCoeff = giOutputRGB2YUV_CscCoeff_GDCM[iCscMode];
        else
            psiCscCoeff = giOutputRGB2YUV_CscCoeff_VDCM[iCscMode];
        psiCscOffset = giOutputRGB2YUV_CscOffset[iCscMode];
    } else if(DCM_CSC_TYPE_CMAP == iCscType){
        psiCscCoeff = giCMAP_InputRGB2YUV_CscCoeff[iCscMode];
        psiCscOffset = giCMAP_InputRGB2YUV_CscOffset[iCscMode];
    }

    stCfg0.mCFG0_C0 = SIGN_MAG(psiCscCoeff[0], iSignMagBitLocCoeff);
    stCfg0.mCFG0_C1 = SIGN_MAG(psiCscCoeff[1], iSignMagBitLocCoeff);
    stCfg1.mCFG1_C2 = SIGN_MAG(psiCscCoeff[2], iSignMagBitLocCoeff);
    stCfg1.mCFG1_C3 = SIGN_MAG(psiCscCoeff[3], iSignMagBitLocCoeff);
    stCfg2.mCFG2_C4 = SIGN_MAG(psiCscCoeff[4], iSignMagBitLocCoeff);
    stCfg2.mCFG2_C5 = SIGN_MAG(psiCscCoeff[5], iSignMagBitLocCoeff);
    stCfg3.mCFG3_C6 = SIGN_MAG(psiCscCoeff[6], iSignMagBitLocCoeff);
    stCfg3.mCFG3_C7 = SIGN_MAG(psiCscCoeff[7], iSignMagBitLocCoeff);
    stCfg4.mCFG4_C8 = SIGN_MAG(psiCscCoeff[8], iSignMagBitLocCoeff);

    stCfg5.mCFG5_OFF1 = SIGN_MAG(psiCscOffset[0], iSignMagBitLocOff);
    stCfg6.mCFG6_OFF2 = SIGN_MAG(psiCscOffset[1], iSignMagBitLocOff);
    stCfg7.mCFG7_OFF3 = SIGN_MAG(psiCscOffset[2], iSignMagBitLocOff);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG0, stCfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG1, stCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG2, stCfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG3, stCfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG4, stCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG5, stCfg5.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG6, stCfg6.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_CSC_C13O18F12_CFG7, stCfg7.u32);


#ifdef ENABLE_DCM_DEBUG
    UINT uiCount = 0;
    for(uiCount = 0; uiCount < 9; uiCount++){
        VPP_FE_DCM_DBG_PRINTF("Coeff[%d] = %d, In Sign magnitude format = 0x%x\n", uiCount, psiCscCoeff[uiCount], SIGN_MAG(psiCscCoeff[uiCount], iSignMagBitLocCoeff));
    }
    for(uiCount = 0; uiCount < 3; uiCount++){
        VPP_FE_DCM_DBG_PRINTF("Offset[%d] = %d, In Sign magnitude format = 0x%x\n", uiCount, psiCscOffset[uiCount], SIGN_MAG(psiCscOffset[uiCount], iSignMagBitLocOff));
    }
#endif

    return 0;
}

INT VPP_FE_DCM_Load_OutEOTF_LUT(VPP_OBJ *vpp_obj, INT uiPlaneId, INT iMode)
{
    UINT16 *pusLut = NULL, usCount = 0;
    UINT32 uiRegAddr = 0, RegValue = 0;
    UINT32 uiRegBaseAddr = 0;
    T32HDRLUT_RAM stDcmRam;

    stDcmRam.u32 = 0;
    VPP_DCM_INFO("%s:  mode %d\r\n", __FUNCTION__,iMode);

    switch(iMode){
        case VPP_FE_DCM_OUT_OETF_TYPES_HDR2SDR:
        case VPP_FE_DCM_OUT_OETF_TYPES_HLG2SDR:
            pusLut = SDROutOETF_LUT_HDR2SDR;
            break;
        case VPP_FE_DCM_OUT_OETF_TYPES_SDR2HLG:
            pusLut = HLGOutOETF_LUT_SDR2HLG;
            break;
        case VPP_FE_DCM_OUT_OETF_TYPES_HDR2HLG:
            pusLut = HLGOutOETF_LUT_HDR2HLG;
            break;
        case VPP_FE_DCM_OUT_OETF_TYPES_HLG2HDR:
            pusLut = HDR10OutOETF_LUT_HLG2HDR;
            break;
        case VPP_FE_DCM_OUT_OETF_TYPES_SDR2HDR:
            pusLut = HDR10OutOETF_LUT_SDR2HDR;
            break;
        default:
            pusLut = SDROutOETF_LUT_HDR2SDR;
            break;
    }

    //Enable loading of IEOTF LUT
    stDcmRam.uRAM_rw_en = 1;
    stDcmRam.uRAM_rd_sel = 0;
    stDcmRam.uRAM_hdr_sel = 0;
    uiRegBaseAddr = DCM_LUT_BASE(vpp_obj, uiPlaneId);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegBaseAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    uiRegAddr = uiRegBaseAddr + RA_HDRLUT_BANK1;
    //Load LUT into registers.
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\tVDCM EOTF LUT is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    for(usCount = 0; usCount < VPP_FE_DCM_EOTF_OETF_LUT_SIZE; usCount += 2){
        RegValue = VPP_FE_DCM_PACK_EOTF_OETF(pusLut[usCount], pusLut[usCount + 1]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, RegValue);
        uiRegAddr += 4;
        VPP_FE_DCM_DBG_PRINTF("0x%x = 0x%x %d %d \n", uiRegAddr, RegValue, pusLut[usCount], pusLut[usCount + 1]);
    }

    stDcmRam.uRAM_rw_en = 0;
    stDcmRam.uRAM_rd_sel = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegBaseAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    return 0;
}

INT VPP_FE_DCM_Load_InOETF_LUT(VPP_OBJ *vpp_obj, INT planeID, INT iMode)
{
    UINT16 *pusLut = NULL, usCount = 0;
    UINT32 uiRegAddr = 0, uiRegValue = 0;
    UINT32 uiRegBaseAddr = 0;
    T32HDRLUT_RAM stDcmRam;

    stDcmRam.u32 = 0;
    VPP_DCM_INFO("%s:  mode %d\r\n", __FUNCTION__,iMode);

    switch(iMode){
        //We need to load In Luts for Out OETF
        case VPP_FE_DCM_IN_EOTF_TYPES_SDR:
            pusLut = SDRInEOTF_LUT;
            break;
        case VPP_FE_DCM_IN_EOTF_TYPES_HLG:
            pusLut = HLGInEOTF_LUT;
            break;
        case VPP_FE_DCM_IN_EOTF_TYPES_HDR2SDR:
            pusLut = HDR10InEOTF_LUT_SDROut;
            break;
        case VPP_FE_DCM_IN_EOTF_TYPES_HDR2HLG:
            pusLut = HDR10InEOTF_LUT_HLGOut;
            break;
        default:
            pusLut = SDRInEOTF_LUT;
            break;
    }

    //Enable loading of IEOTF LUT
    stDcmRam.uRAM_rw_en = 1;
    stDcmRam.uRAM_rd_sel = 0;
    uiRegBaseAddr = DCM_LUT_BASE(vpp_obj, planeID);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegBaseAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    uiRegAddr = uiRegBaseAddr + RA_HDRLUT_BANK0;
    //Load LUT into registers.
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\tVDCM OETF LUT is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    for(usCount = 0; usCount < VPP_FE_DCM_EOTF_OETF_LUT_SIZE; usCount += 2){
        uiRegValue = VPP_FE_DCM_PACK_EOTF_OETF(pusLut[usCount], pusLut[usCount + 1]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr , uiRegValue);
        uiRegAddr += 4;
        VPP_FE_DCM_DBG_PRINTF("0x%x = 0x%x %d %d \n", uiRegAddr, uiRegValue, pusLut[usCount], pusLut[usCount + 1]);
    }

    stDcmRam.uRAM_rw_en = 0;
    stDcmRam.uRAM_rd_sel = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegBaseAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    return 0;
}

static INT VPP_FE_DCM_Load_LSH_2D_LUT_Bypass(VPP_OBJ *vpp_obj, INT uiPlaneId)
{
    UINT16 LSH_2D_LUT00[VPP_FE_DCM_LSH_2D_LUT_SEG_0_SIZE][3] = {0};
    UINT16 LSH_2D_LUT01[VPP_FE_DCM_LSH_2D_LUT_SEG_1_SIZE][3] = {0};
    UINT16 LSH_2D_LUT10[VPP_FE_DCM_LSH_2D_LUT_SEG_2_SIZE][3] = {0};
    UINT16 LSH_2D_LUT11[VPP_FE_DCM_LSH_2D_LUT_SEG_3_SIZE][3] = {0};

    UINT32 uiCount = 0, uiRegAddr = 0, uiBaseRegAddr = 0, uiRegValue = 0;
    T32HDRLUT_RAM stDcmRam;

    VPP_DCM_INFO("%s:  pl %d\r\n", __FUNCTION__,uiPlaneId);
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_0_SIZE; uiCount++){
        if(uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_0_SIZE){
            LSH_2D_LUT00[uiCount][0] = 4095;
            LSH_2D_LUT00[uiCount][1] = 4095;
            LSH_2D_LUT00[uiCount][2] = 0;
        }
        if(uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_1_SIZE){
            LSH_2D_LUT01[uiCount][0] = 4095;
            LSH_2D_LUT01[uiCount][1] = 4095;
            LSH_2D_LUT01[uiCount][2] = 0;
            LSH_2D_LUT10[uiCount][0] = 4095;
            LSH_2D_LUT10[uiCount][1] = 4095;
            LSH_2D_LUT10[uiCount][2] = 0;
        }
        if(uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_3_SIZE){
            LSH_2D_LUT11[uiCount][0] = 4095;
            LSH_2D_LUT11[uiCount][1] = 4095;
            LSH_2D_LUT11[uiCount][2] = 0;
        }
    }

    stDcmRam.u32 = 0;
    //Enable loading of IEOTF LUT
    stDcmRam.uRAM_rw_en = 1;
    stDcmRam.uRAM_rd_sel = 0;
    stDcmRam.uRAM_hdr_sel = 0;

    uiBaseRegAddr = DCM_LUT_BASE(vpp_obj, uiPlaneId);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiBaseRegAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT00 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    uiRegAddr = uiBaseRegAddr;
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_0_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(LSH_2D_LUT00[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(LSH_2D_LUT00[uiCount][0], LSH_2D_LUT00[uiCount][1], LSH_2D_LUT00[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("0x%x = 0x%x \n ", uiRegAddr, uiRegValue);
    }

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT01 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_1_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(LSH_2D_LUT01[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(LSH_2D_LUT01[uiCount][0], LSH_2D_LUT01[uiCount][1], LSH_2D_LUT01[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("0x%x = 0x%x \n ", uiRegAddr, uiRegValue);
    }

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT10 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_2_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(LSH_2D_LUT10[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(LSH_2D_LUT10[uiCount][0], LSH_2D_LUT10[uiCount][1], LSH_2D_LUT10[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("0x%x = 0x%x \n ", uiRegAddr, uiRegValue);
    }

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT11 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_3_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(LSH_2D_LUT11[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(LSH_2D_LUT11[uiCount][0], LSH_2D_LUT11[uiCount][1], LSH_2D_LUT11[uiCount][2]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("0x%x = 0x%x \n ", uiRegAddr, uiRegValue);
    }

    stDcmRam.uRAM_rw_en = 0;
    stDcmRam.uRAM_rd_sel = 0;
    stDcmRam.uRAM_hdr_sel = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiBaseRegAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    return 0;
}


static INT VPP_FE_DCM_Load_LSH_2D_LUT(VPP_OBJ *vpp_obj, INT uiPlaneId, INT iLutType)
{
    UINT32 uiCount = 0, uiRegAddr = 0, uiRegValue = 0;
    UINT32 uiRegBaseAddr = 0;
    T32HDRLUT_RAM stDcmRam;
    INT (*piLut)[6] = NULL;

    stDcmRam.u32 = 0;
    VPP_DCM_INFO("%s:  luttype %d\r\n", __FUNCTION__,iLutType);

    //Enable loading of IEOTF LUT
    stDcmRam.uRAM_rw_en = 1;
    stDcmRam.uRAM_rd_sel = 0;
    stDcmRam.uRAM_hdr_sel = 0;
    uiRegBaseAddr = DCM_LUT_BASE(vpp_obj, uiPlaneId);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegBaseAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    uiRegAddr = uiRegBaseAddr;

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT00 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    if(uiPlaneId == PLANE_MAIN)
        piLut = LSH2DLUT[iLutType][0];
    else
        piLut = LSH2DLUT_GDCM[iLutType][0];
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_0_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("%d %d %d\n ", piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
    }

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT01 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    if(uiPlaneId == PLANE_MAIN)
        piLut = LSH2DLUT[iLutType][1];
    else
        piLut = LSH2DLUT_GDCM[iLutType][1];
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_1_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("%d %d %d\n ", piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
    }

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT10 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    if(uiPlaneId == PLANE_MAIN)
        piLut = LSH2DLUT[iLutType][2];
    else
        piLut = LSH2DLUT_GDCM[iLutType][2];
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_2_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("%d %d %d\n ", piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
    }

    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t VDCM LUT11 is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    if(uiPlaneId == PLANE_MAIN)
        piLut = LSH2DLUT[iLutType][3];
    else
        piLut = LSH2DLUT_GDCM[iLutType][3];
    for(uiCount = 0; uiCount < VPP_FE_DCM_LSH_2D_LUT_SEG_3_SIZE; uiCount++){
        uiRegValue = VPP_FE_DCM_LSH_PACK_MSB(piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK3, uiRegValue);
        uiRegValue = VPP_FE_DCM_LSH_PACK_LSB(piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDRLUT_BANK2, uiRegValue);
        uiRegAddr = uiRegAddr + 4;

        VPP_FE_DCM_DBG_PRINTF("%d %d %d\n ", piLut[uiCount][3], piLut[uiCount][4], piLut[uiCount][5]);
    }

    stDcmRam.uRAM_rw_en = 0;
    stDcmRam.uRAM_rd_sel = 0;
    stDcmRam.uRAM_hdr_sel = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegBaseAddr + RA_HDRLUT_RAM, stDcmRam.u32);

    return 0;
}

static void VPPLoadDcmGainThrLut(UINT32 DcmGainThrLutBaseAddr, BCMBUF *pbcmbuf, UINT16 *pLut)
{
    UINT32 uiCount = 0, uiRegOffset = 0;
    T32YSCNT_CNT stYsCnt;
    stYsCnt.u32 = 0;
    for(uiCount = 0; uiCount < MAX_THR_GAIN_ENTRY; uiCount += 3){
        stYsCnt.uCNT_VAL_0i = pLut[uiCount];
        stYsCnt.uCNT_VAL_1i = pLut[uiCount + 1];
        stYsCnt.uCNT_VAL_2i = pLut[uiCount + 2];

        VPP_BCMBUF_Write(pbcmbuf, DcmGainThrLutBaseAddr + uiRegOffset, stYsCnt.u32);
        VPP_FE_DCM_DBG_PRINTF("Y_S_THR_GAIN: 0x%x = 0x%x, %d, %d, %d \n ", DcmGainThrLutBaseAddr + uiRegOffset, stYsCnt.u32, pLut[uiCount],  pLut[uiCount + 1], pLut[uiCount + 2]);

        uiRegOffset += 4;
    }
}

static INT VPP_FE_DCM_Load_Y_S_GAIN_THR_LUT(VPP_OBJ *vpp_obj, INT planeID, UINT32 uiMode)
{
    UINT32 GainAddr = 0, ThrAddr = 0;
    VPP_DCM_INFO("%s:  mode %d\r\n", __FUNCTION__,uiMode);

    //Load Y and S GAIN LUT
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t Y & S GAIN is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    GainAddr = DCM_REG_BASE(vpp_obj, planeID) + DCM_REG(planeID, YGAIN);
    VPPLoadDcmGainThrLut(GainAddr, vpp_obj->pVbiBcmBuf, gusYgain[uiMode]);
    GainAddr = DCM_REG_BASE(vpp_obj, planeID) + DCM_REG(planeID, SGAIN);
    VPPLoadDcmGainThrLut(GainAddr, vpp_obj->pVbiBcmBuf, gusSgain[uiMode]);

    //Load Y and S Thr LUT
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");
    VPP_FE_DCM_DBG_PRINTF("\t Y & S Thresholds is :\n");
    VPP_FE_DCM_DBG_PRINTF("******************************** :\n");

    ThrAddr = DCM_REG_BASE(vpp_obj, planeID) + DCM_REG(planeID, YTHR);
    VPPLoadDcmGainThrLut(ThrAddr, vpp_obj->pVbiBcmBuf, gusYthr[uiMode]);
    ThrAddr = DCM_REG_BASE(vpp_obj, planeID) + DCM_REG(planeID, STHR);
    VPPLoadDcmGainThrLut(ThrAddr, vpp_obj->pVbiBcmBuf, gusSthr[uiMode]);

    return 0;
}

INT VPP_FE_DCM_ConfigAoiWindow(VPP_OBJ *vpp_obj, UINT32 uiEnable, VPP_WIN *pstAoiWin)
{
    T32VDCM_HWIN stHwin;
    T32VDCM_VWIN stVwin;
    T32VDCM_YAOI stYaoi;
    T32VDCM_CAOI stCaoi;
    PLANE *plane = NULL;
    UINT32 uiBaseAddr = 0;

    plane = &(vpp_obj->plane[PLANE_MAIN]);

    stHwin.u32 = stVwin.u32 = stYaoi.u32 = stCaoi.u32 = 0;

    stYaoi.u32 = plane->m_uiDcmAoiReg;

    stHwin.uHWIN_STR = pstAoiWin->x;
    stHwin.uHWIN_END = pstAoiWin->x + pstAoiWin->width;

    stVwin.uVWIN_STR = pstAoiWin->y;
    stVwin.uVWIN_END = pstAoiWin->y + pstAoiWin->height;

    if(uiEnable){
        stYaoi.uYAOI_aoi_enb = 1;
    }else{
        stYaoi.uYAOI_aoi_enb = 0;
    }
    stYaoi.uYAOI_YBLK = 0;

    stCaoi.uCAOI_UBLK = 0x8080;
    stCaoi.uCAOI_VBLK = 0x8080;

    uiBaseAddr = DCM_REG_BASE(vpp_obj, PLANE_MAIN);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiBaseAddr + RA_VDCM_HWIN, stHwin.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiBaseAddr + RA_VDCM_VWIN, stVwin.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiBaseAddr + RA_VDCM_YAOI, stYaoi.u32);

    plane->m_uiDcmAoiReg = stYaoi.u32;
    return 0;
}

INT VPP_FE_DCM_ConfigSnrFullRangeExpansion(VPP_OBJ *vpp_obj, INT32 uiPlaneId, INT32 iHdrMode)
{
    UINT32 uiRegAddr = 0;
    PLANE *plane = &vpp_obj->plane[uiPlaneId];
    DCM_PARAM *pstVdcmParams;
    T32VDCM_CFG0 stVdcmCfg0;
    T32VDCM_CFG1 stVdcmCfg1;
    T32VDCM_CFG2 stVdcmCfg2;
    T32VDCM_CFG3 stVdcmCfg3;
    T32VDCM_CFG4 stVdcmCfg4;
    T32VDCM_CFG5 stVdcmCfg5;
    T32MAINPL_CTRL4 stMainPlCtrl4;

    VPP_DCM_INFO("%s:  pl %d mode %d\r\n", __FUNCTION__,uiPlaneId, iHdrMode);
    stVdcmCfg0.u32 = stVdcmCfg1.u32 = stVdcmCfg2.u32 = stVdcmCfg3.u32 = stVdcmCfg4.u32 = stVdcmCfg5.u32 = 0 ;

    if(uiPlaneId != PLANE_MAIN){
        pstVdcmParams = &gstGdcmParams[iHdrMode];
    }else{
        pstVdcmParams = &gstVdcmParams[iHdrMode];
    }

    uiRegAddr = DCM_REG_BASE(vpp_obj, uiPlaneId);

    stVdcmCfg0.uCFG0_FRbypass = pstVdcmParams->m_iFrBypass;
    stVdcmCfg0.uCFG0_SNRbypass = pstVdcmParams->m_iSnrBypass;
    stVdcmCfg0.uCFG0_IOETFbypass = pstVdcmParams->m_iOetfBypass;
    stVdcmCfg0.uCFG0_CMPbypass = pstVdcmParams->m_iCmapBypass;
    stVdcmCfg0.uCFG0_IEOTFbypass = pstVdcmParams->m_iEotfBypass;

    stVdcmCfg1.uCFG1_min        = pstVdcmParams->m_iFrMin;
    stVdcmCfg1.uCFG1_max        = pstVdcmParams->m_iFrMax;
    stVdcmCfg2.uCFG2_ofs        = pstVdcmParams->m_iFrOffset;
    stVdcmCfg2.uCFG2_gain       = pstVdcmParams->m_iFrGain;
    stVdcmCfg3.uCFG3_snrgain    = pstVdcmParams->m_iSnrGain;
    stVdcmCfg3.uCFG3_snrlimit   = pstVdcmParams->m_iSnrLimit;
    stVdcmCfg3.uCFG3_snrofs     = pstVdcmParams->m_iSnrOffset;
    stVdcmCfg4.uCFG4_ghue       = pstVdcmParams->m_iGhue;
    stVdcmCfg4.uCFG4_gsat       = pstVdcmParams->m_iGsat;
    stVdcmCfg5.uCFG5_glgt       = pstVdcmParams->m_iGlgt;

    if(PLANE_MAIN == uiPlaneId) {
        stMainPlCtrl4.u32 = vpp_obj->mainpl_ctrl4.u32;
        stMainPlCtrl4.uCTRL4_vdcm_snr_bypass = pstVdcmParams->m_iSnrBypass;
        stMainPlCtrl4.uCTRL4_vdcm_CRMap_bypass = pstVdcmParams->m_iCmapBypass;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);
        vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
        plane->stVdcmCfg0.u32 = stVdcmCfg0.u32;
    }

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId, CFG0) , stVdcmCfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId, CFG1) , stVdcmCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId, CFG2) , stVdcmCfg2.u32);
    if(PLANE_MAIN == uiPlaneId) {
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_VDCM_CFG3 , stVdcmCfg3.u32);
    }
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId, CFG4) , stVdcmCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId, CFG5) , stVdcmCfg5.u32);


    VPP_FE_DCM_DBG_PRINTF("Full Range: Bypass = %d, Min = %d, Max = %d, Gain = %d Offset = %d \n", pstVdcmParams->m_iFrBypass, pstVdcmParams->m_iFrMin, pstVdcmParams->m_iFrMax,  pstVdcmParams->m_iFrGain, pstVdcmParams->m_iFrOffset);
    VPP_FE_DCM_DBG_PRINTF("m_iFrBypass = %d, m_iSnrBypass = %d, m_iOetfBypass = %d, m_iCmapBypass = %d, EotfBypass = %d\n", pstVdcmParams->m_iFrBypass, pstVdcmParams->m_iSnrBypass, pstVdcmParams->m_iOetfBypass,  pstVdcmParams->m_iCmapBypass,  pstVdcmParams->m_iEotfBypass);
    VPP_FE_DCM_DBG_PRINTF("SNR: Gain = %d, Limit = %d, Offset = %d\n", pstVdcmParams->m_iSnrGain, pstVdcmParams->m_iSnrLimit, pstVdcmParams->m_iSnrOffset);
    VPP_FE_DCM_DBG_PRINTF("GHUe = %d, GSAT = %d, GLGT = %d\n", pstVdcmParams->m_iGhue,  pstVdcmParams->m_iGsat, pstVdcmParams->m_iGlgt);

    return 0;
}

INT VPP_FE_DCM_Init(VPP_OBJ *vpp_obj)
{
    T32MAINPL_CTRL4 stMainPlCtrl4;
    /**
      Remove VDCM bypass. Otherwise clock is disabled and LUTs will not be loaded.
     **/
    VPP_DCM_INFO("Enabling clock for VDCM\n");
    stMainPlCtrl4.u32 = vpp_obj->mainpl_ctrl4.u32;

    stMainPlCtrl4.uCTRL4_vdcm_bypass = 0;
    stMainPlCtrl4.uCTRL4_VDCM_VITM_sel  = 1;
    stMainPlCtrl4.uCTRL4_vdcm_snr_bypass = 0;
    stMainPlCtrl4.uCTRL4_vdcm_CRMap_bypass = 0;
    stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_bypass = 0;
    stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_sel = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

    VPP_DCM_INFO("Disabling clock for VDCM\n");
    stMainPlCtrl4.uCTRL4_vdcm_bypass = 1;
    stMainPlCtrl4.uCTRL4_VDCM_VITM_sel  = 0;
    stMainPlCtrl4.uCTRL4_vdcm_snr_bypass = 1;
    stMainPlCtrl4.uCTRL4_vdcm_CRMap_bypass = 1;
    stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_bypass = 1;
    stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_sel = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

    vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;

    return 0;
}


INT VPP_FE_DCM_GDCM_Init(VPP_OBJ *vpp_obj, INT uiPlaneId)
{
    /**
      Remove GDCM bypass. Otherwise clock is disabled and LUTs will not be loaded.
     **/
    VPP_DCM_INFO("Enabling clock for GDCM\n");
#ifdef ENABLE_PIP_GFX0_PATH
    if(PLANE_PIP == uiPlaneId){
        vpp_obj->gfx0_ctrl0.uCTRL0_gdcm_bypass = 0;
        vpp_obj->gfx0_ctrl1.uCTRL1_fall_bk_en = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL1, vpp_obj->gfx0_ctrl1.u32);
    } else
#endif
    if(PLANE_GFX1 == uiPlaneId){
        vpp_obj->gfx1_ctrl1.uCTRL1_gdcm_bypass = 0;
        vpp_obj->gfx1_ctrl1.uCTRL1_fall_bk_en = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL1, vpp_obj->gfx1_ctrl1.u32);
    }

    /**
      Bypass GDCM. Will be enabled once GDCM is in path.
     **/
    VPP_DCM_INFO("Disabling clock for GDCM\n");
#ifdef ENABLE_PIP_GFX0_PATH
    if(PLANE_PIP == uiPlaneId){
        vpp_obj->gfx0_ctrl0.uCTRL0_gdcm_bypass = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL1, vpp_obj->gfx0_ctrl1.u32);
    } else
#endif
    if(PLANE_GFX1 == uiPlaneId){
        vpp_obj->gfx1_ctrl1.uCTRL1_gdcm_bypass = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL1, vpp_obj->gfx1_ctrl1.u32);
    }

    return 0;
}

INT VPP_FE_DCM_ConfigLUTs(VPP_OBJ *vpp_obj, INT32 uiPlaneId, INT32 uiInHdrMode, INT32 OutHdrMode)
{
    INT iInCscMode = 0, iInEotf = 0, iCmapInCsc = 0, iCmapOutCsc = 0, iOutOetf = 0, iOutCscMode = 0, iVdcmParams = 0, iLsh2D = LSH_2D_LUT_MAX;
    UINT32 uiY_S_Thr_Gain = 0;

    VPP_DCM_INFO("%s:  pl %d mode %d %d\r\n", __FUNCTION__,uiPlaneId, uiInHdrMode, OutHdrMode);
    VPP_FE_DCM_DBG_PRINTF("In HDR Mode = %d, Out Hdr Mode = %d\n", uiInHdrMode, OutHdrMode);
    //Configure input section of VDCM
    switch(uiInHdrMode){
        case VPP_FE_DCM_IN_MODE_SDR_709:
        default:
            {
                //Input Section of VDCM
                iInCscMode = VPP_FE_DCM_INPUT_CSC_YUV2GBR_SDR;
                iInEotf = VPP_FE_DCM_IN_EOTF_TYPES_SDR;

                //Output Section of VDCM
                iCmapInCsc = VPP_FE_DCM_CMAP_INPUT_CSC_GBR2YUV_SDR;

                switch(OutHdrMode){
                    case VPP_FE_DCM_OUT_MODE_SDR_709:
                    default:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_SDR2SDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_HDR2SDR;    //Fix Me
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HDR2SDR;
                            iVdcmParams = DCM_PARAMS_TYPES_BYPASS;
                            break;
                        }
                    case VPP_FE_DCM_OUT_MODE_HLG:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_SDR2HDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_SDR2HLG;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_SDR2HLG;
                            iVdcmParams = DCM_PARAMS_TYPES_SDR2HLG;
                            iLsh2D = LSH_2D_LUT_SDR2HLG;
                            break;
                        }
                    case VPP_FE_DCM_OUT_MODE_HDR10:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_SDR2HDR;
                            iOutOetf = (INT)VPP_FE_DCM_OUT_OETF_TYPES_SDR2HDR;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_SDR2HDR;
                            iVdcmParams = DCM_PARAMS_TYPES_SDR2HDR;
                            iLsh2D = LSH_2D_LUT_SDR2HDR;
                            break;
                        }
                }
            }
            break;

        case VPP_FE_DCM_IN_MODE_HLG:
            {
                //Input Section of VDCM
                iInCscMode = VPP_FE_DCM_INPUT_CSC_YUV2GBR_HDR;
                iInEotf = VPP_FE_DCM_IN_EOTF_TYPES_HLG;

                //Output Section of VDCM
                iCmapInCsc = VPP_FE_DCM_CMAP_INPUT_CSC_GBR2YUV_HLG;

                switch(OutHdrMode){
                    case VPP_FE_DCM_OUT_MODE_SDR_709:
                    default:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_HDR2SDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_HLG2SDR;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HLG2SDR;
                            iVdcmParams = DCM_PARAMS_TYPES_HLG2SDR;
                            iLsh2D = LSH_2D_LUT_HLG2SDR;
                            break;
                        }
                    case VPP_FE_DCM_OUT_MODE_HLG:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_HDR2SDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_SDR2HLG;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HDR2HLG;
                            iVdcmParams = DCM_PARAMS_TYPES_BYPASS;
                            break;
                        }
                    case VPP_FE_DCM_OUT_MODE_HDR10:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_HDR2SDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_HLG2HDR;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HLG2HDR;
                            iVdcmParams = DCM_PARAMS_TYPES_HLG2HDR;
                            iLsh2D = LSH_2D_LUT_HLG2HDR;
                            break;
                        }
                }
            }
            break;
        case VPP_FE_DCM_IN_MODE_HDR10:
            {
                //Input Section of VDCM
                iInCscMode = VPP_FE_DCM_INPUT_CSC_YUV2GBR_HDR;
                iInEotf = VPP_FE_DCM_IN_EOTF_TYPES_HDR2SDR;

                //Output Section of VDCM
                iCmapInCsc = VPP_FE_DCM_CMAP_INPUT_CSC_GBR2YUV_HDR;

                switch(OutHdrMode){
                    case VPP_FE_DCM_OUT_MODE_SDR_709:
                    default:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_HDR2SDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_HDR2SDR;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HDR2SDR;
                            iVdcmParams = DCM_PARAMS_TYPES_HDR2SDR;
                            iLsh2D = LSH_2D_LUT_HDR2SDR;
                            break;
                        }
                    case VPP_FE_DCM_OUT_MODE_HLG:
                        {
                            iInEotf = VPP_FE_DCM_IN_EOTF_TYPES_HDR2HLG;
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_HDR2SDR;
                            iOutOetf = VPP_FE_DCM_OUT_OETF_TYPES_HDR2HLG;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HDR2HLG;
                            iVdcmParams = DCM_PARAMS_TYPES_HDR2HLG;
                            iLsh2D = LSH_2D_LUT_HDR2HLG;
                            break;
                        }
                    case VPP_FE_DCM_OUT_MODE_HDR10:
                        {
                            iCmapOutCsc = VPP_FE_DCM_CMAP_OUTPUT_CSC_YUV2GBR_HDR2SDR;
                            iOutOetf = (INT)HDR10OutOETF_LUT_HLG2HDR;
                            iOutCscMode = VPP_FE_DCM_OUTPUT_CSC_GBR2YUV_HDR2HLG;
                            iVdcmParams = DCM_PARAMS_TYPES_BYPASS;
                            break;
                        }
                }
            }
            break;
    }

    VPP_FE_DCM_Load_Y_S_GAIN_THR_LUT(vpp_obj, uiPlaneId, uiY_S_Thr_Gain);
    VPP_FE_DCM_ConfigSnrFullRangeExpansion(vpp_obj, uiPlaneId, iVdcmParams);

    //Configure Input Section
    if(PLANE_MAIN == uiPlaneId)
        VPP_FE_DCM_Load_Y2R_Csc(vpp_obj, uiPlaneId, DCM_CSC_TYPE_NORMAL, iInCscMode);
    VPP_FE_DCM_Load_OutEOTF_LUT(vpp_obj, uiPlaneId, iOutOetf);

    //Configure CMAP Section.
    VPP_FE_DCM_Load_R2Y_Csc(vpp_obj, uiPlaneId, DCM_CSC_TYPE_CMAP, iCmapInCsc);
    VPP_FE_DCM_Load_Y2R_Csc(vpp_obj, uiPlaneId, DCM_CSC_TYPE_CMAP, iCmapOutCsc);

    //Configure OutOETF
    VPP_FE_DCM_Load_InOETF_LUT(vpp_obj, uiPlaneId, iInEotf);

    //Configure Out CSC
    if(PLANE_MAIN == uiPlaneId)
        VPP_FE_DCM_Load_R2Y_Csc(vpp_obj, uiPlaneId, DCM_CSC_TYPE_NORMAL, iOutCscMode);

    if(LSH_2D_LUT_MAX == iLsh2D){
        VPP_FE_DCM_Load_LSH_2D_LUT_Bypass(vpp_obj, uiPlaneId);
    } else{
        VPP_FE_DCM_Load_LSH_2D_LUT(vpp_obj, uiPlaneId, iLsh2D);
    }
    VPP_DCM_INFO("planeID = %d, iInCscMode = %d, iOutOetf = %d, iCmapInCsc = %d, iCmapOutCsc = %d, iInEotf = %d, iLsh2D = %d, iVdcmParams = %d, iOutCscMode = %d\n", uiPlaneId, iInCscMode, iOutOetf, iCmapInCsc, iCmapOutCsc, iInEotf, iLsh2D, iVdcmParams, iOutCscMode);

    //Store OETF in plane
    vpp_obj->plane[PLANE_MAIN].m_iDcmOetf = iOutOetf;

    return 0;
}

static void GetDcmInOutMode(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo, INT *inputFmt, INT *outputFmt)
{
    static const INT output_fmt[VPP_OUTFORMAT_MAX] = {VPP_FE_DCM_OUT_MODE_SDR_709,
        VPP_FE_DCM_OUT_MODE_HDR10,
        VPP_FE_DCM_OUT_MODE_SDR_709,
        VPP_FE_DCM_OUT_MODE_HDR10,
        VPP_FE_DCM_OUT_MODE_HLG,
        VPP_FE_DCM_OUT_MODE_HDR10};

    if (isHDR(pinfo)){
        *inputFmt = VPP_FE_DCM_IN_MODE_HDR10;
    } else if(isHLG(pinfo)){
        *inputFmt = VPP_FE_DCM_IN_MODE_HLG;
    } else if(isSDR2020(pinfo)){
        *inputFmt = VPP_FE_DCM_IN_MODE_HDR10;
    } else
        *inputFmt = VPP_FE_DCM_IN_MODE_SDR_709;

    if(vpp_obj->m_outformat < VPP_OUTFORMAT_MAX) {
        *outputFmt = output_fmt[vpp_obj->m_outformat];
    } else {
        *outputFmt = VPP_FE_DCM_OUT_MODE_SDR_709;
    }
    return;
}

static void get_hdrinfo_frame(VPP_OBJ *vpp_obj, INT32 OutHdrMode)
{
    VPP_HDMI_PKT_HDR_INFO_FRAME *phdrInfo = &vpp_obj->hdrInfo;
    if(OutHdrMode == VPP_FE_DCM_IN_MODE_HDR10){
        vpp_obj->transfer_characteristics = VPP_TRANSFER_CHAR_SMPTE_ST_2048;
        phdrInfo->display_primaries_x[0] = 8500;
        phdrInfo->display_primaries_y[0] = 39850;
        phdrInfo->display_primaries_x[1] = 6550;
        phdrInfo->display_primaries_y[1] = 2300;
        phdrInfo->display_primaries_x[2] = 35400;
        phdrInfo->display_primaries_y[2] = 14600;
        phdrInfo->white_point_x = 15635;
        phdrInfo->white_point_y = 16450;

        phdrInfo->max_display_mastering_lumi = 250;
        phdrInfo->min_display_mastering_lumi = 50;
        phdrInfo->Maximum_Content_Light_Lvl = 250;
        phdrInfo->Maximum_Frame_Average_Light_Lvl = 0;
    } else if(OutHdrMode == VPP_FE_DCM_IN_MODE_HLG){
        vpp_obj->transfer_characteristics = VPP_TRANSFER_CHAR_ARIB_B67;
        memset(phdrInfo, 0, sizeof(VPP_HDMI_PKT_HDR_INFO_FRAME));
    } else{
        vpp_obj->transfer_characteristics = VPP_TRANSFER_CHAR_BT709;
    }
    return;
}

void dcmHandler(VPP_OBJ *vpp_obj,  INT uiPlaneId, VBUF_INFO *pstVbufInfo)
{
    UINT32 uiUpdateRequired = 0;
    PLANE *plane = NULL;
    T32MAINPL_CTRL4 stMainPlCtrl4;
    T32MAINPL_CTRL0 stMainPlCtrl0;
    VPP_WIN stAoiWin;
    INT inpMode,outMode = 0;

    plane = &(vpp_obj->plane[uiPlaneId]);

    //avoid configuring dcm for dolby vision enabled/IPT overlay cases
    if(((PLANE_MAIN == uiPlaneId) && pstVbufInfo->m_dv_present_mode) ||
            ((PLANE_MAIN != uiPlaneId) && VPP_DV_GetCapability(vpp_obj, DV_CAP_STATUS))){
        //Ignore dolby vision streams and reset dcm modes
        if(plane->m_iDcmOutMode != VPP_FE_DCM_OUT_MODE_MAX) {
            plane->m_iDcmInMode = VPP_FE_DCM_IN_MODE_MAX;
            plane->m_iDcmOutMode = VPP_FE_DCM_OUT_MODE_MAX;
        }
        return;
    }

    if(uiPlaneId == PLANE_MAIN){
        //Get input & output configuration
        GetDcmInOutMode(vpp_obj, pstVbufInfo, &inpMode, &outMode);

        if((plane->iDcmInHdrMode != inpMode) || (plane->iDcmOutHdrMode != outMode) || vpp_obj->need_dcm_update ){
            vpp_obj->need_dcm_update = 0;
            VPP_DCM_INFO("Fmt change: %d -> %d %d -> %d mode [%d]\r\n", plane->iDcmInHdrMode, inpMode, plane->iDcmOutHdrMode, outMode, vpp_obj->m_HdrMode);
            //Change in input output modes
            plane->iDcmInHdrMode = inpMode;
            plane->iDcmOutHdrMode = outMode;
            plane->m_uiDcmEnableUpdated = 1;
            if(inpMode != outMode){
                //enable VDCM for conversion cases
                plane->m_uiDcmEnable = 1;
            } else{
                //disable VDCM for passthrough cases
                plane->m_uiDcmEnable = 0;
                if(outMode != VPP_FE_DCM_OUT_MODE_SDR_709){  //handling PT cases in HDR_ON mode
                    VPP_BE_HDR_GetMetadata(vpp_obj, pstVbufInfo);
                }
            }
            //SetInputVideoCSC executed after dcmHandler(). Below code re-configures CSC upon output HdrMode change.
            if(outMode != VPP_FE_DCM_OUT_MODE_SDR_709)
                VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj, PLANE_MAIN, VPP_CSC_YUV_2020_TO_sRGB);
            else
                VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj, PLANE_MAIN, VPP_CSC_YUV_709_TO_sRGB);
        }
    }

    if(plane->m_uiDcmEnableUpdated){
        uiUpdateRequired = 1;
        plane->m_uiDcmEnableUpdated = 0;

        if(PLANE_MAIN == uiPlaneId) {
            int chan;
#ifdef ENABLE_PIP_GFX0_PATH
            chan = CHAN_PIP;
#else
            chan = CHAN_GFX1;
#endif
            do {
                if(CPCB_OF_CHAN(vpp_obj, chan) == CPCB_1) {
                    if(vpp_obj->m_outformat != VPP_OUTFORMAT_SDR2020) {
                        vpp_obj->plane[chan].m_uiDcmEnable = (plane->iDcmOutHdrMode != VPP_FE_DCM_OUT_MODE_SDR_709);
                    }
                    else {
                        /*disable GDCM for BT2020 output for better PQ*/
                        vpp_obj->plane[chan].m_uiDcmEnable = 0;
                    }
                    vpp_obj->plane[chan].iDcmInHdrMode = VPP_FE_DCM_IN_MODE_SDR_709;
                    vpp_obj->plane[chan].iDcmOutHdrMode = plane->iDcmOutHdrMode;
                    vpp_obj->plane[chan].m_uiDcmEnableUpdated = 1;
                }
                ++chan;
            }while(chan <= CHAN_GFX1);
        }
    }

    if(uiUpdateRequired){
        stMainPlCtrl4.u32 = vpp_obj->mainpl_ctrl4.u32;
        stMainPlCtrl0.u32 = vpp_obj->mainplCtrl0.u32;

        if(1 == plane->m_uiDcmEnable){
            if(PLANE_MAIN == uiPlaneId){
                //Disable THDR & VITM.
                stMainPlCtrl0.uCTRL0_vptop_tHdr_ctrl = 0;
                stMainPlCtrl4.uCTRL4_tHDR_bypass = 1;
                stMainPlCtrl4.uCTRL4_tHDR_DolbyCVM_sel = 0;
                stMainPlCtrl4.uCTRL4_ITMO_top_bypass = 1;
                stMainPlCtrl4.uCTRL4_ITMO_YUV444_in_sel = 0;

                if(VPP_DV_GetDvCompMode(vpp_obj)){
                    stMainPlCtrl4.uCTRL4_tHDR_DolbyCVM_sel = 1;
                }

                //Enable VDCM.
                stMainPlCtrl4.uCTRL4_vdcm_bypass = 0;
                stMainPlCtrl4.uCTRL4_VDCM_VITM_sel  = 1;
                stMainPlCtrl4.uCTRL4_vdcm_snr_bypass = 0;
                stMainPlCtrl4.uCTRL4_vdcm_CRMap_bypass = 0;
                stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_bypass = 0;
                stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_sel = 1;
            }
#ifdef ENABLE_PIP_GFX0_PATH
            else if(PLANE_PIP == uiPlaneId){
                //Enable GDCM for GFX0
                vpp_obj->gfx0_ctrl0.uCTRL0_gdcm_bypass = 0;
                vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_gdcm_sel = 1;
                vpp_obj->gfx0_ctrl1.uCTRL1_fifo_rd_sel = 2;
                vpp_obj->m_stCpcb0_ctrl10.uCTRL10_input_swap_sel2 = 0x24;
            }
#endif
            else if(PLANE_GFX1 == uiPlaneId){
                //Enable GDCM for GFX1
                vpp_obj->gfx1_ctrl1.uCTRL1_fifo_rd_sel = 2;
                vpp_obj->gfx1_ctrl1.uCTRL1_gfxpl_gdcm_sel = 1;
                vpp_obj->gfx1_ctrl1.uCTRL1_gdcm_bypass = 0;
                vpp_obj->m_stCpcb0_ctrl10.uCTRL10_input_swap_sel3 = 0x24;
            }
        } else{
            //Disable VDCM.
            if(PLANE_MAIN == uiPlaneId){
                stMainPlCtrl4.uCTRL4_vdcm_bypass = 1;
                stMainPlCtrl4.uCTRL4_VDCM_VITM_sel  = 0;
                stMainPlCtrl4.uCTRL4_vdcm_snr_bypass = 1;
                stMainPlCtrl4.uCTRL4_vdcm_CRMap_bypass = 1;
                stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_bypass = 1;
                stMainPlCtrl4.uCTRL4_vitm_vdcm_linbuff_sel = 0;
            }
#ifdef ENABLE_PIP_GFX0_PATH
            else if(PLANE_PIP == uiPlaneId){
                //Disable GDCM for GFX0
                vpp_obj->gfx0_ctrl0.uCTRL0_gdcm_bypass = 1;
                vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_gdcm_sel = 0;
                vpp_obj->gfx0_ctrl1.uCTRL1_fifo_rd_sel = 0;
                vpp_obj->m_stCpcb0_ctrl10.uCTRL10_input_swap_sel2 = 0x24;

            }
#endif
            else if(PLANE_GFX1 == uiPlaneId){
                //Enable GDCM for GFX1
                vpp_obj->gfx1_ctrl1.uCTRL1_fifo_rd_sel = 0;
                vpp_obj->gfx1_ctrl1.uCTRL1_gfxpl_gdcm_sel = 0;
                vpp_obj->gfx1_ctrl1.uCTRL1_gdcm_bypass = 1;
                vpp_obj->m_stCpcb0_ctrl10.uCTRL10_input_swap_sel3 = 0x24;
            }
            plane->m_iDcmInMode = VPP_FE_DCM_IN_MODE_MAX;
            plane->m_iDcmOutMode = VPP_FE_DCM_OUT_MODE_MAX;
        }

        if(PLANE_MAIN == uiPlaneId){
            vpp_obj->mainplCtrl0.u32 = stMainPlCtrl0.u32;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0 , stMainPlCtrl0.u32);
            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);
        }
#ifdef ENABLE_PIP_GFX0_PATH
        else if(PLANE_PIP == uiPlaneId){
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL0, vpp_obj->gfx0_ctrl0.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL1, vpp_obj->gfx0_ctrl1.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL10, vpp_obj->m_stCpcb0_ctrl10.u32);

        }
#endif
        else if(PLANE_GFX1 == uiPlaneId){
             VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL1, vpp_obj->gfx1_ctrl1.u32);
             VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL10, vpp_obj->m_stCpcb0_ctrl10.u32);
        }
    }

    if(0 == plane->m_uiDcmEnable){
        return;
    }

    if(PLANE_MAIN == uiPlaneId){
        if(plane->m_iDcmInMode != plane->iDcmInHdrMode ||
                plane->m_iDcmOutMode != plane->iDcmOutHdrMode){
            //Configure all LUTs
            VPP_FE_DCM_ConfigLUTs(vpp_obj, uiPlaneId, plane->iDcmInHdrMode, plane->iDcmOutHdrMode);

            plane->m_iDcmInMode = plane->iDcmInHdrMode;
            plane->m_iDcmOutMode = plane->iDcmOutHdrMode;
        }

        if(plane->m_uiDcmAoiWinUpdated){
            plane->m_uiDcmAoiWinUpdated = 0;

            stAoiWin.x = plane->m_stDcmAoiWin.x;
            stAoiWin.y = plane->m_stDcmAoiWin.y;
            stAoiWin.width = plane->m_stDcmAoiWin.width;
            stAoiWin.height = plane->m_stDcmAoiWin.height;

            VPP_FE_DCM_ConfigAoiWindow(vpp_obj, plane->m_uiDcmAoiEnable, &stAoiWin);
        }
        get_hdrinfo_frame(vpp_obj, plane->m_iDcmOutMode);
    } else {
        if(plane->m_iDcmOutMode != plane->iDcmOutHdrMode){
            VPP_FE_DCM_ConfigLUTs(vpp_obj, uiPlaneId, plane->iDcmInHdrMode, plane->iDcmOutHdrMode);
            plane->m_iDcmOutMode = plane->iDcmOutHdrMode;
        }
#ifdef ENABLE_PIP_GFX0_PATH
        if(PLANE_PIP == uiPlaneId){
            //Start GFX0 GDCM
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_GDCM_CTRL , 1);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_GDCM_CTRL , 0);
        }
        else
#endif
        if(PLANE_GFX1 == uiPlaneId){
            //Start GFX1 GDCM
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_GDCM_CTRL , 1);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_GDCM_CTRL , 0);
        }
    }
}

INT SetDcmPQValues(VPP_OBJ *vpp_obj, int uiPlaneId, int Hue, int Sat, int Lght)
{
    T32VDCM_CFG4 stVdcmCfg4;
    T32VDCM_CFG5 stVdcmCfg5;
    unsigned int uiRegAddr;

    uiRegAddr = DCM_REG_BASE(vpp_obj, uiPlaneId);
    GA_REG_WORD32_READ(uiRegAddr + DCM_REG(uiPlaneId,CFG4) , &stVdcmCfg4.u32);
    GA_REG_WORD32_READ(uiRegAddr + DCM_REG(uiPlaneId,CFG5) , &stVdcmCfg5.u32);
    stVdcmCfg4.uCFG4_ghue       = Hue;
    stVdcmCfg4.uCFG4_gsat       = Sat;
    stVdcmCfg5.uCFG5_glgt       = Lght;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId,CFG4) , stVdcmCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + DCM_REG(uiPlaneId,CFG5) , stVdcmCfg5.u32);

    VPP_DCM_INFO("PQ: pl %d H: %d S: %d L: %d\r\n", uiPlaneId, Hue, Sat, Lght);
    return 0;
}
#endif
#endif
