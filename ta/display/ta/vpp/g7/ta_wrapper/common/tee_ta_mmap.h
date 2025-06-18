/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _TEE_TA_MMAP_H_
#define _TEE_TA_MMAP_H_

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#include "tee_api_syna.h"
#endif

#include "com_type.h"

TEE_Result TEE_TA_Register_RegRegion(void *phy_addr, UINT32 size, UINT32 opt1, UINT32 opt2, void **va);
void* TEE_TA_RegPhyToVirt(UINT32 phy_addr);

#endif // _TEE_TA_MMAP_H_