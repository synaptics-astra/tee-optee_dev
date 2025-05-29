/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CSC_ICSC_C_

#include "vpp_csc_icsc.h"
#include "vpp_csc_icsccfg.h"
#include "vpp_pq.h"

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
INT VPP_CSC_ICSC_SetDefaults(VPP_OBJ* vpp_obj,VPP_CSC_TYPE Channel)
{
    int RetCode = MV_VPP_OK;

    /* Default values to all the registers */
    if (Channel == VPP_CSC_MAIN)
        RetCode = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,VPP_CSC_YUV_601_TO_RGB);
#ifdef ENABLE_PIP_GFX0_PATH
    else if (Channel == VPP_CSC_PIP)
        RetCode = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,VPP_CSC_YUV_601_TO_RGB);
#endif
#ifdef ENABLE_GFX2_VOP2_PATH
    else if (Channel == VPP_CSC_VOP2)
        RetCode = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,VPP_CSC_RGB_TO_YUV_709);
#endif
    else if (Channel == VPP_CSC_VPTOP)
        RetCode = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,VPP_CSC_RGB_TO_YUV_709);

    return RetCode;
}

/*------------------------------------------------------------------------
 * Description: This function loads the input CSC matrices
 *
 * Arguments  :
 *                Channel  - Main/Pip
 *
 * Returns    : On success returns MV_VPP_OK .
 *
 * Notes      : None
 *------------------------------------------------------------------------
 */
INT VPP_CSC_ICSC_SetColorSpaceConversion(VPP_OBJ* vpp_obj,
        VPP_CSC_TYPE Channel,
        VPP_CSC_MODE CscMode)
{
    int retStatus = MV_VPP_OK;
    UINT32 RegAddr = 0;
    UINT32 (*pOffset)[VPP_CSC_MAX_ICSC_OFF] = 0;
    UINT32 coeff[PQ_COEFF_MATRIX_ROWS][PQ_COEFF_MATRIX_COLUMNS];
    UINT32 offset[PQ_OFFSET_MATRIX_ROWS];

    T32CSC_C17O24_CFG0 csc_cfg0;
    T32CSC_C17O24_CFG1 csc_cfg1;
    T32CSC_C17O24_CFG2 csc_cfg2;
    T32CSC_C17O24_CFG3 csc_cfg3;
    T32CSC_C17O24_CFG4 csc_cfg4;
    T32CSC_C17O24_CFG5 csc_cfg5;
    T32CSC_C17O24_CFG6 csc_cfg6;
    T32CSC_C17O24_CFG7 csc_cfg7;
    T32CSC_C17O24_CFG8 csc_cfg8;
    T32CSC_C17O24_CFG9 csc_cfg9;
    T32CSC_C17O24_CFG10 csc_cfg10;
    T32CSC_C17O24_CFG11 csc_cfg11;

    if ((Channel < VPP_CSC_MAIN) && (Channel >= VPP_MAX_CSC))
        return MV_VPP_EBADPARAM;
    if (!((CscMode >= VPP_CSC_BYPASS_MODE && CscMode < VPP_CSC_MAX_CONV_TYPES)))
        return MV_VPP_EBADPARAM;

    if (Channel == VPP_CSC_MAIN) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_MainPl_CSC;
        pOffset = gICscWindowOffset_12bits;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if (Channel == VPP_CSC_PIP) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_PIP_CSC;
        pOffset = gICscWindowOffset_10bits;
    }
#endif
#ifdef ENABLE_GFX2_VOP2_PATH
    else if (Channel == VPP_CSC_VOP2) {
        RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2+ RA_VOP2_CSC ;
        pOffset = gICscWindowOffset_12bits;
    }
