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
//! \file genPool.h
//!
//! \brief general Pool header file
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    August 1 2019,        Zhiliang Wang
//!     V 2.00,    June 12 2020,         Zhiliang Wang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////


#ifndef __GENPOOL_H__
#define __GENPOOL_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TA_COMPILE
typedef uint32_t            UINT32;
#else
typedef unsigned int        UINT32;
#endif

#define POOL_SUCCESS      0
#define POOL_ERROR       -1

typedef enum {
    POOL_TYPE_PAGE_TABLE     = 0x0,
    POOL_TYPE_META_DATA      = 0x1,
    POOL_TYPE_META_MAX
}POOL_Type;

typedef struct {
    void  *pool;
    UINT32 poolType;
    UINT32 chunkSize;
    UINT32 chunkNbr;
    void   *baseVirAddr;
    uintptr_t basePhyAddr;
    void   *poolLock;
}POOL_CTX, *POOL_Handle;

typedef struct {;
    void*     virAddr;
    uintptr_t phyAddr;
    UINT32 size;
}BUF_Addr;

typedef struct {;
    void*  baseVirAddr;
    uintptr_t basePhyAddr;
    UINT32 poolType;
    UINT32 poolSize;
    UINT32 chunkSize;
    UINT32 avail;
    UINT32 allocNr;
    UINT32 maxAvailBlock;
}POOL_Property;

int  POOL_Create(POOL_Type type, UINT32 chunkSize, UINT32 chunkNbr,
                     uintptr_t phyAddr, void* virAddr, POOL_Handle *handle);
void POOL_Destroy(POOL_Handle handle);
int  POOL_Allocate(POOL_Handle handle, UINT32 size, BUF_Addr *addr);
void POOL_Free(POOL_Handle handle, BUF_Addr *addr);
int  POOL_GetProperty(POOL_Handle handle, POOL_Property *prop);

#ifdef __cplusplus

}

#endif

#endif /* __GENPOOL_H__ */
