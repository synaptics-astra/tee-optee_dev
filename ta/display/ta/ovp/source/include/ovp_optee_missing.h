/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_OPTEE_MISSING_H_
#define _OVP_OPTEE_MISSING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ovp_common.h"

TEE_Result TEE_Unmap_Memory_FromAllInstances(uint32_t mem_id);
TEE_Result TEE_Map_Memory_ToAllInstances(uint32_t mem_id, void **va, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* _OVP_OPTEE_MISSING_H_ */
