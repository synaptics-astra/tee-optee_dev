/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_SCL_H_
#define _VPP_SCL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_scl_data.h"

/*************************************************/
/********************** SCL APIs ******************/
/*************************************************/

/*******************************************************************
 * FUNCTION: Reset all VPP SCL blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_SCL_Reset(VPP_OBJ *vpp_obj);

/*********************************************************************
 * FUNCTION: initialize all VPP SCL blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ********************************************************************/
int VPP_SCL_Config(VPP_OBJ *vpp_obj);

/***************************************************************************************
 * FUNCTION: set BE FIFO for loopback
 * PARAMS: pCpcb0Tot - CPCB0 output total resolution
 *               pCpcb0Res - CPCB0 active resolution
 * RETURN:  1 - succeed
 *                others - failed to load CPCB BE FIFO values to BCM buffer
 ***************************************************************************************/
int VPP_SCL_SetBeFifo(VPP_OBJ *vpp_obj, PVPP_CPCB0_OUTPUT_RES pCpcb0Tot, PVPP_CPCB0_OUTPUT_RES pCpcb0Res);

#ifdef __cplusplus
}
#endif

#endif

