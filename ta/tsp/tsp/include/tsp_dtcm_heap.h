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

#ifndef _TEE_TSP_DTCM_HEAP_H_
#define _TEE_TSP_DTCM_HEAP_H_

void *dtcm_heap_create(void *mem_start, int mem_size);
void dtcm_heap_release(void *pHeap);
intptr_t dtcm_mem_alloc(void *pHeap, int mem_size, int align_size);
void dtcm_mem_free(void *pHeap, intptr_t mem_start);
int dtcm_heap_is_empty(void *pHeap);

#endif /* _TEE_TSP_DTCM_HEAP_H_ */
