/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file genPool.c
//!
//! \brief general Pool Implementation
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    August 1 2019,        Zhiliang Wang
//!     V 2.00,    June 12 2020,         Zhiliang Wang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "genPool.h"

#define SIZEUP(a,b)    ((a) * (b))
#define SIZEDW(a,b)    ((a + b - 1) / (b))

#define  INVALID_ADDR   (-1)

#ifndef TA_COMPILE
#include "OSAL_api.h"
#include "amp_logger.h"

#define PRINT(...)         AMPLOGD(__VA_ARGS__)
#define PRINT_ERROR(...)   AMPLOGE(__VA_ARGS__)

#define CREATE_MUTEX(lock)     MV_OSAL_Mutex_Create(lock)
#define DESTORY_MUTEX(lock)    MV_OSAL_Mutex_Destroy(lock)
#define LOCK_MUTEX(lock)       MV_OSAL_Mutex_Lock(lock)
#define UNLOCK_MUTEX(lock)     MV_OSAL_Mutex_Unlock(lock)

#else
#define LOG_TAG         "[genPool_inTA]"

#include "config_log.h"
#include "tee_internal_api.h"
#include "ta_log.h"

#define PRINT(...)          TA_LOGH(__VA_ARGS__)
#define PRINT_ERROR(...)    TA_LOGE(__VA_ARGS__)

#define CREATE_MUTEX(lock)     POOL_SUCCESS
#define DESTORY_MUTEX(lock)
#define LOCK_MUTEX(lock)
#define UNLOCK_MUTEX(lock)

#endif

typedef struct mm_cell_t {
    int occupied;
    uintptr_t start;
    int size;
    struct mm_cell_t * next;
} MM_CELL;

typedef struct mm_heap_t {
    void * mem_start;
    int mem_size;
    int avail;
    int allocNr;

    MM_CELL head;
} MM_HEAP;

#define ADDR_ALIGN(uiAddr, uiAlign)     (uiAddr + uiAlign - 1) & ~(uiAlign - 1);
static void gen_heap_cell_merge(MM_HEAP * m);

static void * gen_heap_create(void * mem_start, int mem_size)
{
    MM_HEAP * m = malloc(sizeof(MM_HEAP));

    if(m) {
        m->mem_start = mem_start;
        m->mem_size = mem_size;
        m->avail = mem_size;
        m->allocNr = 0;

        m->head.occupied = 0;
        m->head.start = (uintptr_t) mem_start;
        m->head.size = mem_size;
        m->head.next = NULL;
    }
    return m;
}

static void gen_heap_release(void * pHeap)
{
    MM_HEAP * m = (MM_HEAP *)pHeap;

    if(m) {
        MM_CELL * p = &m->head;
        for( ; p; p = p->next) {
            if(p->occupied) {
                p->occupied = 0;
            }
            if(m->head.next == NULL) break;
        }
        gen_heap_cell_merge(m);
        free(m);
    }
}

static int gen_mem_alloc(void * pHeap, int mem_size, int align_size)
{
    MM_HEAP * m = (MM_HEAP *)pHeap;

    if(mem_size & (align_size-1)) {
        mem_size += align_size - (mem_size & (align_size-1));
    }

    if(m && mem_size > 0) {
        MM_CELL * p = &m->head;

        while(p) {
            if(p->occupied || mem_size > p->size) {
                p = p->next;
            }else{
                if(mem_size < p->size) {
                    MM_CELL * p1 = p->next;

                    p->next = (MM_CELL *)malloc(sizeof(MM_CELL));
                    if(p->next == NULL) {
                        PRINT("mm_cell malloc denied\n");
                        break;
                    }

                    p->next->occupied = 0;
                    p->next->start = p->start + mem_size;
                    p->next->size = p->size - mem_size;
                    p->next->next = p1;

                    p->size = mem_size;
                }
                p->occupied = 1;
                m->avail -= mem_size;
                m->allocNr++;
                return p->start;
            }
        }
    }

    return -1;
}

static void gen_mem_free(void * pHeap, int mem_start)
{
    MM_HEAP * m = (MM_HEAP *)pHeap;

    if(m && mem_start >= 0) {
        MM_CELL * p = &m->head;
        for( ; p; p = p->next) {
            if(p->occupied && p->start == (uintptr_t)mem_start) {
                p->occupied = 0;
                m->avail += p->size;
                gen_heap_cell_merge(m);
                m->allocNr--;
                break;
            }
        }
    }
}

static void gen_heap_cell_merge(MM_HEAP * m)
{
    if(m)
    {
        MM_CELL * p1 = &m->head, * p2 = m->head.next;

        while(p2)
        {
            if(p1->occupied == 0 && p2->occupied == 0)
            {
                p1->size += p2->size;
                p1->next = p2->next;

                free(p2);
                p2 = p1->next;
            }
            else
            {
                p1 = p2;
                p2 = p2->next;
            }
        }
    }
}

