/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _OVPS_APIFUNCS_H_
#define _OVPS_APIFUNCS_H_

#include "vpp_module.h"

INT OVPS_NGPTV_EnableBypass(VPP_OBJ* vpp_obj, UINT32 uiBypassOn);
int OVPS_NotifyEvent(VPP_OVP_OBJ *ovp_obj, UINT32 event, UINT32 infoID);

#endif

