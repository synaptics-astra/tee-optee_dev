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
#include "drm_tee_memory.h"
#include "tee_internal_api.h"
#include "drm_tee_log.h"

//NOTE: This macro need to be removed after TZ MZ support is ready.
#define DRM_MEM_REGION_CHECK_DISABLE
#ifdef DRM_MEM_REGION_CHECK_DISABLE
HRESULT drm_mem_check_nosecure(const void *buffer, size_t size)
{
    return S_OK;
}

HRESULT drm_mem_check_secure(const void *buffer, size_t size)
{
    return S_OK;
}
#else
HRESULT drm_mem_check_nosecure(const void *buffer, size_t size)
{
    HRESULT result = E_INVALIDARG;
    TEE_MemRegion region;

    if(S_OK != TEE_FindMemRegion(&region, buffer, size)){
        DRM_ERR("%s:%d\n", __FUNCTION__, __LINE__);
        return result;
    }

    if(TEE_MR_IS_SECURE(&region)){
        DRM_ERR("%s:%d\n", __FUNCTION__, __LINE__);
        return result;
    }

    return S_OK;
}


HRESULT drm_mem_check_secure(const void *buffer, size_t size)
{
    HRESULT result = E_INVALIDARG;
    TEE_MemRegion region;

    if(S_OK != TEE_FindMemRegion(&region, buffer, size)){
        DRM_ERR("%s:%d\n", __FUNCTION__, __LINE__);
        return result;
    }

    if(TEE_MR_IS_NONSECURE(&region)){
        DRM_ERR("%s:%d\n", __FUNCTION__, __LINE__);
        return result;
    }

    return S_OK;
}
#endif

#ifdef CONFIG_OPTEE
void *DRM_TEE_SHM_Malloc(size_t size, DRM_TEE_SHM_HANDLE *phShm)
{
    void *va = NULL;

    if(size == 0){
        DRM_ERR("%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    va =  TEE_Malloc(size, TEE_MALLOC_FILL_ZERO);

    *phShm = (DRM_TEE_SHM_HANDLE)va;
    return va;
}

void DRM_TEE_SHM_Free(DRM_TEE_SHM_HANDLE shm)
{
	TEE_Free(shm);

    return;
}
#else // CONFIG_TZK
void *DRM_TEE_SHM_Malloc(size_t size, DRM_TEE_SHM_HANDLE *phShm)
{
    HRESULT result = S_OK;
    void *va = NULL;
    uint32_t shmid;

    if(size == 0){
        DRM_ERR("%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    result =  TEE_AllocateSharedMemory2(size, &shmid, &va);

    if(result != TEE_SUCCESS){
        DRM_ERR("TEE_AllocateSharedMemory2 faild return is 0x%08x\n", result);
        return NULL;
    }
    *phShm = (DRM_TEE_SHM_HANDLE)shmid;

    return va;
}

void DRM_TEE_SHM_Free(DRM_TEE_SHM_HANDLE shm)
{
    if(IS_VALID_SHM_HANDLE(shm)) {
        TEE_UnmapSharedMemory(shm);
    }
    return;
}
#endif
