/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRBR__
#define __BDRBR__

#include "vpp_cmu_common.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

/* Taper sizes */
typedef enum tagVPP_CMU_BR_TAPERSIZE
{
    VPP_CMU_BR_TAPERSIZE16 = 0,
    VPP_CMU_BR_TAPERSIZE32 ,
    VPP_CMU_BR_TAPERSIZE64 ,
    VPP_CMU_BR_TAPERSIZE128,
    VPP_CMU_BR_TAPERSIZE256,
    VPP_CMU_BR_TAPERSIZE512,
} VPP_CMU_BR_TAPERSIZE;

/* Brightness Levels*/
typedef enum tagVPP_CMU_BR_LEVEL
{
    VPP_CMU_BR_LEVEL0 = 0,
    VPP_CMU_BR_LEVEL1,
    VPP_CMU_BR_LEVEL2,
    VPP_CMU_BR_LEVEL3,
    VPP_CMU_BR_LEVEL4,
    VPP_CMU_BR_LEVEL5,
    VPP_CMU_BR_LEVEL6,
    VPP_CMU_BR_LEVEL7,
    VPP_CMU_BR_LEVEL8,
    VPP_CMU_BR_LEVEL9,
    VPP_CMU_BR_LEVEL10,
    VPP_CMU_BR_LEVEL11,
    VPP_CMU_BR_LEVEL12,
    VPP_CMU_BR_LEVEL13,
    VPP_CMU_BR_LEVEL14,
    VPP_CMU_BR_MAX_LEVEL
} VPP_CMU_BR_LEVEL;

/* One side or Two side Tapers */
typedef enum tagVPP_CMU_BR_TAPER
{
    VPP_CMU_BR_ONESIDE_TAPER = 0,// Normal Image
    VPP_CMU_BR_TWOSIDE_TAPER,    // Image is Letter box
    VPP_CMU_BR_NO_TAPER
} VPP_CMU_BR_MODE;

/* Structure for Brightness Parameters */
typedef struct tagVPP_CMU_BR_PARAMS
{
    UCHAR   EnBrightnessLevel;
    UCHAR   Taper;     //one side taper or Two side Taper
    UCHAR   TaperSize;  // Size of the taper 16/32/64/128/256/512
    UINT16  BrThreshold1;
    UINT16  BrThreshold2;
}VPP_CMU_BR_PARAMS, *PVPP_CMU_BR_PARAMS;

/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */

INT VPP_CMU_BR_SetDefaults(VPP_OBJ* vpp_obj,
                                VPP_CHANNEL_TYPE Channel,
                                VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType);

INT VPP_CMU_BR_SetTaperEnable(VPP_OBJ* vpp_obj,
                                VPP_CHANNEL_TYPE Channel,BOOL Enable);

INT VPP_CMU_BR_SetTaperSize(VPP_OBJ* vpp_obj,
                                VPP_CHANNEL_TYPE Channel,
                                VPP_CMU_BR_TAPERSIZE TaperSize);

INT VPP_CMU_BR_SetBrightnessLevel(VPP_OBJ* vpp_obj,
                                VPP_CHANNEL_TYPE Channel,
                                VPP_CMU_BR_LEVEL BrLevel);

INT VPP_CMU_BR_SetThresholds(VPP_OBJ* vpp_obj,
                                VPP_CHANNEL_TYPE Channel,
                                UINT16 Threshold1, UINT16 Threshold2);

#endif //__BDRBR__
