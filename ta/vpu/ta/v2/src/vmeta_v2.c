// SPDX-License-Identifier: Proprietary
/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2022-2023 Synaptics Incorporated.
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
//! \file entry.c
//! \brief vmeta ta v2 code.
//!
//! Purpose:
//!     This module handle vmeta tz v2 commands.
//!
//!  Note:
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>

#include <tee_internal_api.h>

#include "macros_m.h"
#include <vpu_obj_mgr.h>
#include <vpu_version.h>
#include <vpu_dbg.h>
#include <vmeta_log.h>
#include <vpu_ta_common.h>

#include <queue.h>
#include "mem_utils.h"
#include "vpu_fw_data.h"
#include "vpu_entry.h"

#ifdef CONFIG_OPTEE
#include "vpu_plugin.h"
#endif // CONFIG_OPTEE

#define VPU_TA_VERSION_MINOR        (26U)
#define VPU_TA_ENC_API_VER_MINOR    (26U)
#define VPU_TA_DEC_API_VER_MINOR    (36U)

#define IS_TILE_OUTPUT_CFG(dec_cfg) \
    (1 == dec_cfg->tile_ref_flag && \
     dec_cfg->output_mode == VDEC_OUTPUT_TILE)

#define IS_MULTI_FORMATS_CFG(dec_cfg) \
    ((1 == dec_cfg->tile_ref_flag && \
     VDEC_OUTPUT_SPUV == dec_cfg->output_mode) || \
     dec_cfg->output_mode == VDEC_OUTPUT_TILE_420SP_AUTO || \
     dec_cfg->output_mode == VDEC_OUTPUT_MULTI_CHANNEL)

#define STREAM_PART_SEQ_HEADER      (2U)
#define ES_HEADER_BUF_LEN           (1024U)

#define VPRO_SOC_CLK_BIT_MASK       (0x0000000C)
#define VENC_FMT_ID_VP8 ('\0' << 24 | 'V' << 16 | 'P' << 8 | '8')

/* each type buf slots would have the same size */
#define N_BUFSLOTS          (ARRAY_SIZE(((struct syna_vpu_ctrl *)0)->inbufs))

#define IS_AUX_POOL_ENABLED(ctx)            (ctx->fw_pixel_mode == VDEC_OUTPUT_SPUV)

#define TEST_BIT(bit_idx, mask) ((0x1 << (bit_idx)) & (mask))
#define SET_BIT(bit_idx, mask) ((mask) |= (0x1 << (bit_idx)))
#define CLR_BIT(bit_idx, mask) ((mask) &= ~(0x1 << (bit_idx)))

#define VPU_FLUSH_BUF_IDX           (MAX_SLOT - 1)
#if 0
#define VPU_BUF_TRACE               TA_LOGL
#else
#define VPU_BUF_TRACE(...)
#endif

#if 0
#define VPU_CMD_HANDER_TRACE        TA_LOGL
#else
#define VPU_CMD_HANDER_TRACE(...)
#endif

QUEUE(fifo_p, uintptr_t, 32)
/***************************** debug feature macro region BEGIN **********************************/
// enable secure buffer checking for TZ 4.0
#define MAX_PARAM       (4U)

/***************************** debug feature macro region END ************************************/

enum {
    VPU_CALLBACK_INVALID,
    VPU_CALLBACK_PANIC,
    VPU_CALLBACK_WAITINT,
    VPU_CALLBACK_EVENT,
    VPU_CALLBACK_H1_WAIT_INT_EVENT,
};

typedef enum {
    eUnknown,
    eInput,
    eOutput,
    eForbidden,
    eVpuId, // enum, should in [0, VPU_HW_IP_NUM)
    eNum,
} IOType;

typedef enum {
    // fixed size
    eVpuConstSize,
    eVdecStrmCfgSize,
    eVencStrmCfgSize,
    eSynaVpuCtrlSize,
} BufferSizeType;

/* bitstream buf for the decoder, we need to store base addr in secure
 * region in case of the overflow attack.
 */


#define MAX_SLOT 32
typedef struct i_vdec_pool_t {
    struct i_vdec_bs_buf  strm_desc[MAX_SLOT];
    struct vdec_vid_buf_desc_t  ref_desc[MAX_SLOT];
    struct vdec_vid_buf_desc_t  dis_desc[MAX_SLOT];
}i_vdec_pool_t;

typedef struct i_venc_pool_t {
    struct venc_strm_buf_desc_t strm_desc[MAX_SLOT];
    struct venc_vid_buf_desc_t  vid_desc[MAX_SLOT];
    struct venc_vid_buf_desc_t  ref_desc[MAX_SLOT];
    struct venc_vid_buf_desc_t  ds_desc[MAX_SLOT];
}i_venc_pool_t;

typedef struct {
    IOType iotype;
    uint32_t perm;
    TEE_MemoryOperation op;
    BufferSizeType buftype;
} CheckParam;

typedef struct {
    bool        got_header;
    uint8_t     header[ES_HEADER_BUF_LEN];
    uint32_t    header_len;
} i_venc_head_mgr_t;

struct i_vpu_ctx {
    /* self */
    struct syna_tz_generic_buf buf;

    struct syna_tz_generic_buf ctx_hw_buf;
    /* point to a FW instance context */
    void *strm_ctx;
    void *desc_pool;
    /**
     * We don't know which slot is occupied, we didn't request the
     * register and unregister should follow an order. So we made
     * use binary flags here to store the buffer security attribute
     * for each type(input, output, auxiliary) of buffer.
     */
    uint32_t in_sec_flags;
    uint32_t in_insec_flags;
    uint32_t out_sec_flags;
    uint32_t out_insec_flags;
    uint32_t aux_sec_flags;
    uint32_t aux_insec_flags;
    uint32_t ds_sec_flags;
    uint32_t ds_insec_flags;

    unsigned int codec_fmt;
    /* decoder output format and encoder input format */
    unsigned int fw_pixel_mode;
    /* For timestamp */
    struct queue_fifo_p src_bufs;
    /**
     * display bufs for decoder timestamp
     * NOTE: AV1 won't use this
     */
    struct queue_fifo_p sink_bufs;

    /* v4g decoder mtr buf based */
    struct syna_tz_generic_buf mtr_buf;
    /* whether this instance is a secure context */
    bool secure;
    bool is_encoder;

    uint32_t cur_encoding_buf_idx;
    bool     ltr_en;
    /**
     * Whether to merge header, SPS/PPS are separated with slice data by
     * default.
     */
    bool merge_header;
    bool enc_ds_en;
    i_venc_head_mgr_t header_mgr;
    bool vp8_enc;
    bool meta_mv_en;
};

typedef struct __MV_Info_t {
    uint8_t mbMode; /*6:P16x16, 7:P16x8, 8:P8x16, 9:P8x8*/
    uint8_t reserved;
    uint16_t mbCount;
    int8_t mvY[4];
    int16_t mvX[4];
}MVInfo_t;

typedef struct {
    uint32_t command_id;
    uint32_t (*cmd_handler) (VPU_SESSION_CTX *session_ctx,
                             uint32_t parameters_type, TEE_Param params[4]);
    uint32_t typesExpect;
    uint32_t paramsNum;
    CheckParam params[MAX_PARAM];
} TA_CMD_HANDLER;

static uint32_t getBufSize(BufferSizeType buftype)
{
    switch (buftype) {
        case eVpuConstSize:
            return ALIGN(sizeof(struct syna_vpu_fw_data), 4096);
        case eVdecStrmCfgSize:
            return ALIGN(sizeof(vdec_strm_config_t), 4096);
        case eVencStrmCfgSize:
            return ALIGN(sizeof(venc_strm_config_t), 4096);
        case eSynaVpuCtrlSize:
            return ALIGN(sizeof(struct syna_vpu_ctrl), 4096);
        default:
            break;
    }

    return 0; // fail at buffer size checking
}

static int merge_header(struct i_vpu_ctx *ctx, uint32_t idx)
{
    venc_strm_buf_desc_t *buf_desc = NULL;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    uint8_t *dest = NULL;
    uint8_t *src = NULL;

    buf_desc = &pool->strm_desc[idx];
    buf_desc->id = idx;

    dest = (uint8_t *)buf_desc->vaddr + ctx->header_mgr.header_len;
    src = (uint8_t *)buf_desc->vaddr;

    memmove(dest, src, buf_desc->valid_data);
    memcpy(buf_desc->vaddr, ctx->header_mgr.header, ctx->header_mgr.header_len);
    TEE_CleanCache((void *)buf_desc->vaddr, (size_t)buf_desc->size);

    buf_desc->valid_data += ctx->header_mgr.header_len;

    VPU_BUF_TRACE("[%s():%d], header size:%lu\n", __func__, __LINE__,
                  ctx->header_mgr.header_len);

    ctx->header_mgr.got_header = false;
    ctx->header_mgr.header_len = 0;

    return 0;
}

static inline u32 read32(const volatile void *addr)
{
    u32 val;
    asm volatile("ldr %0, %1"
                 : "=r" (val)
                 : "Qo" (*(volatile u32 *)addr));
    return val;
}

static inline void write32(volatile void *addr, uint32_t val)
{
    asm volatile("str %1, %0"
                 : : "Qo" (*(volatile u32 *)addr), "r" (val));
}

static void
vMetaMem32Read(void *pa, uint32_t n, uint32_t *buf, void *user_data)
{
    uint32_t i, *src, *dst;

    src = pa;
    dst = buf;
    for(i=0; i<n; i++)  *dst++ =*src++;
}

static void
vMetaMem32Write(void *pa, uint32_t n, uint32_t *buf, void *user_data)
{
    uint32_t i, *src, *dst;

    dst = pa;
    src = buf;
    for(i=0; i<n; i++)  *dst++ =*src++;
}

static void sl2_buf_drain(uint32_t pa, uint32_t size, void *va)
{
    TEE_CleanCache((void *)va, (size_t)size);
    return;
}

static void sl2_buf_request(uint32_t pa, uint32_t size, void *va)
{
    TEE_InvalidateCache((void *)va, (size_t)size);
    return;
}

static void vpuPanic(uint32_t code)
{
    VMETA_LOGF("firmware panic with code=%d", code);
    return;
}

static inline void idx_queue_store(struct idx_queue *q, uint32_t key,
                                   uint32_t value)
{
    q->index[key] = value;
    q->pop++;
}

static uint32_t idx_queue_load(struct idx_queue *q, uint32_t key)
{
    return q->index[key];
}

static inline void idx_queue_clear(struct idx_queue *q)
{
    memset(q, 0, sizeof(*q));
}

static inline int release_internal_ctx(VPU_SESSION_CTX *session,
                                       struct syna_vpu_ctrl *ctrl,
                                       struct i_vpu_ctx **ctx)
{
    *ctx = (struct i_vpu_ctx *) vpuObjMgrRelease(session->objMgr,
                                                 (void *)((uintptr_t)
                                                 ctrl->ctrl_buf.addr));
    if (!(*ctx))
        return TEE_ERROR_ITEM_NOT_FOUND;

    if (*ctx == (struct i_vpu_ctx *)(ctrl->ctx_buf.vaddr + ctrl->ctx_buf.offset))
        return 0;

    /* memory corrupted */
    return TEE_ERROR_CORRUPT_OBJECT;
}

static inline int get_internal_ctx(VPU_SESSION_CTX *session,
                                   struct syna_vpu_ctrl *ctrl,
                                   struct i_vpu_ctx **ctx)
{
    *ctx = (struct i_vpu_ctx *) vpuObjMgrGet(session->objMgr,
                                             (void *)((uintptr_t)
                                             ctrl->ctrl_buf.addr));
    if (!(*ctx))
        return TEE_ERROR_ITEM_NOT_FOUND;

    if (*ctx == (struct i_vpu_ctx *)(ctrl->ctx_buf.vaddr + ctrl->ctx_buf.offset))
        return 0;

    /* memory corrupted */
    return TEE_ERROR_CORRUPT_OBJECT;
}

static void get_motion_vector(struct syna_vpu_ctrl *ctrl, uint32_t idx,
                              venc_seq_desc_t *seq_desc) {
    uint32_t mbInfoSize = 0;
    uint32_t mv_size = sizeof(MVInfo_t);
    const vpu_hw_info_t *p_vpu_info = vpu_get_hw_info();
    /**
    * https://synaptics.atlassian.net/wiki/spaces/IOT/pages/40071266437/encoder+Motion+vector
    */
    if (p_vpu_info->hw_version == 0x70000) // 680 h1v5
        mbInfoSize = 32;
    else if (p_vpu_info->hw_version == 0x70001) // 640 h1v7
        mbInfoSize = 48;

    uint32_t mbPerRow = (seq_desc->width + 15) / 16;
    uint32_t mbPerCol = (seq_desc->height + 15) / 16;
    uint32_t mbPerFrame = mbPerRow * mbPerCol;
    struct syna_vpu_tz_buf *buf = &ctrl->outbufs[idx];
    char *dst = (char *)buf->planes[1].vaddr;
    char *mbInfo = seq_desc->mb_info;
    if (!dst || !mbInfo) {
        TA_LOGL("err: no vir addr for plane1 %p %p %p\n", dst, seq_desc,
                mbInfo);
        return;
    }
    if (buf->planes[1].size < mbPerFrame * mv_size) {
        TA_LOGL("err: no enough space for motion vector\n");
        return;
    }
    //copy first 16 bytes for motion vector
    for (uint32_t i = 0; i < mbPerFrame; i++) {
        memcpy(dst, mbInfo, mv_size);
        dst += mv_size;
        mbInfo += mbInfoSize;
    }
    buf->bytesused[1] = mbPerFrame * mv_size;
    VMETA_LOGD("get motion vector done (%d * %d) = %d bytes\n", mbPerFrame,
            mv_size, mbPerFrame * mv_size);
    return;
}
/*
 * validate the security rules of a syna_vpu_tz_buf
 * @return return true when the buffer follows all the security
 *         restrictions in the current validation level option.
 *         else return false.
 * @param buf
 * @param level level 0 means only check the security attributes of the memid
 */
