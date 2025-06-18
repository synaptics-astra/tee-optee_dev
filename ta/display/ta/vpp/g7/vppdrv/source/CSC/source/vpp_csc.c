/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_CSC_C_
#define _VPP_CSC_C_

#include "vpp_module.h"
#include "vpp_csc.h"
#include "vpp_csc_icsc.h"

/***************************************************************************************
 * Description: reset CSC
 * PARAMS: none
 * RETURN:  1 - succeed
 *                others - failed to load CSC register default values to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_Reset(VPP_OBJ *vpp_obj)
{
    /*reset main and PIP channel*/
    VPP_CSC_ICSC_SetDefaults(vpp_obj, CHAN_MAIN);
    VPP_CSC_ICSC_SetDefaults(vpp_obj, CHAN_PIP);

    return (MV_VPP_OK);
}

#endif

