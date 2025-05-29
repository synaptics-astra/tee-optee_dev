/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRFTDC__
#define __BDRFTDC__

/*-----------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------------
 */
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cmu_common.h"

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */
typedef enum tag_VPP_FTDC_MODE
{
    VPP_FTDC_MODE1 = 0,
    VPP_FTDC_MODE2,
    VPP_FTDC_MODE3,
    VPP_FTDC_MODE4
} VPP_FTDC_MODE;

typedef enum tag_VPP_CMU_FTDC_REGION
{
    VPP_CMU_FTDC_REG1 = 0,
    VPP_CMU_FTDC_REG2,
    VPP_CMU_FTDC_REG3,
    VPP_CMU_FTDC_REG4,
    VPP_CMU_FTDC_REG5,
    VPP_CMU_FTDC_REG6
} VPP_CMU_FTDC_REGION,PVPP_CMU_FTDC_REGION;

/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */
INT VPP_CMU_FTDC_SetDefaults(VPP_OBJ* vpp_obj,
                    VPP_CMU_INPUT_SIGNAL_TYPE CmuInputType);

INT VPP_CMU_FTDC_Enable(VPP_OBJ* vpp_obj,VPP_CHANNEL_TYPE Channel, BOOL Enable);

/*
*This function will set Parameters of FTDC
*/
INT VPP_CMU_FTDC_SetParams(VPP_OBJ* vpp_obj,PVPP_CMU_FTDC_PARAMS pFtdcParams);

/*
*  This function will set the FTDC correction mode.
*/
INT VPP_CMU_FTDC_SetMode(VPP_OBJ* vpp_obj,VPP_FTDC_MODE Mode);

#endif
