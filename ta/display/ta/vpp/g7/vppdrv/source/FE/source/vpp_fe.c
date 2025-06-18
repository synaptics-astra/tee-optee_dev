/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_FE_C_

#include "vpp_module.h"
#include "vpp_cfg.h"
#include "vpp_fe.h"

#include "vpp_fe_dlr.h"
#include "vpp_fe_deint.h"
#include "vpp_fe_patgen.h"
#include "vpp_clkGate.h"
#include "vpp_offlineScl.h"
#ifdef CONFIG_AMP_DCM  //VPP_BRINGUP : To fix the compilation issue
#include "vpp_fe_dcm_common.h"
#endif

              ///////////////////////////////////////////
              //                                       //
              //    FE functions for Berlin            //
              //                                       //
              ///////////////////////////////////////////

/*******************************************************************
 * FUNCTION: Reset all VPP FE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_FE_Reset(VPP_OBJ *vpp_obj)
{
    /* reset FE data loader */
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_0);
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_1);
#ifdef ENABLE_OVP
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_2);
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_3);
#endif
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_0);
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_1);
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_GFX1_0);
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_GFX2_0);
    FE_DLR_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_VMX_0);

    /* reset pattern generator */
    FE_PATGEN_SetDefaults(vpp_obj);

    /* reset FE DE-INT */
    FE_DEINT_SetDefaults(vpp_obj);

    return (MV_VPP_OK);
}


/*********************************************************************
 * FUNCTION: initialize all VPP FE blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ********************************************************************/
int VPP_FE_Config(VPP_OBJ *vpp_obj)
{
    /* config FE data loader */
    /* clear asynchronous FIFO between client and dHub */
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_0);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_1);
#ifdef ENABLE_OVP
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_2);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_3);
#endif
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_0);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_1);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_EL_PIPE);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX1_0);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_VMX_0);
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX2_0);
    //Load default values for offlineRD channel
    FE_OfflineRd_LoadDefaultVal(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0);
        //Load Default Values of VDCM
#ifdef CONFIG_AMP_DCM
    VPP_FE_DCM_Init(vpp_obj);
    VPP_FE_DCM_GDCM_Init(vpp_obj, PLANE_PIP);
    VPP_FE_DCM_GDCM_Init(vpp_obj, PLANE_GFX1);
    VPP_FE_DCM_GDCM_Init(vpp_obj, PLANE_GFX2);
#endif

    return (MV_VPP_OK);
}


