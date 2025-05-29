/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_PQ_C_

#include "vpp_module.h"
#include "vpp_pq_prv.h"
#include "vpp_pq.h"
#include "math.h"
#include "vpp_csc_common.h"
#include "vpp_csc_icsc.h"

//excluding sign bit, offset window for main plane-11bit and pip-9bit
const float bdr_gcsc_offset_mul_factor[GCSC_NUM_CHANNELS] = {2048.0, 512.0};

static void InitGlobalMatrix(VPP_CSC_TYPE Channel)
{
    /*
    {
        {Hue   0           0            }
        {0     Contrast    0            }
        {0     0            Saturation  }
    }
    {0  Brightness  0}
    */
    GlobalTransformCoeff[Channel][0][1] = 0;
    GlobalTransformCoeff[Channel][1][0] = 0;
    GlobalTransformCoeff[Channel][1][2] = 0;
    GlobalTransformCoeff[Channel][2][1] = 0;
    GlobalTransformCoeff[Channel][1][1] = 1; /* Contrast */
    GlobalTransformCoeff[Channel][0][0] = 1; /* Hue */
    GlobalTransformCoeff[Channel][0][2] = 0;
    GlobalTransformCoeff[Channel][2][0] = 0;
    GlobalTransformCoeff[Channel][2][2] = 1;
    GlobalTransformOffset[Channel][0] = 0;
    GlobalTransformOffset[Channel][1] = 0;   /* Brightness */
    GlobalTransformOffset[Channel][2] = 0;
}

/*------------------------------------------------------------------------------*/
void VPP_Multiply3x3Matrix(float * MatA, float * MatB, float * MatC)
{
    UCHAR i, j, k;

    float  (*MatrixA)[3][3] = (float  (*)[3][3])MatA;
    float  (*MatrixB)[3][3] = (float  (*)[3][3])MatB;
    float  (*MatrixC)[3][3] = (float  (*)[3][3])MatC;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            *(*(*(MatrixC)+i)+j) = 0;
            for (k = 0; k < 3; k++) {
                *(*(*(MatrixC)+i)+j)+= (*(*(*(MatrixA)+i)+k)) * (*(*(*(MatrixB)+k)+j));
            }
        }
    }
}

/*------------------------------------------------------------------------------*/
void VPP_Multiply3x1Matrix(float * MatA, float * MatB, float * MatC)
{
    UCHAR i, j;

    float  (*MatrixA)[3][3] = (float  (*)[3][3])MatA;
    float  (*MatrixB)[3] = (float  (*)[3])MatB;
    float  (*MatrixC)[3] = (float  (*)[3])MatC;

    for (i = 0; i < 3; i++) {
        *(*(MatrixC) + i) = 0;
        for (j = 0; j < 3; j++) {
            *(*(MatrixC) + i) +=  (*(*(MatrixB) + j)) * (*(*(*(MatrixA)+i)+j)) ;
        }
    }
}

/*------------------------------------------------------------------------------*/
static void Add3x1Matrix(float * MatA, float * MatB, float * MatC)
{
    UCHAR i;

    float  (*MatrixA)[3] = (float  (*)[3])MatA;
    float  (*MatrixB)[3] = (float  (*)[3])MatB;
    float  (*MatrixC)[3] = (float  (*)[3])MatC;

    for (i = 0; i < 3; i++) {
        *(*(MatrixC) + i) = *(*(MatrixA) + i) + *(*(MatrixB) + i);
    }
}

/*------------------------------------------------------------------------------*/
static void Sub3x1Matrix(float * MatA, float * MatB, float * MatC)
{
    UCHAR i;
    float  (*MatrixA)[3] = (float  (*)[3])MatA;
    float  (*MatrixB)[3] = (float  (*)[3])MatB;
    float  (*MatrixC)[3] = (float  (*)[3])MatC;

    for (i = 0; i < 3; i++) {
        *(*(MatrixC) + i) = *(*(MatrixA) + i) - *(*(MatrixB) + i);
    }
}

/*------------------------------------------------------------------------------*/

