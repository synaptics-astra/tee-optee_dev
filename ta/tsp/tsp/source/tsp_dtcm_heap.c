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
//! \file tsp_dtcm_heap.c
//!
//! \brief TSP DTCM HEAP Implementation
//!
//!
//!     Version     Date                    Author
//!     V 1.00,     OCT 12 2022,            wangzl
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include "tsp_dtcm_heap.h"

#define ADDR_ALIGN(uiAddr, uiAlign)     (uiAddr + uiAlign - 1) & ~(uiAlign - 1);

typedef struct mm_cell_t {
    int occupied;
    uintptr_t start;
    int size;
    struct mm_cell_t *next;
} MM_CELL;

typedef struct mm_heap_t {
    void *mem_start;
    int mem_size;
    MM_CELL head;
} MM_HEAP;

void dtcm_heap_cell_merge(MM_HEAP * m);

void * dtcm_heap_create(void *mem_start, int mem_size)
{
    MM_HEAP *m = malloc(sizeof(MM_HEAP));

    if(m) {
        m->mem_start = mem_start;
        m->mem_size = mem_size;

        m->head.occupied = 0;
        m->head.start = (uintptr_t)mem_start;
        m->head.size = mem_size;
        m->head.next = NULL;
    }
    return m;
}

void dtcm_heap_release(void *pHeap)
{
    MM_HEAP *m = (MM_HEAP *)pHeap;

    if(m) {
        MM_CELL *p = &m->head;
        for( ; p; p = p->next) {
            if(p->occupied) {
                p->occupied = 0;
            }
            if(m->head.next == NULL) break;
        }
        dtcm_heap_cell_merge(m);
        free(m);
    }
}

intptr_t dtcm_mem_alloc(void *pHeap, int mem_size, int align_size)
{
    MM_HEAP *m = (MM_HEAP *)pHeap;

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
                        printf("mm_cell malloc denied\n");
                        break;
                    }

                    p->next->occupied = 0;
                    p->next->start = p->start + mem_size;
                    p->next->size = p->size - mem_size;
                    p->next->next = p1;

                    p->size = mem_size;
                }
                p->occupied = 1;
                return p->start;
            }
        }
    }
    return -1;
}

void dtcm_mem_free(void *pHeap, intptr_t mem_start)
{
    MM_HEAP *m = (MM_HEAP *)pHeap;

    if(m && mem_start >= 0) {
        MM_CELL * p = &m->head;
        for( ; p; p = p->next) {
            if(p->occupied && p->start == (uintptr_t)mem_start) {
                p->occupied = 0;
                dtcm_heap_cell_merge(m);
                break;
            }
        }
    }
}

void dtcm_heap_cell_merge(MM_HEAP *m)
{
    if(m)
    {
        MM_CELL *p1 = &m->head, *p2 = m->head.next;

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

int dtcm_heap_is_empty(void *pHeap)
{
    MM_HEAP *m = (MM_HEAP *)pHeap;
    MM_CELL *p = NULL;

    if (m) {
        p = &m->head;
        for ( ; p; p = p->next) {
            if (p->occupied) {
                return 0;
            }
        }
    }
    return 1;
}

