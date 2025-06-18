/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_APIFUNCS_FASTLOGO_H_
#define _VPP_APIFUNCS_FASTLOGO_H_

#include "Galois_memmap.h"
#include "galois_io.h"
#include "vpp_module.h"
#include "tee_ta_osal.h"
#include "vpp_apifuncs.h"
#include "vpp.h"

void MV_VPP_EnableSemIntr(VPP_OBJ *vpp_obj, int cpcbID, int flag);
void MV_VPP_Enable_ISR_Interrupt(VPP_OBJ *vpp_obj, int cpcbID, int flag);
void MV_VPP_EnableInterrupt(VPP_OBJ *vpp_obj, int cpcbID);
void MV_VPP_DisableInterrupt(VPP_OBJ *vpp_obj, int cpcbID);
void MV_VPP_get_interrupt_message(VPP_OBJ *vpp_obj, UINT32 *pIntrMsg);
int MV_VPP_Stop(VPP_OBJ *vpp_obj);

int MV_VPP_ClearSCDCReservedBit(VPP_OBJ *vpp_obj);

int MV_VPPOBJ_EnableInterrupt(int handle, int flag);

void MV_VPP_Clk_Ctrl(void);

extern void VPP_GlobalIntr_Config(VPP_OBJ *vpp_obj, int cpcbID);
#endif //_VPP_APIFUNCS_FASTLOGO_H_
