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
//! \file ptm_ta.c
//!
//! \brief PTM TA Implementation
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    JUL 2019,                Hao Chen
//!     V 1.01,    AUG 2019,                zhwan
//!     V 2.00,    APR 2020,                zhwan
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "PTM_TA"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
#include "config_log.h"
#include "tee_internal_api.h"
#include "ta_log.h"
#include "ptm_cmd.h"
#include "ptm_ta.h"
#include "mem_region_userdata.h"

//#define ENABLE_PTM_DEBUG
//#define FORCE_CHECK_OVERLAP  //when set_mpt, tee api will also check overlap
#define PAGE_SIZE   0x1000
#define ZONE_MAX_NUM    16

// Physical Contiguous Memory entryPairs should equal 1.
#define IS_CONTINGUOUS(entryPairs)    ((entryPairs == 1))

typedef struct __ptm_zone {
    uint32_t    u_StartAddr;
    uint32_t    u_EndAddr;
} PTM_Zone;

 typedef struct __ptm_zone_info {
    PTM_Zone  zone[ZONE_MAX_NUM];
    uint32_t  active_zone_cnt;
 } PTM_ZoneInfo;

static PTM_ZoneInfo       gPtmZoneinfo;
static PTM_MemoryManager  gPtmMgr;

static TEE_Result internal_getZoneMemInfo(PTM_ZoneInfo* pPtmMeminfo)
{
    uint32_t regionCnt = 0;
    uint32_t zoneCnt = 0;
    TEE_MemRegion* regionBuf = NULL;

    regionCnt = TEE_GetMemRegionCount(0, 0);
    regionBuf = (TEE_MemRegion*)TEE_Malloc(regionCnt*sizeof(TEE_MemRegion),TEE_MALLOC_FLAG_DEFAULT);
    if(!regionBuf){
        TA_LOGE( "Faild to malloc buffer for mem region:%p\n",regionBuf);
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    TEE_GetMemRegionList(regionBuf, regionCnt, 0, 0);

    /*filter zone mem region*/
    while(regionCnt--){
        TEE_MemRegion* mr = &regionBuf[regionCnt];
        TA_LOGH( "%s      0x%x         0x%x         0x%x         0x%x\n",
                mr->name,
                mr->base,
                mr->size,
                TEE_MR_IS_SECURE(mr),
                TEE_MR_RSV_ZONE(mr));

        if (zoneCnt >= ZONE_MAX_NUM) {
            TA_LOGE( "Zone Count beyond maximum size\n");
            return TEE_ERROR_OVERFLOW;
        }

        // CMA
        if (TEE_MR_USER(mr) == TEE_MR_USER_ION && TEE_MR_ION_FOR_NONSECURE(mr)
            && TEE_MR_ION_FOR_CACHEABLE(mr)
            && !TEE_MR_RSV_ZONE(mr)) {
            pPtmMeminfo->zone[zoneCnt].u_StartAddr = mr->base;
            pPtmMeminfo->zone[zoneCnt].u_EndAddr = mr->base + (mr->size - 1);
            zoneCnt++;
        }

        // Secure
        if (TEE_MR_USER(mr) == TEE_MR_USER_ION && TEE_MR_ION_FOR_SECURE(mr)
            && !TEE_MR_RSV_ZONE(mr)) {
            pPtmMeminfo->zone[zoneCnt].u_StartAddr = mr->base;
            pPtmMeminfo->zone[zoneCnt].u_EndAddr = mr->base + (mr->size - 1);
            zoneCnt++;
        }

        //pt zone
        if (TEE_MR_USER(mr) == TEE_MR_USER_PT) {
            gPtmMgr.poolPhyAddr = (paddr_t)mr->base;
            gPtmMgr.poolSize = mr->size;
        }
    }
    pPtmMeminfo->active_zone_cnt = zoneCnt;
    TEE_Free(regionBuf);

    return TEE_SUCCESS;
}

static TEE_Result internal_create_PT_mgr(void)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t chunkSize = 0;
    uint32_t chunkNum = 0;
    size_t tmpSize;

    memset(&gPtmMgr, 0, sizeof(gPtmMgr));
    res = internal_getZoneMemInfo(&gPtmZoneinfo);
    if (res != TEE_SUCCESS) {
        TA_LOGF("failed get zone mem info\n");
        return res;
    }
    //check pt zone in mem layout
    if ((gPtmMgr.poolPhyAddr == 0) || (gPtmMgr.poolSize == 0)) {
        TA_LOGF("failed to parser pt base %"PRIxPTR" 0x%x\n", gPtmMgr.poolPhyAddr,
                gPtmMgr.poolSize);
        return TEE_ERROR_ITEM_NOT_FOUND;
    }

    chunkSize = CHUNK_SIZE;
    chunkNum = gPtmMgr.poolSize / CHUNK_SIZE; // chunkSize * chunkNum = PTSize

    res = TEE_Register_Memory(gPtmMgr.poolPhyAddr, gPtmMgr.poolSize,
                            &gPtmMgr.poolHdlShmId);
    if (res != TEE_SUCCESS) {
        TA_LOGF("Faild to register memid for pt pool %"PRIxPTR", 0x%x, 0x%x\n",
                gPtmMgr.poolPhyAddr, gPtmMgr.poolSize, res);
        return res;
    }

    res = TEE_Map_Memory(gPtmMgr.poolHdlShmId, &gPtmMgr.poolVirAddr, &tmpSize);
    if (res != TEE_SUCCESS) {
        TA_LOGF("Failed to mmap pt pool 0x%x, 0x%x\n", gPtmMgr.poolHdlShmId, res);
        goto END_FAIL;
    }

    memset(gPtmMgr.poolVirAddr, INVALID_ADDR, chunkSize * chunkNum); //init to invalid values

    res = POOL_Create(POOL_TYPE_PAGE_TABLE, chunkSize, chunkNum, (uintptr_t)gPtmMgr.poolPhyAddr,
                      gPtmMgr.poolVirAddr, &(gPtmMgr.poolHandle));
    if(res != POOL_SUCCESS) {
        TA_LOGF( "Faild to create mem pool, error: 0x%x\n", res);
        goto END_FAIL;
    }

    gPtmMgr.pPTList = std_list_ctor(NULL, NULL);
    if(gPtmMgr.pPTList == NULL){
        TA_LOGF( "Faild to create pt list, malloc failed\n");
        res = TEE_ERROR_OUT_OF_MEMORY;
        goto END_FAIL;
    }
    gPtmMgr.chunkSize = chunkSize;
    gPtmMgr.chunkNum = chunkNum;
    gPtmMgr.bInit = TRUE;
    return res;

END_FAIL:
    TA_LOGF( "Faild to create PTM server\n");
    if(gPtmMgr.poolHandle) {
        POOL_Destroy(gPtmMgr.poolHandle);
    }
    if (gPtmMgr.poolVirAddr) {
        TEE_Unmap_Memory(gPtmMgr.poolHdlShmId);
    }
    if (gPtmMgr.poolHdlShmId) {
        TEE_Unregister_Memory(gPtmMgr.poolHdlShmId);
    }
    if(gPtmMgr.pPTList) {
        std_list_dtor(gPtmMgr.pPTList);
    }
    return res;
}

