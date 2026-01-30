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

#ifndef _TEE_TSP_MEM_MAP_H_
#define _TEE_TSP_MEM_MAP_H_

#include "tee_internal_api.h"

typedef struct __ta_tsp_buff__ {
    uint32_t memID;
    uint32_t eMemType;
    void *phyAddr;
    void *virAddr;
    uint32_t uBufSize;

    //for m2m case, outbuf maybe duplicated from inbuf, other cases shall be mostly not
    bool   bDup;
    bool   bRegister;
}TA_TSP_BUFF_T, TA_M2M_BUFF_T;

//for tsp registers
TEE_Result tspmem_create_iomap(void);
uintptr_t tspmem_virt_to_phy(uintptr_t uVirtAddr);
uintptr_t tspmem_phy_to_virt(uintptr_t uPhyAddr);

/*--------------- APIs for buffer from ca->ta, ta->ta -----------------*/

// bRegister is for internal shm only
TEE_Result tspbuf_create_with_pa(TA_TSP_BUFF_T *pTspBuf,
                                            uintptr_t phyAddr,
                                            uint32_t bufSize,
                                            bool bRegister);
TEE_Result tspbuf_create_with_memid(TA_TSP_BUFF_T *pTspBuf,
                                                  uint32_t memID,
                                                  uint32_t offset,
                                                  uint32_t bufSize);
TEE_Result tspbuf_create_with_va(TA_TSP_BUFF_T *pTspBuf,
                                            uintptr_t virAddr,
                                            uint32_t bufSize);
TEE_Result tspbuf_create_alloc(TA_TSP_BUFF_T *pTspBuf,
                                            uint32_t bufSize);
void tspbuf_release(TA_TSP_BUFF_T *pTspBuf);

/* this function is introduced to reduce the redundant  sys call (map/unmap/...) if the two buff
 * belong to one same memID
 */
TEE_Result tspbuf_create_twobuf_with_pa(uintptr_t phyAddr1, uint32_t bufSize1,
                                        uintptr_t phyAddr2, uint32_t bufSize2,
                                        TA_TSP_BUFF_T *pTspBuf1,
                                        TA_TSP_BUFF_T *pTspBuf2);

#endif /* _TEE_TSP_MEM_MAP_H_ */
