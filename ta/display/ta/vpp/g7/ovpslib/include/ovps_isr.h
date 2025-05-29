/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _OVPS_ISR_H_
#define _OVPS_ISR_H_

#include "vpp_module.h"
#include "ovps_api.h"

int OVPS_Update_frame(VPP_OBJ *vpp_obj,OVPS_CLIENT_REQUEST *pstOvpsRequest);
void OVPS_Process(VPP_OBJ *vpp_obj, bool isSkipUpdateFE);

#endif