static TEE_Result internal_check_PT_mgr(void)
{
    if(gPtmMgr.bInit)
        return TEE_SUCCESS;
    else
        return internal_create_PT_mgr();
}

static PTM_AllocUnit* internal_alloc_PT_unit(uint32_t nents)
{
    PTM_AllocUnit *pPtUnit = NULL;

    pPtUnit = (PTM_AllocUnit *)TEE_Malloc(sizeof(PTM_AllocUnit),TEE_MALLOC_FLAG_DEFAULT);
    if(!pPtUnit){
        TA_LOGF( "Faild to malloc mem for alloc unit\n");
        return NULL;
    }
    TEE_MemFill(pPtUnit, 0, sizeof(PTM_AllocUnit));

    pPtUnit->sgTable = (scatter_list_t *)TEE_Malloc(SIZE_OF_SCATTERLIST(nents), \
                            TEE_MALLOC_FLAG_DEFAULT);
    if(!pPtUnit->sgTable){
        TA_LOGF( "Faild to malloc mem for scatter list\n");
        TEE_Free(pPtUnit);
        return NULL;
    }
    pPtUnit->sgTable->num = nents;

    pPtUnit->userList = std_list_ctor(NULL, NULL);
    if(!pPtUnit->userList){
        TA_LOGF( "Faild to create pt user list, malloc failed\n");
        TEE_Free(pPtUnit->sgTable);
        TEE_Free(pPtUnit);
        return NULL;
    }

    return pPtUnit;
}

static void internal_free_PT_unit(PTM_AllocUnit* pPtUnit)
{
    if(!pPtUnit)
        return;
    if(pPtUnit->sgTable) {
        TEE_Free(pPtUnit->sgTable);
    }
    if(pPtUnit->userList) {
        std_list_dtor(pPtUnit->userList);
    }
    TEE_Free(pPtUnit);
}

static PT_LIST_NODE* internal_search_PT_node(PT_LIST *pPTList, uint32_t ptPhyAddr)
{
    PTM_AllocUnit *pPtUnit = NULL;
    PT_LIST_NODE* pPtNode = NULL;

    while((std_list_iter(pPTList, &pPtNode) == TEE_SUCCESS)
        && (pPtNode != NULL))
    {
        pPtUnit =(PTM_AllocUnit *)pPtNode->pData;
        if(pPtUnit->ptPhyAddr == ptPhyAddr) {
            return pPtNode;
        }
    }
    return NULL;
}

