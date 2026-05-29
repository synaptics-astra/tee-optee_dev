/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _TEE_TA_TYPES_H_
#define _TEE_TA_TYPES_H_

#include "com_type.h"

typedef struct __TEE_TA__REGISTER_REGION_NODE__ {
    UINT32 phy_addr;
    UINT32 phy_addr_end;
    void *virt_addr;
    UINT32 size;
} TEE_TA_REGISTER_REGION_NODE;

typedef struct _TEE_TA_REG_REGION_
{
    TEE_TA_REGISTER_REGION_NODE regRegion[MAX_TEE_TA_REG_REGION];
    UINT32 RegRegionCnt;
} TEE_TA_REG_REGION;

#endif // _TEE_TA_TYPES_H_
