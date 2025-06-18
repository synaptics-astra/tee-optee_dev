/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _BCMBUF_UTIL_H_
#define _BCMBUF_UTIL_H_

void VPP_Reset_CFGQ(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_Select_Buffer(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_Switch_Buffer(VPP_OBJ *vpp_obj, BOOL isClearBuf, int cpcbID);
void VPP_Switch_CFGQ(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_AutoPush_Handler(VPP_OBJ *vpp_obj, int cpcbID, int intrtype);
#endif
