/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_CSC_OCSC_H_
#define __VPP_CSC_OCSC_H_

#include "vpp_csc_common.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define VPP_MAX_OCSC_COEFF_MAT_ROW 3
#define VPP_MAX_OCSC_COEFF_MAT_COL 3
#define VPP_MAX_OCSC_COEFF 9
#define VPP_MAX_OCSC_OFFSET            3
#define VPP_DV_OCSC_SCALE_2P        0xD
#define VPP_DV_OCSC_PI              3.14159


#define VPP_DV_OCSC_COEFF_RGB2YUV_LIMITED_SCALE_2P 15
#define VPP_DV_OCSC_COEFF_UNITMATRIX_SCALE_2P      15

/* unit Matrix value RGB gain  from 0 to 1.9999 */
#define VPP_OCSC_COEF_FOR_GAIN_0_TO_1P99   (1 << 14)
/* unit Matrix value RGB gain  from 0 to 0.9999 */
#define VPP_OCSC_COEF_FOR_GAIN_0_TO_0P99  ((1 << VPP_DV_OCSC_COEFF_UNITMATRIX_SCALE_2P) -1)
#define VPP_OCSC_UNIT_MAT_COEF VPP_OCSC_COEF_FOR_GAIN_0_TO_0P99

#define VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_R_ROW \
            {VPP_OCSC_UNIT_MAT_COEF, 0, 0}
#define VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_G_ROW \
            {0, VPP_OCSC_UNIT_MAT_COEF, 0}
#define VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_B_ROW \
            {0, 0, VPP_OCSC_UNIT_MAT_COEF}

#define VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_R_ROW, \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_G_ROW, \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_B_ROW

#define VPP_OCSC_GBR_UNIT_MATRIX_ARR_COEFFS \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_G_ROW, \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_B_ROW, \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_R_ROW

#define VPP_OCSC_GRB_UNIT_MATRIX_ARR_COEFFS \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_G_ROW, \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_R_ROW, \
           VPP_OCSC_RGB_UNIT_MATRIX_ARR_COEFFS_B_ROW

#define VPP_OCSC_UNIT_MATRIX_ARR_COEFFS  VPP_OCSC_GRB_UNIT_MATRIX_ARR_COEFFS



typedef enum VPP_DV_OCSC_SUB_MODES_T
{
    VPP_DV_OCSC_COEFF_RGB2YUV_LIMITED = 0,
    VPP_DV_OCSC_COEFF_RGB2YUV_FULL,
    VPP_DV_OCSC_COEFF_RGB2YUV_r2020_LIMITED,
    VPP_DV_OCSC_COEFF_RGB2YUV_r2020_FULL,
    VPP_DV_OCSC_COEFF_RGB_BYPASS,
    VPP_MAX_OCSC_SUB_MODES_MAX
}VPP_DV_OCSC_SUB_MODES;


#define INT_TO_SIGN16_CONVERT(x)    ((x<0)?((x*(-1)) + (1<<15)):x)
#define SIGN16_TO_SHORT_INT_CONVERT(OcscCoeff)  \
                                    OcscCoeff & 0x8000 ? (OcscCoeff & 0x7fff) * -1 : OcscCoeff

#define IS_OCSC_BCHS_UPDATED(vpp_obj)           (vpp_obj->gBrightness[VPP_DP1] != VPP_DEFAULT_BRIGHTNESS || \
                                                vpp_obj->gContrast[VPP_DP1]   != VPP_DEFAULT_CONTRAST   || \
                                                vpp_obj->gHue[VPP_DP1]        != VPP_DEFAULT_HUE        || \
                                                vpp_obj->gSaturation[VPP_DP1] != VPP_DEFAULT_SATURATION )

#define IS_RGB_GAIN_UPDATED(vpp_obj)               (vpp_obj->clrtemp.Rgain != VPP_DEFAULT_RGAIN || \
                                                    vpp_obj->clrtemp.Ggain != VPP_DEFAULT_GGAIN || \
                                                    vpp_obj->clrtemp.Ggain != VPP_DEFAULT_GGAIN )


#define IS_OCSC_COLORPARAM_UPDATED(vpp_obj)    (IS_RGB_GAIN_UPDATED(vpp_obj) || IS_OCSC_BCHS_UPDATED(vpp_obj))


/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */

/***************************************************************************************
 * Description: set brightness
 * PARAMS: Channel - channel selection
 *         Brightness - brightness value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load brightness value to BCM buffer
 ***************************************************************************************/
void  VPP_DV_OCSC_YUV_2_RGB_ConverSion(double bchsCoeff[][VPP_MAX_OCSC_COEFF_MAT_COL],
                                                            INT32 R2YOffset[]);
void VPP_OCSC_BCHS(VPP_OBJ *vpp_obj, double CT[][VPP_MAX_OCSC_COEFF_MAT_COL], INT32 CO[]);

void VPP_DM_OCSC_ApplyGainAndSetCSC_Coeff(VPP_OBJ *vpp_obj, UINT32 RegAddr, \
                                                      INT16 m33Csc[][VPP_MAX_OCSC_COEFF_MAT_COL]);

#endif
