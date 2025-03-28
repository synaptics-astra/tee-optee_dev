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
//! \file vpu_obj_mgr.c
//!
//! \brief Source file of VPU object manager.
//!
//! Purpose: These APIs will facilitate managing objects in TZ.
//!
//!     Version    Date                     Author
//!     V 1.00,    Feb 24 2014,             Lei Wang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vpu_hal_mem.h"
#include "vpu_obj_mgr.h"
#ifdef TA_COMPILE
#include "vmeta_log.h"
#else
#include "vmeta_sched_api.h"
#endif

#define VPU_OBJ_SLOTS_NUM 32

typedef struct {
    void *key[VPU_OBJ_SLOTS_NUM];
    void *value[VPU_OBJ_SLOTS_NUM];
    int count;
} VPU_OBJECTS_TABLE;

void* vpuObjMgrCreate(void) {
    VPU_OBJECTS_TABLE *pTable = (VPU_OBJECTS_TABLE *) vpu_hal_malloc(sizeof(VPU_OBJECTS_TABLE));
    if (pTable != NULL) {
        vpu_hal_memset(pTable, 0, sizeof(VPU_OBJECTS_TABLE));
    }
    return pTable;
}

void vpuObjMgrDestroy(void *mgr) {
    VPU_OBJECTS_TABLE *pTable = mgr;
    if (pTable == NULL)
        return;

    if (pTable->count > 0) {
        VMETA_LOGE("free ObjMgr while still needed?\n");
    }
    vpu_hal_free(mgr);
    return;
}

int vpuObjMgrSet(void *mgr, void *key, void *value) {
    unsigned int i = 0;
    VPU_OBJECTS_TABLE *pTable = mgr;
    if (pTable == NULL || (key == NULL) || (value == NULL))
        return -1;

    for (i = 0; i < VPU_OBJ_SLOTS_NUM; i++) {
        if (pTable->key[i] == key)
            return -2;
    }

    for (i = 0; i < VPU_OBJ_SLOTS_NUM; i++) {
        if (pTable->key[i] == NULL) {
            pTable->key[i] = key;
            pTable->value[i] = value;
            pTable->count++;
            return 0;
        }
    }
    return -3;
}

void *vpuObjMgrGet(void *mgr, void *key) {
    unsigned int i = 0;
    VPU_OBJECTS_TABLE *pTable = mgr;
    if (pTable == NULL)
        return NULL;

    for (i = 0; i < VPU_OBJ_SLOTS_NUM; i++) {
        if (pTable->key[i] == key) {
            return pTable->value[i];
        }
    }
    return NULL;
}

void *vpuObjMgrRelease(void* mgr, void *key) {
    unsigned int i = 0;
    void *ret;
    VPU_OBJECTS_TABLE *pTable = mgr;
    if (pTable == NULL)
        return NULL;

    for (i = 0; i < VPU_OBJ_SLOTS_NUM; i++) {
        if (pTable->key[i] == key) {
            pTable->key[i] = NULL;
            ret = pTable->value[i];
            pTable->value[i] = NULL;
            pTable->count--;
            return ret;
        }
    }
    return NULL;
}

int vpuObjMgrGetTotalNumber(void *mgr) {
    VPU_OBJECTS_TABLE *pTable = mgr;
    if (pTable == NULL)
        return 0;

    return VPU_OBJ_SLOTS_NUM;
}

void *vpuObjMgrGetWithIndex(void *mgr, unsigned int index) {
    VPU_OBJECTS_TABLE *pTable = mgr;
    if (pTable == NULL)
        return NULL;

    if (index < VPU_OBJ_SLOTS_NUM)
        return pTable->value[index];

    return NULL;
}

