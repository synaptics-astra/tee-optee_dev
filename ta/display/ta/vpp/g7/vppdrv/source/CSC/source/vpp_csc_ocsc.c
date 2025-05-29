/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_CSC_OCSC_C_
#define _VPP_CSC_OCSC_C_

#include "vpp_csc_ocsc.h"
#include "vpp_pq.h"
#include "math.h"

#define     VPP_DV_OFFSET_MULTIPLIER    16
#define     VPP_DV_OFFSET_DIVIDER       (1.0 * VPP_DV_OFFSET_MULTIPLIER)

// COEFFICIENTS MATRIX
const INT16 VppDVOcsc_Coeff_Matrix[VPP_MAX_OCSC_SUB_MODES_MAX] \
                                         [VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL] =
{
    //RGB2YUV 709_LIMITED
    {{0x1749, 0x4E53, 0X07E7}, {0X8CD7, 0XAB2E, 0X3803}, {0X3803, 0XB2E1, 0X8524}},
    //RGB2YUV 709_FULL
    {{0x1B37, 0x5B8A, 0X093D}, {0X8EAB, 0XB156, 0X4000}, {0X4000, 0XBA22, 0X85DF}},
    //RGB2YUV 2020_LIMITED
    {{0X1CC5, 0X4A41, 0X067E}, {0X8FA5, 0XA860, 0X3803}, {0X3803, 0XB383, 0X8482}},
    //RGB2YUV 2020_FULL
    {{0X21A0, 0X56C8, 0X0797}, {0X91E0, 0XAE21, 0X4000}, {0X4000, 0XBADB, 0X8526}},
    /* To get RGB output from RGB2YUV OCSC module, we have to set the unit matrix*/
    {VPP_OCSC_UNIT_MATRIX_ARR_COEFFS} //VPP_DV_OCSC_COEFF_RGB_BYPASS
};

//RGB2YUV OFFSETS
const INT32 VppDVOcsc_RGB2YUV_Offset[VPP_MAX_OCSC_SUB_MODES_MAX][VPP_MAX_OCSC_OFFSET] =
{
    {0x100, 0x800, 0x800}, //RGB2YUV 709_LIMITED
    {0x000, 0x800, 0x800}, //RGB2YUV 709_FULL
    {0x100, 0x800, 0x800}, //RGB2YUV 2020_LIMITED
    {0x000, 0x800, 0x800}, //RGB2YUV 2020_FULL
    {0x000, 0x000, 0x000}  //RGB_BYPASS
};

const float VppDVOcsc_YUV2RGB_Coeff[VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL]=
{
    // YUV2RGB 709_LIMITED Coefficient
    {1.168917388, 0.000217981, 1.799274702}, {1.168735588, -0.213983176, -0.534918442}, \
                                             {1.168940278, 2.120255699, 0.000085605}
};

// YUV2RGB 709_LIMITED Offset
static const float Y2ROff[VPP_MAX_OCSC_OFFSET] = {-249.0377416, 77.1596377, -290.1067314};

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
void VPP_OCSC_BCHS(VPP_OBJ *vpp_obj, double CT[][VPP_MAX_OCSC_COEFF_MAT_COL], INT32 CO[])
{
    float Br = (float)vpp_obj->gBrightness[VPP_DP1]; //0 to 4095
    float Cn = (float)vpp_obj->gContrast[VPP_DP1]; //0 to 2
    float hue = (float)vpp_obj->gHue[VPP_DP1]; //0 to 360 degree
    float Sat = (float)vpp_obj->gSaturation[VPP_DP1]; // 0 to 2
    double ang = (hue * VPP_DV_OCSC_PI) / 180.0; //in radian
    double   M[3][3], O[3];

    // Y = 0.299R + 0.587G + 0.114B + 16
    // U = -0.147R + -0.289G + 0.436B + 128
    // V = 0.615R + -0.515G + -0.100B + 128
    // Y' = ((Y - 16) x Co) + Br + 16
    // U'' = (((U - 128)   x   Cos(Hue) + (V - 128) x Sin(Hue))  x  C x S) + 128
    // V'' = (((V - 128)   x   Cos(Hue) - (U - 128) x Sin(Hue))  x  C x S) + 128

    VPPAPI_DBG_INFO(" CSC_OCSC BCHS B[%f]C[%f]H[%f]S[%f]\n", Br, Cn, hue, Sat);

   //Y
    M[0][0] = CT[0][0] * Cn;
    M[0][1] = CT[0][1] * Cn;
    M[0][2] = CT[0][2] * Cn;
    O[0] = CO[0] + Br;

   //Cb
    M[1][0] = ((CT[1][0] * cos(ang)) + (CT[2][0] * sin(ang)))*Cn*Sat;
    M[1][1] = ((CT[1][1] * cos(ang)) + (CT[2][1] * sin(ang)))*Cn*Sat;
    M[1][2] = ((CT[1][2] * cos(ang)) + (CT[2][2] * sin(ang)))*Cn*Sat;
    O[1] = CO[1];

    //Cr
    M[2][0] = (CT[2][0] * cos(ang) - CT[1][0] * sin(ang))*Cn*Sat;
    M[2][1] = (CT[2][1] * cos(ang) - CT[1][1] * sin(ang))*Cn*Sat;
    M[2][2] = (CT[2][2] * cos(ang) - CT[1][2] * sin(ang))*Cn*Sat;
    O[2] = CO[2];

    CT[0][0] = M[0][0];
    CT[0][1] = M[0][1];
    CT[0][2] = M[0][2];
    CT[1][0] = M[1][0];
    CT[1][1] = M[1][1];
    CT[1][2] = M[1][2];
    CT[2][0] = M[2][0];
    CT[2][1] = M[2][1];
    CT[2][2] = M[2][2];
    CO[0] = O[0];
    CO[1] = O[1];
    CO[2] = O[2];

}

