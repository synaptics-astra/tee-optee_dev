/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_BR_CFG_C_

#include "vpp_cmu_brcfg.h"

/*Low Brightness*/
const VPP_CMU_BR_PARAMS CmuBrInputParamsLow =
{
     0,    //EnBrightnessLevel;
     1,    //Taper;         //one side taper or Two side Taper
     2,    //TaperSize;      // Size of the taper 16/32/64/128/256/512
     64,   //BrThreshold1;   0 64
     940   //BrThreshold2;   1023  94
};

/*Mid Brightness*/
const VPP_CMU_BR_PARAMS CmuBrInputParamsMid =
{
     6,    //EnBrightnessLevel;
     1,    //Taper;         //one side taper or Two side Taper
     2,    //TaperSize;      // Size of the taper 16/32/64/128/256/512
     64,   //BrThreshold1;   0
     940   //BrThreshold2; 1023
};

/*High Brightness*/
const VPP_CMU_BR_PARAMS CmuBrInputParamsHigh =
{
     15,   //EnBrightnessLevel;
     1,    //Taper;         //one side taper or Two side Taper
     4,    //TaperSize;      // Size of the taper 16/32/64/128/256/512
     0,    //BrThreshold1;   0
     1022  //BrThreshold2; 1023
};