static bool syna_vpu_tz_buf_sec_validate(struct syna_vpu_tz_buf * const buf,
                                         unsigned int level)
{
    unsigned int i;
    bool ret = false;

    for (i = 0; i < ARRAY_SIZE(buf->planes); i++) {
        /* MVTP or MTR could be in the non-secure region */
        if (i > 1)
            goto bail;

#ifndef ENABLE_SMM_TA
        /* TODO: ptm */
        if (buf->planes[i].type == MEM_ID_SCATTER && (!buf->planes[i].memid))
            return false;
#endif

        if (buf->planes[i].memid && buf->planes[i].size) {
            ret = TEE_Check_Memory_Attr(buf->planes[i].memid,
                                        MEMATTR_SECURE);
            if (!ret)
                goto bail;
        }
    }

bail:
    return ret;
}

static TEE_Result internal_get_FW_const(struct syna_vpu_fw_info* dest,
                                      const vpu_const_t* src,
                                      uint32_t hw_version)
{
    dest->vpu_context_size = src->vpu_context_size;
    dest->vpu_hw_context_size = src->vpu_hw_context_size;

    dest->vdec_strm_context_size = src->vdec_strm_context_size;
    dest->vdec_strm_hw_context_size = src->vdec_strm_hw_context_size;

    static_assert(N_BUFSLOTS == (1 << 5), "number of buf slot changed!");
    dest->vdec_pool_size = sizeof(struct i_vdec_pool_t);

    dest->venc_strm_context_size = src->venc_strm_context_size;
    dest->venc_strm_hw_context_size = src->venc_strm_hw_context_size;
    dest->venc_pool_size = sizeof(struct i_venc_pool_t);

    dest->hw_version = hw_version;
    dest->version_string = (uintptr_t)src->version_string;

    VMETA_LOGI("VPU lib version_string=%s, productId=0x%x, hw_version=0x%x "
               "vpu %d,%d vdec %d,%d venc %d,%d, pool vdec %d, venc %d\n",
               src->version_string, dest->rsvd, dest->hw_version,
               dest->vpu_context_size, dest->vpu_hw_context_size,
               dest->vdec_strm_context_size, dest->vdec_strm_hw_context_size,
               dest->venc_strm_context_size, dest->venc_strm_hw_context_size,
               dest->vdec_pool_size, dest->venc_pool_size);

    return TEE_SUCCESS;
}

static void internal_get_HW_const(struct syna_vpu_hw_info* dest,
                               const vpu_hw_info_t* src)
{
    dest->hw_version = src->hw_version;
    dest->version_string = 0;//don't pass string to NTW
    dest->vmeta_enable = src->vmeta_enable;
    dest->v2g_enable = src->v2g_enable;
    dest->g2_enable = src->g2_enable;
    dest->g1_enable = src->g1_enable;
    dest->h1_0_enable = src->h1_0_enable;
    dest->h1_1_enable = src->h1_1_enable;
}

static TEE_Result wrap_vpu_get_const(struct syna_vpu_fw_data *data,
                                     uint32_t vpu_type)
{
    const vpu_hw_info_t *p_vpu_info = vpu_get_hw_info();
    const struct vpu_const_t *p;
    uint32_t hw_version = p_vpu_info->hw_version;
    uint32_t fw_version;
    TEE_Result res = TEE_SUCCESS;

    VMETA_LOGI(
        "vmeta build fw_version='%s'/'%s' sizeof(vdec_seq_desc_t)=%d tz=%s",
        p_vpu_info->fw_version, p_vpu_info->version_string,
        sizeof(vdec_seq_desc_t), "5.0");

    fw_version = atoi((char *)p_vpu_info->fw_version);
    if (fw_version != VPU_LIB_VERSION) {
        VMETA_LOGF("error version mismatch firmware version %d != header file "
                   "version %d",
                   fw_version, VPU_LIB_VERSION);
        res = TEE_ERROR_NOT_SUPPORTED;
        return res;
    }

    internal_get_HW_const(&data->hw_info, p_vpu_info);
    /* fw team set C string from svn */
    data->hw_info.fw_version = (uintptr_t)VPU_LIB_VERSION;

    switch (vpu_type) {
    case VPU_H1_0:
    case VPU_H1_1:
        if (!p_vpu_info->h1_0_enable)
            return TEE_ERROR_NOT_SUPPORTED;
        break;
    case VPU_V2G:
        if (!p_vpu_info->v2g_enable)
            return TEE_ERROR_NOT_SUPPORTED;
        break;
    default:
        return TEE_ERROR_NOT_SUPPORTED;
        break;
    }

    p = vpu_get_const(vpu_type);
    res = internal_get_FW_const(&data->fw_info, p, hw_version);
    if (res)
        VMETA_LOGE("fail to get_const for type 0x%x res=%x", vpu_type, res);

    return res;
}

static TEE_Result ta_vpu_get_const(VPU_SESSION_CTX * session_ctx,
                                   uint32_t parameters_type,
                                   TEE_Param params[4])
{
    uint32_t api_major, api_minor;
    uint32_t vpuId;
    size_t size;
    int ret;
    struct syna_vpu_fw_data *vpu_const = NULL;

    vpu_const = (struct syna_vpu_fw_data *)params[0].memref.buffer;
    size = (uint32_t)params[0].memref.size;
    if (!vpu_const || size != getBufSize(eVpuConstSize)) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    vpuId = session_ctx->vpu_type;
    ret = wrap_vpu_get_const(vpu_const, vpuId);
    if (ret)
        goto bail;

    switch (vpuId) {
    case VPU_V2G:
        api_major = VPU_TA_DEC_API_VER_MAJOR;
        api_minor = VPU_TA_DEC_API_VER_MINOR;
        break;
    case VPU_H1_0:
    case VPU_H1_1:
        api_major = VPU_TA_ENC_API_VER_MAJOR;
        api_minor = VPU_TA_ENC_API_VER_MINOR;
        break;
    default:
        api_major = 0;
        api_minor = 0;
        break;
    }

    params[1].value.a = VPU_TA_VERSION_MAKE(VPU_TA_VERSION_MAJOR,
                                            VPU_TA_VERSION_MINOR,
                                            api_major,
                                            api_minor);
bail:
    params[1].value.b = ret;
    return TEE_SUCCESS;
}

static void vdec_tee_event_cb(void *strm_cntxt, void *strm_status)
{
    /* Not used */
    return;
}

#ifdef CONFIG_OPTEE
static TEE_Result tee_cb_plugin_ping(int cmd, int sub_cmd)
{
    TEE_Result ret = TEE_SUCCESS;
    TEE_UUID uuid = VPU_PLUGIN_UUID;

    ret = tee_invoke_supp_plugin(&uuid, cmd, sub_cmd, NULL, 0, NULL);
    if (ret)
        VMETA_LOGE("invoke plugin failed with code 0x%x", ret);

    return ret;
}
#endif // CONFIG_OPTEE

static void venc_tee_event_cb(void *strm_cntxt, void *strm_status)
{
    venc_strm_status_t *venc_strm_status = (venc_strm_status_t*)strm_status;
    struct syna_vpu_ctrl *ctrl = (void *)venc_strm_status->user_data;

    if (venc_strm_status->wait_for_int) {
#ifdef CONFIG_OPTEE
        ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INTERRUPTER;
        tee_cb_plugin_ping(H1_PLUGIN_CMD_POLL_INT, 0);
#else
        uint32_t cb_cmd_id = VPU_CALLBACK_H1_WAIT_INT_EVENT;
        uint32_t cb_param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
                TEE_PARAM_TYPE_NONE,
                TEE_PARAM_TYPE_NONE,
                TEE_PARAM_TYPE_NONE);
        TEE_Param cb_params[4];

        /* NOTE: If this flag is not set, CA should not wait for IRQ */
        ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INTERRUPTER;

        /* CA doesn't really need those two */
        cb_params[0].value.a = (uint32_t)venc_strm_status->user_data;
        cb_params[0].value.b = (uint32_t)venc_strm_status;

        TEE_Callback(cb_cmd_id, cb_param_type, cb_params, 0, NULL);
#endif
    }
}

static uint32_t h1_malloc(uint32_t size)
{
    return (uintptr_t) TEE_Malloc(size, TEE_MALLOC_FLAG_DEFAULT);
}

static void h1_free(void* vaddr)
{
    TEE_Free(vaddr);
}

