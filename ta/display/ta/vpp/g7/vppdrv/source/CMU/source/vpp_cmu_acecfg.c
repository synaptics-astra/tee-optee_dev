/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_CMU_ACE_CFG_C_

#include "vpp_cmu_acecfg.h"

const VPP_CMU_ACE_PARAMS gCmuAceInputParams[VPP_CMU_ACE_MAX_RANGE][VPP_CMU_ACE_MAX_MODE] =
{
    {
        { //VPP_CMU_ACE_OFF
           0,    // Mode;   Off
           0,    // ColorCompensationLevel;
           0,
           0,    // WeightingFactor1;
           0,    // WeightingFactor2;
           0,    // WeightingFactor3;
           0,    // ThresHold0;
           0,    // ThresHold1;
           1360, // ThresHold2;
           2720, // ThresHold3;
           4080, // ThresHold4;
           4080, // ThresHold5;
           0,    // OutPut0;  //White
           4092, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           40,   // MaximumSlope1;
           30,   // MaximumSlope2;
           20    // MaximumSlope3;
        },
        {//VPP_CMU_ACE_MANUAL
           1,    // Mode;   Manual
           1,    // ColorCompensationLevel;
           1,
           10,   // WeightingFactor1;
           5,    // WeightingFactor2;
           2,    // WeightingFactor3;
           0,    // ThresHold0;
           0,    // ThresHold1;
           1360, // ThresHold2;
           2720, // ThresHold3;
           4080, // ThresHold4;
           4080, // ThresHold5;
           0,    // OutPut0;  //White
           4092, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           40,   // MaximumSlope1;
           30,   // MaximumSlope2;
           20    // MaximumSlope3;
        },
        {//VPP_CMU_ACE_LOW
           1,    // Mode;   Low
           2,    // ColorCompensationLevel;
           1,
           10,   // WeightingFactor1;
           5,    // WeightingFactor2;
           2,    // WeightingFactor3;
           0,    // ThresHold0;
           0,    // ThresHold1;
           1360, // ThresHold2;
           2720, // ThresHold3;
           4080, // ThresHold4;
           4080, // ThresHold5;
           0,    // OutPut0;  //White
           4092, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           40,   // MaximumSlope1;
           30,   // MaximumSlope2;
           20    // MaximumSlope3;
        },
        {//VPP_CMU_ACE_MEDIUM
           1,    // Mode;   Mid
           2,    // ColorCompensationLevel;
           1,
           20,   // WeightingFactor1;
           10,   // WeightingFactor2;
           4,    // WeightingFactor3;
           0,    // ThresHold0;
           0,    // ThresHold1;
           1360, // ThresHold2;
           2720, // ThresHold3;
           4080, // ThresHold4;
           4080, // ThresHold5;
           0,    // OutPut0;  //White
           4092, // OutPut5;  // Black,
           160,  // GuardBand2;
           64,   // GuardBand3;
           44,   // MaximumSlope1;
           35,   // MaximumSlope2;
           26    // MaximumSlope3;
        },
        {// VPP_CMU_ACE_HIGH
           1,    // Mode;   High
           3,    // ColorCompensationLevel;
           0,
           30,   // WeightingFactor1;
           20,   // WeightingFactor2;
           8,    // WeightingFactor3;
           0,    // ThresHold0;
           0,    // ThresHold1;
           1360, // ThresHold2;
           2720, // ThresHold3;
           4080, // ThresHold4;
           4080, // ThresHold5;
           0,    // OutPut0;  //White
           4092, // OutPut5;  // Black,
           160,  // GuardBand2;
           64,   // GuardBand3;
           48,   // MaximumSlope1;
           40,   // MaximumSlope2;
           32    // MaximumSlope3;
        }
    },
    {
        {// VPP_CMU_ACE_OFF
           0,    // Mode;   Off
           0,    // ColorCompensationLevel;
           0,
           0,    // WeightingFactor1;
           0,    // WeightingFactor2;
           0,    // WeightingFactor3;
           0,    // ThresHold0;
           256,  // ThresHold1;
           1344, // ThresHold2;
           2688, // ThresHold3;
           3760, // ThresHold4;
           4080, // ThresHold5;
           256,  // OutPut0;  //White
           3760, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           40,   // MaximumSlope1;
           30,   // MaximumSlope2;
           20    // MaximumSlope3;
        },
        {// VPP_CMU_ACE_MANUAL
           1,    // Mode;   Manual
           1,    // ColorCompensationLevel;
           1,
           10,   // WeightingFactor1;
           5,    // WeightingFactor2;
           2,    // WeightingFactor3;
           0,    // ThresHold0;
           256,  // ThresHold1;
           1344, // ThresHold2;
           2688, // ThresHold3;
           3760, // ThresHold4;
           4080, // ThresHold5;
           256,  // OutPut0;  //White
           3760, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           40,   // MaximumSlope1;
           30,   // MaximumSlope2;
           20    // MaximumSlope3;
        },
        {// VPP_CMU_ACE_LOW
           1,    // Mode;   Low
           2,    // ColorCompensationLevel;
           1,
           10,   // WeightingFactor1;
           5,    // WeightingFactor2;
           2,    // WeightingFactor3;
           0,    // ThresHold0;
           256,  // ThresHold1;
           1344, // ThresHold2;
           2688, // ThresHold3;
           3760, // ThresHold4;
           4080, // ThresHold5;
           256,  // OutPut0;  //White
           3760, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           40,   // MaximumSlope1;
           30,   // MaximumSlope2;
           20    // MaximumSlope3;
        },
        {// VPP_CMU_ACE_MEDIUM
           1,    // Mode;   Mid
           2,    // ColorCompensationLevel;
           1,
           20,   // WeightingFactor1;
           10,   // WeightingFactor2;
           4,    // WeightingFactor3;
           0,    // ThresHold0;
           256,  // ThresHold1;
           1344, // ThresHold2;
           2688, // ThresHold3;
           3760, // ThresHold4;
           4080, // ThresHold5;
           256,  // OutPut0;  //White
           3760, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           44,   // MaximumSlope1;
           35,   // MaximumSlope2;
           26    // MaximumSlope3;
        },
        {// VPP_CMU_ACE_HIGH
           1,    // Mode;   High
           3,    // ColorCompensationLevel;
           1,
           30,   // WeightingFactor1;
           20,   // WeightingFactor2;
           8,    // WeightingFactor3;
           0,    // ThresHold0;
           256,  // ThresHold1;
           1344, // ThresHold2;
           2688, // ThresHold3;
           3760, // ThresHold4;
           4080, // ThresHold5;
           256,  // OutPut0;  //White
           3760, // OutPut5;  // Black
           160,  // GuardBand2;
           64,   // GuardBand3;
           48,   // MaximumSlope1;
           40,   // MaximumSlope2;
           32    // MaximumSlope3;
        }
    }
};