static BOOL internal_check_buffer_overlap(PT_LIST *pPTList,
                                                 uint64_t *pAddr,
                                                 uint32_t nents)
{
#ifdef FORCE_CHECK_OVERLAP
    PT_LIST_NODE* pPtNode = NULL;

    while((std_list_iter(pPTList, &pPtNode) == TEE_SUCCESS)
        && (pPtNode != NULL))
    {
        scatter_list_t *SGTable =((PTM_AllocUnit *)pPtNode->pData)->sgTable;
        for(uint32_t i = 0; i < nents; i++){
            uint32_t basePhyAddr = (uint32_t)pAddr[2 * i];
            uint32_t pageNum = (uint32_t)pAddr[2 * i + 1];
            for(uint32_t j = 0; j < SGTable->num; j++) {
                if(basePhyAddr > (SGTable->s_item[j].addr + (SGTable->s_item[j].npage - 1) * 0x1000 )) {

                }else if(SGTable->s_item[j].addr > (basePhyAddr + (pageNum - 1) * 0x1000)) {

                }else {
                    TA_LOGH("find buffer overlap\n");
                    return TRUE;
                }
            }
        }
    }
#endif
    return FALSE;
}

static BOOL internal_is_zone(PTM_ZoneInfo* pPtmMeminfo, uint64_t *pAddr)
{
    // linux kernel is 64bit, TEE is 32bit. So pAddr passed in is uint64_t while the
    // pPtmMeminfo's range address got from TEE is uint32_t. Since the memory size is not beyond
    // 0xFFFFFFFF. So this OK to convert uint64_t to uint32_t for now.
    uint32_t basePhyAddr =  (uint32_t)pAddr[0];
    uint32_t pageNum = (uint32_t)pAddr[1];

    for (uint32_t zoneCnt = 0; zoneCnt < pPtmMeminfo->active_zone_cnt; ++zoneCnt) {
        if ((pPtmMeminfo->zone[zoneCnt].u_StartAddr <= basePhyAddr)
            && (pPtmMeminfo->zone[zoneCnt].u_EndAddr >= (basePhyAddr + (pageNum * PAGE_SIZE))))
                return TRUE;
    }

    return FALSE;
}

static TEE_Result internal_check_callFromTA(void)
{
    TEE_Result res = TEE_SUCCESS;
    TEE_Identity identity;
    TEE_UUID uuidZero = {0};

    res = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if (res != TEE_SUCCESS) {
        TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", res);
    } else {
        if (!memcmp(&identity.uuid, &uuidZero, sizeof(TEE_UUID))) {
            TA_LOGE("fail: call from CA, should from TA\n");
            res = TEE_ERROR_ACCESS_DENIED;
        } else {
            res = TEE_SUCCESS;
        }
    }
    return res;
}

//bRef:  1: ref,  0: unref
static TEE_Result internal_update_callerTATrace(PTM_AllocUnit *pPtUnit, BOOL bRef)
{
    TEE_Result res = TEE_SUCCESS;
    TEE_Identity identity;

    res = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if (res != TEE_SUCCESS) {
        TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", res);
    } else if(pPtUnit) {
        struct ptm_ta_user *user = NULL;
        std_list_node *pUserNode = NULL;
        BOOL bCallerExist = FALSE;

        while((std_list_iter(pPtUnit->userList, &pUserNode) == TEE_SUCCESS)
            && (pUserNode != NULL))
        {
            user =(struct ptm_ta_user *)pUserNode->pData;
            if(!memcmp(&user->UserUuid, &identity.uuid, sizeof(TEE_UUID))) {
                bCallerExist = TRUE;
                break;
            }
        }

        if(bRef){
            if(bCallerExist) {
                user->refCount++;
            }else {
                user = (struct ptm_ta_user *)TEE_Malloc(sizeof(struct ptm_ta_user),
                        TEE_MALLOC_FLAG_DEFAULT);
                if(!user){
                    TA_LOGF( "Faild to malloc mem for alloc user of pt\n");
                    res = TEE_ERROR_OUT_OF_MEMORY;
                }else {
                    memcpy(&user->UserUuid, &identity.uuid, sizeof(TEE_UUID));
                    user->refCount = 1;
                    std_list_push_back(pPtUnit->userList, user);
                }
            }
        }else {
            if(bCallerExist) {
                user->refCount--;
                if(user->refCount == 0){
                    std_list_remove(pPtUnit->userList, pUserNode);
                    TEE_Free(user);
                }
            }else {
                TA_LOGE( "Fail: this TA caller 0x%x has no refcount of this PT!\n", identity.uuid.timeLow);
                res = TEE_ERROR_ITEM_NOT_FOUND;
            }
        }
    }

    return res;
}

static void internal_debug_callerTATrace(PTM_AllocUnit *pPtUnit)
{
    struct ptm_ta_user *user = NULL;
    std_list_node *pUserNode = NULL;
    int32_t i = 0;

    while((std_list_iter(pPtUnit->userList, &pUserNode) == TEE_SUCCESS)
        && (pUserNode != NULL))
    {
        user =(struct ptm_ta_user *)pUserNode->pData;
        TA_LOGE("exist user(%d): UUID(0x%x 0x%x 0x%x last 0x%x) refcount(0x%x)\n", i++,
                user->UserUuid.timeLow, user->UserUuid.timeMid,
                user->UserUuid.timeHiAndVersion, user->UserUuid.clockSeqAndNode[7], user->refCount);
    }

}