static TEE_Result ta_vpu_open(VPU_SESSION_CTX * session_ctx,
                              uint32_t parameters_type, TEE_Param params[4])
{
    uint32_t vpu_type = session_ctx->vpu_type;
    const struct vpu_const_t *p;
    vpu_config_t *vpu_cfg;
    uint32_t memid;
    size_t size, ctx_buf_size, hw_ctx_size;
    size_t ctx_buf_off, hw_ctx_off;
    int ret;
    struct syna_tz_generic_buf ctx_mem = {0};

    if (session_ctx->ctx_mem_id) {
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    memid = params[0].value.a;
    size = params[0].value.b;

    p = vpu_get_const(vpu_type);
    ctx_buf_off = ALIGN(BERLIN_VPU_CFG_RESERVED_SIZE, 4096);
    hw_ctx_off = ctx_buf_off + ALIGN(p->vpu_context_size, 4096);
    hw_ctx_size = ALIGN(p->vpu_hw_context_size, 4096);
    ctx_buf_size = hw_ctx_off + hw_ctx_size;

    if (memid == 0 || size < ctx_buf_size) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    TZMEM_INIT_MEMID(&ctx_mem, MEM_ID_CONTINOUS, memid, size, 0, 0);
    ret = vpu_tzmem_get(&ctx_mem);
    if (ret)
        goto bail;

    session_ctx->ctx_mem_id = memid;

    session_ctx->fw_ctx_vaddr = (void *)(ctx_mem.vaddr + ctx_buf_off);

    vpu_cfg = (vpu_config_t *)ctx_mem.vaddr;
    vpu_cfg->hw_context_addr = ctx_mem.addr + hw_ctx_off;
    vpu_cfg->hw_context_vaddr = (void *)(ctx_mem.vaddr + hw_ctx_off);

    vpu_cfg->mem32_read = vMetaMem32Read;
    vpu_cfg->mem32_write = vMetaMem32Write;
    vpu_cfg->sl2_buf_drain = sl2_buf_drain;
    vpu_cfg->sl2_buf_request = sl2_buf_request;
    vpu_cfg->dbg_panic = vpuPanic;
    vpu_cfg->dbg_printf = vMetaFwPrint;
    //firmware release version from svn29742_20191114
    //will default output @err, if dbg_print=null
    //then error log will output to console
    //vpu_cfg->dbg_printf = NULL;
    vpu_cfg->dbg_ahblog = NULL;
    vpu_cfg->hw_id = vpu_type;
    vpu_cfg->event_callback = vdec_tee_event_cb;
    vpu_cfg->mem_malloc = h1_malloc;
    vpu_cfg->mem_free = h1_free;
    vpu_cfg->low_v2g_clock = NULL;
    vpu_cfg->high_v2g_clock = NULL;

    vpu_cfg->io_base = (uintptr_t)session_ctx->io_base;
    vpu_cfg->fast_io_base = (uintptr_t)session_ctx->io_base_phy;
    vpu_cfg->external_clock_disable = NULL; // not used for vxg

    if(VPU_H1_0 != vpu_type) {
        ret = vpu_open(session_ctx->fw_ctx_vaddr, vpu_cfg);
    } else {
        vpu_cfg->event_callback = venc_tee_event_cb;
        ret = venc_open(session_ctx->fw_ctx_vaddr, vpu_cfg);
    }

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vpu_close(VPU_SESSION_CTX * session_ctx,
                               uint32_t parameters_type, TEE_Param params[4])
{
    uint32_t memid;
    int32_t ret;

    ret = vpu_close(session_ctx->fw_ctx_vaddr);

    memid = session_ctx->ctx_mem_id;
    TEE_Unmap_Memory(memid);
    TEE_MemUnref(memid);
    session_ctx->ctx_mem_id = 0;

#ifdef CONFIG_OPTEE
    if (session_ctx->vpu_type == VPU_H1_0)
        tee_cb_plugin_ping(H1_PLUGIN_CMD_DEINIT, 0);
#endif // CONFIG_OPTEE

    session_ctx->enc_plugin_init = 0;

    params[0].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_create_instance(VPU_SESSION_CTX * session_ctx,
                                          uint32_t parameters_type,
                                          TEE_Param params[4])
{

    struct syna_vpu_ctrl *ctrl;
    const struct vpu_const_t *vc;
    struct i_vpu_ctx *ctx;
    struct syna_tz_generic_buf tzmem = {0};
    uint32_t ctrl_size, ctx_mem_id, ctx_mem_size;
    size_t size, ctx_size, ctx_hw_size;
    TEE_Result ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;
    ctrl_size = (uint32_t)params[0].memref.size;
    if (ctrl == 0 || ctrl->ctrl_buf.addr == 0 || (ctrl_size != getBufSize(eSynaVpuCtrlSize))) {
        VMETA_LOGE("invalid param, ctrl 0x%x, addr 0x%x, ctrl_size 0x%x(0x%x)\n", ctrl,
            ctrl->ctrl_buf.addr, ctrl_size, getBufSize(eSynaVpuCtrlSize));
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    ctrl->ctrl_buf.memid = 0;
    ctrl->ctrl_buf.vaddr = (uintptr_t)ctrl;
    ctrl->ctrl_buf.size = ctrl_size;
    //check if a valid ctrl
    if (vpuObjMgrGet(session_ctx->objMgr, (void *)((uintptr_t)ctrl->ctrl_buf.addr))) {
        VMETA_LOGE("no release pVDCtx in sec_ctxs");
        ret = TEE_ERROR_BAD_STATE;
        goto failed_ctrl;
    }

    //get ctx mem
    ctx_mem_id = ctrl->ctx_buf.memid;
    ctx_mem_size = ctrl->ctx_buf.size;

    vc = vpu_get_const(session_ctx->vpu_type);
    ctx_size = ALIGN(vc->vdec_strm_context_size, 4096);
    ctx_hw_size = ALIGN(vc->vdec_strm_hw_context_size, 4096);
    size = BERLIN_VPU_CTX_RESERVED_SIZE + ctx_size + ctx_hw_size;
    if (ctx_mem_size < size) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto failed_ctrl;
    }

    TZMEM_INIT_MEMID(&tzmem, MEM_ID_CONTINOUS, ctx_mem_id, ctx_mem_size, 0, 0);
    ret = vpu_tzmem_get(&tzmem);
    if (ret) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto failed_ctrl;
    }
    ctx = (struct i_vpu_ctx *)tzmem.vaddr;
    TEE_MemFill(ctx, 0, BERLIN_VPU_CTX_RESERVED_SIZE);
    TEE_MemMove(&(ctx->buf), &tzmem, sizeof(tzmem));
    TEE_MemMove(&(ctrl->ctx_buf), &tzmem, sizeof(tzmem));
    ctx->is_encoder = false;

    ctx->desc_pool = (void *)((uintptr_t)ctx + size);
    size += sizeof(i_vdec_pool_t);

    size = ALIGN(size, 4096);
    /* double check or check the whole size */
    if (size > ctx->buf.size) {
        VMETA_LOGF("secure context buf size is not enough, want: %d", size);
        goto failed_ctx;
    }
    /* Fixed offset */
    ctx->strm_ctx = ((void *)((uintptr_t)ctx + BERLIN_VPU_CTX_RESERVED_SIZE));
    ctx->ctx_hw_buf.addr = ctx->buf.addr + BERLIN_VPU_CTX_RESERVED_SIZE
                                         + ctx_size;
    ctx->ctx_hw_buf.vaddr = (uintptr_t)((uintptr_t)ctx->strm_ctx + ctx_size);
    ctx->ctx_hw_buf.size = ctx_hw_size;

    if (vpuObjMgrSet(session_ctx->objMgr, (void *)((uintptr_t)ctrl->ctrl_buf.addr), ctx) < 0) {
        VMETA_LOGF("Fail to save vdec secure context: %p", ctx);
        ret = TEE_ERROR_BAD_STATE;
        goto failed_ctx;
    }

    params[1].value.a = ret;
    return TEE_SUCCESS;

/* failure */
failed_ctx:
    vpu_tzmem_put(&ctx->buf);
failed_ctrl:
    vpu_tzmem_put(&ctrl->ctrl_buf);
bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_config_stream(VPU_SESSION_CTX * session_ctx,
                                        uint32_t parameters_type,
                                        TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    struct vdec_strm_config_t *pStrmcfg;
    size_t size;
    int32_t ret;

    /**
     * The input parameter should be a per-TA-instance only virtual address,
     * which is registered at create_instance() stage. Or we can't access
     * it at all. It is the only insecure buffer this instance of TA
     * could access. So if user input a guess secure virtual address,
     * it doesn't matter, REE can't access it.
     * And in get_internal_ctx() we would check whether it matches
     * the address we stored in the internal secure context's buffer.
     * If we need to access any sensetive data from it, we would copy
     * them to the stack, likes this function and ta_vpu_reg_buf().
     */
    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }
    /* try to access the temporary memmory first */
    pStrmcfg = (struct vdec_strm_config_t *)params[1].memref.buffer;
    size = params[1].memref.size;
    if (size != getBufSize(eVdecStrmCfgSize)) {
        VMETA_LOGE("invalid param, size=%d, getBufSize(eVencStrmCfgSize)=%d\n",
                   size, getBufSize(eVdecStrmCfgSize));
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    pStrmcfg->mmu_enable = 0;
    /* NOTE: flag mapping */
    if (ctrl->v4g_ext_cfg.mmu.en)
        pStrmcfg->mmu_enable = VXG_MMU_VM_ENABLE;
    if (ctrl->v4g_ext_cfg.mmu.bank_mapping)
        pStrmcfg->mmu_enable |= VXG_MMU_BM_ENABLE;
    if (ctrl->v4g_ext_cfg.mmu.page_mapping) {
        pStrmcfg->mmu_enable |= (VXG_MMU_PM_ENABLE
                             | (ctrl->v4g_ext_cfg.mmu.pm_h_log2_minus2
                                     * VXG_MMU_PM_PH_8)
                             | (ctrl->v4g_ext_cfg.mmu.pm_w_log2_minus6
                                     * VXG_MMU_PM_PW_128));
    }
    if (ctrl->v4g_ext_cfg.mmu.shuffle)
        pStrmcfg->mmu_enable |= VXG_MMU_SH_ENABLE;

    /* From vpu_select_output_mode() Tile setting */
    if (VDEC_OUTPUT_TILE == pStrmcfg->output_mode) {
        pStrmcfg->force_ref_out = 1;
        pStrmcfg->mtuTile = 6;
        pStrmcfg->mtuDisp = 6;

        pStrmcfg->pp_vscale = 1;
        pStrmcfg->pp_hscale_numer = 6;
        pStrmcfg->pp_hscale_denom = 6;
    }

    if (pStrmcfg->mtr_enable) {
        if (ctrl->mtr_base.memid || ctrl->mtr_base.addr) {
            TEE_MemMove(&(ctx->mtr_buf), &ctrl->mtr_base,
                        sizeof(struct syna_tz_generic_buf));

            ctx->mtr_buf.gfp_flags |= BERLIN_GFP_FLAG_NO_MAPPING;
            ret = vpu_tzmem_get(&ctx->mtr_buf);
            if (ret) {
                VMETA_LOGF("can't register mtr base pool");
                goto bail;
            }
            pStrmcfg->mtr_meta_base = ctx->mtr_buf.addr
                                         + ctx->mtr_buf.offset;
        } else {
            pStrmcfg->mtr_meta_base = 0UL;
        }
    }

    ctx->fw_pixel_mode = pStrmcfg->output_mode;

    if (!pStrmcfg->user_data) {
        VMETA_LOGI("opt_params: dv=%d,dvMode=%d,hdr10+=%d,noReorder=%d,lowLatency=%d\n",
            ctrl->v4g_ext_cfg.opt.dvEn, ctrl->v4g_ext_cfg.opt.dvMode,
            ctrl->v4g_ext_cfg.opt.hdr10PlusEn, ctrl->v4g_ext_cfg.opt.noReorder,
            ctrl->v4g_ext_cfg.opt.lowLatencyEn);

        ctx->codec_fmt = pStrmcfg->format;

        pStrmcfg->user_data = ctrl;
        pStrmcfg->vdec_cntxt = session_ctx->fw_ctx_vaddr;
        pStrmcfg->hw_context_addr = ctx->ctx_hw_buf.addr
                                       + ctx->ctx_hw_buf.offset;
        pStrmcfg->hw_context_vaddr = (void *)(ctx->ctx_hw_buf.vaddr
                                        + ctx->ctx_hw_buf.offset);
        ret = vdec_open_stream(ctx->strm_ctx, pStrmcfg);
    } else if (ctrl == pStrmcfg->user_data) {
        ret = vdec_configure_stream(ctx->strm_ctx, pStrmcfg);
    } else {
        /* unlikely */
        VMETA_LOGE("corrupted config or instance\n");
        ret = TEE_ERROR_CORRUPT_OBJECT;
    }

bail:
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

static int vdec_push_es_buf(struct i_vpu_ctx *ctx, struct syna_vpu_ctrl *ctrl,
                            uint32_t idx)
{
    struct syna_vpu_tz_buf *buf;
    struct i_vdec_bs_buf *bs_buf;
    vdec_strm_buf_desc_t *input_desc;
    size_t offset, bytesused;
    int ret;
    i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

    bs_buf = &pool->strm_desc[idx];
    input_desc = &(bs_buf->desc);

    buf = &(ctrl->inbufs[idx]);
    offset = buf->planes[0].offset;
    bytesused = buf->bytesused[0];
    /* prevent the overflow attack */
    if (offset > bs_buf->buf.size)
        return TEE_ERROR_BAD_PARAMETERS;
    if (bytesused > bs_buf->buf.size)
        return TEE_ERROR_BAD_PARAMETERS;

    if (idx == VPU_FLUSH_BUF_IDX) {
        //queue a dummy buffer with size = 0 to indicate eos flush
        input_desc->addr = 0xaaaabbbb;
        input_desc->addr_virtual = (void *)0xaaaabbbb;
        input_desc->size = 0;
        input_desc->id = idx;
    } else {
        input_desc->addr = bs_buf->buf.addr + offset;
        input_desc->addr_virtual = (void *)(bs_buf->buf.vaddr + offset);
        input_desc->size = bytesused - offset;
        input_desc->id = idx;
        TEE_InvalidateCache(input_desc->addr_virtual, input_desc->size);
    }

    VPU_BUF_TRACE("+++++ push_es %ld %p addr(%x, %p), sz(%x)\n", idx,
        input_desc, input_desc->addr, input_desc->addr_virtual,
        input_desc->size);
    ret = vdec_push_stream_buffer(ctx->strm_ctx, input_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;

    return 0;
}

/*
 * When the display buffer is the same buffer as output buffer,
 * push_display() would be called first.
 */
static int vdec_push_ref_buf(struct i_vpu_ctx *ctx, struct syna_vpu_ctrl *ctrl,
                             uint32_t idx)
{
    vdec_vid_buf_desc_t *buf_desc;
    int ret;
    i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

    buf_desc = &pool->ref_desc[idx];
    buf_desc->id = idx;

    VPU_BUF_TRACE("+++++ push_ref %ld %p addr(%x, %x), sz(%x, %x)\n", idx,
    buf_desc, buf_desc->base_addr, buf_desc->base_addr_uv,
    buf_desc->size, buf_desc->size_uv);
    ret = vdec_push_reference_buffer(ctx->strm_ctx, buf_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;

    return 0;
}

static int vdec_push_disp_buf(struct i_vpu_ctx *ctx,
                              struct syna_vpu_ctrl *ctrl,
                              uint32_t idx)
{
    struct syna_vpu_tz_buf *buf;
    vdec_vid_buf_desc_t *buf_desc;
    int ret;

    i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

    if (IS_AUX_POOL_ENABLED(ctx)) {
        buf_desc = &pool->dis_desc[idx];
        buf = &(ctrl->auxbufs[idx]);
    } else {
        buf_desc = &pool->ref_desc[idx];
        buf = &(ctrl->outbufs[idx]);
    }

    VPU_BUF_TRACE("+++++ push_dis %ld %p addr(%x, %x), sz(%x, %x)", idx,
    buf_desc, buf_desc->base_addr, buf_desc->base_addr_uv,
    buf_desc->size, buf_desc->size_uv);

    buf_desc->id = idx;
    ret = vdec_push_display_buffer(ctx->strm_ctx, buf_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;


    /* TODO: support more pixel formats */
    buf->bytesused[0] = 0;
    buf->bytesused[1] = 0;

    /* TODO: doesn't work for VPx codec, which has ref-only buffer */
    buf->timestamp = 0;

    if (ctx->codec_fmt == SYNA_VXG_AV1)
        return 0;

    /* NOTE: AV1 won't reach here */
    uintptr_t tmp_ptr = (uintptr_t)buf;
    if (queue_fifo_p_push(&ctx->sink_bufs, &tmp_ptr))
        return TEE_ERROR_OVERFLOW;

    return 0;
}

static TEE_Result ta_vdec_push_buffer(VPU_SESSION_CTX * session_ctx,
                                      uint32_t parameters_type,
                                      TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    unsigned int idx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    idx = params[1].value.b;
    if (idx >= N_BUFSLOTS) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    switch(params[1].value.a) {
    /* decoder input fw queue */
    case SYNA_VPU_FW_VDEC_STRM:
        ret = vdec_push_es_buf(ctx, ctrl, idx);
        break;
    /* decoder HW output queue */
    case SYNA_VPU_FW_VDEC_REF:
        ret = vdec_push_ref_buf(ctx, ctrl, idx);
        break;
    /* decoder display order(maybe post-proccessing) queue */
    case SYNA_VPU_FW_VDEC_DISP:
        ret = vdec_push_disp_buf(ctx, ctrl, idx);
        break;
    default:
        ret = TEE_ERROR_BAD_PARAMETERS;
        break;
    }

bail:
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

static int vdec_pop_es_buf(struct i_vpu_ctx *ctx,
                           struct syna_vpu_ctrl *ctrl,
                           uint32_t *idx)
{
    vdec_strm_buf_desc_t *buf_desc;
    int ret;
    i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

    ret = vdec_pop_stream_buffer(ctx->strm_ctx, &buf_desc);
    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
        goto bail;
    }

    if (buf_desc) {
        if ((void *)buf_desc < (void *)&pool->strm_desc[0]
            ||((void *)buf_desc > (void *)&pool->strm_desc[MAX_SLOT - 1]))
        {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            goto bail;
        }
        *idx = buf_desc->id;
    } else {
        ret = TEE_ERROR_BUSY;
    }

bail:
    return ret;
}

static int vdec_pop_ref_buf(struct i_vpu_ctx *ctx, struct syna_vpu_ctrl *ctrl,
                            uint32_t *idx)
{
    struct syna_vpu_tz_buf *buf;
    struct vdec_vid_buf_desc_t *buf_desc;
    int ret;
    i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

    ret = vdec_pop_reference_buffer(ctx->strm_ctx, &buf_desc);

    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
        goto bail;
    }

    if (buf_desc) {
        if (((void *)buf_desc < (void *)&pool->ref_desc[0]) ||
             ((void *)buf_desc > (void *)&pool->ref_desc[MAX_SLOT - 1])) {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            return ret;
        }

        *idx = buf_desc->id;

        buf = &(ctrl->outbufs[*idx]);
        /* TODO: support more pixel formats */
        buf->bytesused[0] = buf->planes[0].offset + buf_desc->size;
        buf->bytesused[1] = buf->planes[1].offset + buf_desc->size_uv;
    } else {
        ret = TEE_ERROR_BUSY;
    }

bail:
    return ret;
}

static int vdec_tag_tu_output_timestamp(struct i_vpu_ctx *ctx,
                                        struct syna_vpu_tz_buf *dst_buf)
{
    struct syna_vpu_tz_buf *src_buf;
    uintptr_t tmp_ptr = 0;
    int ret = 0;

    if (queue_fifo_p_count(&ctx->src_bufs)) {
        queue_fifo_p_pop(&ctx->src_bufs, &tmp_ptr);
        if (tmp_ptr) {
            src_buf = (struct syna_vpu_tz_buf *)tmp_ptr;
            dst_buf->timestamp = src_buf->timestamp;
        }
    } else {
        VMETA_LOGE("missing src buf for timestamp here");
        ret = TEE_ERROR_SHORT_BUFFER;
    }

    return ret;
}

/* From recycle_dis_buf */
static int vdec_parse_pic_desc(struct i_vpu_ctx *ctx,
                               struct syna_vpu_tz_buf *buf,
                               const vdec_vid_buf_desc_t *buf_desc)
{
    const vdec_pic_desc_t *pic_desc, *pic_desc1;
    u32 idx;

    //fallback to 0 if non valid, for code compliance
    idx = buf_desc->pic[1].is_valid ? 1 : 0;
    pic_desc = &(buf_desc->pic[idx]);

    /**
     * NOTE: is_coded is always true and is_skipped is always false now,
     * only the firmware for the vmeta hardware uses those two variables.
     */

    if (pic_desc->is_aborted || pic_desc->slice_miss) {
        buf->flags = BERLIN_BUF_FLAG_ERROR;
        return 0;
    }

    /* discard for display, only used for reference */
    if (!pic_desc->is_valid) {
        buf->flags = 0;
        return 0;
    }

    if ('P' == pic_desc->pic_type)
        buf->flags = BERLIN_BUF_FLAG_PFRAME;
    else if ('I' == pic_desc->pic_type)
        buf->flags = BERLIN_BUF_FLAG_KEYFRAME;
    else if ('B' == pic_desc->pic_type)
        buf->flags = BERLIN_BUF_FLAG_BFRAME;

    if (pic_desc->is_fld) {
        if (buf_desc->pic[1].is_valid)
            pic_desc1 = &(buf_desc->pic[1]);
        else
            pic_desc1 = NULL;

        if (buf_desc->pic[0].is_valid)
            pic_desc = &(buf_desc->pic[0]);

        if (pic_desc1 != pic_desc) {
            if (pic_desc->is_btm)
                buf->field = SYNA_FRAME_FIELD_INTERLACED_BT;
            else
                buf->field = SYNA_FRAME_FIELD_INTERLACED_TB;
        } else {
            if (pic_desc->is_btm)
                buf->field = SYNA_FRAME_FILED_BOTTOM;
            else
                buf->field = SYNA_FRAME_FILED_TOP;
        }
    } else {
        buf->field = SYNA_FRAME_FILED_NONE;
    }

    buf->flags |= BERLIN_BUF_FLAG_DONE;

    /* TODO: support more pixel formats */
    buf->bytesused[0] = buf->planes[0].offset + buf_desc->size;
    buf->bytesused[1] = buf->planes[1].offset + buf_desc->size_uv;

    /* TODO: better description */
    if (pic_desc->error_mbs_in_pic > 0) {
        if (pic_desc->error_mbs_in_pic < pic_desc->total_mbs_in_pic)
            buf->errors = pic_desc->error_mbs_in_pic;
        else
            buf->flags |= BERLIN_BUF_FLAG_ERROR;
    }

    /* from fillFrameInfo() */
    if (pic_desc->is_compressed) {
        buf->flags |= BERLIN_BUF_FLAG_COMPRESSED;
    } else {
        buf->flags &= ~BERLIN_BUF_FLAG_COMPRESSED;
    }

    //mtrr need send out both for mtr=1 or mmu=1
    TEE_MemMove(&buf->mtr_cfg0, (void *)pic_desc->dis_y_mtrr_cfg,
                sizeof(pic_desc->dis_y_mtrr_cfg));
    TEE_MemMove(&buf->mtr_cfg1, (void *)pic_desc->dis_c_mtrr_cfg,
                sizeof(pic_desc->dis_c_mtrr_cfg));

    buf->startx = pic_desc->dis_left;
    buf->starty = pic_desc->dis_top;
    buf->width = MIN(pic_desc->dis_width, pic_desc->valid_dis_width);
    buf->height = MIN(pic_desc->dis_height, pic_desc->valid_dis_height);

    if (ctx->codec_fmt == SYNA_VXG_AV1)
        return vdec_tag_tu_output_timestamp(ctx, buf);

    return 0;
}

static int vdec_pop_disp_buf(struct i_vpu_ctx *ctx, struct syna_vpu_ctrl *ctrl,
                             uint32_t *idx)
{
    struct syna_vpu_tz_buf *buf;
    struct vdec_vid_buf_desc_t *buf_desc;
    void *start;
    void *end;
    int ret;
    i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

    ret = vdec_pop_display_buffer(ctx->strm_ctx, &buf_desc);
    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
        goto bail;
    }

    if (buf_desc) {
        if (IS_AUX_POOL_ENABLED(ctx)) {
            start = &pool->dis_desc[0];
            buf = ctrl->auxbufs;
            end = &pool->dis_desc[MAX_SLOT - 1];
        } else {
            start = &pool->ref_desc[0];
            buf = ctrl->outbufs;
            end = &pool->ref_desc[MAX_SLOT - 1];
        }

        if (((void *)buf_desc < start || (void *)buf_desc > end)) {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            return ret;
        }

        *idx = buf_desc->id;
        buf += (*idx);

        vdec_parse_pic_desc(ctx, buf, buf_desc);
    } else {
        ret = TEE_ERROR_BUSY;
    }

bail:
    return ret;
}

static TEE_Result ta_vdec_pop_buffer(VPU_SESSION_CTX * session_ctx,
        uint32_t parameters_type, TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    uint32_t idx = 0;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    switch(params[1].value.a) {
    case SYNA_VPU_FW_VDEC_STRM:
        ret = vdec_pop_es_buf(ctx, ctrl, &idx);
        break;
    case SYNA_VPU_FW_VDEC_REF:
        ret = vdec_pop_ref_buf(ctx, ctrl, &idx);
        break;
    case SYNA_VPU_FW_VDEC_DISP:
        ret = vdec_pop_disp_buf(ctx, ctrl, &idx);
        break;
    default:
        return TEE_ERROR_BAD_PARAMETERS;
        break;
    }

    if (ret >= 0)
        ret = 0;

bail:
    params[1].value.b = idx;
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

static int vdec_loop_pop_es_buf(struct syna_vpu_ctrl *ctrl,
                                struct i_vpu_ctx *ctx)
{
    int ret;
    uint32_t idx = 0;
    struct idx_queue *q = &ctrl->sbuf;

    idx_queue_clear(q);

    do {
        ret = vdec_pop_es_buf(ctx, ctrl, &idx);

        if ((int)TEE_ERROR_BUSY == ret) {
            if (!q->pop) {
                VMETA_LOGE("can't pop any buffer from FW, why this event?");
                ret = TEE_ERROR_NO_DATA;
            }
            break;
        } else if ((int)TEE_ERROR_NO_DATA == ret) {
            VMETA_LOGE("loop_pop_es try to pop empty queue");
            break;
        } else {
            idx_queue_store(q, q->pop, idx);
            VPU_BUF_TRACE("\n ++++ pop es %ld %ld\n", idx, q->pop);
        }
    } while (--ret);

    return ret;
}

static int vdec_loop_pop_ref_buf(struct syna_vpu_ctrl *ctrl,
                                 struct i_vpu_ctx *ctx)
{
    int ret;
    uint32_t idx = 0;
    struct idx_queue *q = &ctrl->rbuf;

    idx_queue_clear(q);

    do {
        ret = vdec_pop_ref_buf(ctx, ctrl, &idx);

        if ((int)TEE_ERROR_BUSY == ret) {
            if (!q->pop) {
                VMETA_LOGE("can't pop any buffer from FW, why this event?");
                ret = TEE_ERROR_NO_DATA;
            }
            break;
        } else if ((int)TEE_ERROR_NO_DATA == ret) {
            VMETA_LOGE("loop_pop_ref try to pop empty queue");
            break;
        } else {
            idx_queue_store(q, q->pop, idx);
            VPU_BUF_TRACE("\n ++++ pop refbuf %ld %ld\n", idx, q->pop);
        }
    } while (--ret);

    return ret;
}

static int vdec_loop_pop_disp_buf(struct syna_vpu_ctrl *ctrl,
                                  struct i_vpu_ctx *ctx)
{
    int ret;
    uint32_t idx = 0;
    struct idx_queue *q = &ctrl->dbuf;

    idx_queue_clear(q);

    do {
        ret = vdec_pop_disp_buf(ctx, ctrl, &idx);

        if ((int)TEE_ERROR_BUSY == ret) {
            if (!q->pop) {
                VMETA_LOGE("can't pop any buffer from FW, why this event?");
                ret = TEE_ERROR_NO_DATA;
            }
            break;
        } else if ((int)TEE_ERROR_NO_DATA == ret) {
            VMETA_LOGE("loop_pop_dis try to pop empty queue");
            break;
        } else {
            idx_queue_store(q, q->pop, idx);
            VPU_BUF_TRACE("\n ++++ pop disbuf %ld %ld\n", idx, q->pop);
        }
    } while (--ret);
    return ret;
}

static bool vdec_check_new_seq(struct i_vpu_ctx *ctx,
                               struct syna_vpu_ctrl *ctrl,
                               const vdec_seq_desc_t *seq)
{
    if (ctrl->seq_desc.luma_size < seq->ref_size_y)
        return true;

    if (ctrl->seq_desc.chroma_size < seq->ref_size_uv)
        return true;

    if (ctrl->seq_desc.tctx_size < seq->tctx_buf_size)
        return true;

    /* meta size is related to data size, no need to check it again */

    if (ctrl->seq_desc.max_ref_nums < seq->num_ref_bufs)
        return true;

    if ((ctrl->seq_desc.width == seq->dis_width)
        && (ctrl->seq_desc.height == seq->dis_height))
    {
        if (ctrl->seq_desc.max_dpb_size < seq->num_dis_bufs)
            return true;

        if (IS_AUX_POOL_ENABLED(ctx)) {
            /* dither update here */
            if (ctrl->seq_desc.disp_luma_size < seq->dis_size_y)
                return true;

            if (ctrl->seq_desc.disp_chroma_size < seq->dis_size_uv)
                return true;
        }
    } else {
        return true;
    }

    return false;
}

static int vdec_update_seq(struct syna_vpu_ctrl *ctrl,
                           const vdec_seq_desc_t *seq)
{
    ctrl->seq_desc.luma_size = seq->ref_size_y;
    ctrl->seq_desc.chroma_size = seq->ref_size_uv;
    ctrl->seq_desc.meta_luma_size = seq->meta_buf_size;
    ctrl->seq_desc.meta_chroma_size = seq->meta_buf_size_uv;
    ctrl->seq_desc.tctx_size = seq->tctx_buf_size;

    ctrl->seq_desc.startx = seq->dis_left;
    ctrl->seq_desc.starty = seq->dis_top;

    ctrl->seq_desc.width = seq->dis_width;
    ctrl->seq_desc.height = seq->dis_height;
    ctrl->seq_desc.disp_luma_size = seq->dis_size_y;
    ctrl->seq_desc.disp_chroma_size = seq->dis_size_uv;

    ctrl->seq_desc.max_ref_nums = seq->num_ref_bufs & UINT16_MAX;
    ctrl->seq_desc.max_dpb_size = seq->num_dis_bufs & UINT16_MAX;

    /**
     * FIXME: using the tiled format here
     * only work for YUV 4:2:0
     */
    if (PIC_BITDEPTH_10 == seq->bit_depth)
        ctrl->seq_desc.format = SYNA_VXG_NV15;
    else if (PIC_BITDEPTH_8 == seq->bit_depth)
        ctrl->seq_desc.format = SYNA_VXG_NV12;
    else
        ctrl->seq_desc.format = UINT32_MAX;

    return 0;
}

static int vdec_tag_output_timestamp(struct i_vpu_ctx *ctx,
                                     vdec_pic_desc_t *pic_desc)
{
    struct syna_vpu_tz_buf *dst_buf;
    struct syna_vpu_tz_buf *src_buf;
    bool decoded_frame;
    int ret = 0;

    switch (ctx->codec_fmt) {
    case SYNA_VXG_VP8:
    case SYNA_VXG_VP9:
        if (pic_desc->pic_type == 'r')
            decoded_frame = false;
        else
            decoded_frame = true;
        break;
    case SYNA_VXG_AV1:
        /* AV1 would always show a frame in a temporal unit */
        return 0;
        break;
    default:
        decoded_frame = true;
    }

    /* We don't tag timestamp to those ref only(non-display) frame */
    if (decoded_frame) {
        if (queue_fifo_p_count(&ctx->src_bufs)) {
            uintptr_t tmp_ptr = 0;
            if (!queue_fifo_p_pop(&ctx->sink_bufs, &tmp_ptr)) {
                dst_buf = (struct syna_vpu_tz_buf *)tmp_ptr;
                tmp_ptr = 0;
                queue_fifo_p_pop(&ctx->src_bufs, &tmp_ptr);
                if (tmp_ptr) {
                    src_buf = (struct syna_vpu_tz_buf *)tmp_ptr;
                    dst_buf->timestamp = src_buf->timestamp;
                }
                /**
                 * TODO: tag timestamp with the rest of src bufs which
                 * are consisted of this dst buf. Firmware didn't support
                 * slice, field nor tile(coded term in H.265) output.
                 */
            } else {
                VMETA_LOGE("display timestamp FIFO is empty, instance is in a bad state");
            }
        } else {
            VMETA_LOGE("missing src buf for timestamp here");
            ret = TEE_ERROR_SHORT_BUFFER;
        }
    } else {
        VMETA_LOGE("non display frame");
    }

    return ret;
}

static TEE_Result ta_vdec_decode_stream(VPU_SESSION_CTX * session_ctx,
        uint32_t parameters_type, TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    vdec_strm_status_t *fw_status = NULL;
    struct syna_vpu_tz_buf *src_buf;
    vdec_strm_buf_desc_t *src_buf_desc = NULL;
    struct idx_queue *q;
    bool loop = true;
    bool regular_check_display = false;
    int ret;
    unsigned int i;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    if (!ctx->secure) {
        if (ctx->in_sec_flags || ctx->out_sec_flags || ctx->aux_sec_flags)
            ctx->secure = true;
    }

    /* data won't be leaked to insecure region */
    if (ctx->secure) {
        if (ctx->out_insec_flags || ctx->aux_insec_flags) {
            ret = TEE_ERROR_SECURITY;
            goto bail;
        }
    }

    /* push reference buffers here */
    if (ctrl->status.flags & BERLIN_VPU_STATUS_PUSH_OUTPUT_BUFS) {
        q = &ctrl->rbuf;
        for (i = 0; i < q->push; i++) {
            ret = vdec_push_ref_buf(ctx, ctrl,
                                    idx_queue_load(q, i));
            if (ret) {
                ctrl->status.flags = BERLIN_VPU_STATUS_ERROR;
                ctrl->status.flags |= BERLIN_VPU_STATUS_PUSH_OUTPUT_BUFS;
                q->push = i;
                goto bail;
            }
        }
        q->push = 0;
        ctrl->status.flags &= ~BERLIN_VPU_STATUS_PUSH_OUTPUT_BUFS;
    }

    /* push display buffers here */
    if (ctrl->status.flags & BERLIN_VPU_STATUS_PUSH_AUX_BUFS) {
        q = &ctrl->dbuf;
        for (i = 0; i < q->push; i++) {
            ret = vdec_push_disp_buf(ctx, ctrl,
                                     idx_queue_load(q, i));
            if (ret) {
                ctrl->status.flags = BERLIN_VPU_STATUS_ERROR;
                ctrl->status.flags |= BERLIN_VPU_STATUS_PUSH_AUX_BUFS;
                q->push = i;
                goto bail;
            }
        }
        q->push = 0;
        ctrl->status.flags &= ~BERLIN_VPU_STATUS_PUSH_AUX_BUFS;
    }

    /* push stream (ES) buffer here */
    if (ctrl->status.flags & BERLIN_VPU_STATUS_PUSH_INPUT_BUFS) {
        q = &ctrl->sbuf;
        if (q->push > 1)
            VMETA_LOGE("more than one buffer for the input");

        for (i = 0; i < q->push; i++) {
            ret = vdec_push_es_buf(ctx, ctrl,
                                   idx_queue_load(q, i));
            if (ret) {
                ctrl->status.flags = BERLIN_VPU_STATUS_ERROR;
                ctrl->status.flags |= BERLIN_VPU_STATUS_PUSH_INPUT_BUFS;
                q->push = i;
                goto bail;
            }
        }
        q->push = 0;
        /* NOTE: clear flags here */
        ctrl->status.flags = 0;
    }

    do {
        ret = vdec_decode_stream(ctx->strm_ctx, &fw_status);
        if (ret < 0 || NULL == fw_status) {
            ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
            break;
        }

        if (fw_status->wait_for_int) {
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INTERRUPTER;
            ret = 0;
            break;
        }

        switch (fw_status->event_id) {
        case VDEC_EVENT_NONE:
            /**
             * Pan Yang explains:
             * when the fw reaches the stage that a frame is decoded,
             * the fw would clear strm->status.event_id and strm->fsm_state
             * then return. This could be regarded as an SYNCPOINT.
             */
            break;
        case VDEC_EVENT_START_OF_SEQUENCE:
            /**
             * NOTE: in the new FW (svn33920M_20221107), it would always pop
             * all ref & disp bufs before this event. We should return to CA,
             * let CA decided which buffer could be re-pushed to FW.
             */

            if (fw_status->seq_desc
                && vdec_check_new_seq(ctx, ctrl, fw_status->seq_desc))
            {
                /* CA may re-alloc here, release the interanl ref */
                vpu_tzmem_put(&ctx->mtr_buf);

                ret = vdec_update_seq(ctrl, fw_status->seq_desc);

                if (fw_status->safe_for_switch)
                    ctrl->status.flags |= BERLIN_VPU_STATUS_PROC_END_SAFE;

                ctrl->status.flags |= BERLIN_VPU_STATUS_NEW_SEQUENCE;
                loop = false;
            } else {
                ctrl->status.flags |= BERLIN_VPU_STATUS_POP_BREAK;
                loop = false;
            }
            break;
        case VDEC_EVENT_START_OF_PICTURE:
            /**
             * NOTE: amp does timestamp here, the start of a a field or
             * frame(progressive).
             */
            src_buf_desc = fw_status->sbuf_desc;
            if (src_buf_desc) {
                i_vdec_pool_t *pool = (i_vdec_pool_t *)ctx->desc_pool;

                if ((void *)src_buf_desc < (void *)&pool->strm_desc[0]
                    || (void *)src_buf_desc > (void *)&pool->strm_desc[MAX_SLOT - 1])
                    break;

                i = src_buf_desc->id;
                src_buf = &(ctrl->inbufs[i]);
                uintptr_t tmp_ptr = (uintptr_t)src_buf;
                queue_fifo_p_push(&ctx->src_bufs, &tmp_ptr);
            } else {
                VMETA_LOGE("@err start of pic no src");
            }
            if (ctrl->rbuf.pop || ctrl->dbuf.pop)
            {
                ctrl->status.flags |= BERLIN_VPU_STATUS_POP_BREAK;
                loop = false;
            }
            break;
        case VDEC_EVENT_ALLOC_STR_BUF:
            /**
             * NOTE: Pan Yang explains:
             * before start of frame, the actual decoding starts, it
             * could be safe to switch out the current instance,
             * if you meet any problem here, please contact FW
             * team to solve it.
             */
            if (fw_status->safe_for_switch)
                ctrl->status.flags |= BERLIN_VPU_STATUS_PROC_END_SAFE;

            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INPUT_BUF;
            loop = false;
            break;
        case VDEC_EVENT_ALLOC_REF_BUF:
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_OUTPUT_BUF;
            loop = false;
            ret = 0;
            break;
        case VDEC_EVENT_ALLOC_DIS_BUF:
            if (IS_AUX_POOL_ENABLED(ctx))
                ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_AUX_BUF;
            else
                ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_OUTPUT_BUF;

            loop = false;
            ret = 0;
            break;
        case VDEC_EVENT_SYNCWORD:
            /* TODO: SEI data */
            break;
        case VDEC_EVENT_END_OF_PICTURE:
            src_buf_desc = fw_status->sbuf_desc;
            if (!src_buf_desc)
                VMETA_LOGD("src buf could be incomplete");

            regular_check_display = true;
            break;
        case VDEC_EVENT_CHECK_STR_BUF:
            ret = vdec_loop_pop_es_buf(ctrl, ctx);
            if (ret) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            }
            break;
        case VDEC_EVENT_CHECK_REF_BUF:
            ret = vdec_loop_pop_ref_buf(ctrl, ctx);
            if (ret) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            }
            break;
        case VDEC_EVENT_CHECK_DIS_BUF:
            ret = vdec_loop_pop_disp_buf(ctrl, ctx);
            if (ret) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            } else if (!regular_check_display) {
                /**
                 * NOTE: this usually happens before START_OF_SEQUENCE event,
                 * from new FW (svn34130_20230217), it would always pop
                 * all ref & disp bufs before START_OF_SEQUENCE. We could
                 * clear the timestamp queue here.
                 */
                queue_fifo_p_init(&ctx->sink_bufs);
            }
            break;
        case VDEC_EVENT_END_OF_FRAME:
            if (!fw_status->pic_desc) {
                VMETA_LOGE("no pic desc");
                ret = TEE_ERROR_CORRUPT_OBJECT;
                goto bail;
            }
            ret = vdec_tag_output_timestamp(ctx, fw_status->pic_desc);
            ctrl->status.flags |= BERLIN_VPU_STATUS_DONE;
            break;
        case VDEC_EVENT_START_OF_FRAME:
            queue_fifo_p_init(&ctx->src_bufs);
            regular_check_display = true;
            break;
        case VDEC_EVENT_END_OF_STREAM:
            ctrl->status.flags |= BERLIN_VPU_STATUS_DONE;
            loop = false;
            break;
        case VDEC_EVENT_END_OF_SEQUENCE:
            /* We don't care about the old sequence */
            break;
        case VDEC_EVENT_SWITCH_POINT:
            loop = false;
            break;
        default:
            break;
        }
    } while (loop);

bail:
    params[1].value.a = ret;
    params[1].value.b = fw_status->event_id;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_drain_pool(VPU_SESSION_CTX * session_ctx,
        uint32_t parameters_type, TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    switch(params[1].value.a) {
    case SYNA_VPU_FW_VDEC_STRM:
        ret = vdec_flush_stream(ctx->strm_ctx, 1U);
        if (ret) {
            VMETA_LOGE("vdec failed to drain ES: 0x%08x",
                       ret);
            goto bail;
        }

        ret = vdec_loop_pop_es_buf(ctrl, ctx);
        if (ret != (int)TEE_ERROR_NO_DATA && ret) {
            VMETA_LOGE("vdec pop es buf failed: 0x%08x(%d done)",
                       ret, ctrl->sbuf.pop);
            goto bail;
        }

        queue_fifo_p_init(&ctx->sink_bufs);

        ret = vdec_loop_pop_ref_buf(ctrl, ctx);
        if (ret != (int)TEE_ERROR_NO_DATA && ret) {
            VMETA_LOGE("vdec pop ref buf failed: 0x%08x(%d done)",
                       ret, ctrl->rbuf.pop);
            goto bail;
        }

        ret = vdec_loop_pop_disp_buf(ctrl, ctx);
        if (ret != (int)TEE_ERROR_NO_DATA && ret) {
            VMETA_LOGE("vdec pop disp buf failed: 0x%08x(%d done)",
                       ret, ctrl->dbuf.pop);
            goto bail;
        }

        ret = vpu_tzmem_put(&ctx->mtr_buf);

        break;
    case SYNA_VPU_FW_VDEC_REF:
    case SYNA_VPU_FW_VDEC_DISP:
        ret = vdec_flush_stream(ctx->strm_ctx, 0U);
        if (ret) {
            VMETA_LOGE("vdec failed to drain ref & disp: 0x%08x",
                       ret);
            goto bail;
        }

        ret = vdec_loop_pop_ref_buf(ctrl, ctx);
        if (ret != (int)TEE_ERROR_NO_DATA && ret) {
            VMETA_LOGE("vdec pop ref buf failed: 0x%08x(%d done)",
                       ret, ctrl->rbuf.pop);
            goto bail;
        }

        ret = vdec_loop_pop_disp_buf(ctrl, ctx);
        if (ret != (int)TEE_ERROR_NO_DATA && ret) {
            VMETA_LOGE("vdec pop disp buf failed: 0x%08x(%d done)",
                       ret, ctrl->dbuf.pop);
            goto bail;
        }

        ret = vpu_tzmem_put(&ctx->mtr_buf);

        break;
    default:
        ret = TEE_ERROR_BAD_PARAMETERS;
        break;
    }

bail:
    params[2].value.a = ret;
    return TEE_SUCCESS;
}


static TEE_Result ta_vdec_int_timeout(VPU_SESSION_CTX * session_ctx,
                                      uint32_t parameters_type,
                                      TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    VMETA_LOGE("HW int timeout, call VPU API to recover");
    ret = vdec_int_timeout(ctx->strm_ctx);

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_reset_instance(VPU_SESSION_CTX * session_ctx,
                                         uint32_t parameters_type,
                                         TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;
    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto failed;
    }

    /* NOTE: non-zero means error from FW */
    ret = vdec_close_stream(ctx->strm_ctx);

    TEE_MemFill(&ctrl->status, 0, sizeof(ctrl->status));

    if (ret) {
        VMETA_LOGE("vdec_close_stream() failed: 0x%08x", ret);
        goto failed;
    }

    ret = vdec_loop_pop_es_buf(ctrl, ctx);
    if (ret != (int)TEE_ERROR_NO_DATA && ret) {
        VMETA_LOGE("vdec pop es buf failed: 0x%08x(%d done)",
                   ret, ctrl->sbuf.pop);
        goto failed;
    }
    /**
     * NOTE: we don't remove buf from m2m OUTPUT ready queue so we don't need
     * let the CA driver return them here.
     */
    ctrl->sbuf.pop = 0;

    ret = vdec_loop_pop_ref_buf(ctrl, ctx);
    if (ret != (int)TEE_ERROR_NO_DATA && ret) {
        VMETA_LOGE("vdec pop ref buf failed: 0x%08x(%d done)",
                   ret, ctrl->rbuf.pop);
        goto failed;
    }

    ret = vdec_loop_pop_disp_buf(ctrl, ctx);
    if (ret != (int)TEE_ERROR_NO_DATA && ret) {
        VMETA_LOGE("vdec pop disp buf failed: 0x%08x(%d done)",
                   ret, ctrl->dbuf.pop);
        goto failed;
    }
    queue_fifo_p_init(&ctx->sink_bufs);

    ret = vpu_tzmem_put(&ctx->mtr_buf);

failed:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_stream_switch_in(VPU_SESSION_CTX *session_ctx,
                                           uint32_t parameters_type,
                                           TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    ret = vdec_stream_switch_in(ctx->strm_ctx);

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_stream_switch_out(VPU_SESSION_CTX * session_ctx,
                                            uint32_t parameters_type,
                                            TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    ret = vdec_stream_switch_out(ctx->strm_ctx);
bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vpu_ctx_switch_inst(VPU_SESSION_CTX * session_ctx,
                                         uint32_t parameters_type,
                                         TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    void *strm_ctx;
    int ret;

    if (0 == params[0].memref.size && 0 == params[1].memref.size) {
        VMETA_LOGE("No available context in the parameters\n");
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;
    if (ctrl) {
        ret = get_internal_ctx(session_ctx, ctrl, &ctx);
        if (ret) {
            VMETA_LOGE("Failed to get ctx for switching out");
            ret = TEE_ERROR_CORRUPT_OBJECT;
            goto bail;
        }
        strm_ctx = ctx->strm_ctx;

        if (ctx->is_encoder)
            ret = venc_stream_switch_out(strm_ctx);
        else
            ret = vdec_stream_switch_out(strm_ctx);

        if (ret) {
            VMETA_LOGE("can't switch out %p(%p)", ctrl, strm_ctx);
            goto bail;
        }
    }

    ctrl = (struct syna_vpu_ctrl *)params[1].memref.buffer;
    if (ctrl) {
        ret = get_internal_ctx(session_ctx, ctrl, &ctx);
        if (ret) {
            VMETA_LOGE("Failed to get ctx for switching in");
            ret = TEE_ERROR_CORRUPT_OBJECT;
            goto bail;
        }

        strm_ctx = ctx->strm_ctx;

        if (ctx->is_encoder)
            ret = venc_stream_switch_in(strm_ctx);
        else
            ret = vdec_stream_switch_in(strm_ctx);

        if (ret) {
            VMETA_LOGE("can't switch in %p(%p)", ctrl, strm_ctx);
            goto bail;
        }
    }

bail:
    params[2].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_vdec_destroy_context(VPU_SESSION_CTX * session_ctx,
                                          uint32_t parameters_type,
                                          TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = release_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Failed to get pVDCtx: 0x%08x", ret);
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    /**
     * NOTE: we should drop this ref to mtr buf in ta_vdec_reset_instance(),
     * if we can ensure that would be called and mtr buf is not assigned
     * (likes ta_vdec_config_stream()), we can remove it here.
     */
    ret = vpu_tzmem_put(&ctx->mtr_buf);
    if (ret)
        goto bail;

    /* release the access to the secure context */

   if (ctrl->ctrl_buf.vaddr && ctrl->ctrl_buf.memid) {
        ret = vpu_tzmem_put(&ctrl->ctrl_buf);
        if (ret) {
            VMETA_LOGE("failed unmap ctrl buf(memid 0x%x), ret 0x%x\n", ctrl->ctrl_buf.memid, ret);
        }
    }
    if (ctrl->ctx_buf.vaddr && ctrl->ctx_buf.memid) {
        ret = vpu_tzmem_put(&ctrl->ctx_buf);
        if (ret) {
            VMETA_LOGE("failed unmap ctrl ctx_buf(memid 0x%x), ret 0x%x\n", ctrl->ctx_buf.memid, ret);
        }
    }

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_venc_create_instance(VPU_SESSION_CTX * session_ctx,
                                          uint32_t parameters_type,
                                          TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    const struct vpu_const_t *vc;
    struct i_vpu_ctx *ctx;
    struct syna_tz_generic_buf tzmem = {0};
    uint32_t ctrl_size, ctx_mem_id, ctx_mem_size;
    size_t size, ctx_size, ctx_hw_size;
    TEE_Result ret = TEE_SUCCESS;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;
    ctrl_size = (uint32_t)params[0].memref.size;
    if (ctrl == 0 || ctrl->ctrl_buf.addr == 0 || (ctrl_size != getBufSize(eSynaVpuCtrlSize))) {
        VMETA_LOGE("invalid param, ctrl 0x%x, addr 0x%llx, ctrl_size 0x%x(0x%x)\n", ctrl,
            ctrl->ctrl_buf.addr, ctrl_size, getBufSize(eSynaVpuCtrlSize));
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    ctrl->ctrl_buf.memid = 0;
    ctrl->ctrl_buf.vaddr = (uintptr_t)ctrl;
    ctrl->ctrl_buf.size = ctrl_size;
    //check if a valid ctrl
    if (vpuObjMgrGet(session_ctx->objMgr, (void *)((uintptr_t)ctrl->ctrl_buf.addr))) {
        VMETA_LOGE("no release pVDCtx in sec_ctxs");
        ret = TEE_ERROR_BAD_STATE;
        goto failed_ctrl;
    }

    //get ctx mem
    ctx_mem_id = ctrl->ctx_buf.memid;
    ctx_mem_size = ctrl->ctx_buf.size;

    vc = vpu_get_const(session_ctx->vpu_type);
    ctx_size = ALIGN(vc->venc_strm_context_size, 4096);
    ctx_hw_size = ALIGN(vc->venc_strm_hw_context_size, 4096);
    size = BERLIN_VPU_CTX_RESERVED_SIZE + ctx_size + ctx_hw_size;
    if (ctx_mem_size < size) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto failed_ctrl;
    }

    TZMEM_INIT_MEMID(&tzmem, MEM_ID_CONTINOUS, ctx_mem_id, ctx_mem_size, 0, 0);
    ret = vpu_tzmem_get(&tzmem);
    if (ret) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto failed_ctrl;
    }
    ctx = (struct i_vpu_ctx *)tzmem.vaddr;
    TEE_MemFill(ctx, 0, BERLIN_VPU_CTX_RESERVED_SIZE);
    TEE_MemMove(&(ctx->buf), &tzmem, sizeof(tzmem));
    TEE_MemMove(&(ctrl->ctx_buf), &tzmem, sizeof(tzmem));
    ctx->is_encoder = true;

    ctx->desc_pool = (void *)((uintptr_t)ctx + size);
    size += sizeof(i_venc_pool_t);

    size = ALIGN(size, 4096);
    /* double check or check the whole size */
    if (size > ctx->buf.size) {
        VMETA_LOGF("secure context buf size is not enough, want: %d", size);
        goto failed_ctx;
    }
    /* Fixed offset */
    ctx->strm_ctx = (void *)((uintptr_t)ctx + BERLIN_VPU_CTX_RESERVED_SIZE);
    ctx->ctx_hw_buf.addr = ctx->buf.addr + BERLIN_VPU_CTX_RESERVED_SIZE
                                         + ctx_size;
    ctx->ctx_hw_buf.vaddr = (uintptr_t)ctx->strm_ctx + ctx_size;
    ctx->ctx_hw_buf.size = ctx_hw_size;

    if (vpuObjMgrSet(session_ctx->objMgr, (void *)((uintptr_t)ctrl->ctrl_buf.addr), ctx) < 0) {
        VMETA_LOGF("Fail to save vdec secure context: %p", ctx);
        ret = TEE_ERROR_BAD_STATE;
        goto failed_ctx;
    }

    params[1].value.a = ret;
    return TEE_SUCCESS;

/* failure */
failed_ctx:
    vpu_tzmem_put(&ctx->buf);
failed_ctrl:
    vpu_tzmem_put(&ctrl->ctrl_buf);
bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_venc_close_stream(VPU_SESSION_CTX * session_ctx,
                                       uint32_t parameters_type,
                                       TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    ret = venc_close_stream(ctx->strm_ctx);
bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_venc_config_stream(VPU_SESSION_CTX * session_ctx,
                                        uint32_t parameters_type,
                                        TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    struct venc_strm_config_t *strm_cfg;
    size_t size;
    int ret;

#ifdef CONFIG_OPTEE
    if (!session_ctx->enc_plugin_init) {
        ret = tee_cb_plugin_ping(H1_PLUGIN_CMD_INIT, 0);
        if (ret == TEE_SUCCESS)
            session_ctx->enc_plugin_init = 1;
    }
#endif // CONFIG_OPTEE

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;
    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    /* check and map cfg memory */
    size = params[1].memref.size;
    if (size != getBufSize(eVencStrmCfgSize)) {
        VMETA_LOGE("invalid param, size=%d, getBufSize(eVencStrmCfgSize)=%d\n", size, getBufSize(eVencStrmCfgSize));
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }
    strm_cfg = (struct venc_strm_config_t *)params[1].memref.buffer;
    strm_cfg->user_data = (void *)ctrl;
    strm_cfg->venc_cntxt = session_ctx->fw_ctx_vaddr;
    strm_cfg->hw_context_addr = ctx->ctx_hw_buf.addr
                                    + ctx->ctx_hw_buf.offset;
    strm_cfg->hw_context_vaddr = (void *)(ctx->ctx_hw_buf.vaddr
                                    + ctx->ctx_hw_buf.offset);
    ctx->ltr_en = strm_cfg->ref_num_in_buf > 0;
    ctx->merge_header = (ctrl->h1_extra_cfg.header_mode == 1);
    ctx->enc_ds_en = (strm_cfg->ds_width && strm_cfg->ds_height);
    ctx->meta_mv_en = !!strm_cfg->outputMbInfo;
    if (strm_cfg->format == VENC_FMT_ID_VP8)
        ctx->vp8_enc = true;

    ret = venc_open_stream(ctx->strm_ctx, strm_cfg);

bail:
    params[2].value.a = ret;
    return TEE_SUCCESS;
}

static int venc_push_frame_buf(struct i_vpu_ctx *ctx,
                               struct syna_vpu_ctrl *ctrl, uint32_t idx)
{
    struct syna_vpu_tz_buf *buf;
    venc_vid_buf_desc_t *buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    ctx->cur_encoding_buf_idx = idx;
    buf_desc = &pool->vid_desc[idx];
    buf_desc->id = idx;
    buf = &(ctrl->inbufs[idx]);

    if (buf_desc->base_addr_virtual &&
        !(buf->planes[0].gfp_flags & BERLIN_GFP_FLAG_NO_CACHE_CLEAN))
        TEE_CleanCache(buf_desc->base_addr_virtual, buf->planes[0].size);
    if (buf_desc->base_addr_uv_virtual &&
        !(buf->planes[1].gfp_flags & BERLIN_GFP_FLAG_NO_CACHE_CLEAN))
        TEE_CleanCache(buf_desc->base_addr_uv_virtual, buf->planes[1].size);
    if (buf_desc->base_addr_uv1_virtual &&
        !(buf->planes[2].gfp_flags & BERLIN_GFP_FLAG_NO_CACHE_CLEAN))
        TEE_CleanCache(buf_desc->base_addr_uv1_virtual, buf->planes[2].size);

    VPU_BUF_TRACE("++++++++ push_vid %ld %p addr(%x, %p), uv(%x, %p)\n", idx,
        buf_desc, buf_desc->base_addr,  buf_desc->base_addr_virtual,
        buf_desc->base_addr_uv, buf_desc->base_addr_uv_virtual);

    ret = venc_push_video_buffer(ctx->strm_ctx, buf_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;

    return 0;
}

static int venc_push_es_buf(struct i_vpu_ctx *ctx,
                            struct syna_vpu_ctrl *ctrl, uint32_t idx)
{

    venc_strm_buf_desc_t *buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    buf_desc = &pool->strm_desc[idx];
    buf_desc->id = idx;

    VPU_BUF_TRACE("++++++++ push_es %ld %p addr(%x, %p), sz(%x)\n", idx,
    buf_desc, buf_desc->addr,  buf_desc->vaddr, buf_desc->size);

    ret = venc_push_stream_buffer(ctx->strm_ctx, buf_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;

    return 0;
}

static int venc_push_ref_buf(struct i_vpu_ctx *ctx,
                             struct syna_vpu_ctrl *ctrl, uint32_t idx)
{

    venc_vid_buf_desc_t* buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    buf_desc = &pool->ref_desc[idx];
    buf_desc->id = idx;
    VPU_BUF_TRACE("++++++++ push_ref %ld %p addr(%x, %p), uv(%x, %p)\n", idx,
    buf_desc, buf_desc->base_addr,  buf_desc->base_addr_virtual,
    buf_desc->base_addr_uv, buf_desc->base_addr_uv_virtual);
    ret = venc_push_reference_buffer(ctx->strm_ctx, buf_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;
    return 0;
}

static int venc_push_ds_buf(struct i_vpu_ctx *ctx, struct syna_vpu_ctrl *ctrl,
                            uint32_t idx)
{

    venc_vid_buf_desc_t *buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    buf_desc = &pool->ds_desc[idx];
    buf_desc->id = idx;
    VPU_BUF_TRACE("++++++++ push_ds %ld %p addr(%x, %p), uv(%x, %p)\n", idx,
                  buf_desc, buf_desc->base_addr, buf_desc->base_addr_virtual,
                  buf_desc->base_addr_uv, buf_desc->base_addr_uv_virtual);
    ret = venc_push_ds_buffer(ctx->strm_ctx, buf_desc);
    if (0 == ret)
        return TEE_ERROR_OVERFLOW;
    else if (ret < 0)
        return TEE_ERROR_BAD_STATE;
    return 0;
}

static TEE_Result ta_venc_push_buf(VPU_SESSION_CTX *session_ctx,
                                   uint32_t parameters_type,
                                   TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    uint32_t idx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    idx = params[1].value.b;
    if (idx >= N_BUFSLOTS) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    switch(params[1].value.a) {
    /* encoder fw graphics input queue */
    case SYNA_VPU_FW_VENC_VID:
        ret = venc_push_frame_buf(ctx, ctrl, idx);
        break;
    /* encoder fw encoded bitstream output */
    case SYNA_VPU_FW_VENC_STRM:
        ret = venc_push_es_buf(ctx, ctrl, idx);
        break;
    /* encoder fw reconstruction buf queue */
    case SYNA_VPU_FW_VENC_RECON:
        ret = venc_push_ref_buf(ctx, ctrl, idx);
        break;
    case SYNA_VPU_FW_VENC_DS:
        ret = venc_push_ds_buf(ctx, ctrl, idx);
        break;
#if 0
    case 3:
        /* Will not implement for H1, due to bad performance */
        /* ret = venc_push_pp_buf(pVECtx, ctrl, params[1].value.b); */
#endif
    default:
        ret = TEE_ERROR_BAD_PARAMETERS;
        break;
    }

bail:
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

static int venc_pop_ds_buf(struct i_vpu_ctx *ctx, struct syna_vpu_ctrl *ctrl,
                           uint32_t *idx)
{
    struct venc_vid_buf_desc_t *buf_desc;
    struct syna_vpu_tz_buf *buf;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    ret = venc_pop_ds_buffer(ctx->strm_ctx, &buf_desc);

    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
    } else if (ret > 0 && buf_desc) {
        if (((void *)buf_desc) < (void *)&pool->ds_desc[0] ||
            ((void *)buf_desc) > (void *)&pool->ds_desc[MAX_SLOT - 1]) {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            return ret;
        }
        buf = &(ctrl->dsbufs[buf_desc->id]);
        if (!(buf->planes[0].gfp_flags & BERLIN_GFP_FLAG_NO_CACHE_INVALIDATE)) {
            TEE_InvalidateCache((void *)buf_desc->base_addr_virtual,
                                (size_t)buf->planes[0].size);
        }
        *idx = buf_desc->id;
        VPU_BUF_TRACE("++++++++ pop ds %ld\n", *idx);
    }
    return ret;
}

static int venc_pop_frame_buf(struct i_vpu_ctx *ctx, uint32_t *idx)
{
    struct venc_vid_buf_desc_t *buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    ret = venc_pop_video_buffer(ctx->strm_ctx, &buf_desc);

    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
    } else if (ret > 0 && buf_desc) {
        if (((void *)buf_desc) < (void *)&pool->vid_desc[0]
            || ((void *)buf_desc) > (void *)&pool->vid_desc[MAX_SLOT - 1])
        {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            return ret;
        }
        *idx = buf_desc->id;
     VPU_BUF_TRACE("++++++++ pop vid %ld\n", *idx);
    }
    return ret;
}

static int venc_pop_es_buf(struct i_vpu_ctx *ctx,
                           struct syna_vpu_ctrl *ctrl, uint32_t *idx)
{
    struct syna_vpu_tz_buf *buf;
    struct venc_strm_buf_desc_t *buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    ret = venc_pop_stream_buffer(ctx->strm_ctx, &buf_desc);

    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
    } else if (ret > 0 && buf_desc) {
        if ((((void *)buf_desc) < (void *)&pool->strm_desc[0]) ||
            ((void *)buf_desc > (void *)&pool->strm_desc[MAX_SLOT - 1])) {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            return ret;
        }

        *idx = buf_desc->id;
        buf = &(ctrl->outbufs[*idx]);
        buf->flags = 0;

        if (STREAM_PART_SEQ_HEADER == buf_desc->part) {
            buf->flags |= BERLIN_BUF_FLAG_SEQ_HEADER;
            if (ctx->merge_header) {
                if (buf_desc->valid_data > ES_HEADER_BUF_LEN) {
                    VPU_BUF_TRACE("++++++++ insufficient header buf size, "
                                  "buf:%d, actual:%d\n",
                                  ES_HEADER_BUF_LEN, buf_desc->valid_data);
                    return TEE_ERROR_SHORT_BUFFER;
                }
                memcpy(ctx->header_mgr.header, buf_desc->vaddr,
                       MIN(ES_HEADER_BUF_LEN, buf_desc->valid_data));
                ctx->header_mgr.header_len = buf_desc->valid_data;
                ctx->header_mgr.got_header = true;
                // CA never push first DS buf corresponding to first ES buf.
                // As this ES buf will be repushed, do the same for DS buf.
                if (ctx->enc_ds_en)
                    venc_push_ds_buf(ctx, ctrl, *idx);
                return TEE_ERROR_CANCEL;
            }
        } else if (ctx->header_mgr.got_header && ctx->merge_header) {
            buf->flags &= ~BERLIN_BUF_FLAG_SEQ_HEADER;
            merge_header(ctx, *idx);
        }

        buf->bytesused[0] = buf_desc->valid_data;

        // Unnecessary invalidate cache ops cases:
        // 1. VP8 ES
        //   Owing to VP8 HW output are multiple scatter chunks, after HW done,
        //   FW will invalidate cache, and memmove to get one buf with
        //   contiguous data.
        // 2. H264 SPS/PPS header
        //   They are written by FW, no need to invalidate cache.
        if (ctx->vp8_enc || (buf->flags & BERLIN_BUF_FLAG_SEQ_HEADER)) {
            TEE_CleanCache((void *)buf_desc->vaddr,
                           (size_t)buf_desc->valid_data);
        } else if (!(buf->planes[0].gfp_flags & BERLIN_GFP_FLAG_NO_CACHE_INVALIDATE)) {
            TEE_InvalidateCache((void *)buf_desc->vaddr,
                                (size_t)buf_desc->valid_data);
        }

        VPU_BUF_TRACE("++++++++ pop es %ld, sz 0x%x\n", *idx, buf->bytesused[0]);
    }

    return ret;

}

static int venc_pop_ref_buf(struct i_vpu_ctx *ctx, uint32_t *idx)
{
    struct venc_vid_buf_desc_t *buf_desc;
    i_venc_pool_t *pool = (i_venc_pool_t *)ctx->desc_pool;
    int ret;

    ret = venc_pop_reference_buffer(ctx->strm_ctx, &buf_desc);

    if (0 == ret) {
        ret = TEE_ERROR_NO_DATA;
    } else if (ret > 0 && buf_desc) {
        if (((void *)buf_desc < (void *)&pool->ref_desc[0]) ||
            ((void *)buf_desc > (void *)&pool->ref_desc[MAX_SLOT - 1])) {
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            return ret;
        }
        *idx = buf_desc->id;
        VPU_BUF_TRACE("++++++++ pop ref %ld\n", *idx);
    }
    return ret;

}

static TEE_Result ta_venc_pop_buf(VPU_SESSION_CTX *session_ctx,
                                  uint32_t parameters_type,
                                  TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    uint32_t idx = 0;
    int ret = 0;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    switch(params[1].value.a) {
    case SYNA_VPU_FW_VENC_VID:
        ret = venc_pop_frame_buf(ctx, &idx);
        break;
    case SYNA_VPU_FW_VENC_STRM:
        ret = venc_pop_es_buf(ctx, ctrl, &idx);
        break;
    case SYNA_VPU_FW_VENC_RECON:
        ret = venc_pop_ref_buf(ctx, &idx);
        break;
    case SYNA_VPU_FW_VENC_DS:
        ret = venc_pop_ds_buf(ctx, ctrl, &idx);
        break;
#if 0
    case 3:
        /* ret = venc_pop_pp_buf(pVECtx, ctrl, params[1].value.b); */
        break;
#endif
    default:
        ret = TEE_ERROR_BAD_PARAMETERS;
        break;
    }

bail:
    params[1].value.b = idx;
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

static TEE_Result ta_venc_encode_stream(VPU_SESSION_CTX * session_ctx,
                                        uint32_t parameters_type,
                                        TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    venc_strm_status_t *status;
    uint32_t idx = 0;
    bool loop = true;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto failed;
    }

    if (!ctx->secure) {
        if (ctx->in_sec_flags || ctx->out_sec_flags || ctx->aux_sec_flags)
            ctx->secure = true;
    }

    if (ctx->secure) {
        /* encoder didn't have a secure endpoint */
        ret = TEE_ERROR_SECURITY;
        goto failed;

        if (ctx->out_insec_flags || ctx->aux_insec_flags) {
            ret = TEE_ERROR_SECURITY;
            goto failed;
        }
    }
    idx_queue_clear(&ctrl->sbuf);
    idx_queue_clear(&ctrl->dbuf);
    idx_queue_clear(&ctrl->rbuf);
    idx_queue_clear(&ctrl->dsbuf);

    do {
        ret = venc_encode_stream(ctx->strm_ctx, &status);

#if 0
        if (status->wait_for_int) {
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INTERRUPTER;
            loop = false;
            break;
        }
#endif
        if ((ret < 0) || (NULL == status)) {
            ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
            break;
        }

        switch(status->event_id) {
        case VENC_EVENT_END_OF_STREAM:
            loop = false;
            ret = 0;
            break;
        case VENC_EVENT_ALLOC_REF_BUF:
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_AUX_BUF;
            loop = false;
            ret = 0;
            break;
        case VENC_EVENT_ALLOC_VID_BUF:
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INPUT_BUF;
            loop = false;
            ret = 0;
            break;
        case VENC_EVENT_ALLOC_DS_BUF:
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_DS_BUF;
            loop = false;
            ret = 0;
            break;
        case VENC_EVENT_ALLOC_STR_BUF:
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_OUTPUT_BUF;
            loop = false;
            ret = 0;
            break;
        case VENC_EVENT_SECOND_PASS_START:
            if (ctx->ltr_en)
                status->param_2ndpass->ltrf |= 0x80000000;
            if (TEST_BIT(ctx->cur_encoding_buf_idx,
                         ctrl->venc_2ndpass_para_stats))
                memcpy(status->param_2ndpass,
                       &ctrl->venc_2ndpass_para[ctx->cur_encoding_buf_idx],
                       sizeof(venc_2ndpass_param_t));
            break;
        case VENC_EVENT_SECOND_PASS_END:
            if (TEST_BIT(ctx->cur_encoding_buf_idx,
                         ctrl->venc_2ndpass_para_stats))
                memset(status->param_2ndpass, 0x0,
                       sizeof(venc_2ndpass_param_t));
            break;
        case VENC_EVENT_CHECK_DS_BUF:
            ret = venc_pop_ds_buf(ctx, ctrl, &idx);

            if ((int)TEE_ERROR_NO_DATA == ret) {
                VMETA_LOGE("no ds buffer for popping.");
            } else if (ret < 0) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            } else {
                idx_queue_store(&ctrl->dsbuf, ctrl->dsbuf.pop, idx);
            }
            break;
        case VENC_EVENT_CHECK_VID_BUF:
            ret = venc_pop_frame_buf(ctx, &idx);

            if ((int)TEE_ERROR_NO_DATA == ret) {
                VMETA_LOGE("no frame buffer for popping, why this event?");
            } else if (ret < 0) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            } else {
                idx_queue_store(&ctrl->dbuf, ctrl->dbuf.pop, idx);
            }
            break;
        case VENC_EVENT_CHECK_REF_BUF:
            ret = venc_pop_ref_buf(ctx, &idx);

            if ((int)TEE_ERROR_NO_DATA == ret) {
                VMETA_LOGE("no ref buffer for popping, why this event?");
            } else if (ret < 0) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            } else {
                idx_queue_store(&ctrl->rbuf, ctrl->rbuf.pop, idx);
            }
            break;
        case VENC_EVENT_CHECK_STR_BUF:
        case VENC_EVENT_END_OF_FRAME:
            /* NOTE: vmeta encoder should not pop es buffer here */
            ret = venc_pop_es_buf(ctx, ctrl, &idx);

            if ((int)TEE_ERROR_NO_DATA == ret) {
                VMETA_LOGE("no es buffer for popping, why this event?");
            } else if ((int)TEE_ERROR_CANCEL == ret) {
                venc_push_es_buf(ctx, ctrl, idx);
            } else if (ret < 0) {
                ctrl->status.flags |= BERLIN_VPU_STATUS_ERROR;
                loop = false;
            } else {
                idx_queue_store(&ctrl->sbuf, ctrl->sbuf.pop, idx);
                if (ctx->meta_mv_en) {
                    get_motion_vector(ctrl, idx, status->seq_desc);
                }
            }
            break;
        case VENC_EVENT_SWITCH_POINT:
            /*
             * NOTE: h1 fw would only send SWITCH_POINT event after
             * a CHECK_STR_BUF event is sent. That means the current
             * frame is encoded. Unless you push more input frame
             * into fw, which behavior is not supposed to happen,
             * fw would need a new frame to encode.
             */
            ctrl->status.flags |= BERLIN_VPU_STATUS_WAIT_INPUT_BUF;
            ctrl->status.flags |= BERLIN_VPU_STATUS_DONE;
            loop = false;
            ret = 0;
            break;
        default:
            break;
        }
    } while(loop);

failed:
    params[1].value.a = ret;

    return TEE_SUCCESS;
}

static TEE_Result ta_venc_stream_switch_in(VPU_SESSION_CTX * session_ctx,
                                           uint32_t parameters_type,
                                           TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    ret = venc_stream_switch_in(ctx->strm_ctx);
bail:
    params[1].value.a = ret;

    return TEE_SUCCESS;
}

static TEE_Result ta_venc_stream_switch_out(VPU_SESSION_CTX * session_ctx,
                                            uint32_t parameters_type,
                                            TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    if (get_internal_ctx(session_ctx, ctrl, &ctx)) {
        VMETA_LOGE("Fail to get pVECtx");
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    ret = venc_stream_switch_out(ctx->strm_ctx);

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_venc_destroy_instance(VPU_SESSION_CTX * session_ctx,
                                           uint32_t parameters_type,
                                           TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    struct i_vpu_ctx *ctx;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = release_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret) {
        VMETA_LOGE("Fail to get pVECtx: 0x%08x", ret);
        ret = TEE_ERROR_CORRUPT_OBJECT;
        goto bail;
    }

    if (ctrl->ctrl_buf.vaddr && ctrl->ctrl_buf.memid) {
        ret = vpu_tzmem_put(&ctrl->ctrl_buf);
        if (ret) {
            VMETA_LOGE("failed unmap ctrl buf(memid 0x%x), ret 0x%x\n", ctrl->ctrl_buf.memid, ret);
        }
    }
    if (ctrl->ctx_buf.vaddr && ctrl->ctx_buf.memid) {
        ret = vpu_tzmem_put(&ctrl->ctx_buf);
        if (ret) {
            VMETA_LOGE("failed unmap ctrl ctx_buf(memid 0x%x), ret 0x%x\n", ctrl->ctx_buf.memid, ret);
        }
    }

bail:
    params[1].value.a = ret;
    return TEE_SUCCESS;
}

static TEE_Result ta_set_level(VPU_SESSION_CTX *session_ctx,
                               uint32_t parameters_type, TEE_Param params[4])
{
    params[1].value.a = wrap_set_level(params[0].value.a);
    return TEE_SUCCESS;
}

static TEE_Result vpu_clk_ctrl(void *clk_addr, uint32_t enable)
{
    volatile uint32_t val;

    val = read32(clk_addr);
    if (enable) {
        val |= 1;
    } else {
        val &=(~1);
    }

    write32(clk_addr, val);
    return TEE_SUCCESS;
}

static TEE_Result ta_vpu_clk_ctrl(VPU_SESSION_CTX *session_ctx,
                                  uint32_t parameters_type,
                                  TEE_Param params[4])
{
    int ret;

    ret = vpu_clk_ctrl(session_ctx->clk_ctrl_va, params[0].value.a);
    params[1].value.a = ret;

    return TEE_SUCCESS;
}


static TEE_Result ta_vpu_reg_buf(VPU_SESSION_CTX * session_ctx,
                                 uint32_t parameters_type,
                                 TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    uint32_t type = params[1].value.a;
    uint32_t idx = params[1].value.b;
    struct i_vpu_ctx *ctx;
    struct syna_vpu_tz_buf buf;
    /* this from a shared memory */
    struct syna_vpu_tz_buf *buf_in;
    void *pool = NULL;
    uint32_t *sec_flags;
    uint32_t *insec_flags;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret)
        goto bail;

    if (idx >= N_BUFSLOTS) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    switch (type) {
    case 0:
        buf_in = &(ctrl->inbufs[idx]);
        sec_flags = &(ctx->in_sec_flags);
        insec_flags = &(ctx->in_insec_flags);
        break;
    case 1:
        buf_in = &(ctrl->outbufs[idx]);
        sec_flags = &(ctx->out_sec_flags);
        insec_flags = &(ctx->out_insec_flags);
        break;
    case 2:
        /**
         * NOTE: auxiliary buf may not be enabled in a context, so don't
         * use the convenient pointer here.
         */
        buf_in = &(ctrl->auxbufs[idx]);
        sec_flags = &(ctx->aux_sec_flags);
        insec_flags = &(ctx->aux_insec_flags);
        break;
    case SYNA_VPU_FW_VENC_DS:
        buf_in = &(ctrl->dsbufs[idx]);
        sec_flags = &(ctx->ds_sec_flags);
        insec_flags = &(ctx->ds_insec_flags);
        break;
    default:
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto bail;
    }

    if (session_ctx->vpu_type == VPU_V2G) {
        i_vdec_pool_t *dec_pool = ctx->desc_pool;
        if (type == SYNA_VPU_FW_VDEC_STRM)
            pool = dec_pool->strm_desc;
        else if (type == SYNA_VPU_FW_VDEC_REF)
            pool = dec_pool->ref_desc;
        else if (type == SYNA_VPU_FW_VDEC_DISP)
            pool = dec_pool->dis_desc;
    } else {
        i_venc_pool_t *enc_pool = ctx->desc_pool;
        if (type == SYNA_VPU_FW_VENC_VID)
            pool = enc_pool->vid_desc;
        else if (type == SYNA_VPU_FW_VENC_STRM)
            pool = enc_pool->strm_desc;
        else if (type == SYNA_VPU_FW_VENC_RECON)
            pool = enc_pool->ref_desc;
        else if (type == SYNA_VPU_FW_VENC_DS)
            pool = enc_pool->ds_desc;
    }

    TEE_MemMove(&buf, buf_in, sizeof(buf));

    ret = vpu_reg_buffer(&buf, pool, idx, buf.buf_type);
    if (ret) {
        goto bail;
    }

    if (syna_vpu_tz_buf_sec_validate(&buf, 0)) {
        *sec_flags |= 1U << idx;
        *insec_flags &= ~(1U << idx);
    } else {
        *sec_flags &= ~(1U << idx);
        *insec_flags |= 1U << idx;
    }

    /* feedback result, only for debug */
    TEE_MemMove(buf_in, &buf, sizeof(buf));

bail:
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

static TEE_Result ta_vpu_unreg_buf(VPU_SESSION_CTX * session_ctx,
                                   uint32_t parameters_type,
                                   TEE_Param params[4])
{
    struct syna_vpu_ctrl *ctrl;
    uint32_t type = params[1].value.a;
    uint32_t idx = params[1].value.b;
    struct syna_vpu_tz_buf *buf;
    struct i_vpu_ctx *ctx;
    void *pool;
    int ret;

    ctrl = (struct syna_vpu_ctrl *)params[0].memref.buffer;

    ret = get_internal_ctx(session_ctx, ctrl, &ctx);
    if (ret)
        goto bail;

    if (session_ctx->vpu_type == VPU_V2G) {
        i_vdec_pool_t *dec_pool = ctx->desc_pool;
        switch (type) {
        case SYNA_VPU_FW_VDEC_STRM:
            pool = &dec_pool->strm_desc[0];
            buf = &ctrl->inbufs[idx];
            break;
        case SYNA_VPU_FW_VDEC_REF:
            pool = &dec_pool->ref_desc[0];
            buf = &ctrl->outbufs[idx];
            break;
        case SYNA_VPU_FW_VDEC_DISP:
            pool = &dec_pool->dis_desc[0];
            buf = &ctrl->auxbufs[idx];
            break;
        default:
            ret = TEE_ERROR_BAD_PARAMETERS;
            goto bail;
        }
    } else {
        i_venc_pool_t *enc_pool = ctx->desc_pool;
        switch (type) {
        case SYNA_VPU_FW_VENC_VID:
            pool = &enc_pool->vid_desc[0];
            buf = &ctrl->inbufs[idx];
            break;
        case SYNA_VPU_FW_VENC_STRM:
            pool = &enc_pool->strm_desc[0];
            buf = &ctrl->outbufs[idx];
            break;
        case SYNA_VPU_FW_VENC_RECON:
            pool = &enc_pool->ref_desc[0];
            buf = &ctrl->auxbufs[idx];
            break;
        case SYNA_VPU_FW_VENC_DS:
            pool = &enc_pool->ds_desc[0];
            buf = &ctrl->dsbufs[idx];
            break;
        default:
            ret = TEE_ERROR_BAD_PARAMETERS;
            goto bail;
        }
    }

    ret = vpu_unreg_buffer(buf, pool, idx, buf->buf_type);

bail:
    params[2].value.a = ret;

    return TEE_SUCCESS;
}

// REMIND: This table must follow VMETA_CMD enum seq
// parameters_type:
// - CA side use tee_client_const.h/TEEC_xxx
// - TA side use tee_internal_core_common.h/TEE_PARAM_TYPE_xxx
// - CA -> TA mapping (TZ do this part)
//      - TEEC_MEMREF_WHOLE(0xC) -> TEE_PARAM_TYPE_MEMREF_INOUT(0x7)
//      - TEEC_MEMREF_PARTIAL_xxx -> TEE_PARAM_TYPE_MEMREF_xxx
static const TA_CMD_HANDLER ta_cmd_handler[] =
{
    {
        VMETA_GET_CONST_V2,
        ta_vpu_get_const,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        1,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eVpuConstSize},
        }
    },
    {
        VMETA_OPEN_V2,
        ta_vpu_open,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        1,
        {
            /* both decoder and encoder would call here, it is not easy
             * to figure its context size, would check it in the function.
             */
            {eUnknown, TEE_NONSECURE, TEE_MT_COPY, 0},
        }
    },
    {
        VMETA_CLOSE_V2,
        ta_vpu_close,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        0,
        {
        }
    },
    {
        VPU_REG_BUF_V2,
        ta_vpu_reg_buf,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VPU_UNREG_BUF_V2,
        ta_vpu_unreg_buf,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VPU_SET_DEBUG_LEVEL_V2,
        ta_set_level,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        1,
        {{eNum}}
    }, // log level
    {
        VPU_CLK_CTRL_V2,
        ta_vpu_clk_ctrl,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        1,
        {
            {eNum},
        }
    }, // clock control
    {
        VDEC_CREATE_INSTANCE_V2,
        ta_vdec_create_instance,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VDEC_CONFIG_STREAM_V2,
        ta_vdec_config_stream,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        3,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eVdecStrmCfgSize},
            {eNum},
        }
    },
    {
        VDEC_PUSH_BUFFER_V2,
        ta_vdec_push_buffer,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        3,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
            {eNum},
        }
    },
    {
        VDEC_POP_BUFFER_V2,
        ta_vdec_pop_buffer,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        3,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
            {eNum},
        }
    },
    {
        VDEC_DECODE_STREAM_V2,
        ta_vdec_decode_stream,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VDEC_DRAIN_POOL_V2,
        ta_vdec_drain_pool,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        3,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
            {eNum},
        }
    },
    {
        VDEC_ABORT_HW_V2,
        ta_vdec_int_timeout,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VDEC_RESET_INSTANCE_V2,
        ta_vdec_reset_instance,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VDEC_STREAM_SWITCH_IN_V2,
        ta_vdec_stream_switch_in,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INOUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VDEC_STREAM_SWITCH_OUT_V2,
        ta_vdec_stream_switch_out,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VDEC_DESTROY_INSTANCE_V2,
        ta_vdec_destroy_context,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        1,
        {{eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize}}
    },
    {
        VENC_CREATE_INSTANCE_V2,
        ta_venc_create_instance,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VENC_CONFIG_STREAM_V2,
        ta_venc_config_stream,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eVencStrmCfgSize},
        }
    },
    {
        VENC_PUSH_BUFFER_V2,
        ta_venc_push_buf,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
            {eNum},
        }
    },
    {
        VENC_POP_BUFFER_V2,
        ta_venc_pop_buf,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
            {eNum},
        }
    },
    {
        VENC_ENCODE_STREAM_V2,
        ta_venc_encode_stream,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VENC_CLOSE_STREAM_V2,
        ta_venc_close_stream,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VENC_STREAM_SWITCH_IN_V2,
        ta_venc_stream_switch_in,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VENC_STREAM_SWITCH_OUT_V2,
        ta_venc_stream_switch_out,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        2,
        {
            {eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },
    {
        VENC_DESTROY_INSTANCE_V2,
        ta_venc_destroy_instance,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
                        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE),
        1,
        {{eOutput, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize}}

    },
    {
        VPU_CTX_SWITCH_INST_v2,
        ta_vpu_ctx_switch_inst,
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_VALUE_OUTPUT, TEE_PARAM_TYPE_NONE),
        3,
        {
            /* Nullable */
            {eUnknown, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            /* Nullable */
            {eUnknown, TEE_NONSECURE, TEE_MT_COPY, eSynaVpuCtrlSize},
            {eNum},
        }
    },

    {VMETA_CMD_MAX_V2, NULL, 0, 0}
};

