/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _TEE_TA_MMAP_DHUB_C_

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#include "tee_api_syna.h"
#endif

#include "tee_ta_mmap.h"
#include "tee_ta_mmap_helper.h"
#include "dhub_module.h"

TEE_Result TEE_TA_Register_RegRegion(void *phy_addr, UINT32 size, UINT32 opt1, UINT32 opt2, void **va) {
        return TEE_TA_Register_RegRegion_Local(phy_addr, size, opt1, opt2, va, &dhub_obj.dhubRegRegion);
}

void* TEE_TA_RegPhyToVirt(UINT32 phy_addr) {
        return TEE_TA_RegPhyToVirt_Local(phy_addr, &dhub_obj.dhubRegRegion);
}