#define TA_DEBUG_UUID   {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x23}}
static TEE_Result internal_check_dump(PTM_AllocUnit* pPtUnit)
{
    TEE_Result res = TEE_SUCCESS;
    TEE_Identity identity;
    TEE_UUID debugTaUuid = TA_DEBUG_UUID;

    res = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if (res != TEE_SUCCESS) {
        TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", res);
    } else {
        if(pPtUnit->mptRuleIndex) { //secure memory
            if (memcmp(&identity.uuid, &debugTaUuid, sizeof(TEE_UUID))) {
                TA_LOGE("fail: secure memory dump only support for debug TA\n");
                res = TEE_ERROR_SECURITY;
            }
        }
    }
    return res;
}
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    TA_LOGH("TA_CreateEntryPoint enter\n");
    return internal_create_PT_mgr();
}


void TA_EXPORT TA_DestroyEntryPoint(void)
{
    if(gPtmMgr.bInit) {
        POOL_Destroy(gPtmMgr.poolHandle);
        TEE_Unmap_Memory(gPtmMgr.poolHdlShmId);
        TEE_Unregister_Memory(gPtmMgr.poolHdlShmId);
        std_list_dtor(gPtmMgr.pPTList);
        gPtmMgr.bInit = FALSE;
    }
    return;
}


TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(
    uint32_t        paramTypes,
    TEE_Param        params[4],
    void**            sessionContext)
{
    PTM_SessionContext* pPtmCtx = NULL;
    TEE_Result res = TEE_SUCCESS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    pPtmCtx = (PTM_SessionContext*)TEE_Malloc(sizeof(PTM_SessionContext),TEE_MALLOC_FLAG_DEFAULT);
    if(!pPtmCtx){
        TA_LOGF( "Faild to malloc mem for TA CTX\n");
        return TEE_ERROR_OUT_OF_MEMORY;
    }
    pPtmCtx->pPtmMgr = &gPtmMgr;

    *sessionContext = (void *)pPtmCtx;

    return TEE_SUCCESS;


}

void TA_EXPORT TA_CloseSessionEntryPoint(void*  sessionContext)
{
    PTM_SessionContext* pPtmCtx = (PTM_SessionContext*)sessionContext;
    if(pPtmCtx) {
        TEE_Free(pPtmCtx);
    }
}

static void internal_statistics(PTM_MemoryManager* pPtmMgr)
{
    POOL_Property prop;

    if(pPtmMgr && pPtmMgr->poolHandle){
        if(POOL_GetProperty(pPtmMgr->poolHandle, &prop) == POOL_SUCCESS) {
            TA_LOGM("------ Mem pool statistics -----\n");
            TA_LOGM("basePhyAddr: %"PRIxPTR", baseVirAddr: %p\n", prop.basePhyAddr,prop.baseVirAddr);
            TA_LOGM("poolType: %d poolSize: 0x%x ChunkSize: 0x%x\n",
                    prop.poolType, prop.poolSize, prop.chunkSize);
            TA_LOGM("avail: 0x%x(%2d) allocNr: 0x%x maxAvailBlock: 0x%x\n",
                    prop.avail, (prop.avail*100)/prop.poolSize, prop.allocNr, prop.maxAvailBlock);
            TA_LOGM("--------------------------------\n");
        }
    }
}