static const char *showVmetaCmd(uint32_t cmd) {
#define GET_ENUM_NAME(var)   case var: return #var;
    switch (cmd) {
    GET_ENUM_NAME(VMETA_GET_CONST_V2);
    GET_ENUM_NAME(VMETA_OPEN_V2);
    GET_ENUM_NAME(VMETA_CLOSE_V2);
    GET_ENUM_NAME(VPU_REG_BUF_V2);
    GET_ENUM_NAME(VPU_UNREG_BUF_V2);
    GET_ENUM_NAME(VPU_SET_DEBUG_LEVEL_V2);
    GET_ENUM_NAME(VPU_CLK_CTRL_V2);
    GET_ENUM_NAME(VDEC_CREATE_INSTANCE_V2);
    GET_ENUM_NAME(VDEC_CONFIG_STREAM_V2);
    GET_ENUM_NAME(VDEC_PUSH_BUFFER_V2);
    GET_ENUM_NAME(VDEC_POP_BUFFER_V2);
    GET_ENUM_NAME(VDEC_DECODE_STREAM_V2);
    GET_ENUM_NAME(VDEC_DRAIN_POOL_V2);
    GET_ENUM_NAME(VDEC_ABORT_HW_V2);
    GET_ENUM_NAME(VDEC_RESET_INSTANCE_V2);
    GET_ENUM_NAME(VDEC_STREAM_SWITCH_IN_V2);
    GET_ENUM_NAME(VDEC_STREAM_SWITCH_OUT_V2);
    GET_ENUM_NAME(VDEC_DESTROY_INSTANCE_V2);
    GET_ENUM_NAME(VENC_CREATE_INSTANCE_V2);
    GET_ENUM_NAME(VENC_CONFIG_STREAM_V2);
    GET_ENUM_NAME(VENC_PUSH_BUFFER_V2);
    GET_ENUM_NAME(VENC_POP_BUFFER_V2);
    GET_ENUM_NAME(VENC_ENCODE_STREAM_V2);
    GET_ENUM_NAME(VENC_CLOSE_STREAM_V2);
    GET_ENUM_NAME(VENC_STREAM_SWITCH_IN_V2);
    GET_ENUM_NAME(VENC_STREAM_SWITCH_OUT_V2);
    GET_ENUM_NAME(VENC_DESTROY_INSTANCE_V2);
    GET_ENUM_NAME(VPU_CTX_SWITCH_INST_v2);
    default:
        return "Unknown";
    }
}

