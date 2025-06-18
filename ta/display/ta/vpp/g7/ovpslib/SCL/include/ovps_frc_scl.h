/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _OVP_FRC_SCL_H_
#define _OVP_FRC_SCL_H_

#include "vpp_module.h"
#include "ovps_api.h"

INT OVPSCL_FRC_SCL_SetSclCtrlParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl);

INT OVPSCL_FRC_SCL_SetFrcParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pFrcRes);

#endif