#endif
    else if (Channel == VPP_CSC_VPTOP) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP_CSC;
        pOffset = gICscWindowOffset_10bits;
    }

    if((Channel == VPP_CSC_VOP2) || (Channel == VPP_CSC_VPTOP)) {
        csc_cfg0.mCFG0_C0 = gICscWindowCoeff[CscMode][0][0];
        csc_cfg1.mCFG1_C1 = gICscWindowCoeff[CscMode][0][1];
        csc_cfg2.mCFG2_C2 = gICscWindowCoeff[CscMode][0][2];
        csc_cfg3.mCFG3_C3 = gICscWindowCoeff[CscMode][1][0];
        csc_cfg4.mCFG4_C4 = gICscWindowCoeff[CscMode][1][1];
        csc_cfg5.mCFG5_C5 = gICscWindowCoeff[CscMode][1][2];
        csc_cfg6.mCFG6_C6 = gICscWindowCoeff[CscMode][2][0];
        csc_cfg7.mCFG7_C7 = gICscWindowCoeff[CscMode][2][1];
        csc_cfg8.mCFG8_C8 = gICscWindowCoeff[CscMode][2][2];

        csc_cfg9.mCFG9_OFF1 = pOffset[CscMode][0];
        csc_cfg10.mCFG10_OFF2 = pOffset[CscMode][1];
        csc_cfg11.mCFG11_OFF3 = pOffset[CscMode][2];
    }
    else {
        retStatus = VPP_PQ_SetupGlobalCSC(vpp_obj,Channel,CscMode);
        if(retStatus != MV_VPP_OK)
            return MV_VPP_EBADPARAM;
        /* fill the coeff and offset and use to reg write */
        retStatus = VPP_PQ_CalcAndSetPQReg(vpp_obj,Channel,coeff,offset);
        if(retStatus != MV_VPP_OK)
            return MV_VPP_EBADPARAM;
        csc_cfg0.mCFG0_C0 = coeff[0][0];
        csc_cfg1.mCFG1_C1 = coeff[0][1];
        csc_cfg2.mCFG2_C2 = coeff[0][2];
        csc_cfg3.mCFG3_C3 = coeff[1][0];
        csc_cfg4.mCFG4_C4 = coeff[1][1];
        csc_cfg5.mCFG5_C5 = coeff[1][2];
        csc_cfg6.mCFG6_C6 = coeff[2][0];
        csc_cfg7.mCFG7_C7 = coeff[2][1];
        csc_cfg8.mCFG8_C8 = coeff[2][2];

        csc_cfg9.mCFG9_OFF1 = offset[0];
        csc_cfg10.mCFG10_OFF2 = offset[1];
        csc_cfg11.mCFG11_OFF3 = offset[2];
    }
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG0, csc_cfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG1, csc_cfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG2, csc_cfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG3, csc_cfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG4, csc_cfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG5, csc_cfg5.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG6, csc_cfg6.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG7, csc_cfg7.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG8, csc_cfg8.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG9, csc_cfg9.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG10, csc_cfg10.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_CSC_C17O24_CFG11, csc_cfg11.u32);

    return retStatus;
}

