/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file bm_heapManager.c
//!
//! \brief  memory/heap manager implementation
//!
//! Purpose:
//!
//! Version,    Date and Author :
//!   V 1.00,                   Shanmugam Ramachandran
//!   V 1.10,   Mar 11 2022,    Shanmugam Ramachandran
//! Note:
////////////////////////////////////////////////////////////////////////////////

#define _HEAP_MANAGER_C_
#if defined (TZ_3_0 ) && defined (VPP_ENABLE_INTERNAL_MEM_MGR)
#include "vpp_module.h"
#include "bm_heap.h"

//Note: Both start_addr & start_addr_virt is always virtual or physical address
/*Dynamic memory size is reduced by BM_SHM_ONE_TIME_ALLOCATION_SIZE
 * Thus total size is is sum of dynamic and static/one-time allocation size*/
#define bitmap_is_addr_invalid(Addr, Size) \
        (((UINT)(Addr) < (UINT)(handle->start_addr)) || \
         ((UINT)(((UINT)Addr)+Size) >= (((UINT)handle->start_addr_virt)+(handle->size+BM_SHM_ONE_TIME_ALLOCATION_SIZE))))

static UINT checkbit(UINT *x, INT p)
{
    x += p/BM_BSZ;
    return ((*x) &  BM_MAX_BITMASK_VAL >> (BM_BSZ-p%BM_BSZ-1));
}

static UINT setbits(UINT *x, INT p, INT n , UINT8 bit)
{
    UINT pos = 0;
    UINT8 bitpos = 0;

    x += p/BM_BSZ;
    pos = p;
    bitpos = pos%BM_BSZ;

    BM_LOGI("bit[%d] pos:%d\n",bit,pos);
    while (pos < p+n) {
        if (bit)
            *x |= (BM_MAX_BITMASK_VAL >> (BM_BSZ-pos%BM_BSZ-1));
        else
            *x &= ~(BM_MAX_BITMASK_VAL >> (BM_BSZ-pos%BM_BSZ-1));

        bitpos++;
        if(bitpos == BM_BSZ) {
            x++;
            bitpos = 0;
        }
        pos++;
    }
    BM_LOGI("bit[%d] pos:%d %d\n",bit,pos,x[pos]);

    return BM_S_OK;
}

static UINT *bitmap_alloc_free(bitmap_mem *handle, UINT* Addr)
{
    UINT nReq;
    UINT mempos = 0;
    UINT blockSz = 0;

    if (handle == NULL)
        return NULL;

    if (bitmap_is_addr_invalid(Addr, 0)) {
        BM_LOGF("#free fail return \n");
        return NULL;
    }

    blockSz = 1 << handle->align;
    mempos = ((UINT)Addr - (UINT)handle->start_addr)/blockSz;
    nReq = handle->sizeblock[mempos];
    BM_LOGI("Size[%d] mempos:%d\n",nReq,mempos);

    if (nReq) {
        setbits(handle->bitblock, mempos, nReq,0);
        handle->sizeblock[mempos]=0;
    }

    return BM_S_OK;
}

static UINT *bitmap_alloc_mem(bitmap_mem *handle, const UINT size)
{
    UINT nTotal;
    UINT nReq;
    UINT bitpos = 0;
    UINT count = 0;
    UINT mempos = 0;
    UINT pos = 0;
    UINT *addr = NULL;
    UINT *bit = NULL;
    UINT blockSz = 0;

    if (handle == NULL)
        return NULL;

    if (!size || size > handle->size)
        return NULL;

    /*find free unit*/
    bit = handle->bitblock;
    blockSz = 1 << handle->align;
    nTotal = handle->size >> handle->align;
    nReq = (size == blockSz)? 1 : (size % blockSz) ? ((size + blockSz - 1) >> handle->align):(size>>handle->align);
    BM_LOGI("nTotal[%d] nReq[%d] size:%d blockSz:%d %d %d\n",nTotal, nReq,size,blockSz,size % blockSz, size>>handle->align);

    while (pos*BM_BSZ < nTotal && bitpos < nTotal) {
            if( checkbit(bit+pos, bitpos%BM_BSZ)==0) { //, min(nT,BM_BSZ)))
                mempos = mempos ? mempos:pos*BM_BSZ+bitpos+1;
                count++;
                if (count == nReq)
                    break;
            } else {
                mempos = 0;
                count = 0;
            }

        bitpos++;
        if (bitpos == BM_BSZ) {
            pos++;
            BM_LOGL("alloc[0x%x] mempos[0x%x] pos[0x%x] a:%d\n",(int)addr,mempos,pos,bitpos);
            bitpos = 0;
        }
    }

    if (count == nReq) {
        mempos--;
        addr = (UINT*)(handle->start_addr + mempos*(1 << handle->align));
        setbits(handle->bitblock, mempos, nReq,1);
        handle->sizeblock[mempos] = nReq;
        BM_LOGI("alloc[0x%x] mempos[0x%x] pos[0x%x] a:%d\n",(int)addr,mempos,pos,mempos*(1 << handle->align));
    }

    if (bitmap_is_addr_invalid(addr, size)) {
        BM_LOGF("#alloc fail return \n");
        return NULL;
    }

    return addr;
}

