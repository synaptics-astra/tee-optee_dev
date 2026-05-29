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

////////////////////////////////////////////////////////////////////////////////
//! \file tsp_ta.c
//!
//! \brief TSP TA Implementation
//!
//!
//!     Version     Date                    Author
//!     V 1.00,     Aug 12 2016,            wangzl
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tsp_base_config.h"
#include <string.h>
#ifndef CONFIG_OPTEE
#include "tee_internal_api.h"
#include "tee_internal_core_mem_api.h"
#else
#include "tee_api_syna.h"
#endif
#include "tsp_shared.h"
#include "tsp_mem_map.h"
#include "tsp_cmd.h"

#define LOG_TAG "TSP_TA"
#include "tsp_config_log.h"
#include "ta_log.h"

#ifdef TZ_3_0
static uintptr_t tsp_map_addr = 0;
#endif

#define ALIGN4K(x)      ((((size_t)(x)) + 0xfff) & (~0xfff))

#define VALIDATE_PHY_BUFFER(phyAddr,size) \
    do{ \
      if(phyAddr == 0 || size == 0 || size >  ((uintptr_t)(-1) - (uintptr_t)(phyAddr))){ \
           TA_LOGE( "Not valid phy mem, start=0x%lx size=0x%d\n",(uintptr_t)phyAddr, size); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
    }while(0)

TEE_Result tspmem_create_iomap(void)
{
    TEE_Result res = TEE_SUCCESS;

#ifdef TZ_3_0
    void *va;
    if (tsp_map_addr) {
        TA_LOGH( "tspmem_create_iomap already created!\n");
        return TEE_SUCCESS;
    }

    res = TEE_Map_Register((paddr_t)TSP_FIGO_REG_BASE, TSP_FIGO_REG_SIZE*TSP_MAX_FIGO_NUM, 0, 0, &va);
    if(res == TEE_SUCCESS){
        tsp_map_addr = (uintptr_t)va;
    }
    else
        TA_LOGF( "tspmem_create_iomap fail pa=0x%x,res=%d\n",TSP_FIGO_REG_BASE, res);
#endif

    return res;
}


uintptr_t tspmem_virt_to_phy(uintptr_t uVirtAddr)
{
#ifdef TZ_3_0
    if (tsp_map_addr &&
        (uVirtAddr >= tsp_map_addr) &&
        (uVirtAddr < (tsp_map_addr + TSP_FIGO_REG_SIZE*TSP_MAX_FIGO_NUM)))
    {
        return (TSP_FIGO_REG_BASE + uVirtAddr -
                tsp_map_addr);
    }
    TA_LOGE("invalid virtual address:%lx\n", uVirtAddr);
    return 0;
#else
    return uVirtAddr;
#endif
}

uintptr_t tspmem_phy_to_virt(uintptr_t uPhyAddr)
{
#ifdef TZ_3_0
    if (tsp_map_addr &&
        (uPhyAddr >= TSP_FIGO_REG_BASE) &&
        (uPhyAddr <(TSP_FIGO_REG_BASE + TSP_FIGO_REG_SIZE*TSP_MAX_FIGO_NUM))) {
        return (uPhyAddr - TSP_FIGO_REG_BASE +
                tsp_map_addr);
    }
    TA_LOGE("invalid physical address:%lx\n", uPhyAddr);
    return 0;
#else
    return uPhyAddr;
#endif
}

TEE_Result tspbuf_create_with_pa(TA_TSP_BUFF_T *pTspBuf,
                                            uintptr_t phyAddr,
                                            uint32_t bufSize,
                                            bool bRegister)
{
    TEE_Result res = TEE_SUCCESS;
    paddr_t phyStartAddr;
    uint32_t offset;
    void  *virStartAddr;
    size_t  mapSize;
    bool bDoReister = false;

    VALIDATE_PHY_BUFFER(phyAddr, bufSize);

    res = TEE_GetMemid_By_Phys(&pTspBuf->memID, phyAddr, bufSize);
    if (TEE_SUCCESS != res) {
        if(bRegister) {
            phyStartAddr = (paddr_t)(phyAddr & (~0xfff));
            offset = phyAddr & 0xfff;
            mapSize = ALIGN4K(bufSize + offset);
            res = TEE_Register_Memory(phyStartAddr, mapSize, &pTspBuf->memID);
            if (TEE_SUCCESS != res) {
                TA_LOGE("TEE_Register_Memory failed for phyAddr: 0x%lx, size: 0x%x, res=0x%x\n",
                    phyAddr, bufSize, res);
                return res;
            }else {
                bDoReister = true;
            }
        }else {
            TA_LOGE("TEE_GetMemid_By_Phys failed for phyAddr: 0x%lx, size: 0x%x, res=0x%x\n",
                phyAddr, bufSize, res);
            return res;
        }
    }

    if(!bDoReister) {
        res = TEE_GetPhys_By_Memid(pTspBuf->memID, &phyStartAddr);
        if (TEE_SUCCESS != res) {
            TA_LOGE("TEE_GetPhys_By_Memid failed for memID: 0x%x, res=0x%x\n",
                pTspBuf->memID, res);
            return res;
        }
    }


    res = TEE_Map_Memory(pTspBuf->memID, &virStartAddr, &mapSize);
    if (TEE_SUCCESS != res) {
        TA_LOGE("TEE_Map_Memory failed for memID 0x%x, res=0x%x\n", pTspBuf->memID, res);
        if(bDoReister) {
            TEE_Unregister_Memory(pTspBuf->memID);
        }
        return res;
    }

    pTspBuf->phyAddr = (void *)phyAddr;
    pTspBuf->virAddr = (uint8_t*)virStartAddr + (phyAddr - (uintptr_t)phyStartAddr);
    pTspBuf->uBufSize = bufSize;
    pTspBuf->eMemType = TSP_MEM_PARAM_PHY_ADDR;
    pTspBuf->bRegister = bDoReister;
    return TEE_SUCCESS;
}

TEE_Result tspbuf_create_with_memid(TA_TSP_BUFF_T *pTspBuf,
                                                  uint32_t memID,
                                                  uint32_t offset,
                                                  uint32_t bufSize)
{
    TEE_Result res = TEE_SUCCESS;
    size_t toSize = 0;
    paddr_t phyStartAddr;
    void* virStartAddr = NULL;

    res = TEE_GetPhys_By_Memid(memID, &phyStartAddr);
    if (res != TEE_SUCCESS) {
        TA_LOGE("TEE_GetPhys_By_Memid(0x%x) failed, ret=0x%x\n", memID, res);
        return res;
    }

    res = TEE_Map_Memory(memID, &virStartAddr, &toSize);
    if(res != TEE_SUCCESS){
        TA_LOGE("TEE_Map_Memory(0x%x) failed, ret=0x%x\n", memID, res);
        return res;
    }

    if(toSize < (offset + bufSize)) {
        TA_LOGE("error: MapSize(0x%lx) < offset(0x%x)+ bufSize(0x%x)\n",
            toSize, offset, bufSize);
        TEE_Unmap_Memory(memID);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    pTspBuf->memID = memID;
    pTspBuf->phyAddr = (void*)(phyStartAddr + offset);
    pTspBuf->virAddr = (uint8_t*)virStartAddr + offset;
    pTspBuf->uBufSize = bufSize;
    pTspBuf->eMemType = TSP_MEM_PARAM_SHM_ID;
    return res;
}

TEE_Result tspbuf_create_with_va(TA_TSP_BUFF_T *pTspBuf,
                                            uintptr_t virAddr,
                                            uint32_t bufSize)
{
    //for memref input, not need physical address and memid
    pTspBuf->virAddr = (void *)virAddr;
    pTspBuf->uBufSize = bufSize;
    pTspBuf->eMemType = TSP_MEM_PARAM_MEMREF;
    return TEE_SUCCESS;
}

TEE_Result tspbuf_create_alloc(TA_TSP_BUFF_T *pTspBuf,
                                            uint32_t bufSize)
{
    TEE_Result res = TEE_SUCCESS;
    void  *pVirAddr;
    paddr_t pPhyAddr;
    uint32_t memID = 0;

#ifndef CONFIG_OPTEE
    res = TEE_AllocateSharedMemory2(bufSize, &memID, &pVirAddr);
    if (res != TEE_SUCCESS) {
        TA_LOGE("TEE_AllocateSharedMemory2 failed, ret=0x%x\n", res);
        return res;
    }
    res = TEE_GetPhys_By_Memid(memID, &pPhyAddr);
    if (res != TEE_SUCCESS) {
        TA_LOGE("TEE_GetPhys_By_Memid(0x%x) failed, ret=0x%x\n", memID, res);
        TEE_Unmap_Memory(memID);
        return res;
    }
#else
    // memid will not used, this buffer is used locallly at TA
    pVirAddr = TEE_Malloc(bufSize, 0);
    if (pVirAddr == NULL) {
        TA_LOGE("TEE_Malloc failed, ret=0x%x\n", res);
        return TEE_ERROR_OUT_OF_MEMORY;
    }
    pPhyAddr = TEE_VirtToPhys(pVirAddr);
#endif
    memset(pVirAddr, 0, bufSize);
    pTspBuf->memID = memID;
    pTspBuf->phyAddr = (void*)pPhyAddr;
    pTspBuf->virAddr = pVirAddr;
    pTspBuf->uBufSize = bufSize;
#ifndef CONFIG_OPTEE
    pTspBuf->eMemType = TSP_MEM_PARAM_SHM_ID;
#else
    pTspBuf->eMemType = TSP_MEM_PARAM_HEAP;
#endif
    return TEE_SUCCESS;
}
void tspbuf_release(TA_TSP_BUFF_T *pTspBuf)
{
    //use uVirAddr instead of memID, since 0 is valid for memID
    if(pTspBuf->virAddr && !pTspBuf->bDup && pTspBuf->eMemType != TSP_MEM_PARAM_MEMREF
        && pTspBuf->eMemType != TSP_MEM_PARAM_HEAP) {
        TEE_Unmap_Memory(pTspBuf->memID);
        if(pTspBuf->bRegister) {
            TEE_Unregister_Memory(pTspBuf->memID);
        }
    }

    if (pTspBuf->eMemType == TSP_MEM_PARAM_HEAP) {
        if(pTspBuf->virAddr) {
            TEE_Free(pTspBuf->virAddr);
        }
    }
    pTspBuf->virAddr = 0;
}

TEE_Result tspbuf_create_twobuf_with_pa(uintptr_t phyAddr1, uint32_t bufSize1,
                                                uintptr_t phyAddr2, uint32_t bufSize2,
                                                TA_TSP_BUFF_T *pTspBuf1,
                                                TA_TSP_BUFF_T *pTspBuf2)
{
    TEE_Result res = TEE_SUCCESS;
    paddr_t  phyStartAddr2;
    size_t  mapSize;
    void  *virStartAddr2;

    //tspbuf_create_with_pa will validate phyAddr1
    VALIDATE_PHY_BUFFER(phyAddr2, bufSize2);

    res = tspbuf_create_with_pa(pTspBuf1, phyAddr1, bufSize1, false);
    if (TEE_SUCCESS != res) {
        TA_LOGE("Ttspbuf_create_with_pa failed for phyAddr1: 0x%lx, size: 0x%x, res=0x%x\n",
            phyAddr1, bufSize1, res);
        return res;
    }

    res = TEE_GetMemid_By_Phys(&pTspBuf2->memID, phyAddr2, bufSize2);
    if (TEE_SUCCESS != res) {
        TA_LOGE("TEE_GetMemid_By_Phys failed for phyAddr2: 0x%lx, size: 0x%x, res=0x%x\n",
            phyAddr2, bufSize2, res);
        goto error;
    }

    if(pTspBuf1->memID == pTspBuf2->memID) {
        pTspBuf2->phyAddr = (void *)phyAddr2;
        if(phyAddr2 > phyAddr1) {
            pTspBuf2->virAddr = (uint8_t *)(pTspBuf1->virAddr) + (phyAddr2 - phyAddr1);
        }else {
            pTspBuf2->virAddr = (uint8_t *)(pTspBuf1->virAddr) - (phyAddr1 - phyAddr2);
        }
        pTspBuf2->uBufSize = bufSize2;
        pTspBuf2->eMemType = TSP_MEM_PARAM_PHY_ADDR;
        pTspBuf2->bDup = true;
    }else {
        res = TEE_GetPhys_By_Memid(pTspBuf2->memID, &phyStartAddr2);
        if (TEE_SUCCESS != res) {
            TA_LOGE("TEE_GetPhys_By_Memid failed for memID2: 0x%x, res=0x%x\n",
                pTspBuf1->memID, res);
            goto error;
        }

        res = TEE_Map_Memory(pTspBuf2->memID, &virStartAddr2, &mapSize);
        if (TEE_SUCCESS != res) {
            TA_LOGE("TEE_Map_Memory failed for memID2 0x%x, res=0x%x\n", pTspBuf2->memID, res);
            goto error;
        }

        pTspBuf2->phyAddr = (void *)(uintptr_t)phyAddr2;
        pTspBuf2->virAddr = (uint8_t*)virStartAddr2 + (phyAddr2 - (uintptr_t)phyStartAddr2);
        pTspBuf2->uBufSize = bufSize2;
        pTspBuf2->eMemType = TSP_MEM_PARAM_PHY_ADDR;
    }

    return TEE_SUCCESS;

error:
    tspbuf_release(pTspBuf1);
    return res;
}

