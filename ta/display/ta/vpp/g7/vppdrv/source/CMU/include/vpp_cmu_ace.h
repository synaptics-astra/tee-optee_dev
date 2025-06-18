/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRACE__
#define __BDRACE__

#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cmu_common.h"

/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */
INT VPP_CMU_ACE_SetDefaults(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,
        VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType);

INT VPP_CMU_ACE_Enable(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel, BOOL Enable);

INT VPP_CMU_ACE_SetParams(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel,
        PVPP_CMU_ACE_PARAMS pAceParams,
        VPP_CMU_ACE_RANGE Range,
        VPP_CMU_ACE_MODE Mode, BOOL DefautVal);

INT VPP_CMU_ACE_SetWeightingFactor(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,
        VPP_CMU_ACE_REGION Region, UCHAR Value);

INT VPP_CMU_ACE_SetMaxSlope(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,
        VPP_CMU_ACE_MAXSLOPE Maxslope, UCHAR Value);

INT VPP_CMU_ACE_SetGuardBand(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,
        VPP_CMU_ACE_GUARDBAND GuardBand, UCHAR Value);

INT VPP_CMU_ACE_SetThreshHold(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,
        VPP_CMU_ACE_THOLD ThLevel, UCHAR Value);

INT VPP_CMU_ACE_SetColorCompensationLevel(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,UCHAR Value);

INT VPP_CMU_ACE_SetOutput(VPP_OBJ* vpp_obj,
        VPP_CHANNEL_TYPE Channel,
        VPP_CMU_ACE_OUTPUT OutPut, USHORT Value);

#endif //__BDRACE__