/***************************************************************************************
 * Description: set brightness
 * PARAMS: Channel - channel selection
 *         Brightness - brightness value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load brightness value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetBrightness(VPP_OBJ *vpp_obj, VPP_CSC_TYPE Channel, INT16 Brightness)
{
    INT retStatus = MV_VPP_OK;
    float BrightVal;

    Brightness = VPP_CSC_CAP_VALUE(Brightness, 0, VPP_CSC_BRIGHTNESS_CAP_VALUE_MAX);

    //map to [GCSC_MIN_BRIGHTNESS to GCSC_MAX_BRIGHTNESS]
    BrightVal = (float)Brightness * VPP_CSC_BRIGHTNESS_RANGE_FACTOR - GCSC_MAX_BRIGHTNESS;

    if ((BrightVal < GCSC_MIN_BRIGHTNESS || BrightVal > GCSC_MAX_BRIGHTNESS))
        return MV_VPP_EBADPARAM;

    vpp_obj->gBrightness[Channel] = BrightVal;

    retStatus = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,gMode[Channel]);
    return retStatus;
}

/***************************************************************************************
 * Description: set contrast
 * PARAMS: Channel - channel selection
 *         Contrast - contrast value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load contrast value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetContrast(VPP_OBJ *vpp_obj, INT Channel, INT16 Contrast)
{
    INT retStatus = MV_VPP_OK;
    float   tempContrast;

    Contrast = VPP_CSC_CAP_VALUE(Contrast, 0, 500);

    tempContrast = (float)Contrast * 0.004;

    VPPDBG_PQ_LOG("tempContrast val to process =%lf \n",tempContrast);
    if ((tempContrast < GCSC_MIN_CONTRAST)  || (tempContrast > GCSC_MAX_CONTRAST))
        return MV_VPP_EBADPARAM;

    vpp_obj->gContrast[Channel] = tempContrast;
    retStatus = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,gMode[Channel]);
    return retStatus;
}

/***************************************************************************************
 * Description: set hue
 * PARAMS: Channel - channel selection
 *         Hue - hue value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load hue value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetHue(VPP_OBJ *vpp_obj, INT Channel, INT16 Hue)
{
    float HueVal;
    INT retStatus = MV_VPP_OK;

    Hue = Hue * 2;
    Hue = VPP_CSC_CAP_VALUE(Hue, 0, 1000);

    if (Hue >= 500)
        HueVal = (float)Hue * 0.36 - 180;
    else
        HueVal = (float)Hue * 0.36 + 180;

    VPPDBG_PQ_LOG("HueVal to process =%lf \n",HueVal);

    if ((HueVal < GCSC_MIN_HUE)  || (HueVal > GCSC_MAX_HUE))
        return MV_VPP_EBADPARAM;

    vpp_obj->gHue[Channel] = HueVal;
    retStatus = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,gMode[Channel]);
    return retStatus;
}

/***************************************************************************************
 * Description: set saturation
 * PARAMS: Channel - channel selection
 *         Saturation - saturation value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load saturation value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetSaturation(VPP_OBJ *vpp_obj, INT Channel, INT16 Sat)
{
    INT retStatus = MV_VPP_OK;
    float Saturation;

    Sat = VPP_CSC_CAP_VALUE(Sat, 0, 500);

    Saturation = (float)Sat * 0.004;
    VPPDBG_PQ_LOG("Saturation val to process =%lf \n",Saturation);
    if ((Saturation < GCSC_MIN_SAT)  || (Saturation > GCSC_MAX_SAT))
        return MV_VPP_EBADPARAM;

    vpp_obj->gSaturation[(Channel)] = Saturation;
    retStatus = VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,Channel,gMode[Channel]);
    return retStatus;
}

INT VPP_CSC_GetBCHS_setvalues(VPP_OBJ *vpp_obj, UINT32 PlaneId, UINT32 mode, float *value )
{
    if (!(PlaneId == 0 || PlaneId == 1))
        return MV_VPP_EBADPARAM;

    if ((mode < GET_PQ_PIC_BRIGHTNESS) || (mode > GET_PQ_PIC_HUE))
        return MV_VPP_EBADPARAM;

   switch (mode) {
     case GET_PQ_PIC_BRIGHTNESS:
         {
             *value = (vpp_obj->gBrightness[PlaneId] + 200.0)/0.8;
             break;
         }
     case GET_PQ_PIC_CONTRAST:
         {
             *value = (vpp_obj->gContrast[PlaneId]-0.6)/0.0016;
             break;
         }
     case GET_PQ_PIC_SATURATION:
         {
             *value = vpp_obj->gSaturation[PlaneId]/0.004;
             break;
         }
     case GET_PQ_PIC_HUE:
         {
             if (vpp_obj->gHue[PlaneId] >= 180)
                 *value = (vpp_obj->gHue[PlaneId] - 180 )/0.72;
             else
                 *value = (vpp_obj->gHue[PlaneId] + 180 )/0.72;
             break;
         }
     default:
         {
             *value = 0xdeadbeef;
             break;
         }
   }

   return MV_VPP_OK;
}

