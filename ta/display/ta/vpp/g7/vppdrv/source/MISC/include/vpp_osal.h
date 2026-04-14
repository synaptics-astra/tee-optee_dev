/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_SHM_H_
#define _VPP_SHM_H_

#if !defined(__LINUX_KERNEL__)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif
#include <stdarg.h>

#if defined(CONFIG_TEE) || !defined(CONFIG_AMP_TEE_ENABLE)
#include "OSAL_api.h"
#include "isl/amp_shm.h"
#else
#include "tee_ta_osal.h"
#endif
#ifdef TZ_3_0
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
#include "bm_heap.h"
#endif
#endif

#define VPP_SHM_SYSTEM        0

/**< This memory type will be deprecated, For new users please
 * do NOT use any more
 */
#define VPP_SHM_NONSECURE_NONCACHE  1

/**< This memory will be dynamically allocated for component input/out
     buffer, like ES buffer etc. */
#define VPP_SHM_NONSECURE        2

/**< it is secure memory space. */
#define VPP_SHM_SECURE         3

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t VPP_SHM_HANDLE;

enum {
VPP_SHM_TYPE_STATIC = 0,
VPP_SHM_TYPE_DYNAMIC,
};

#ifdef TZ_3_0
UINT  VPP_OSAL_SHM_Malloc_Internal(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm, unsigned int useLocalMemMgr);
#endif
/*SHM_Dynamic alloc/free interface defined to enable following in different worlds
* Clear world   :  to continue to use heap instead of SHM allocation
* Trusted world :  to allocated memory from SHM (internal/external); in TZ some of dynamic allocation may have to be moved to  */

unsigned int  VPP_OSAL_SHM_Dynamic_Malloc(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm);
unsigned int VPP_OSAL_SHM_Dynamic_Free(VPP_SHM_HANDLE  phShm);

/* Memory allocation functions */
void *VPP_Malloc(size_t size);
void VPP_Free(void *ptr);
void *VPP_Memset(void * ptr, int value, size_t num);
void *VPP_Memcpy(void * destination, const void * source, size_t num);

/* Shared memory allocation functions */
unsigned int  VPP_SHM_Malloc_Only(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm);
unsigned int  VPP_SHM_Malloc(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm);
unsigned int VPP_SHM_Free(VPP_SHM_HANDLE  phShm);
void *VPP_SHM_GetVirtualAddress(VPP_SHM_HANDLE  phShm, unsigned int offset);
void *VPP_SHM_GetPhysicalAddress(VPP_SHM_HANDLE  phShm, unsigned int offset);
unsigned int VPP_SHM_CleanCache(VPP_SHM_HANDLE  phShm, unsigned int offset, unsigned int  size);
#define VPP_SHM8_MALLOC(x, y, z)    VPP_SHM_Malloc(x, y, 8, z)
#define VPP_SHM32_MALLOC(x, y, z)  VPP_SHM_Malloc(x, y, 32, z)
#define VPP_SHM64_MALLOC(x, y, z)  VPP_SHM_Malloc(x, y, 64, z)
#define VPP_SHM1K_MALLOC(x, y, z)  VPP_SHM_Malloc(x, y, 1024, z)

#define VPP_SHM32_MALLOC_ONLY(x, y, z)  VPP_SHM_Malloc_Only(x, y, 32, z)
#define VPP_SHM1K_MALLOC_ONLY(x, y, z)  VPP_SHM_Malloc_Only(x, y, 1024, z)

//define VPP_OSAL_SHM_ALLOC_ONLY before this file inclusion for allocation that need not be freed
#ifdef VPP_OSAL_SHM_ALLOC_ONLY
#undef VPP_SHM32_MALLOC
#define VPP_SHM32_MALLOC VPP_SHM32_MALLOC_ONLY
#undef VPP_SHM1K_MALLOC
#define VPP_SHM1K_MALLOC VPP_SHM1K_MALLOC_ONLY
#endif //VPP_OSAL_SHM_ALLOC_ONLY

/* Register access functions */
void VPP_REG_WORD32_WRITE(unsigned int addr, unsigned int val);
void VPP_REG_WORD32_READ(unsigned int addr, unsigned int *pVal);

/* Debug functions */
void VPP_DEBUG_Printf(const char *fmt, ...);

TEE_Result VPP_OSAL_TEE_Callback(
    uint32_t        commandID,
    uint32_t        paramTypes,
    TEE_Param       params[4],
    uint32_t        cancellationRequestTimeout,
    uint32_t*       returnOrigin);

UINT32 VPP_OSAL_GetCurTeeTimeInMS(void);

#if defined( __LINUX_KERNEL__ )
unsigned int rand(void);
#endif


#ifdef __cplusplus
}
#endif

#endif

