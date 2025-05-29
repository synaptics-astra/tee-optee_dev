/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifdef CONFIG_AMP_TEE_ENABLE
#include "ta_log.h"
#include "vpp_cmd.h"
#include "tee_internal_api.h"
#elif defined( __LINUX_KERNEL__ )
#include <linux/printk.h>
#include <linux/delay.h>
#include "mrvl_io.h"
#include "dma_buf.h"
#endif

#include "galois_io.h"
#include "vpp_osal.h"
#include "vpp_debug.h"
#include "vpp_api.h"
#include "OSAL_api.h"
#include "vpp_module.h"
#include "vpp_intr_monitor.h"

/* Memory allocation functions */
void *VPP_Malloc(size_t size) {
#ifdef CONFIG_AMP_TEE_ENABLE
#if defined (TZ_3_0) && defined (VPP_ENABLE_INTERNAL_MEM_MGR)
    return BM_MALLOC(size);
#else
    return TEE_Malloc(size, TEE_MALLOC_ZEROS);
#endif
#elif defined( __LINUX_KERNEL__ )
    return kmalloc(size, GFP_KERNEL);
#else
    return malloc(size);
#endif
}

void VPP_Free(void *ptr) {
#ifdef CONFIG_AMP_TEE_ENABLE
#if defined (TZ_3_0) && defined (VPP_ENABLE_INTERNAL_MEM_MGR)
    BM_FREE(ptr);
#else
    TEE_Free(ptr);
#endif
#elif defined( __LINUX_KERNEL__ )
    kfree(ptr);
#else
    free(ptr);
#endif
}

void *VPP_Memset(void * ptr, int value, size_t num){
#ifdef CONFIG_AMP_TEE_ENABLE
    TEE_MemFill(ptr, value, num);
    return ptr;
#else
    return memset(ptr, value, num);
#endif
}

void *VPP_Memcpy(void * destination, const void * source, size_t num) {
#ifdef CONFIG_AMP_TEE_ENABLE
    TEE_MemMove(destination, (void *)source, num);
    return destination;
#else
    return memcpy(destination, source, num);
#endif
}

#ifdef TZ_3_0
/* Shared Memory allocation functions */
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
void VPP_OSAL_SHM_Init(UINT32 uiShmPA, UINT32 uiShmVirt, UINT32 uiShmSize, bitmap_mem *Memhandle) {
    int align=5;    //2^5 = 32 bytes
    BM_INIT(uiShmPA, (UINT*)uiShmVirt, uiShmSize, align, Memhandle);
}
#endif

