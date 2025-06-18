/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_dv_frame_dump.h
//! \brief      The VPP DV frame dump framework header file
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef __VPP_DV_FRAME_DUMP_H__
#define __VPP_DV_FRAME_DUMP_H__

#include "vpp_module.h"
#ifndef VPP_IN_TRUST_ZONE
#include "isl/amp_shm.h"
#else
#include "tee_ta_osal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VPP_DV_FRAME_DUMP_SET_USED_BIT(int_ptr, frame_num) ((*(int_ptr+(frame_num/VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD))) |= (1<<(frame_num%VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD)))
#define VPP_DV_FRAME_DUMP_GET_USED_BIT(int_ptr, frame_num) ((*(int_ptr+(frame_num/VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD))) & (1<<(frame_num%VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD)))

/******* vpp frame dump APIs ********/
/*********************************************************
 * FUNCTION: Initialize the VPP dump framework
 * PARAMS: *pDumpConfig - pointer to a VPP DUMP Config structure
 *          vpp_obj - VPP object containing dump details
 ********************************************************/
void VPP_DV_FrameDump_Init(VPP_OBJ *vpp_obj, VPP_DV_FRAME_DUMP *pDumpConfig);

/*********************************************************
 * FUNCTION: Get the Configuration and status of VPP dump framework
 * PARAM: *pDumpConfig - pointer to a VPP DUMP Config/Status structure
 *         vpp_obj - VPP object containing dump details
 * RETURN:  1: VPP frame dump module is initilaized, 0: not initialized
 ********************************************************/
int VPP_DV_FrameDump_GetConfig(VPP_OBJ *vpp_obj, VPP_DV_FRAME_DUMP *pDumpConfig);

/*********************************************************
 * FUNCTION: Move the ndx and get the next/new buffer for dumping
 * PARAMS: vpp_obj - VPP object containing plane details - offline buffer
 * RETURN: phy-address of the next/new dump buffer
 * NOTE: 1. When ndx is 0, then we may have to do VOP RESET/FE_DLR_VopRst()
 *          for DITHER dump path
 ********************************************************/
unsigned int VPP_DV_FrameDump_GetNewFrame(VPP_OBJ *vpp_obj);

/*********************************************************
 * FUNCTION: Get the current buffer for dumping
 * PARAMS: vpp_obj - VPP object containing plane details - offline buffer
 * RETURN: phy-address of the current dump buffer
 ********************************************************/
unsigned int VPP_DV_FrameDump_GetCurrFrame(VPP_OBJ *vpp_obj);

/*********************************************************
 * FUNCTION: Overlay Screen Capture Function
 * PARAMS: vpp_obj - VPP object containing plane details - offline buffer
 ********************************************************/
void VPP_Ovl_Capture_Handler(VPP_OBJ *vpp_obj, PLANE *plane);

#ifdef __cplusplus
}
#endif
#endif