static int gen_heap_max_avail_cell(void * pHeap)
{
    int maxCell = 0;
    MM_HEAP * m = (MM_HEAP *)pHeap;

    if(m) {
        MM_CELL * p = &m->head;

        while(p) {
            if((!p->occupied) && (p->size > maxCell)) {
                maxCell = p->size;
            }
            p = p->next;
        }
    }

    return maxCell;
}

int POOL_Create(POOL_Type type, UINT32 chunkSize, UINT32 chunkNbr,
                     uintptr_t phyAddr, void* virAddr, POOL_Handle *handle)
{
    POOL_Handle pPoolCtx;

    if(type >= POOL_TYPE_META_MAX || (chunkSize == 0) || (chunkNbr == 0) ||(!handle))
        return POOL_ERROR;

    pPoolCtx = malloc(sizeof(POOL_CTX));
    if (pPoolCtx != NULL) {
        memset(pPoolCtx, 0, sizeof(POOL_CTX));

        if(CREATE_MUTEX(&pPoolCtx->poolLock) != POOL_SUCCESS){
            PRINT_ERROR("create mutex fail\n");
            goto error;
        }

        pPoolCtx->pool = gen_heap_create(0, chunkNbr);
        if(pPoolCtx->pool){
            pPoolCtx->poolType = type;
            pPoolCtx->chunkSize = chunkSize;
            pPoolCtx->chunkNbr = chunkNbr;
            pPoolCtx->basePhyAddr = phyAddr;
            pPoolCtx->baseVirAddr = virAddr;
            *handle = pPoolCtx;
            PRINT("create: chunkNbr=0x%x chunkSize=0x%x phyAdd=%"PRIuPTR" virAddr=%p\n", chunkNbr,
                    chunkSize, phyAddr, virAddr);
            return POOL_SUCCESS;
        }else {
            DESTORY_MUTEX(&pPoolCtx->poolLock);
        }
    }
error:
    if(pPoolCtx)
        free(pPoolCtx);
    *handle = NULL;
    return POOL_ERROR;
}

void POOL_Destroy(POOL_Handle handle)
{
    if(!handle) {
       PRINT_ERROR("null point:  handle\n");
       return;
    }

    gen_heap_release(handle->pool);
    handle->pool = NULL;
    DESTORY_MUTEX(&handle->poolLock);
    free(handle);
}

int POOL_Allocate(POOL_Handle handle, UINT32 size, BUF_Addr *addr)
{
    if(!handle || !addr) {
       PRINT_ERROR("null point:  handle %p, addr %p\n", handle, addr);
       return POOL_ERROR;
    }

    LOCK_MUTEX(handle->poolLock);

    int ret = POOL_ERROR;
    int allocSize = SIZEDW(size, handle->chunkSize);
    int offset = gen_mem_alloc(handle->pool, allocSize, 1);
    if(offset != INVALID_ADDR){
        offset *= handle->chunkSize;
        addr->phyAddr = offset + handle->basePhyAddr;
        addr->virAddr = (void*) (offset + (uintptr_t) handle->baseVirAddr);
        addr->size = SIZEUP(allocSize, handle->chunkSize);
        ret =  POOL_SUCCESS;
    }
    PRINT("allocate: offset=0x%x size=0x%x phyAdd=%"PRIuPTR" virAddr=%p\n", (UINT32)offset,
            addr->size, addr->phyAddr, addr->virAddr);

    UNLOCK_MUTEX(handle->poolLock);

    return ret;
}

void POOL_Free(POOL_Handle handle, BUF_Addr *addr)
{
    if(!handle || !addr) {
       PRINT_ERROR("null point:  handle %p, addr %p\n", handle, addr);
       return;
    }

    LOCK_MUTEX(handle->poolLock);

    int offset = (addr->phyAddr - handle->basePhyAddr)/ handle->chunkSize;

    PRINT("free: offset=0x%x size=0x%x phyAdd=%"PRIuPTR" virAddr=%p\n", offset*handle->chunkSize,
            addr->size, addr->phyAddr, addr->virAddr);
    gen_mem_free(handle->pool, offset);

    UNLOCK_MUTEX(handle->poolLock);
}

int POOL_GetProperty(POOL_Handle handle, POOL_Property *prop)
{
    if(!handle || !prop || !handle->pool) {
       PRINT_ERROR("null point:  handle %p, prop %p, pool %p\n", handle, prop, handle->pool);
       return POOL_ERROR;
    }

    LOCK_MUTEX(handle->poolLock);

    MM_HEAP *pool = (MM_HEAP *)handle->pool;
    prop->basePhyAddr = handle->basePhyAddr;
    prop->baseVirAddr = handle->baseVirAddr;
    prop->poolType = handle->poolType;
    prop->poolSize = handle->chunkNbr;
    prop->chunkSize = handle->chunkSize;
    prop->avail = pool->avail;
    prop->allocNr = pool->allocNr;
    prop->maxAvailBlock = gen_heap_max_avail_cell(handle->pool);

    UNLOCK_MUTEX(handle->poolLock);
    return POOL_SUCCESS;
}

