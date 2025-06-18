/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file bm_heap.h
//!
//! \brief  memory/heap manager interfaces
//!
//! Purpose:
//!
//! Version,    Date and Author :
//!   V 1.00,                   Shanmugam Ramachandran
//!   V 1.10,   Mar 11 2022,    Shanmugam Ramachandran
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __BM_HEAP__H__
#define __BM_HEAP__H__
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
#include "tee_internal_api.h"
#include "com_type.h"

//Max bit position/mask value
#define BM_MAX_BITMASK_VAL 0x80000000
//Max Number of bits per word
#define BM_BSZ 32
#define BM_S_OK 0
#define BM_S_FAIL -1
#define BM_ENABLE_SHM_MUTEX
#define BM_INIT(uiShmPA, uiShmVA, uiShmSize, align, Memhandle)  bitmap_pool_init(uiShmPA, uiShmSize, align, uiShmVA, Memhandle)

#define BM_MALLOC(size)     bm_malloc(gMemhandle,size)
#define BM_FREE(ptr)        bm_free(gMemhandle, ptr)
#define BM_REALLOC(src,size) bm_realloc(gMemhandle, src, size)

#ifdef BITMAP_LOGGER
#define BM_LOGI(...)     VPPAPI_DBG_INFO(__VA_ARGS__)
#define BM_LOGE(...)     VPPAPI_DBG_ERROR(__VA_ARGS__)
#else
#define BM_LOGI(...)
#define BM_LOGE(...)
#endif

#ifdef BITMAP_LOGGER_DETAILED
#define BM_LOGL(...)     VPPAPI_DBG_INFO(__VA_ARGS__)
#else
#define BM_LOGL(...)
#endif

#define BM_LOGF(...)     VPPAPI_DBG_FIELD(__VA_ARGS__)

#ifdef BM_ENABLE_SHM_MUTEX
#define BM_SHM_MUTEX_Create(...)    MV_OSAL_Mutex_Create(__VA_ARGS__)
#define BM_SHM_MUTEX_Lock(...)      MV_OSAL_Mutex_Lock(__VA_ARGS__)
#define BM_SHM_MUTEX_Unlock(...)    MV_OSAL_Mutex_Unlock(__VA_ARGS__)
#else
#define BM_SHM_MUTEX_Create(...)
#define BM_SHM_MUTEX_Lock(...)
#define BM_SHM_MUTEX_Unlock(...)
#endif


typedef struct bitmap_memory
{
    UINT32 start_addr_virt;
    UINT32 start_addr_phy;
    UINT32 start_addr_virt_only;
    void *start_addr;
    UINT size;
    UINT *bitblock;
    UINT align;
    UINT *sizeblock;
#ifdef BM_ENABLE_SHM_MUTEX
    MV_OSAL_HANDLE_MUTEX_t osal_shm_lock;
#endif
}bitmap_mem;

#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
#ifdef _HEAP_MANAGER_C_
bitmap_mem *gMemhandle;
#else
extern bitmap_mem *gMemhandle;
#endif
#endif

#define BM_SHM_ALLOC_ALING_ROUNDUP(size, align)  ((size + align - 1) & (~(align - 1)))
#define BM_MALLOC_ONLY(size, align)     bm_malloc_only(gMemhandle, size, align)
UINT* bm_malloc_only(bitmap_mem *handle, const UINT size, const UINT align);

//below three added to enable mutex lock .. which wil be of use later when we use
//mutliple instance of heap manager
UINT* bm_malloc(bitmap_mem *handle, const UINT size);
UINT *bm_free(bitmap_mem *handle, UINT* Addr);
UINT *bm_realloc(bitmap_mem *handle, UINT* Addr, UINT size);

//Where Align = N in terms of 2^N i.e for 32byte alignment, then align=N=5
INT bitmap_pool_init(UINT32 PAddr, UINT Size, UINT Align, UINT *Addr,bitmap_mem *Memhandle);
void VPP_OSAL_SHM_Init(unsigned int uiShmPA, unsigned int uiShmVA, unsigned int uiShmSize,bitmap_mem *Memhandle);
#endif //VPP_ENABLE_INTERNAL_MEM_MGR
#endif //__BM_HEAP__H__