static INT ComputeAndSetGlobalCSC(VPP_OBJ* vpp_obj, VPP_CSC_TYPE Channel, \
                            UINT32 coeff[][PQ_COEFF_MATRIX_COLUMNS], UINT32 offset[])
{
    UCHAR i, j;
    UINT32 retStatus = MV_VPP_OK;
    float _FAR temp;

    /*
     * A * G
     */
    VPP_Multiply3x3Matrix((float _FAR *) IntermediateColorCoeff[Channel],
            (float _FAR *) GlobalTransformCoeff[Channel],
            (float _FAR *) GCscCoeff[Channel]);

    for (i = 0; i < PQ_COEFF_MATRIX_ROWS; i++) {
        for (j = 0; j < PQ_COEFF_MATRIX_COLUMNS; j++) {
            if (GCscCoeff[Channel][i][j] > BDR_GCSC_CSC_COEFF_MAX) {
                VPPAPI_DBG_INFO("ICSC_BCHS ==== BDR_GCSC_CSC_COEFF_MAX as coeff \n");
                temp = BDR_GCSC_CSC_COEFF_MAX;
            } else if (GCscCoeff[Channel][i][j] < BDR_GCSC_CSC_COEFF_MIN) {
                temp = BDR_GCSC_CSC_COEFF_MIN;
                VPPAPI_DBG_INFO("ICSC_BCHS ==== BDR_GCSC_CSC_COEFF_MAX as coeff \n");
            } else
                temp = GCscCoeff[Channel][i][j];

            coeff[i][j] = (UINT32) (((temp >= 0) ?
                        (temp * BDR_GCSC_COEFF_MUL_FACTOR) :
                        ((-temp) * BDR_GCSC_COEFF_MUL_FACTOR + BDR_GCSC_COEFF_SIGN_BIT)) + 0.5);
        }

    }

    /*
     * GV
     */
    VPP_Multiply3x1Matrix((float _FAR *) GlobalTransformCoeff[Channel],
            (float _FAR *) gCmuOffsetMatrix[Channel],
            (float _FAR *) TempOffset1);

    /*
     * H - GV
     */
    Sub3x1Matrix(GlobalTransformOffset[Channel], TempOffset1, TempOffset2);

    /*
     * A*(H-GV)
     */
    VPP_Multiply3x1Matrix((float _FAR *) IntermediateColorCoeff[Channel],
            (float _FAR *) TempOffset2, (float _FAR *) TempOffset3);

    /*
     * A*(H-GV) + B
     */
    Add3x1Matrix((float _FAR *) TempOffset3,
            (float _FAR *) IntermediateColorOffset[Channel],
            (float _FAR *) GCscOffset[Channel]);
    for (i = 0; i < PQ_OFFSET_MATRIX_ROWS; i++) {
        if (GCscOffset[Channel][i] > BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MAX) {
            temp = BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MAX;
            VPPAPI_DBG_INFO("ICSC_BCHS ==== BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MAX as offset \n");
        } else if (GCscOffset[Channel][i] < BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MIN) {
            temp = BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MIN;
            VPPAPI_DBG_INFO("ICSC_BCHS ==== BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MIN as offset \n");
        } else
            temp = GCscOffset[Channel][i];

        temp += GCscRGBOffset[i];
        offset[i] = (UINT32) ((temp >= 0) ?
                (temp  * bdr_gcsc_offset_mul_factor[Channel]) :
                ((-temp) * bdr_gcsc_offset_mul_factor[Channel] + BDR_GCSC_OFFSET_MAIN_AND_PIP_SIGN_BIT));
    }

    return retStatus;
}

INT VPP_PQ_CalcAndSetPQReg(VPP_OBJ *vpp_obj, VPP_CSC_TYPE Channel, \
                        UINT32 coeff[][PQ_COEFF_MATRIX_COLUMNS], UINT32 offset[])
{
    /*  This function is to set the pq as per global pq attributes.
        This is required because ,when there is  mode change,
        the existing pq attributes must be maintained
     */
    INT retStatus = MV_VPP_OK;
    GlobalTransformOffset[Channel][1] = vpp_obj->gBrightness[Channel];

    GlobalTransformCoeff[Channel][1][1] = vpp_obj->gContrast[Channel];
    GlobalTransformCoeff[Channel][0][0] =
        (float  ) (vpp_obj->gContrast[Channel] * (vpp_obj->gSaturation[Channel]) * (cos(vpp_obj->gHue[Channel] * PI_DIV_180)));
    GlobalTransformCoeff[Channel][0][2] =
        (float  ) (vpp_obj->gContrast[Channel] * (vpp_obj->gSaturation[Channel]) * (- sin(vpp_obj->gHue[Channel] * PI_DIV_180)));
    GlobalTransformCoeff[Channel][2][0] =
        (float  ) (vpp_obj->gContrast[Channel] * (vpp_obj->gSaturation[Channel]) * (sin(vpp_obj->gHue[Channel] * PI_DIV_180)));
    GlobalTransformCoeff[Channel][2][2] =
        (float  ) (vpp_obj->gContrast[Channel] * (vpp_obj->gSaturation[Channel]) * (cos(vpp_obj->gHue[Channel] * PI_DIV_180)));

    retStatus = ComputeAndSetGlobalCSC(vpp_obj,Channel,coeff,offset);
    return retStatus;
}

INT VPP_PQ_SetupGlobalCSC(VPP_OBJ* vpp_obj,
        VPP_CSC_TYPE Channel, VPP_CSC_MODE Output)
{
    INT retStatus = MV_VPP_OK;
    if (Output == gMode[Channel])
        return retStatus;
    gMode[Channel] = Output;
    InitGlobalMatrix (Channel);
    /*
     * Compute A = SC
     */
    VPP_Multiply3x3Matrix((float  *) gCmusRGBCoefficients[Channel],
            (float  *) &gCmuColorTransformMatrix[Output],
            (float  *) IntermediateColorCoeff[Channel]);

    /*
     * Compute = SC * V
     */
    VPP_Multiply3x1Matrix((float  *) IntermediateColorCoeff[Channel],
            (float  *) gCmuOffsetMatrix[Channel],
            (float  *) TempOffset1);

    /*
     * Compute SD
     */
    VPP_Multiply3x1Matrix((float  *) gCmusRGBCoefficients[Channel],
            (float  *) &gCmuColorTransformOffset[Output],
            (float  *) TempOffset3);

    /*
     * Compute B = SCV+SD+T
     */

    Add3x1Matrix(TempOffset1, TempOffset3, TempOffset5);

    Add3x1Matrix(TempOffset5,
            (float  *) gCmusRGBOffsets,
            IntermediateColorOffset[Channel]);
    return retStatus;
}