static UINT *bitmap_realloc_mem(bitmap_mem *handle, UINT* Addr, UINT size)
{
    UINT nReq;
    UINT mempos = 0;
    UINT blockSz = 0;
    UINT *dummyAddr = NULL;

    if (handle == NULL)
        return NULL;

    if (bitmap_is_addr_invalid(Addr, 0))
        return NULL;

    blockSz = 1 << handle->align;
    mempos = ((UINT)Addr - (UINT)handle->start_addr)/blockSz;
    nReq = handle->sizeblock[mempos];

    if (Addr != NULL) {
        dummyAddr = bitmap_alloc_mem(handle,size);
        VPP_Memcpy(dummyAddr,Addr,(nReq * (1<<handle->align)));
        bitmap_alloc_free(handle,Addr);
    }

    return dummyAddr;
}

INT bitmap_pool_init(UINT32 PAddr, UINT Size, UINT Align, UINT *Addr,bitmap_mem *Memhandle)
{
    UINT nBlocksz = 0;
    UINT dummyaddr = (UINT)Addr;

    if (Size == 0 || Align < 5 || Addr==NULL)
        return -1;

    VPP_Memset(Addr, 0, Size);

    Memhandle->start_addr_phy  =  (UINT32)PAddr;
    Memhandle->start_addr_virt =  (UINT32)Addr;

    Memhandle->start_addr_virt_only  =  (UINT32)Addr;
    dummyaddr += BM_SHM_ONE_TIME_ALLOCATION_SIZE;
    Size -= BM_SHM_ONE_TIME_ALLOCATION_SIZE;

    Memhandle->size =  Size;
    Memhandle->align = Align;
    Memhandle->bitblock = (UINT*)dummyaddr;
    nBlocksz = Size >> Memhandle->align;
    //1bit per block, 8blocks represented per byte, align to word
    dummyaddr += BM_SHM_ALLOC_ALING_ROUNDUP(((nBlocksz+7)/8),4);
    Memhandle->sizeblock = (UINT*)dummyaddr;

    //Allocate required size to avoid sizeblock overwrite actual heap data
    dummyaddr += nBlocksz * sizeof(Memhandle->sizeblock[0]);
    Memhandle->start_addr =  (UINT*)dummyaddr;

    BM_LOGI("%s:%d: start_addr-%x\n", __func__, __LINE__, dummyaddr);

    BM_SHM_MUTEX_Create(&(Memhandle->osal_shm_lock));

    return BM_S_OK;;
}

//The allocation from the MV_VPPOBJ_Create can be part of one time allocation
UINT* bm_malloc_only(bitmap_mem *handle, const UINT size, const UINT align)
{
    UINT* addr = 0;
    UINT32 aligned_addr, next_addr;
    UINT32 fragmentation;

    BM_SHM_MUTEX_Lock(handle->osal_shm_lock);

    //Align to the memory location
    aligned_addr = BM_SHM_ALLOC_ALING_ROUNDUP(handle->start_addr_virt_only, align);
    fragmentation = aligned_addr - handle->start_addr_virt_only;
    if (fragmentation) {
        BM_LOGI("%s:%d: Fragmentation - %d (%x - %x)\n", __func__, __LINE__,
                fragmentation, aligned_addr, handle->start_addr_virt_only);
    }

    next_addr = aligned_addr + size;
    if (next_addr < (handle->start_addr_virt+BM_SHM_ONE_TIME_ALLOCATION_SIZE)) {
        addr = (UINT*)aligned_addr;
        handle->start_addr_virt_only = next_addr;
    } else {
        addr = NULL;
    }
    BM_LOGI("%s:%d: Allocated-%x/(%d/%x)\n", __func__, __LINE__, (UINT)addr, size, size);

    BM_SHM_MUTEX_Unlock(handle->osal_shm_lock);

    return addr;
}

UINT* bm_malloc(bitmap_mem *handle, const UINT size)
{
    UINT* addr;

    BM_SHM_MUTEX_Lock(handle->osal_shm_lock);

    addr = bitmap_alloc_mem(handle, size);
    BM_LOGI("%s:%d: Allocated-%x/(%d/%x)\n", __func__, __LINE__, (UINT)addr, size, size);

    BM_SHM_MUTEX_Unlock(handle->osal_shm_lock);

    return addr;
}

UINT *bm_free(bitmap_mem *handle, UINT* Addr)
{
    UINT* ret = NULL;

    BM_SHM_MUTEX_Lock(handle->osal_shm_lock);

    if (((void*)Addr) >= handle->start_addr)
        ret = bitmap_alloc_free(handle, Addr);

    BM_SHM_MUTEX_Unlock(handle->osal_shm_lock);

    return ret;
}

UINT *bm_realloc(bitmap_mem *handle, UINT* Addr, UINT size)
{
    UINT *addr = NULL;

    BM_SHM_MUTEX_Lock(handle->osal_shm_lock);

    if (((void*)Addr) >= handle->start_addr)
        addr = bitmap_realloc_mem(handle, Addr, size);

    BM_SHM_MUTEX_Unlock(handle->osal_shm_lock);

    return addr;
}
#endif//TZ_3_0
