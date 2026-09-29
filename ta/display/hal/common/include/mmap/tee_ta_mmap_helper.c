/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_mmap_helper.c
//!
//! \brief Implementation of TEE memory mapping helper functions
//!
//! Purpose: This is used for mapping VPP register Physical to virual address
//!          and vice versa
//!
//!     Version    Date                     Author
//!     V 1.00,    Apr 1 2024,             Shanmugam Ramachandran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_ta_mmap_helper.h"

#include "tee_internal_api.h"
#ifndef CONFIG_OPTEE
#include "tee_ta.h"
#else
#include "tee_api_syna.h"
#endif

TEE_Result TEE_TA_Register_RegRegion_Local(paddr_t phy_addr, UINT32 size, UINT32 opt1, UINT32 opt2, void **va, TEE_TA_REG_REGION *regRegion)
{
    TEE_Result result = TEE_SUCCESS;
    void *virt_addr;

    result = TEE_Map_Register(phy_addr, size, opt1, opt2, &virt_addr);
    if (result == TEE_SUCCESS) {
        *va = virt_addr;
    }

#ifdef CONFIG_OPTEE

    UINT32 regionNdx = regRegion->RegRegionCnt;

    if (regionNdx >= MAX_TEE_TA_REG_REGION) {
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    if (result == TEE_SUCCESS) {
        regRegion->regRegion[regionNdx].phy_addr = phy_addr;
        regRegion->regRegion[regionNdx].phy_addr_end = phy_addr + size;
        regRegion->regRegion[regionNdx].size = size;
        regRegion->regRegion[regionNdx].virt_addr = virt_addr;
        regRegion->RegRegionCnt++;
    }
#endif

    return result;
}

void* TEE_TA_RegPhyToVirt_Local(paddr_t phy_addr, TEE_TA_REG_REGION *regRegion)
{
    void *virt_addr = NULL;

#ifdef CONFIG_OPTEE
    for (unsigned int i = 0; i < regRegion->RegRegionCnt; i++) {
        if (phy_addr >= regRegion->regRegion[i].phy_addr &&
            phy_addr < regRegion->regRegion[i].phy_addr_end) {
            virt_addr = (char *)regRegion->regRegion[i].virt_addr +
                        (phy_addr - regRegion->regRegion[i].phy_addr);
        }
    }
#else
    virt_addr = (void*) phy_addr;
#endif //CONFIG_OPTEE

    return virt_addr;
}

