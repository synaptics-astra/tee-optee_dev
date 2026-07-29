/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2024 Synaptics Incorporated.
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

#include <stdarg.h>

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#else // Default TZK
#include "tee_internal_core_mem_api.h"
#endif // CONFIG_OPTEE
#include "vmeta_log.h"
#include "tee_internal_api.h"
#include "vpu_api.h"
#include "vpu_obj_mgr.h"
#include "vpu_ta_common.h"

#include "vpu_entry.h"

/*********************** Local types definition Start *************************/
/*********************** Local types definition End   *************************/

/**************** Local variables and macros definition Start *****************/
#ifndef TA_VMETA_UUID
#define TA_VMETA_UUID                                                          \
    {                                                                          \
        0x1316a183, 0x894d, 0x43fe,                                            \
        {                                                                      \
            0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf0                     \
        }                                                                      \
    }
#endif
/***************** Local variables and macros definition End ******************/

/********************** Local function definition Start ***********************/
static inline uint32_t read32(void *addr)
{
#ifdef CONFIG_OPTEE
    return *(volatile uint32_t *)addr;
#else // Default TZK
    uint32_t val;

    asm volatile("ldr %0, %1" : "=r"(val) : "Qo"(*(volatile uint32_t *)addr));

    return val;
#endif // CONFIG_OPTEE
}

static void write32(void *addr, uint32_t val)
{
#ifdef CONFIG_OPTEE
    *(volatile uint32_t *)addr = val;
#else // Default TZK
    asm volatile("str %1, %0" : : "Qo"(*(volatile uint32_t *)addr), "r"(val));
#endif // CONFIG_OPTEE
}

static TEE_Result vpu_clk_ctrl(void *clk_addr, uint32_t enable)
{
    volatile uint32_t val;

    val = read32(clk_addr);
    if (enable) {
        val |= 1;
    } else {
        val &= (~1);
    }
    write32(clk_addr, val);
    return TEE_SUCCESS;
}

static inline int vpu_register_resource(VPU_SESSION_CTX *ctx)
{
    unsigned int vpu_type = ctx->vpu_type;
    unsigned int reg_base, gbl_ctrl_base;
    size_t reg_size, gbl_ctrl_size, clk_offset;
    void *reg_base_va, *gbl_ctrl_va;
    TEE_Result ret;

    /** GLobal Registers
     * libvmeta.ta need access two registers in MEMMAP_CHIP_CTRL_REG region
     *       - 0xf7ea0004 for REG_PRODUCTID_EXT
     *       - 0xf7ea0760 for dynamic change VxG's clock
     * so map [0xf7ea0000, 0xf7eb0000]
     * shrink size from MEMMAP_CHIP_CTRL_REG_SIZE(0x10000) to 0x1000
     */
    gbl_ctrl_base = (unsigned int)MEMMAP_CHIP_CTRL_REG_BASE;
    gbl_ctrl_size = (size_t)(MEMMAP_CHIP_CTRL_REG_SIZE >> 4);
    ret = TEE_Map_Register((paddr_t)gbl_ctrl_base, gbl_ctrl_size, 0, 0, &gbl_ctrl_va);
    if (ret)
        return ret;

    ctx->gbl_ctrl_va = gbl_ctrl_va;

    /**
     * VPU Registers
    */
    switch (vpu_type) {
    case VPU_V2G:
        clk_offset = RA_Gbl_decoderClk;
#ifdef MEMMAP_V4G_REG_BASE
        reg_base = (unsigned int)MEMMAP_V4G_REG_BASE;
        reg_size = MEMMAP_V4G_REG_SIZE;
#else
        return TEE_ERROR_NOT_SUPPORTED;
#endif
        break;
    case VPU_H1_0:
    case VPU_H1_1:
        clk_offset = RA_Gbl_encoderClk;
#ifdef MEMMAP_H1_REG_BASE
        reg_base = (unsigned int)MEMMAP_H1_REG_BASE;
        reg_size = MEMMAP_H1_REG_SIZE;
#else
        return TEE_ERROR_NOT_SUPPORTED;
#endif
        break;
    case VPU_VMETA:
    case VPU_G1:
    default:
        return TEE_ERROR_NOT_SUPPORTED;
    }

    ret = TEE_Map_Register((paddr_t)reg_base, reg_size, 0, 0, &reg_base_va);
    if (ret != TEE_SUCCESS)
        return ret;

    ctx->io_base = reg_base_va;
    ctx->io_size = reg_size;
    ctx->io_base_phy = reg_base;

    ctx->clk_ctrl_va = (void *)((uintptr_t)gbl_ctrl_va + clk_offset);

    TA_LOGI("global 0x%x(%p), reg 0x%x(%p), clk %p\n", gbl_ctrl_base,
            ctx->gbl_ctrl_va, ctx->io_base_phy, ctx->io_base,
            ctx->clk_ctrl_va);
    /**
     * Enable clock
    */
    vpu_clk_ctrl(ctx->clk_ctrl_va, 1);
    return 0;
}
/*********************** Local function definition End ************************/