static TEE_Result PTMTA_AllocateMem(void* sessionContext,
                                         uint32_t  paramTypes,
                                         TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t entryPairs, entryNum = 0;
    uint32_t entryNum_y, entryNum_uv, entryNum_alloc, entryNum_y_align;
    uint64_t *bufferAddr = NULL;
    uint32_t i, j, count = 0;
    BUF_Addr memPoolAddr;
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    uint32_t *memPoolCursor = NULL;
    struct ptm_construct_param *constructParam = NULL;
    PTM_AllocUnit *pPtUnit = NULL;
    bool    is_zone = FALSE;
    uint32_t mem_id =0;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_MEMREF_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_MEMREF_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    //get physical mem pages
    bufferAddr = (uint64_t *)params[0].memref.buffer;
    entryPairs = (uint32_t)params[0].memref.size/2/sizeof(uint64_t);
    constructParam = (struct ptm_construct_param *)params[1].memref.buffer;

    if(internal_check_buffer_overlap(pPtmMgr->pPTList, bufferAddr, entryPairs)) {
        TA_LOGE("error: buffer overlay found!\n");
        return TEE_ERROR_BAD_PARAMETERS;
    }

    if (IS_CONTINGUOUS(entryPairs)) {
        is_zone = internal_is_zone(&gPtmZoneinfo, bufferAddr);
    }

    pPtUnit = internal_alloc_PT_unit(entryPairs);
    if(pPtUnit == NULL) {
        return TEE_ERROR_OUT_OF_MEMORY;
    }
    TEE_MemMove(&pPtUnit->allocParam, constructParam, sizeof(struct ptm_construct_param ));

    for(i = 0; i < entryPairs; i++) {
        entryNum += (uint32_t)bufferAddr[2*i + 1];
        pPtUnit->sgTable->s_item[i].addr = (uint32_t)bufferAddr[2*i];
        pPtUnit->sgTable->s_item[i].npage = (uint32_t)bufferAddr[2*i + 1];
        TA_LOGI("addr 0x%x, page num 0x%x\n", (uint32_t)bufferAddr[2*i],(uint32_t)bufferAddr[2*i + 1]);
    }

    TA_LOGH("allocate mem, bufferAddr %p, entryPairs 0x%x, entryNum 0x%x, secureFlag %d\n",
            bufferAddr, entryPairs, entryNum, constructParam->ruleIndex);

    if(constructParam->param.fb_type == SHM_MMU_GENERIC){
        entryNum_alloc = entryNum;
        //use fake value to set entryNum_y, entryNum_uv
        entryNum_y_align = entryNum_y = (entryNum+1)/2;
        entryNum_uv = entryNum - entryNum_y;
    }else if(constructParam->param.fb_type == SHM_MMU_UV_ALIGN) {
        entryNum_y = SIZE2PAGE(constructParam->param.uva_param.y_size);
        if(entryNum_y >= entryNum){
            TA_LOGE("bad params, entryNum 0x%x, y_size 0x%x\n",
                    entryNum, constructParam->param.uva_param.y_size);
            res = TEE_ERROR_BAD_PARAMETERS;
            goto error;
        }
        entryNum_uv = entryNum - entryNum_y;
        entryNum_y_align = ALIGN4K(entryNum_y<<2) >> 2;
        entryNum_alloc = entryNum_y_align + entryNum_uv;
        TA_LOGI("entryNum 0x%x, entryNum_y 0x%x, entryNum_uv 0x%x, entryNum_y_align 0x%x\n",
                entryNum, entryNum_y, entryNum_uv, entryNum_y_align);
    }else {
        TA_LOGE("unknown fb_type 0x%x\n",constructParam->param.fb_type);
        res =  TEE_ERROR_BAD_PARAMETERS;
        goto error;
    }

    if(constructParam->ruleIndex > 0xFF) {
        TA_LOGE("invalid rule index %d\n",constructParam->ruleIndex);
        res =  TEE_ERROR_BAD_PARAMETERS;
        goto error;
    }

    pPtUnit->mptRuleIndex = constructParam->ruleIndex;
    if (!is_zone) {
        pPtUnit->pReservation = TEE_CreateReservation(pPtUnit->sgTable, pPtUnit->mptRuleIndex);
        if(pPtUnit->pReservation == NULL){
            TA_LOGE("fail to Create Reservation(mptRuleIndex = %d)\n", pPtUnit->mptRuleIndex);
            goto error;
        }
        res = TEE_ReserveSecurePages(pPtUnit->pReservation);
        if(res != TEE_SUCCESS){
            TA_LOGE("fail to Reserve SecurePages, ret: 0x%x\n", res);
            goto error;
        }else {
            TA_LOGM("set mpt to rule %d\n", pPtUnit->mptRuleIndex);
        }
        mem_id = pPtUnit->pReservation->mem_id;
    } else {
        res = TEE_Register_Memory((paddr_t)(uintptr_t)bufferAddr[0],
                                   (size_t)bufferAddr[1]*PAGE_SIZE, &mem_id);
        if(res != TEE_SUCCESS){
            TA_LOGE("fail to register memory, ret: 0x%x\n", res);
            goto error;
        }else {
            TA_LOGM("mem id 0x%x registered\n", mem_id);
        }
    }

    //ask pool mgr for empty page entries
    res = POOL_Allocate(pPtmMgr->poolHandle, entryNum_alloc<<2, &memPoolAddr);
    if(res != POOL_SUCCESS) {
        TA_LOGE("fail to allocate mem from pool, ret: 0x%x\n", res);
        goto error;
    }

    //fill the physical mem pages' info into empty page entries from pool mgr
    //page data[0]: 1 is valid;
    //page_data[8:1]: security attribute
    //page_data[31:12] page physical address
    memPoolCursor = (uint32_t *)memPoolAddr.virAddr;
    count = 0;
    for(i = 0; i < entryPairs; i++) {
        uint32_t basePhyAddr = (uint32_t)bufferAddr[2 * i];
        uint32_t pageNum = (uint32_t)bufferAddr[2 * i + 1];
        TA_LOGI("basepagePhyAddr 0x%x : pageNum 0x%x\n", basePhyAddr, pageNum);
        for(j = 0; j < pageNum; j++) {
            *memPoolCursor = (basePhyAddr + j * 0x1000) & 0xFFFFF000; //not must to & 0xFFFFF000
            *memPoolCursor |= ((constructParam->ruleIndex << 1) | 0x1);

            memPoolCursor = (uint32_t*) ((uintptr_t) memPoolCursor + 4);
            count++;
            if(count == entryNum_y){
                memPoolCursor = (uint32_t *)((uintptr_t) memPoolAddr.virAddr + (entryNum_y_align<<2));
            }
        }
    }
    TEE_CleanCache(memPoolAddr.virAddr, entryNum_alloc<<2);

    params[2].value.a = memPoolAddr.phyAddr;
    params[2].value.b = memPoolAddr.size;

    params[3].value.a = mem_id;

    pPtUnit->ptPhyAddr = memPoolAddr.phyAddr;
    pPtUnit->ptSize = memPoolAddr.size;
    pPtUnit->mem_id = mem_id;
    pPtUnit->is_zone = is_zone;
    std_list_push_back(pPtmMgr->pPTList, pPtUnit);

    TA_LOGH("Finish alloc pt %"PRIxPTR", size 0x%x, Rule %d MemID 0x%x IsZone %d\n",
             memPoolAddr.phyAddr, memPoolAddr.size, pPtUnit->mptRuleIndex,
             mem_id, is_zone);

#ifdef ENABLE_PTM_DEBUG
    internal_statistics(pPtmMgr);
#endif
    return TEE_SUCCESS;
error:
    if(pPtUnit->pReservation) {
        if (pPtUnit->pReservation->mem_id != 0)
            TEE_ReleaseSecurePages(pPtUnit->pReservation);
        TEE_DestroyReservation(pPtUnit->pReservation);
    }
    if(pPtUnit->is_zone) {
        if(pPtUnit->mem_id != 0)
            TEE_Unregister_Memory(pPtUnit->mem_id);
    }
    if(pPtUnit){
        internal_free_PT_unit(pPtUnit);
    }
    return res;
}


