/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_CMU_COMMON_
#define _VPP_CMU_COMMON_

/*******************************************************
 * INCLUDES
 *******************************************************/
#include "com_type.h"
#include "vpp_mp.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_regs.h"

/*******************************************************
 * MACROS
 *******************************************************/

#define _FAR

#define TRANSLATE_CMU_AUX_CHANNEL(ch)   ((ch) == VPP_AUX) ? 3 : (ch)

/*****************************************************
*  Enums and Structures
******************************************************/
/*
 * Common for all the VPP modules.
 */

/* Enumeration of different O/P types of CMU */

typedef enum tagVPP_CMU_CSC_MODE
{
    VPP_CMU_BYPASS_MODE = 0,
    VPP_CMU_YUV_601_TO_RGB, /*SD to RGB*/
    VPP_CMU_YUV_709_TO_RGB, /*HD to RGB*/
    VPP_CMU_YUV_601_TO_709, /*601 to 709*/
    VPP_CMU_YUV_709_TO_601, /*709 to 601*/
    VPP_CMU_RGB_TO_YUV_601, /*RGB to YUV*/
    VPP_CMU_RGB_TO_YUV_709, /*RGB to YUV*/
#ifdef BT2020_SUPPORT
    VPP_CMU_RGB_TO_YUV_2020,
    VPP_CMU_YUV_2020_TO_RGB,
    VPP_CMU_sRGB_TO_YUV_2020,
    VPP_CMU_YUV_2020_TO_sRGB,
    VPP_CMU_YUV_601_TO_2020,
    VPP_CMU_YUV_2020_TO_601,
    VPP_CMU_YUV_709_TO_2020,
    VPP_CMU_YUV_2020_TO_709,
#endif
    VPP_CMU_GFX_MODE,       /*Gfx in Gfx out*/
    VPP_CMU_LUMA_STRECH_LOW,
    VPP_CMU_LUMA_STRECH_MILD,
    VPP_CMU_LUMA_STRECH_MED,
    VPP_CMU_LUMA_STRECH_HIGH,
    VPP_CMU_MAX_CONV_TYPES
}VPP_CMU_CSC_MODE;

/*
 * CMU common types
 */

/* Enumeration of different I/P types to CMU */
typedef enum tagVPP_CMU_INPUT_SIGNAL_TYPE
{
    VPP_CMU_YUV_601 = 0,
    VPP_CMU_YUV_709,
    VPP_CMU_RGB_444
}VPP_CMU_INPUT_SIGNAL_TYPE;

/* Enumeration of different color tone types of CMU */

typedef enum tag_VPP_LOCAL_COLOR_TONE_TYPE
{
    VPP_COLOR_TONE_WARM = 0,
    VPP_COLOR_TONE_NORMAL,
    VPP_COLOR_TONE_COOL
}VPP_COLOR_TONE_TYPE;

#endif //_VPP_CMU_COMMON_