UINT  VPP_OSAL_SHM_Malloc_Internal(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm, unsigned int useLocalMemMgr)
#else //TZ_3_0
unsigned int VPP_SHM_Malloc(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm)
#endif
{
#if defined(VPP_DIAGS)
    //following results in virtual address
    if(uiAlign==8)
        *phShm = (uintptr_t)GALOIS_8_MALLOC(uiSize);
    else if(uiAlign==32)
        *phShm = (uintptr_t)GALOIS_32_MALLOC(uiSize);
    else if(uiAlign==64)
        *phShm = (uintptr_t)GALOIS_64_MALLOC(uiSize);
    else if(uiAlign==1024)
        *phShm = (uintptr_t)GALOIS_1K_MALLOC(uiSize);
    else
        *phShm = (uintptr_t)GALOIS_32_MALLOC(uiSize);

    return (MV_VPP_OK);
#elif defined(CONFIG_AMP_TEE_ENABLE)
#ifdef TZ_3_0
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    unsigned int maxChunkSize = uiSize;
#ifdef SHM_MAX_CHUNK_SZ
        maxChunkSize = SHM_MAX_CHUNK_SZ;
#endif
    if (useLocalMemMgr && (uiSize <= maxChunkSize)) {
        *phShm = (VPP_SHM_HANDLE)BM_MALLOC(uiSize);
        if (!*phShm) {
            VPPAPI_DBG_ERROR("VPP_SHM_Malloc failed!\n");
            return TEE_ERROR_OUT_OF_MEMORY;
        }
        return TEE_SUCCESS;
    } else {
#endif //VPP_ENABLE_INTERNAL_MEM_MGR
#endif
    TEE_Result result;
    TEE_MemRegion mr;
    void *vir_addr = NULL;
    uint32_t cb_cmdID = VPP_CB_MALLOCSHM;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = VPP_SHM_SECURE;
    cb_params[0].value.b = uiSize;
    cb_params[1].value.a = uiAlign;
    cb_params[1].value.b = 0xdeadbeef;

    result = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    *phShm = cb_params[1].value.b;

    if (*phShm) {
        vir_addr = VPP_SHM_GetVirtualAddress(*phShm, 0);
        if (TEE_SUCCESS != TEE_FindMemRegion(&mr, vir_addr, uiSize)) {
            VPPAPI_DBG_ERROR("TEE_FindMemRegion failed, phyaddr: %p, uiSize%d!\n",
                cb_params[1].value.b, uiSize);
            result = TEE_ERROR_BAD_PARAMETERS;
            goto VPP_SHM_MALLOC_Exit;
        }

        if (TEE_MR_IS_RESTRICTED(&mr)) {
             VPPAPI_DBG_ERROR("TEE_MR_IS_RESTRICTED check fail!\n");
             result = TEE_ERROR_ACCESS_DENIED;
             goto VPP_SHM_MALLOC_Exit;
        }

        if (!TEE_MR_IS_SECURE(&mr)) {
            VPPAPI_DBG_ERROR("This memory is not a secure memory!\n");
            result = TEE_ERROR_BAD_PARAMETERS;
            goto VPP_SHM_MALLOC_Exit;
        }
    } else {
        result = TEE_ERROR_OUT_OF_MEMORY;
        VPPAPI_DBG_ERROR("VPP_SHM_Malloc failed!\n");
        goto VPP_SHM_MALLOC_Exit;
    }

    *(unsigned int *)(cb_params[1].value.b) = 0xdeadbeef;

    return TEE_SUCCESS;
VPP_SHM_MALLOC_Exit:
    *phShm = 0;
    return result;
#ifdef TZ_3_0
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    }
#endif //VPP_ENABLE_INTERNAL_MEM_MGR
#endif//TZ_3_0
#elif defined( __LINUX_KERNEL__ )
    dma_buf_allocate(uiSize, uiAlign, (dma_buf_handle *)phShm);
    return MV_VPP_OK;
#else
    if(eMemType == VPP_SHM_TYPE_STATIC)
        eMemType = AMP_SHM_STATIC;
    else if(eMemType == VPP_SHM_TYPE_DYNAMIC)
        eMemType = AMP_SHM_DYNAMIC;
    return AMP_SHM_Allocate(eMemType, uiSize, uiAlign, phShm);
#endif
}

UINT VPP_OSAL_SHM_Malloc_Only_Internal(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm, unsigned int useLocalMemMgr) {
    UINT ret = 1;
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    unsigned int maxChunkSize = uiSize;
#ifdef SHM_MAX_CHUNK_SZ
        maxChunkSize = SHM_MAX_CHUNK_SZ;
#endif
    if (useLocalMemMgr && (uiSize <= maxChunkSize)) {
        *phShm = (VPP_SHM_HANDLE)BM_MALLOC_ONLY(uiSize, uiAlign);
        if (!*phShm) {
            ret = VPP_OSAL_SHM_Malloc_Internal(eMemType, uiSize, uiAlign, phShm, useLocalMemMgr);
        } else {
            ret = TEE_SUCCESS;
        }
    } else
#endif //VPP_ENABLE_INTERNAL_MEM_MGR
    {
        ret = VPP_OSAL_SHM_Malloc_Internal(eMemType, uiSize, uiAlign, phShm, useLocalMemMgr);
    }

    if (ret != TEE_SUCCESS) {
        VPPAPI_DBG_ERROR("VPP_SHM_Malloc failed!\n");
    }
    return ret;
}

#ifdef TZ_3_0
unsigned int VPP_SHM_Free_Internal(VPP_SHM_HANDLE  hShm, unsigned int useLocalMemMgr)
#else //TZ_3_0
unsigned int VPP_SHM_Free_Internal(VPP_SHM_HANDLE  hShm)
#endif
{
#if defined(VPP_DIAGS)
    GALOIS_32_FREE((void *)hShm);
    return (MV_VPP_OK);
#elif defined(CONFIG_AMP_TEE_ENABLE)
#ifdef TZ_3_0
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    if (useLocalMemMgr) {
        BM_FREE((UINT*)hShm);
        return TEE_SUCCESS;
    } else {
#endif//VPP_ENABLE_INTERNAL_MEM_MGR
#endif
    TEE_Result result;
    uint32_t cb_cmdID = VPP_CB_FREESHM;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = hShm;
    result = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return result;
#ifdef TZ_3_0
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    }
#endif //VPP_ENABLE_INTERNAL_MEM_MGR
#endif//TZ_3_0
#elif defined( __LINUX_KERNEL__ )
    return dma_buf_release((dma_buf_handle)hShm);
#else
    return AMP_SHM_Release(hShm);
#endif
}