static TEE_Result PTMTA_FreeMem(void* sessionContext,
                                         uint32_t  paramTypes,
                                         TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    PT_LIST_NODE *pPtNode = NULL;
    PTM_AllocUnit *pPtUnit = NULL;
    BUF_Addr addr;
    uint32_t entryNum = 0;

    TA_LOGH("%s enter, pt 0x%x\n", __FUNCTION__, params[0].value.a);

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
        return TEE_ERROR_BAD_PARAMETERS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    addr.phyAddr = params[0].value.a;

    pPtNode = internal_search_PT_node(pPtmMgr->pPTList, addr.phyAddr);
    if(pPtNode == NULL) {
        TA_LOGE("%s, can't find the pageTable entry %"PRIxPTR"\n",__FUNCTION__,addr.phyAddr);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    pPtUnit =(PTM_AllocUnit *)pPtNode->pData;
    addr.size = pPtUnit->ptSize; //use the size stored in ptList, input addr is enough

    if(pPtUnit->refCount) { //currently when refcount != 0, bm driver will query it regularly
        TA_LOGE("can't free the PT(0x%x), still hold, refCount=0x%x\n", \
                pPtUnit->ptPhyAddr, pPtUnit->refCount);
        internal_debug_callerTATrace(pPtUnit); //just enable it recently for debuging vpp/vdec ta
        return TEE_ERROR_BAD_STATE;
    }

    TA_LOGH("free mem, phyAddr(PT) %"PRIxPTR"\n", addr.phyAddr);
    entryNum = addr.size >> 2;

    if(pPtUnit->pReservation) {
        res = TEE_ReleaseSecurePages(pPtUnit->pReservation);
        if(res != TEE_SUCCESS){
            TA_LOGE("%s, ReleaseSecurePages of PT(0x%x) MemId(0x%x) return fail(0x%x)\n",
                __FUNCTION__, pPtUnit->ptPhyAddr, pPtUnit->pReservation->mem_id, res);
            return res;
        }
        TEE_DestroyReservation(pPtUnit->pReservation);
    } else {
        res = TEE_Unregister_Memory(pPtUnit->mem_id);
        if(res != TEE_SUCCESS){
            TA_LOGE("%s, TEE_Unregister_Memory of PT(0x%x) MemId(0x%x) return fail(0x%x)\n",
                __FUNCTION__, pPtUnit->ptPhyAddr, pPtUnit->mem_id, res);
            return res;
        }
    }
    std_list_remove(pPtmMgr->pPTList, pPtNode);
    internal_free_PT_unit(pPtUnit);

    //set all these page entries to free with pool mgr
    POOL_Free(pPtmMgr->poolHandle, &addr);

    TA_LOGH("Finish free mem addr %"PRIxPTR", size 0x%x, entryNum 0x%x\n",
        addr.phyAddr, addr.size, entryNum);

#ifdef ENABLE_PTM_DEBUG
    internal_statistics(pPtmMgr);
#endif

    return res;
}

static TEE_Result PTMTA_Ref(void* sessionContext,
                                 uint32_t  paramTypes,
                                 TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    PT_LIST_NODE *pPtNode = NULL;
    PTM_AllocUnit *pPtUnit = NULL;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
        return TEE_ERROR_BAD_PARAMETERS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    res = internal_check_callFromTA();
    if (res != TEE_SUCCESS) {
        TA_LOGE("check_callFromTA fail\n");
        return res;
    }

    pPtNode = internal_search_PT_node(pPtmMgr->pPTList, params[0].value.a);
    if(pPtNode == NULL) {
        TA_LOGE("%s can't find the pageTable entry 0x%x\n", __FUNCTION__, params[0].value.a);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    pPtUnit =(PTM_AllocUnit *)pPtNode->pData;

    internal_update_callerTATrace(pPtUnit, TRUE);
    if(res == TEE_SUCCESS) {
        pPtUnit->refCount++;
    } else {
        TA_LOGE("fail to ref pt(0x%x), refCount 0x%x, res = 0x%x\n",
            pPtUnit->ptPhyAddr, pPtUnit->refCount, res);
        return res;
    }

    TA_LOGH("Finish ref pt(0x%x),  refCount 0x%x\n",pPtUnit->ptPhyAddr, pPtUnit->refCount);

    return res;
}

static TEE_Result PTMTA_UnRef(void* sessionContext,
                                     uint32_t  paramTypes,
                                     TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    PT_LIST_NODE *pPtNode = NULL;
    PTM_AllocUnit *pPtUnit = NULL;

    TA_LOGH("%s enter, pt 0x%x\n", __FUNCTION__, params[0].value.a);

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
        return TEE_ERROR_BAD_PARAMETERS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    res = internal_check_callFromTA();
    if (res != TEE_SUCCESS) {
       TA_LOGE("check_callFromTA fail\n");
       return res;
    }

    pPtNode = internal_search_PT_node(pPtmMgr->pPTList, params[0].value.a);
    if(pPtNode == NULL) {
        TA_LOGE("%s can't find the pageTable entry 0x%x\n", __FUNCTION__,params[0].value.a);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    pPtUnit =(PTM_AllocUnit *)pPtNode->pData;

    if(pPtUnit->refCount == 0) {
        TA_LOGE("fail: currently already no ref, can't unref\n");
        return TEE_ERROR_BAD_STATE;
    }

    res = internal_update_callerTATrace(pPtUnit, FALSE);
    if(res == TEE_SUCCESS) {
        pPtUnit->refCount--;
    } else {
        TA_LOGE("fail to unref pt(0x%x), refCount 0x%x, res = 0x%x\n",
            pPtUnit->ptPhyAddr, pPtUnit->refCount, res);
        return res;
    }

    TA_LOGH("Finish unref pt(0x%x), refCount 0x%x\n", pPtUnit->ptPhyAddr,pPtUnit->refCount);

    return res;
}

static TEE_Result PTMTA_QueryBufAttr(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    PT_LIST_NODE *pPtNode = NULL;
    PTM_AllocUnit *pPtUnit = NULL;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT)
        return TEE_ERROR_BAD_PARAMETERS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    pPtNode = internal_search_PT_node(pPtmMgr->pPTList, params[0].value.a);
    if(pPtNode == NULL) {
        TA_LOGE("%s can't find the pageTable entry 0x%x\n", __FUNCTION__,  params[0].value.a);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    pPtUnit =(PTM_AllocUnit *)pPtNode->pData;
    params[1].value.a = pPtUnit->allocParam.ruleIndex;

    TA_LOGH("Finish query attribute %d\n", pPtUnit->allocParam.ruleIndex);
    return res;
}
static TEE_Result PTMTA_Statistics(void* sessionContext,
                                         uint32_t  paramTypes,
                                         TEE_Param params[4])
{
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    internal_statistics(pPtmMgr);
    return TEE_SUCCESS;
}

static TEE_Result PTMTA_DumpMem(void* sessionContext,
                                      uint32_t  paramTypes,
                                      TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t totalPageCnt = 0;
    void *toVirAddr = NULL;
    size_t toSize = 0;
    void *fromVirAddr = NULL;
    size_t fromSize = 0;
    PTM_MemoryManager* pPtmMgr = ((PTM_SessionContext*)sessionContext)->pPtmMgr;
    PT_LIST_NODE *pPtNode = NULL;
    PTM_AllocUnit *pPtUnit = NULL;
    scatter_list_t *output_sgTable = NULL;
    TEE_Reservation *pOutReservation = NULL;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT)
        return TEE_ERROR_BAD_PARAMETERS;

    res = internal_check_PT_mgr();
    if(res != TEE_SUCCESS){
        TA_LOGF("%s, check PTM server fail\n", __FUNCTION__);
        return res;
    }

    pPtNode = internal_search_PT_node(pPtmMgr->pPTList, params[0].value.a);
    if(pPtNode == NULL) {
        TA_LOGE("%s can't find the pageTable entry 0x%x\n", __FUNCTION__, params[0].value.a);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    pPtUnit =(PTM_AllocUnit *)pPtNode->pData;

    res = internal_check_dump(pPtUnit);
    if(res != TEE_SUCCESS) {
        TA_LOGE("check dump fail, 0x%x\n", res);
        return res;
    }

    for(uint32_t i = 0; i < pPtUnit->sgTable->num; i++) {
        totalPageCnt += pPtUnit->sgTable->s_item[i].npage;
    }

    if(params[1].value.b < (totalPageCnt * 0x1000)) {
        TA_LOGE("output size is too short 0x%x(input 0x%x)\n", \
                    params[1].value.a, totalPageCnt * 0x1000);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TA_LOGI("dump mem, PTphyAddr 0x%x, toPhyAddr 0x%x, dumpSize 0x%x\n",
            params[0].value.a, params[1].value.a, totalPageCnt * 0x1000);

    res = TEE_Map_Memory(pPtUnit->pReservation->mem_id, &fromVirAddr, &fromSize);
    if(res != TEE_SUCCESS){
        TA_LOGE("TEE_Map_Memory map fail 0x%x\n", res);
        return res;
    }

    //construct output sg_table to do mem map, otherwise params[1] should be mem type for ca->ta,
    //for ta->ta no mem type, still value type, so construct output sg_table seems suit for both ca/ta -> ta
    output_sgTable = (scatter_list_t *)TEE_Malloc(SIZE_OF_SCATTERLIST(1), \
                            TEE_MALLOC_FLAG_DEFAULT);
    if(!output_sgTable){
        TA_LOGF( "Faild to malloc mem for scatter list\n");
        TEE_Unmap_Memory(pPtUnit->pReservation->mem_id);
        return TEE_ERROR_OUT_OF_MEMORY;
    }
    output_sgTable->num = 1;
    output_sgTable->s_item[0].addr = params[1].value.a;
    output_sgTable->s_item[0].npage = totalPageCnt;
    pOutReservation = TEE_CreateReservation(output_sgTable, 0);
    if(!pOutReservation){
        TA_LOGE("TEE_CreateReservation pOutReservation fail\n");
        TEE_Unmap_Memory(pPtUnit->pReservation->mem_id);
        goto ret;
    }

    res = TEE_ReserveSecurePages(pOutReservation);
    if(res != TEE_SUCCESS){
        TA_LOGE("TEE_ReserveSecurePages fail 0x%x\n", res);
        TEE_Unmap_Memory(pPtUnit->pReservation->mem_id);
        goto ret;
    }
    res = TEE_Map_Memory(pOutReservation->mem_id, &toVirAddr, &toSize);
    if(res != TEE_SUCCESS){
        TA_LOGE("TEE_Map_Memory map fail 0x%x\n", res);
        TEE_Unmap_Memory(pPtUnit->pReservation->mem_id);
        goto ret;
    }

    if (fromSize < toSize) {
        TEE_InvalidateCache(fromVirAddr, fromSize);
        memcpy(toVirAddr, fromVirAddr, fromSize);
    }

    TEE_Unmap_Memory(pPtUnit->pReservation->mem_id);
    TEE_Unmap_Memory(pOutReservation->mem_id);

    TA_LOGH("Finish dump mem, real entryNum 0x%x\n",totalPageCnt);

#ifdef ENABLE_PTM_DEBUG
     internal_statistics(pPtmMgr);
#endif
ret:
    if(pOutReservation) {
        if (pOutReservation->mem_id != 0)
            TEE_ReleaseSecurePages(pOutReservation);
        TEE_DestroyReservation(pOutReservation);
    }
    if(output_sgTable) {
        TEE_Free(output_sgTable);
    }
    return res;
}

typedef struct __ptm_ta_cmd_handler__
{
    uint32_t commandID;
    uint32_t (*cmd_handler)(void* sessionContext, uint32_t  paramTypes, TEE_Param params[4]);
} PTM_TA_CMD_HANDLER;

static PTM_TA_CMD_HANDLER ptm_ta_cmd_handler[PTM_CMD_MAX] =
{
    {PTM_INVALID, NULL},
    {PTM_DEBUG, PTMTA_Statistics},
    {PTM_CONSTRUCT_PT, PTMTA_AllocateMem},
    {PTM_FREE_PT, PTMTA_FreeMem},
    {PTM_REF_PT, PTMTA_Ref},
    {PTM_UNREF_PT, PTMTA_UnRef},
    {PTM_QUERY_PT, PTMTA_QueryBufAttr},
    {PTM_DUMP_PT, PTMTA_DumpMem},
};

TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(
    void*            sessionContext,
    uint32_t        commandID,
    uint32_t        paramTypes,
    TEE_Param        params[4])
{
    TEE_Result res = TEE_ERROR_NOT_SUPPORTED;
    TA_LOGH( "TA_InvokeCommandEntryPoint, ID:0x%x\n", commandID);
    if( (commandID > PTM_INVALID && commandID < PTM_CMD_MAX) &&
        ptm_ta_cmd_handler[commandID].cmd_handler)
        res = ptm_ta_cmd_handler[commandID].cmd_handler(sessionContext, paramTypes, params);
    return res;
}

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
    .uuid = TA_PTM_UUID,
    .name = "PTM",
    .singleInstance = true,
    .multiSession = true,
    .instanceKeepAlive = false,
TA_DEFINE_END
#endif
