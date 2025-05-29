/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _INTR_UTIL_H_
#define _INTR_UTIL_H_
#include "vpp_module.h"

void VPP_GlobalIntr_Config(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_DTGUF_Handler(VPP_OBJ *vpp_obj, int cpcbID);
void VPP_AVIO_Reset(void);
#endif //_INTR_UTIL_H_