TEE_Result InitSessionV2(VPU_SESSION_CTX *sessionContext)
{
    return TEE_SUCCESS;
}

void DeinitSessionV2(VPU_SESSION_CTX *sessionContext)
{
}

TEE_Result InvokeCommandV2(VPU_SESSION_CTX *session, uint32_t command_id,
                           uint32_t parameters_type,
                           TEE_Param params[4])
{
    TEE_Result res = TEE_ERROR_NOT_SUPPORTED;
    VPU_SESSION_CTX *session_ctx = session;
    const TA_CMD_HANDLER *handler;
    const CheckParam *cp;
    void *bufptr;
    uint32_t bufsize;
    unsigned int i;
    uint32_t size;

    static_assert(ARRAY_SIZE(ta_cmd_handler) == (VMETA_CMD_MAX_V2 + 1),
                  "VMETA command has a gap value!");

    static_assert(sizeof(struct i_vpu_ctx) < BERLIN_VPU_CTX_RESERVED_SIZE,
                  "i_vpu_ctx too large!");

    static_assert(sizeof(struct syna_venc_strm_config) ==
                  sizeof(struct venc_strm_config_t),
                  "syna_venc_strm_config mismatch FW!");

    static_assert(sizeof(struct syna_vdec_config) ==
                  sizeof(struct vdec_strm_config_t),
                  "syna_vdec_config mismatch FW!");

    if (session_ctx == NULL)
        return TEE_ERROR_ITEM_NOT_FOUND;

    handler = &ta_cmd_handler[command_id];
    if (command_id >= VMETA_CMD_MAX_V2 || handler->cmd_handler == NULL)
        return TEE_ERROR_BAD_PARAMETERS;

    if (handler->paramsNum > MAX_PARAM) {
        VMETA_LOGE("cmd=%u assert wrong paramsNum=%u > %u",
                   command_id, handler->paramsNum, MAX_PARAM);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    if (handler->command_id != command_id) {
        VMETA_LOGE("cmd=%u cannot directly index in CheckParam table", command_id);
        return TEE_ERROR_BAD_STATE;
    }

    for (i = 0; i != handler->paramsNum; i++) {
        cp = &handler->params[i];
        bufptr = (void *)params[i].memref.buffer;
        bufsize = params[i].memref.size;
        switch (cp->iotype) {
            case eInput:
            case eOutput:
                /* memory right has been in each functions when it needs */
                break;
            case eVpuId:
            case eNum: // valication check by each function
            case eUnknown:
                continue; // skip buffer size checking
            case eForbidden:
            default:
                VMETA_LOGE("cmd=%u i=%u assert wrong iotype=%d",
                      command_id, i, cp->iotype);
                return TEE_ERROR_BAD_PARAMETERS;
        }

        size = getBufSize(cp->buftype);
        if (bufsize != size) {
            VMETA_LOGE("cmd=%s(%u) i=%u check ptr=%p size=%u != expect=%u\n",
                       showVmetaCmd(command_id), command_id,
                        i, bufptr, bufsize, size);
            return TEE_ERROR_BAD_PARAMETERS;
        }
    }

    VPU_CMD_HANDER_TRACE("cmd=%s start\n", showVmetaCmd(command_id));
    res = handler->cmd_handler(session_ctx, parameters_type, params);
    VPU_CMD_HANDER_TRACE("cmd=%s end(0x%x)\n", showVmetaCmd(command_id), res);
    return res;
}
