/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_OBJ_MODULE_H_
#define _OVP_OBJ_MODULE_H_

#include "tee_ta_mmap_types.h"

typedef struct tagOVP_OBJ {
	TEE_TA_REG_REGION ovpRegRegion;
}OVP_MEM_OBJ;

#ifdef _TEE_TA_MMAP_OVP_C_
OVP_MEM_OBJ ovp_mem_obj;
#else
extern OVP_MEM_OBJ ovp_mem_obj;
#endif

#endif // _OVP_OBJ_MODULE_H_
