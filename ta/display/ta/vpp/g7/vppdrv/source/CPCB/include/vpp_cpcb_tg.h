/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_TG_H_
#define _VPP_CPCB_TG_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cpcb_common.h"

/**********************************************************************
*Data structure definitions for CPCB TG block driver                                                *
***********************************************************************/
#define  VPP_CPCB_FRST_POSITION    7

/*Enum for CPCB0 and CPCB1 TG plane*/
typedef enum VPP_CPCB_TG_PLANE
{
    TG_PLANE_MIN = 0,
    TG_PLANE0_BASE = TG_PLANE_MIN,
    TG_PLANE1_MAIN,
    TG_PLANE1_MAIN_CROP,
    TG_PLANE2_PIP,
    TG_PLANE2_PIP_CROP,
    TG_PLANE3_GFX0,
    TG_PLANE3_GFX0_CROP,
    TG_PLANE4_BASE_BORDER,
    TG_PLANE5_MAIN_BORDER,
    TG_PLANE6_PIP_BORDER,
    TG_PLANE7_GFX0_BORDER,
    TG_PLANE8_OVL,
    TG_PLANE_MAX
} VPP_CPCB_TG_PLANE,*PVPP_CPCB_TG_PLANE;

/*Enum for CPCB clock select*/
typedef enum VPP_CPCB_CLK_SEL
{
    VPP_CPCB_CLK_DISABLE = -1,
    VPP_CPCB_CLK_D1,
    VPP_CPCB_CLK_D2,
    VPP_CPCB_CLK_D4,
    VPP_CPCB_CLK_D6,
    VPP_CPCB_CLK_D8,
    VPP_CPCB_CLK_D12,
    VPP_CPCB_CLK_D3,
    VPP_CPCB_CLK_MAX

} VPP_CPCB_CLK_SEL;

 /*structure of horizontal and vertical total pixels*/
typedef struct VPP_CPCB_TG_HVTOTAL_T
{
    UINT32  HTotal;      // H Total Active and Inactive Pixels
    UINT32  VTotal;      // V Total Active and Inactive Pixels
    UINT32  Width;
    UINT32  Height;

} VPP_CPCB_TG_HVTOTAL, *PVPP_CPCB_TG_HVTOTAL;

/*timing generator parameters*/
typedef struct VPP_CPCB_TG_PARAMS_T
{
    UINT32  Vsampx;         // Horizontal Pos at which Vertical active data begins
    UINT32  FrameReset;     // Pos in the blank interval when frame reset is generated.
    UINT32  HSyncStartPos;  // H Sync Start Position
    UINT32  HSyncEndPos;    // H Sync End Position
    UINT32  VSyncStartPos;  // V Sync Start Position
    UINT32  VSyncEndPos;    // V Sync End Position
    UINT32  Mode;           //Mode for Bypass/SCL/Overlay path.
    UINT32  Lwin;           //Lock sync and Lock Active
    UINT32  Freeze;         //Freeze control for TG
    UINT32  SyncCtrl;       //Use this for low latency pass through

} VPP_CPCB_TG_PARAMS, *PVPP_CPCB_TG_PARAMS;

/**********************************************************************
*Function definitions for CPCB TG block driver                                                           *
***********************************************************************/

/***************************************************************************************
 * FUNCTION: load the default register values for CPCB TG block
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_LoadDefaultVal(VPP_OBJ *vpp_obj, INT CpcbUnit);

/**************************************************************************************
 * FUNCTION: start the timing generator by set horizontal and vertical total
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                resID - resolution ID
 *                HVtotal - horizontal and vertical total values
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
int CPCB_TG_SetHVTotal(VPP_OBJ *vpp_obj, int CpcbUnit, INT resID, PVPP_CPCB_TG_HVTOTAL pHVtotal);

/**************************************************************************************
 * FUNCTION: set the CPCB TG params
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                resID - resolution ID
 *                CpcbTgParams - TG params
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_SetParams(VPP_OBJ *vpp_obj, INT CpcbUnit, INT resID, PVPP_CPCB_TG_PARAMS pCpcbTgParams);

/**************************************************************************************
 * FUNCTION: set the the window position for one plane in the backend. can be used to disable a plane by setting the window to 0
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *               CpCbPlane - CPCB plane number
 *               WindowPos - plane window position
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_SetPlaneWindow(VPP_OBJ *vpp_obj, INT CpcbUnit, INT CpCbPlane, PVPP_WINDOW pWindowPos);

/**************************************************************************************
 * FUNCTION: set CPCB TG actual clock
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *               ClkSel - CPCB clock selector
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG clock selection to BCM buffer
 ***************************************************************************************/
INT CPCB_TG_ClkSelect(VPP_OBJ *vpp_obj, INT CpcbUnit, INT ClkSel);

#ifdef VPP_UNDERFLOW_FIX
INT VOP_TG_Enable(VPP_OBJ *vpp_obj, int cpcbID, int Enable);
#else
INT VOP_TG_Enable(VPP_OBJ *vpp_obj);
#endif

INT CPCB_TG_Enable(VPP_OBJ *vpp_obj, INT CpcbUnit, INT Enable);

INT CPCB_TG_PlaneEnable(VPP_OBJ *vpp_obj, INT CpcbUnit, INT CpCbPlane, INT Enable);

INT VOP2_TG_Start(VPP_OBJ *vpp_obj, INT CpcbUnit,INT start);

INT VOP_TG_SetParams(VPP_OBJ *vpp_obj, INT HRes, INT VRes);

#endif

