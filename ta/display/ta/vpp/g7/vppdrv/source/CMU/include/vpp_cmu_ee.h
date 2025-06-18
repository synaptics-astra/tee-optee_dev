/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifdef ENABLE_EECMU
#ifndef _VPP_CMU_EE_H_
#define _VPP_CMU_EE_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

/**********************************************************************
*Type definition for FE detail EE                                                                             *
***********************************************************************/

/*enum for motion adaptive LPF's(Low Pass Filter) mode. (LPF_CTRL)*/
typedef enum VPP_FE_DET_LPF_MODE
{
    VPP_FE_DET_LPF_OUTPUT_DISABLED               = 0x00,   // LPF output disabled
    VPP_FE_DET_MOTION_ADAPTIVE_LPF_FILM_MODE     = 0x01,   // Film mode status is high
    VPP_FE_DET_MOTION_ADAPTIVE_LPF_NON_FILM_MODE = 0x02,   // Film mode status is ignored
    VPP_FE_DET_LPF_OUTPUT_ENABLED                = 0x03,   // Enables LPF output.
    VPP_FE_DET_LPF_OUTPUT_END
}VPP_FE_DET_LPF_MODE, *PVPP_FE_DET_LPF_MODE;

/*enum for EE pre-set level*/
typedef enum VPP_DET_PRESET_LEVEL
{
    VPP_FE_DET_TH_LOW = 0,
    VPP_FE_DET_TH_MEDIUM,
    VPP_FE_DET_TH_HIGH,
    VPP_FE_DET_TH_NONE
}VPP_DET_PRESET_LEVEL,*PVPP_DET_PRESET_LEVEL;

#define      VPP_DEFAULT_LTI_GAIN       64
#define      VPP_DEFAULT_CTI_GAIN       64

/**********************************************************************
*Function definition for CPCB EE block driver                                                           *
***********************************************************************/

/***************************************************************************************
 * FUNCTION: load the default register values of FE detail EE and CPCB EE block
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT CMU_EE_LoadDefaultVal(VPP_OBJ *vpp_obj);

/***************************************************************************************
 * FUNCTION: Initialize detail EE and CPCB EE according to configuration
 * PARAMS: DetThSel - Threshold configuration selection.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - invalid input parameters.
 *                MV_VPP_EBCMBUFFULL - BCM buffer is full.
 ***************************************************************************************/
INT CMU_EE_Config(VPP_OBJ *vpp_obj, INT DetThSel);

/***************************************************************************************
 * FUNCTION: Set the register values of EE params
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE values to BCM buffer
 ***************************************************************************************/
INT CMU_EE_SetEEParams(VPP_OBJ *vpp_obj, INT EeUnit, PVPP_CMU_EE_PARAMS pEEParam);

INT CMU_EE_SetEEParamsMode(VPP_OBJ *vpp_obj, INT mode);

#endif/*_VPP_CMU_EE_H_*/
#endif //ENABLE_EECMU
