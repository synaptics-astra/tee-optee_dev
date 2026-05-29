/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#include "tee_api_syna.h"
#endif

#include "tee_ta_mmap.h"
#include "tee_ta_mmap_helper.h"
#define _TEE_TA_MMAP_OVP_C_
#include "ovp_obj_module.h"

TEE_Result TEE_TA_Register_RegRegion(void *phy_addr, UINT32 size, UINT32 opt1, UINT32 opt2, void **va) {
	return TEE_TA_Register_RegRegion_Local(phy_addr, size, opt1, opt2, va, &ovp_mem_obj.ovpRegRegion);
}

void* TEE_TA_RegPhyToVirt(UINT32 phy_addr) {
	return TEE_TA_RegPhyToVirt_Local(phy_addr, &ovp_mem_obj.ovpRegRegion);
}
