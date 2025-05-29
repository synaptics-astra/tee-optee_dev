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
//! \file ptm_ta.h
//!
//! \brief PTM TA defines
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    JUL 2019,                Hao Chen
//!     V 2.00,    APR 2020,                zhwan
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __PTMTA_H__
#define __PTMTA_H__

#include "tee_internal_api.h"
#include "std_list.h"
#include "genPool.h"

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif

#define INVALID_ADDR     0

#define CHUNK_SIZE       4096    //currently VxG needs 4k aligned
#define SIZE2PAGE(x)    ((x + 4095) / 4096)
#define ALIGN4K(x)      ((((uint32_t)(x)) + 0xfff) & (~0xfff))

typedef std_list           PT_LIST;
typedef std_list_node      PT_LIST_NODE;

enum shm_fb_type {
    /* generic physical scatter buffer */
    SHM_MMU_GENERIC,
    /* UV entry also page aligned, must set total size and y size together */
    SHM_MMU_UV_ALIGN
};

struct shm_uv_align_param {
    uint32_t y_size;
};

struct shm_fb_param {
    uint32_t fb_type;
    struct shm_uv_align_param uva_param;
};

struct ptm_construct_param {
    uint32_t ruleIndex;
    struct shm_fb_param param;
};

struct ptm_ta_user {
    TEE_UUID  UserUuid;
    uint32_t  refCount;
};

typedef struct {
    struct ptm_construct_param allocParam;
    scatter_list_t  *sgTable;
    uint32_t  ptPhyAddr;
    uint32_t  ptSize;
    uint32_t  refCount;
    uint32_t  mptRuleIndex; //set to mpt
    uint32_t  mem_id;
    bool      is_zone;
    TEE_Reservation *pReservation;

    //! Debug context
    std_list  *userList;
}PTM_AllocUnit;

typedef struct __ptm_memory_manger__
{
    POOL_Handle poolHandle;
    uint32_t poolHdlShmId;
    paddr_t poolPhyAddr;
    void* poolVirAddr;
    uint32_t chunkSize;
    uint32_t chunkNum;
    PT_LIST *pPTList;
    uint32_t bInit;
    uint32_t poolSize;
} PTM_MemoryManager; //manager for memory Pool and PT list

typedef struct __ptm_session_context__{
     PTM_MemoryManager *pPtmMgr;
}PTM_SessionContext;

#endif /* __PTMTA_H__ */
