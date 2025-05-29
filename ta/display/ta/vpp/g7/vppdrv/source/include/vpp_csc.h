/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CSC_H_
#define _VPP_CSC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/********************** CSC APIs ******************/
/*************************************************/

/***************************************************************************************
 * Description: reset CSC
 * PARAMS: none
 * RETURN:  1 - succeed
 *                others - failed to load CSC register default values to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_Reset(VPP_OBJ *vpp_obj);

#ifdef __cplusplus
}
#endif

#endif

