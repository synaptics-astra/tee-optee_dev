/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRGAMMA__
#define __BDRGAMMA__

/*-----------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------------
 */
#include "vpp_api.h"
#include "vpp_module.h"

#define GM_BCM_BUFFER_SIZE 16440//57440 //(1025*14+10)*4

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

typedef enum tag_VPP_GAMMACORRECTION_CHANNELS
{
    VPP_GAMMACORRECTION_ON_G_OR_Y  = 0,     //     G / Y
    VPP_GAMMACORRECTION_ON_B_OR_CB,         //     B / Cb
    VPP_GAMMACORRECTION_ON_R_OR_CR ,        //     R / Cr
    VPP_GAMMACORRECTION_MAX_CHANNELS
} VPP_GAMMACORRECTION_CHANNELS;


typedef  struct tagVPP_GAMMA_LUT_PARAMS
{
    ENUM_GAMMA_MODE ModeId;
    VPP_GAMMA_FORMAT  GammaFormat;
}VPP_GAMMA_LUT_PARAMS,*PVPP_GAMMA_LUT_PARAMS;


typedef  struct tagVPP_GAMMA_PARAMS
{
    UCHAR BCbGammaEnable;  //Enable B/Cb Gamma Correction
    UCHAR GYGammaEnable;   //Enable G/Y Gamma Correction
    UCHAR RCrGammaEnable;  //Enable R/Cr Gamma Correction
}VPP_GAMMA_PARAMS,*PVPP_GAMMA_PARAMS;


/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */
INT VPP_GAMMA_SetDefaults(VPP_OBJ* vpp_obj,
                INT Channel);

INT VPP_GAMMA_Enable(VPP_OBJ* vpp_obj,
                INT Channel, BOOL Enable);

INT VPP_GAMMA_LoadLUT(VPP_OBJ* vpp_obj,INT Channel,
                PVPP_GAMMA_LUT_PARAMS  pGammaLUTParams);

INT VPP_GAMMA_SetParams(VPP_OBJ* vpp_obj,
                INT Channel,PVPP_GAMMA_PARAMS pGammaParams);

INT VPP_Gamma_LoadRegister(VPP_OBJ* vpp_obj,
                INT Channel, INT GmMode);

INT VPP_Gamma_ReadValue(VPP_OBJ* vpp_obj,INT Channel,
                INT location, INT16 gammaVal[]);

#endif //__BDRGAMMA__