/********************** Export function definition Start **********************/
int vprintf(const char *format, va_list arg);
int vsprintf(char *str, const char *format, va_list arg);
int vprintf(const char *format, va_list arg)
{
    return 0;
}

int vsprintf(char *str, const char *format, va_list arg)
{
    return 0;
}

TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    TEE_Result ret = TEE_SUCCESS;

    return ret;
}

void TA_EXPORT TA_DestroyEntryPoint(void)
{
    return;
}

TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t parameters_type,
                                              TEE_Param params[4],
                                              void **session_id_ptr)
{
    // common code for open session.
    unsigned int vpu_type = params[0].value.a;
    VPU_SESSION_CTX *session_ctx;
    int ret;

    session_ctx = (VPU_SESSION_CTX *)TEE_Malloc(sizeof(VPU_SESSION_CTX),
                                                TEE_MALLOC_FLAG_DEFAULT);
    if (!session_ctx) {
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto bail;
    }

    TEE_MemFill(session_ctx, 0, sizeof(VPU_SESSION_CTX));

    session_ctx->vpu_type = vpu_type;
    ret = vpu_register_resource(session_ctx);
    if (ret)
        goto bail;

    session_ctx->objMgr = vpuObjMgrCreate();
    if (!session_ctx->objMgr) {
        TEE_Free(session_ctx);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto bail;
    }

    // NOTE: currently we have 2 versions, legacy amp ver=1, kernerl ver=2
    if (params[0].value.b == 2) {
        session_ctx->api_ver = 2;
        InitSessionV2((void *)session_ctx);
    } else {
        session_ctx->api_ver = 1;
        InitSessionV1((void *)session_ctx);
    }

    *session_id_ptr = (void *)session_ctx;

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

void TA_EXPORT TA_CloseSessionEntryPoint(void *session)
{
    VPU_SESSION_CTX *session_ctx = session;

    if (!session_ctx)
        return;

    if (session_ctx->api_ver == 2) {
        DeinitSessionV2(session);
    } else {
        DeinitSessionV1(session);
    }

    if (session_ctx->objMgr) {
        vpuObjMgrDestroy(session_ctx->objMgr);
        session_ctx->objMgr = NULL;
    }

    TEE_Unmap_Register(session_ctx->io_base, session_ctx->io_size);
    TEE_Unmap_Register(session_ctx->gbl_ctrl_va,
                       MEMMAP_CHIP_CTRL_REG_SIZE >> 4);
    TEE_Free(session_ctx);
}

TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void *session,
                                                uint32_t command_id,
                                                uint32_t parameters_type,
                                                TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    VPU_SESSION_CTX *session_ctx = session;

    if (session_ctx == NULL)
        return TEE_ERROR_ITEM_NOT_FOUND;

    if (session_ctx->api_ver == 2) {
        ret = InvokeCommandV2(session_ctx, command_id, parameters_type, params);
    } else {
        ret = InvokeCommandV1(session_ctx, command_id, parameters_type, params);
    }

    return ret;
}
/*********************** Export function definition End ***********************/

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
    .uuid               = TA_VMETA_UUID,
    .name               = "VMeta",
    .singleInstance     = false,
    .multiSession       = false,
    .instanceKeepAlive  = false,
TA_DEFINE_END
#endif // CONFIG_OPTEE
