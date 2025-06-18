/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CSC_COMMON_
#define _VPP_CSC_COMMON_

/*******************************************************
 * INCLUDES
 *******************************************************/
#include "com_type.h"
#include "vpp_com.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_regs.h"

/*******************************************************
 * MACROS
 *******************************************************/

/*****************************************************
*  Enums and Structures
******************************************************/
/*
 * Common for all the VPP modules.
 */

typedef enum tagVPP_CSC_TYPE
{
    VPP_CSC_MAIN = 0,
    VPP_CSC_PIP,
    VPP_CSC_VOP2,
    VPP_CSC_VPTOP,
    VPP_MAX_CSC
}VPP_CSC_TYPE, *PVPP_CSC_TYPE;


/* Enumeration of different O/P types of CSC */

typedef enum tagVPP_CSC_MODE
{
    VPP_CSC_BYPASS_MODE = 0,
    VPP_CSC_YUV_601_TO_709,
    VPP_CSC_YUV_709_TO_601,
    VPP_CSC_RGB_TO_YUV_2020,
    VPP_CSC_YUV_2020_TO_RGB,
    VPP_CSC_RGB_TO_YUV_709,
    VPP_CSC_YUV_709_TO_RGB,
    VPP_CSC_RGB_TO_YUV_601,
    VPP_CSC_YUV_601_TO_RGB,
    VPP_CSC_sRGB_TO_YUV_2020,
    VPP_CSC_YUV_2020_TO_sRGB,
    VPP_CSC_sRGB_TO_YUV_709,
    VPP_CSC_YUV_709_TO_sRGB,
    VPP_CSC_sRGB_TO_YUV_601,
    VPP_CSC_YUV_601_TO_sRGB,
    VPP_CSC_YUV_601_TO_YUV_2020,
    VPP_CSC_YUV_2020_TO_YUV_601,
    VPP_CSC_YUV_709_TO_YUV_2020,
    VPP_CSC_YUV_2020_TO_YUV_709,
    VPP_CSC_YUV_2020_TO_RGB_PT,
    VPP_CSC_UV_SWAP,
    VPP_CSC_MAX_CONV_TYPES
}VPP_CSC_MODE,*PVPP_CMU_CSC_MODE;

/*
 * CSC common types
 */

/* Enumeration of different I/P types to CSC */
typedef enum tagVPP_CSC_INPUT_SIGNAL_TYPE
{
    VPP_CSC_YUV_601 = 0,
    VPP_CSC_YUV_709,
    VPP_CSC_RGB_444
}VPP_CSC_INPUT_SIGNAL_TYPE,*PVPP_CMU_INPUT_SIGNAL_TYPE;

#endif //_VPP_CSC_COMMON_
