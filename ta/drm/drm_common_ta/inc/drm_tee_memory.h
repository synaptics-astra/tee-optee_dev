/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */



#ifndef __DRM_COMMON_MEMORY_H__
#define __DRM_COMMON_MEMORY_H__

#ifdef CONFIG_OPTEE
#include "tee_api.h"
#include "tee_internal_api.h"
#else
#include "tee_common.h"
#endif

#include "drm_tee_types.h"

#ifdef CONFIG_OPTEE
typedef void * DRM_TEE_SHM_HANDLE;
#else
typedef uint32_t DRM_TEE_SHM_HANDLE;
#endif

#define INVALID_SHM_HANDLE          ((uint32_t )NULL)
#define IS_VALID_SHM_HANDLE(h)      (h != INVALID_SHM_HANDLE)

#define DRM_TEE_SHM_InvalidateCache TEE_InvalidateCache
#define DRM_TEE_SHM_CleanCache      TEE_CleanCache
#define DRM_TEE_SHM_FlushCache      TEE_FlushCache


#define DRM_TEE_SHM_P2V(pa)         TEE_PhysToVirt((void *)(pa))
#define DRM_TEE_SHM_V2P(va)         (UINT32)TEE_VirtToPhys(va)

void *DRM_TEE_SHM_Malloc(size_t size, DRM_TEE_SHM_HANDLE *pShm);
void DRM_TEE_SHM_Free(DRM_TEE_SHM_HANDLE shm);

/** Check buffer if secure memory.
  * @retval TEE_SUCCESS     Pass the check.
  */
HRESULT drm_mem_check_nosecure(const void *buffer, size_t size);

/** Check buffer if nosecure memory.
  * @retval TEE_SUCCESS     Pass the check.
  */
HRESULT drm_mem_check_secure(const void *buffer, size_t size);

#endif  // #ifndef __DRM_COMMON_MEMORY_H__
