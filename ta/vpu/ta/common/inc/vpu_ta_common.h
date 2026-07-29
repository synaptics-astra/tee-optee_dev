/* SPDX-License-Identifier: Proprietary */
/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2023 Synaptics Incorporated.
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

#ifndef __TEE_TA_VPU_COMMON_H__
#define __TEE_TA_VPU_COMMON_H__

#include <stdio.h>
#include <stdint.h>
#include "vpu_reg_base.h"

typedef struct {
    unsigned int vpu_type;
    unsigned int api_ver;

    void *io_base;
    size_t io_size;
    unsigned int io_base_phy;

    void *gbl_ctrl_va;
    void *clk_ctrl_va;
    /**
     * dict for lookup secure instance context from shm context handle
     * mutable
     */
    void *objMgr;

    uint32_t ctx_mem_id;
    void *fw_ctx_vaddr;
    uint8_t enc_plugin_init;
    void *pRegdlist;
} VPU_SESSION_CTX;

#endif /* __TEE_TA_VPU_COMMON_H__ */