void *VPP_SHM_GetVirtualAddress(VPP_SHM_HANDLE  hShm, unsigned int offset)
{
    void *vir_addr = NULL;
#if defined(VPP_DIAGS)
    vir_addr = (void *)(hShm+offset);
#elif defined(CONFIG_AMP_TEE_ENABLE)
#if defined (VPP_ENABLE_INTERNAL_MEM_MGR) && defined(VPP_ENABLE_MEMID_BY_VIRT_ADDR)
    //Note shm to virtual is 1:1 mapping
    vir_addr = (void *)(hShm + offset);
#else
    vir_addr = (void *)TEE_PhysToVirt((void *)hShm) + offset;
#endif
#elif defined( __LINUX_KERNEL__ )
    dma_buf_getvirtaddr((dma_buf_handle)hShm, offset, &vir_addr);
#else
    AMP_SHM_GetVirtualAddress(hShm, offset, &vir_addr);
#endif
    return vir_addr;
}

void *VPP_SHM_GetPhysicalAddress(VPP_SHM_HANDLE  hShm, unsigned int offset)
{
    void *phy_addr = NULL;
#if defined(VPP_DIAGS)
    phy_addr = (void *)(hShm + offset);
#elif defined(CONFIG_AMP_TEE_ENABLE)
#if defined (VPP_ENABLE_INTERNAL_MEM_MGR) && defined(VPP_ENABLE_MEMID_BY_VIRT_ADDR)
    //Note shm to physical is not 1:1 mapping, since shm are in virtual space
    phy_addr = (void *)(gMemhandle->start_addr_phy + (hShm - gMemhandle->start_addr_virt) + offset);
#else //!VPP_ENABLE_INTERNAL_MEM_MGR
    phy_addr = (void *)(hShm + offset);
#endif //!VPP_ENABLE_INTERNAL_MEM_MGR
#elif defined( __LINUX_KERNEL__ )
    dma_buf_getphysaddr((dma_buf_handle)hShm, offset, &phy_addr);
#else
    AMP_SHM_GetPhysicalAddress(hShm, offset, &phy_addr);
#endif
    return phy_addr;
}

#ifndef CONFIG_VPP_FASTLOGO_SUPPORT
#if defined(VPP_DIAGS)
extern void CleanDCacheRegion(int *adr, int size);
#endif
unsigned int VPP_SHM_CleanCache(VPP_SHM_HANDLE  hShm, unsigned int offset, unsigned int  size)
{
#if defined(VPP_DIAGS)
    int *addr;
    addr = (int *)(hShm+offset);
    CleanDCacheRegion(addr, size);
    return (MV_VPP_OK);
#elif defined(CONFIG_AMP_TEE_ENABLE)
    void *vir_addr;
    vir_addr = VPP_SHM_GetVirtualAddress(hShm, offset);
    return TEE_FlushCache(vir_addr, size);
#elif defined( __LINUX_KERNEL__ )
    /*not required for dma buffer as it is already non-cacheable*/
    return (MV_VPP_OK);
#else
    return AMP_SHM_CleanCache(hShm, offset, size);
#endif
}
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

/* Register write functions */
void VPP_REG_WORD32_WRITE(UINT32 addr, UINT32 val) {
#if defined(VPP_DIAGS)
    ((*((volatile unsigned int *)(addr))) = ((unsigned int)(val)));
#elif defined(CONFIG_AMP_TEE_ENABLE)
    ((*((volatile unsigned int *)TEE_TA_RegPhyToVirt(addr))) = ((unsigned int)(val)));
#elif defined( __LINUX_KERNEL__ )
    ((*((volatile unsigned int *)amp_memmap_phy_to_vir(addr))) = ((unsigned int)(val)));
#else
    ((*((volatile unsigned int *)devmem_phy_to_virt(addr))) = ((unsigned int)(val)));
#endif
    return;
}

