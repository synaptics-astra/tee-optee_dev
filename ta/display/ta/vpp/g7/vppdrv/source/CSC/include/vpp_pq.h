/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_PQ_H_
#define _VPP_PQ_H_
#define _FAR
#define GCSC_MIN_BRIGHTNESS  -800
#define GCSC_MAX_BRIGHTNESS  800
#define VPP_CSC_BRIGHTNESS_CAP_VALUE_MAX 500
#define VPP_CSC_BRIGHTNESS_RANGE_FACTOR \
               ((GCSC_MAX_BRIGHTNESS-GCSC_MIN_BRIGHTNESS)/(VPP_CSC_BRIGHTNESS_CAP_VALUE_MAX*1.0))

#define GCSC_MIN_CONTRAST    0
#define GCSC_MAX_CONTRAST    2

#define GCSC_MIN_SAT         0
#define GCSC_MAX_SAT         2

#define GCSC_MIN_HUE        0
#define GCSC_MAX_HUE        360

#define GCSC_NUM_CHANNELS 2

#define GCSC_NUM_UNITS       2

#define PQ_OFFSET_MATRIX_ROWS      3

#define PQ_COEFF_MATRIX_ROWS       3
#define PQ_COEFF_MATRIX_COLUMNS    3

#define BDR_GCSC_CSC_COEFF_MAX  15
#define BDR_GCSC_CSC_COEFF_MIN -15

#define BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MAX          8191
#define BDR_GCSC_CSC_OFFSET__MAIN_AND_PIP_MIN         -8191

// Coeff format is s.4.12f format, pow(12)
#define BDR_GCSC_COEFF_MUL_FACTOR                       4096

// Coeff format is s.4.12f format, signed bit pos is 17
#define BDR_GCSC_COEFF_SIGN_BIT                         0x10000
#define BDR_GCSC_OFFSET_MAIN_AND_PIP_SIGN_BIT           0x800000

#define PI 3.14159

#define PI_DIV_180 0.0174533

#ifdef ENABLE_PQ_LOG
#define VPPDBG_PQ_LOG(...) VPP_log(VPP_LOG_API, VPP_LOG_INFO, "VPPPQLOG", __VA_ARGS__)
#else
#define VPPDBG_PQ_LOG(...)
#endif

typedef enum {
    GET_PQ_PIC_BRIGHTNESS = 0,
    GET_PQ_PIC_CONTRAST,
    GET_PQ_PIC_SATURATION,
    GET_PQ_PIC_HUE,
}GET_PIC_PARA;

extern int gMode[GCSC_NUM_CHANNELS];

/***************************************************************************************
 * Description: initialise matrix
 * PARAMS: Channel - channel selection
 *         Output - output mode
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to initialise matrix
 ***************************************************************************************/
INT VPP_PQ_SetupGlobalCSC(VPP_OBJ* vpp_obj, VPP_CSC_TYPE Channel, VPP_CSC_MODE Output);

/***************************************************************************************
 * Description: initialise matrix
 * PARAMS: Channel - channel selection
 *         coeff - coeff tables to load
 *         offset - offset table to load
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to initialise matrix
 ***************************************************************************************/
INT VPP_PQ_CalcAndSetPQReg(VPP_OBJ *vpp_obj, VPP_CSC_TYPE Channel, \
                            UINT32 coeff[][PQ_COEFF_MATRIX_COLUMNS], UINT32 offset[]);

void VPP_Multiply3x3Matrix(float * MatA, float * MatB, float * MatC);
void VPP_Multiply3x1Matrix(float * MatA, float * MatB, float * MatC);

#endif //_VPP_PQ_H_