/*
 * Formula for getting RGB from RGB to YUV csc
 * coeff_YtoR * (coef_RtoY * ip_YUV + off_RtoY) + off_YtoR
 * coeff_YtoR * coef_RtoY * ip_YUV + "coeff_YtoR * off_RtoY + off_YtoR"
 */
void  VPP_DV_OCSC_YUV_2_RGB_ConverSion(double bchsCoeff[][VPP_MAX_OCSC_COEFF_MAT_COL],
                                                            INT32 R2YOffset[])
{
    INT16 i, j;
    float ResOff[VPP_MAX_OCSC_OFFSET];
    float R2YOff[VPP_MAX_OCSC_OFFSET];
    float R2YCoeff[VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL];
    float RGBCoeff[VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL];

    VPPAPI_DBG_INFO(" VPP_DV_OCSC_YUV_2_RGB_ConverSion \n" );

    for (i = 0; i < VPP_MAX_OCSC_COEFF_MAT_ROW; i++) {
        for (j = 0; j < VPP_MAX_OCSC_COEFF_MAT_COL; j++) {
            R2YCoeff[i][j] = (float)bchsCoeff[i][j];
        }
    }

    /* 3x3 Matrix Multiplication with Y2R coefficient and R2Y Coefficient for getting RGB coefficient */
    VPP_Multiply3x3Matrix((float*)VppDVOcsc_YUV2RGB_Coeff, (float*)R2YCoeff, (float*)RGBCoeff);

    // RGB is in GRB format
    bchsCoeff[1][0] = RGBCoeff[0][0];
    bchsCoeff[1][1] = RGBCoeff[0][1];
    bchsCoeff[1][2] = RGBCoeff[0][2];

    bchsCoeff[0][0] = RGBCoeff[1][0];
    bchsCoeff[0][1] = RGBCoeff[1][1];
    bchsCoeff[0][2] = RGBCoeff[1][2];

    bchsCoeff[2][0] = RGBCoeff[2][0];
    bchsCoeff[2][1] = RGBCoeff[2][1];
    bchsCoeff[2][2] = RGBCoeff[2][2];

    /* the BCHS  calulation was based on Coefficient actual values,
     * So converting the offset also here
     */
    for (i = 0; i < VPP_MAX_OCSC_COEFF_MAT_COL; i++) {
        R2YOff[i] = (float)R2YOffset[i]/VPP_DV_OFFSET_DIVIDER;
    }

    /* Matrix Multiplication with Y2R coefficient and R2Y offset for calculate RGB offset */
    VPP_Multiply3x1Matrix((float*)VppDVOcsc_YUV2RGB_Coeff, (float*)R2YOff, (float*)ResOff);

    /* Adding Y2R offset with resultant offser of(VppDVOcsc_YUV2RGB_Coeff X R2YOff) for getting the RGB offset */
    for (j = 0; j < VPP_MAX_OCSC_COEFF_MAT_COL; j++) {
        ResOff[j] += Y2ROff[j];
        R2YOffset[j] = (INT32)(ResOff[j] * VPP_DV_OFFSET_MULTIPLIER);
    }
}


void VPP_DM_OCSC_ApplyGainAndSetCSC_Coeff(VPP_OBJ *vpp_obj, UINT32 RegAddr, \
                                                      INT16 m33Csc[][VPP_MAX_OCSC_COEFF_MAT_COL])
{
    float aClrTempGain[] = {vpp_obj->clrtemp.Rgain, vpp_obj->clrtemp.Ggain, vpp_obj->clrtemp.Bgain};
    int i, j;
    UINT32 RegValue;

    for (i = 0; i < VPP_MAX_OCSC_COEFF_MAT_ROW; i++) {
        for (j = 0; j < VPP_MAX_OCSC_COEFF_MAT_COL; j++) {
            RegValue = INT_TO_SIGN16_CONVERT((int16_t)((m33Csc[i][j] * aClrTempGain[j])));
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegValue);
            RegAddr += 4;
        }
    }
}

#endif