void VPP_REG_WORD32_READ(UINT32 addr, UINT32 *pVal){
#if defined(VPP_DIAGS)
    (*(pVal) = (*((volatile unsigned int *)(addr))));
#elif defined(CONFIG_AMP_TEE_ENABLE)
    (*(pVal) = (*((volatile unsigned int *)TEE_TA_RegPhyToVirt(addr))));
#elif defined( __LINUX_KERNEL__ )
    (*(pVal) = (*((volatile unsigned int *)amp_memmap_phy_to_vir(addr))));
#else
    (*(pVal) = (*((volatile unsigned int *)devmem_phy_to_virt(addr))));
#endif
    return;
}

/* Debugging functions */
void VPP_DEBUG_Printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
#ifdef CONFIG_AMP_TEE_ENABLE
//TODO: TRUST ZONE
#elif defined( __LINUX_KERNEL__ )
    vprintk(fmt, args);
#else
    vprintf(fmt, args);
#endif
    va_end(args);
    return;
}

#if defined( __LINUX_KERNEL__ )
unsigned int rand(void){
//TODO: kernel
    return 0;
}
#endif

#ifdef TZ_3_0
UINT  VPP_SHM_Malloc(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm)
{
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    return VPP_OSAL_SHM_Malloc_Internal(eMemType, uiSize, uiAlign, phShm, 1);
#else
    return VPP_OSAL_SHM_Malloc_Internal(eMemType, uiSize, uiAlign, phShm, 0);
#endif
}
UINT  VPP_SHM_Malloc_Only(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm)
{
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    return VPP_OSAL_SHM_Malloc_Only_Internal(eMemType, uiSize, uiAlign, phShm, 1);
#else
    return VPP_OSAL_SHM_Malloc_Only_Internal(eMemType, uiSize, uiAlign, phShm, 0);
#endif
}

#endif

unsigned int  VPP_OSAL_SHM_Dynamic_Malloc(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_SHM_HANDLE  *phShm) {
    return VPP_SHM_Malloc(eMemType, uiSize, uiAlign, phShm);
}

unsigned int VPP_SHM_Free(VPP_SHM_HANDLE  hShm) {
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    return VPP_SHM_Free_Internal(hShm, 1);
#else
    return VPP_SHM_Free_Internal(hShm, 0);
#endif
}

unsigned int VPP_OSAL_SHM_Dynamic_Free(VPP_SHM_HANDLE  phShm) {
    VPP_SHM_Free(phShm);
    return 0;
}

TEE_Result VPP_OSAL_TEE_Callback(
    uint32_t        commandID,
    uint32_t        paramTypes,
    TEE_Param       params[4],
    uint32_t        cancellationRequestTimeout,
    uint32_t*       returnOrigin) {

    TEE_Result cb_res;
    TEE_Result prof_res;
    int unitNum = VPPDRV_PROF_MONITOR_UINT0_TEE_CB_PROF;

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    VPP_OBJ *vpp_obj = NULL;
    if (g_vpp) {
        vpp_obj = g_vpp->vpp_objs[0];
    }
#endif

    prof_res = VPPDRV_vppProfMonitorGen(vpp_obj, unitNum, VPPDRV_INTR_MONITOR_START, VPPDRV_INTR_MONITOR_NOSHOW);

#ifndef CONFIG_OPTEE
    cb_res = TEE_Callback(commandID, paramTypes, params, cancellationRequestTimeout, returnOrigin);
#else
    cb_res = TEE_SUCCESS;
#endif

    prof_res = VPPDRV_vppProfMonitorGen(vpp_obj, unitNum, VPPDRV_INTR_MONITOR_STOP, VPPDRV_INTR_MONITOR_NOSHOW);
    if (prof_res) {
        VPPAPI_DBG_ERROR("%s:%d:VPPDBG:TAVPP: unit-%d: commandID=%d, paramTypes=0x%08x, "
        "{0x%08x,0x%08x}, {0x%08x,0x%08x}, "
        "{0x%08x,0x%08x}, {0x%08x,0x%08x}\n", __func__, __LINE__, unitNum, commandID,
            paramTypes,
            params[0].value.a,
            params[0].value.b,
            params[1].value.a,
            params[1].value.b,
            params[2].value.a,
            params[2].value.b,
            params[3].value.a,
            params[3].value.b);
    }

    return cb_res;
}

