/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_DISPTG_H_
#define _VPP_CPCB_DISPTG_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cpcb_common.h"

/**********************************************************************
*Data structure definitions for CPCB TG block driver                                                *
***********************************************************************/

/*structure of horizontal and vertical total pixels*/
typedef struct VPP_CPCB_DISPTG_HVTOTAL_T
{
    UINT32  HTotal;      // H Total Active and Inactive Pixels
    UINT32  VTotal;      // V Total Active and Inactive Pixels
    UINT32  Width;
    UINT32  Height;

} VPP_CPCB_DISPTG_HVTOTAL, *PVPP_CPCB_DISPTG_HVTOTAL;

/*timing generator parameters*/
typedef struct VPP_CPCB_DISPTG_PARAMS_T
{
    UINT32  Vsampx;         // Horizontal Pos at which Vertical active data begins
    UINT32  FrameReset;     // Pos in the blank interval when frame reset is generated.
    UINT32  HSyncStartPos;  // H Sync Start Position
    UINT32  HSyncEndPos;    // H Sync End Position
    UINT32  VSyncStartPos;  // V Sync Start Position
    UINT32  VSyncEndPos;    // V Sync End Position
    UINT32  VSStartPos;
    UINT32  VSEndPos;
    UINT32  Mode;           //Mode for Bypass/SCL/Overlay path.
    UINT32  Lwin;           //Lock sync and Lock Active
    UINT32  Freeze;         //Freeze control for TG
    UINT32  SyncCtrl;       //Use this for low latency pass through

} VPP_CPCB_DISPTG_PARAMS, *PVPP_CPCB_DISPTG_PARAMS;


/**********************************************************************
*Function definitions for CPCB TG block driver                                                           *
***********************************************************************/

/***************************************************************************************
 * FUNCTION: load the default register values for CPCB TG block
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_DISPTG_LoadDefaultVal(VPP_OBJ *vpp_obj, INT cpcb);

/**************************************************************************************
 * FUNCTION: start the timing generator by set horizontal and vertical total
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                HVtotal - horizontal and vertical total values
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
int CPCB_DISPTG_SetHVTotal(VPP_OBJ *vpp_obj, INT cpcb, PVPP_CPCB_DISPTG_HVTOTAL pHVtotal, INT clrfmt);

  /**************************************************************************************
 * FUNCTION: set the CPCB TG params
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *                CpcbTgParams - TG params
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_DISPTG_SetParams(VPP_OBJ *vpp_obj, INT cpcb, PVPP_CPCB_DISPTG_PARAMS pCpcbTgParams);

  /**************************************************************************************
 * FUNCTION: set the the window position for one plane in the backend. can be used to disable a plane by setting the window to 0
 * PARAMS: CpcbUnit - CPCB unit number(CPCB0, CPCB1, CPCB2)
 *               CpCbPlane - CPCB plane number
 *               WindowPos - plane window position
 * RETURN:  1 - succeed
 *                others - failed to load CPCB TG default values to BCM buffer
 ***************************************************************************************/
INT CPCB_DISPTG_SetPlaneWindow(VPP_OBJ *vpp_obj, INT cpcb, PVPP_WINDOW pWindowPos);

INT CPCB_DISPTG_EnableIntl(VPP_OBJ *vpp_obj, INT cpcb, INT Enable);

INT CPCB_DISPTG_ToggleField(VPP_OBJ *vpp_obj, INT cpcb);

#endif

