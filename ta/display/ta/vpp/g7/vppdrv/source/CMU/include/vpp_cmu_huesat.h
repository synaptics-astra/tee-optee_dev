/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRHUESAT__
#define __BDRHUESAT__

#include "vpp_cmu_common.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */
#define MULTIPLICATIONFACTOR_HUE_AXIS(val)  (((val) * 5 ) >> 1)
#define DIVISIONFACTOR_HUE_AXIS(val)        (((val) << 1) / 5)
#define HUESAT_NUM_UNITS    2

// Hue/Sat Axis.
typedef enum tag_VPP_CMU_HS_AXIS
{
    VPP_CMU_HS_AXIS1 = 0,
    VPP_CMU_HS_AXIS2,
    VPP_CMU_HS_AXIS3,
    VPP_CMU_HS_AXIS4,
    VPP_CMU_HS_AXIS5,
    VPP_CMU_HS_AXIS6,
    VPP_CMU_HS_AXIS7,
    VPP_CMU_HS_AXIS8,
    VPP_CMU_HS_AXIS9,
    VPP_CMU_HS_AXIS10,
    VPP_CMU_HS_AXIS11,
    VPP_CMU_HS_AXIS12,
    VPP_CMU_HS_AXIS13,
    VPP_CMU_HS_AXIS14,
#ifdef VPP_DEBUG
    VPP_CMU_HS_MAX_AXES
#endif
} VPP_CMU_HS_AXIS,*PVPP_CMU_HS_AXIS;

// Different ICR modes.
typedef enum tag_VPP_CMU_ICR_MODE
{
    VPP_CMU_ICR_MODE1 = 0,
    VPP_CMU_ICR_MODE2,
    VPP_CMU_ICR_MODE3,
    VPP_CMU_ICR_MODE4,
    VPP_CMU_ICR_MODE5,
    VPP_CMU_ICR_MODE6,
    VPP_CMU_ICR_MODE7,
    VPP_CMU_ICR_MODE8,
    VPP_CMU_ICR_MODE9,
    VPP_CMU_ICR_RECOVER_AXES,
    VPP_CMU_ICR_USER_DEFINED,
    VPP_CMU_HS_MAX_ICR_MODES
} VPP_CMU_ICR_MODE,*PVPP_CMU_ICR_MODE;

// Huesat control parameters.
typedef enum tagVPP_CMU_HUESAT_CTRL_PARAMS
{
    VPP_CMU_HS_ENABLE = 0,
    VPP_CMU_INTELLISAT_ENABLE,
    VPP_CMU_FLESH_TAPER_TONE,
    VPP_CMU_CTRLS_END
}VPP_CMU_HUESAT_CTRL_PARAMS,*PVPP_CMU_HUESAT_CTRL_PARAMS;

typedef enum tagVPP_CMU_ICR_PARAMS
{
    VPP_CMU_HUESAT_HUE = 0,
    VPP_CMU_HUESAT_HUECHANGE,
    VPP_CMU_HUESAT_SATCHANGE,
    VPP_CMU_GLOBALSAT_CHANGE,
    VPP_CMU_HUESAT_END
}VPP_CMU_ICR_PARAMS,*PVPP_CMU_ICR_PARAMS;

//chroma stretch function selection.
typedef enum tagVPP_CMU_MAXSAT_CST
{
    VPP_CMU_FIXED_LINEAR = 0x00,
    VPP_CMU_NON_LINEAR = 0x01,
    VPP_CMU_END
}VPP_CMU_MAXSAT_CST,*PVPP_CMU_MAXSAT_CST;


/*******  Chroma Type ICR *******/

// Enum for different channels Main/Pip
typedef enum tagVPP_CMU_HS_PRESET_CHANNEL
{
    VPP_CMU_ICR_MAIN = 0,
    VPP_CMU_ICR_PIP,
    VPP_CMU_ICR_CHANNEL_MAX
}VPP_CMU_HS_PRESET_CHANNEL,*PVPP_CMU_HS_PRESET_CHANNEL;

// Enum for different preset levels SD/HD
typedef enum tagVPP_CMU_HS_PRESET_LEVEL
{
    VPP_CMU_ICR_SD = 0,
    VPP_CMU_ICR_HD,
    VPP_CMU_ICR_LEVEL_MAX
}VPP_CMU_HS_PRESET_LEVEL,*PVPP_CMU_HS_PRESET_LEVEL;

/*****  Structure Definitions *****/

// Structure for hue,sat,and axis.
typedef struct tagVPP_CMU_ICR_USER_MODE
{
    UCHAR  Axis[14];
    UCHAR   Sat[14];
    UCHAR   Hue[14];
}VPP_CMU_ICR_USER_MODE, *PVPP_CMU_ICR_USER_MODE;

// Structure fo HueSat Paramters
typedef struct tagVPP_CMU_HUESAT_PARAMS
{
    BOOL    SatEnable;
    BOOL    IntelliSatEnable;
    UCHAR   FleshToneTaperEnable;
    UCHAR   GlobalSaturation;
    UCHAR   IcrMode;

}VPP_CMU_HUESAT_PARAMS, *PVPP_CMU_HUESAT_PARAMS;


/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */

/*
 * Sets the default values to the registers
 */
INT VPP_CMU_HUESAT_SetDefaults(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                               VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType);

/*
 * These functions will set/retrive the Hue Sat parameters.
 * Like SatEnable, IntelliSatEnable, WideGamutEnable .. etc
 */
INT VPP_CMU_HUESAT_SetHueSatAdjustParams(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                                         PVPP_CMU_HUESAT_PARAMS pHueSatParams);


/* This function sets the each individual parameters like SatEnable,
 * IntelliSatEnable, WideGamutEnable .. etc
 */
INT VPP_CMU_HUESAT_SetHueSatControls(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
                                     VPP_CMU_HUESAT_CTRL_PARAMS HSCtrls, BOOL Enable);

/*
 * This function will set/retrive all the ICR mode params
 */
INT VPP_CMU_HUESAT_SetHSIcrMode(VPP_OBJ* vpp_obj,
                                VPP_CHANNEL_TYPE Channel,VPP_CMU_ICR_MODE Mode,
                                PVPP_CMU_ICR_USER_MODE pMode);

/* This functions sets the parameters like Hue, Huechange, Sat change individually
 * This is taken instead of writing 3 different functions seperately.
 */
INT VPP_CMU_HUESAT_SetHSIcrModeParams(VPP_OBJ* vpp_obj,
                                      VPP_CHANNEL_TYPE Channel,
                                      VPP_CMU_ICR_PARAMS pIcrParams,
                                      VPP_CMU_HS_AXIS Axis,UINT16 Value);


#endif

