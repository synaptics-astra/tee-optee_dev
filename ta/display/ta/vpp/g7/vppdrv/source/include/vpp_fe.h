/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_FE_H_
#define _VPP_FE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/********************** FE APIs ******************/
/*************************************************/

/*******************************************************************
 * FUNCTION: Reset all VPP FE blocks
 * PARAMS: *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_FE_Reset(VPP_OBJ *vpp_obj);

/********************************************************************
 * FUNCTION: initialize all VPP FE blocks according to configuration
 * PARAMS: *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 *******************************************************************/
int VPP_FE_Config(VPP_OBJ *vpp_obj);


#ifdef __cplusplus
}
#endif

#endif

