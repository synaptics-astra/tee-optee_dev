#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "mem_region_userdata.h"

#include "vmeta_log.h"
#include "mem_utils.h"

//#define TZMEM_DEBUG

#ifdef TZMEM_DEBUG
#define TZ_MEM_PRINT(tzmem)                                                    \
    TA_LOGL("tz-mem: t:%d id:0x%x, p:%" PRIx32                                   \
          " v:%p, sz: 0x%x, off:0x%x, f:0x%x\n",                               \
          (tzmem)->type, (tzmem)->memid, (uint32_t)((tzmem)->addr),            \
          (void *)((tzmem)->vaddr), (tzmem)->size, (tzmem)->offset,            \
          (tzmem)->gfp_flags);

#define TZ_MEM_GET_TRACE(tzmem)                                                \
  do {                                                                         \
    TA_LOGL("tz-mem get enter %p\n", (tzmem));                              \
    TZ_MEM_PRINT(tzmem);                                                       \
  } while (0)

#define TZ_MEM_PUT_TRACE(tzmem)                                                \
  do {                                                                         \
    TA_LOGL("tz-mem put enter %p\n", (tzmem));                              \
    TZ_MEM_PRINT(tzmem);                                                       \
  } while (0)

#else
#define TZ_MEM_PUT_TRACE(tzmem)
#define TZ_MEM_GET_TRACE(tzmem)
#define TZ_MEM_PRINT(tzmem)
#endif

static TEE_Result get_tzmem_by_memid(struct syna_tz_generic_buf *tzmem)
{
    TEE_Result ret = 0;
    uint32_t memid = tzmem->memid;
    void *vaddr;
    size_t tmpSize;

    if (!memid)
        return ret;

    ret = TEE_MemRef(memid);
    if (ret) {
        VMETA_LOGE("fail: invalid memid: %d\n", memid);
        return ret;
    }

    if (tzmem->type == MEM_ID_CONTINOUS) {
        ret = TEE_GetPhys_By_Memid(memid, (paddr_t *)&tzmem->addr);
        if (ret) {
            VMETA_LOGE("fail: get phy by memid: %d\n", memid);
            goto failed_unref;
        }
    } else if (tzmem->type == MEM_ID_SCATTER) {
    // TODO: get PageTable Phy if smm ta support it
    }

    if (tzmem->gfp_flags & BERLIN_GFP_FLAG_NO_MAPPING) {
        tzmem->vaddr = (uintptr_t)NULL;
        TZ_MEM_PRINT(tzmem);
        return 0;
    }

    ret = TEE_Map_Memory(memid, &vaddr, &tmpSize);
    if (ret || tmpSize < tzmem->size) {
        VMETA_LOGE("can't map %d: ret=0x%x, size=%d, excepted size=%d\n", memid,
                   ret, tmpSize, tzmem->size);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto failed_unref;
    }

    if (tzmem->offset > tmpSize) {
        VMETA_LOGE("memid %d offset overflow, offset: 0x%x, size: 0x%x\n",
                   memid, tzmem->offset, tmpSize);
        ret = TEE_ERROR_OVERFLOW;
        goto failed_unmap;
    }

    tzmem->vaddr = (uintptr_t)vaddr;
    tzmem->size = tmpSize;
    TZ_MEM_PRINT(tzmem);
    return 0;

failed_unmap:
    TEE_Unmap_Memory(memid);
failed_unref:
    TEE_MemUnref(memid);
    return ret;
}

static TEE_Result get_tzmem_by_phy(struct syna_tz_generic_buf *tzmem)
{
    TEE_Result ret = 0;
    uint32_t memid = 0;
    size_t tmpSize = 0;
    void *vaStart = NULL;

    uint32_t phy = (uint32_t)tzmem->addr;
    size_t size = tzmem->size;
    if (!phy || !size) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    ret = TEE_Register_Memory((paddr_t)phy, size, &memid);
    if (ret) {
        VMETA_LOGE("fail: phy reg mem 0x%lx %ld\n", phy, size);
        return ret;
    }

    if (tzmem->gfp_flags & BERLIN_GFP_FLAG_NO_MAPPING) {
        tzmem->memid = memid;
        tzmem->vaddr = (uintptr_t)NULL;
        return 0;
    }

    ret = TEE_Map_Memory(memid, &vaStart, &tmpSize);
    if (ret != TEE_SUCCESS) {
        VMETA_LOGE("fail:mmap ret:0x%lx, id 0x%lx", ret, memid);
        goto failed_unreg;
    }

    tzmem->vaddr = (uintptr_t)vaStart;
    tzmem->memid = memid;

    TZ_MEM_PRINT(tzmem);
    return TEE_SUCCESS;

failed_unreg:
    TEE_Unregister_Memory(memid);
    return ret;
}

TEE_Result vpu_tzmem_get(struct syna_tz_generic_buf *tzmem)
{
    if (!tzmem)
        return TEE_ERROR_BAD_PARAMETERS;

    TZ_MEM_GET_TRACE(tzmem);
    if (tzmem->memid && tzmem->addr &&
        tzmem->gfp_flags & BERLIN_GFP_FLAG_NO_MAPPING)
        return TEE_SUCCESS;

    if (tzmem->type == PHY_CONTINOUS)
        return get_tzmem_by_phy(tzmem);
    else
        return get_tzmem_by_memid(tzmem);
}

TEE_Result  vpu_tzmem_put(struct syna_tz_generic_buf *tzmem)
{
    uint32_t memid = tzmem->memid;
    TEE_Result ret = 0;

    TZ_MEM_PUT_TRACE(tzmem);
    if (!memid)
        return 0;

    if (tzmem->vaddr) {
        ret = TEE_Unmap_Memory(memid);
        if (ret) {
            VMETA_LOGE("unmap 0x%lx failed 0x%lx", memid, ret);
            return ret;
        }
    }

    if (PHY_CONTINOUS == tzmem->type)
        ret = TEE_Unregister_Memory(memid);
    else
        ret = TEE_MemUnref(memid);
    if (ret) {
        VMETA_LOGE("fail:unreg/unref 0x%lx, 0x%lx\n", memid, ret);
        return ret;
    }

    tzmem->vaddr = (uintptr_t)NULL;
    tzmem->memid = 0;
    return TEE_SUCCESS;
}

static int reg_vdec_es_buf(struct syna_vpu_tz_buf *buf,
                            void *pool, uint32_t idx)
{
    struct i_vdec_bs_buf *bs_buf;
    vdec_strm_buf_desc_t *buf_desc;
    int ret;

    ret = vpu_tzmem_get(&buf->planes[0]);
    if (ret) {
        return ret;
    }

    bs_buf = pool;
    bs_buf += idx;
    buf_desc = &(bs_buf->desc);

    TEE_MemFill(buf_desc, 0, sizeof(*buf_desc));
    TEE_MemMove(&(bs_buf->buf), &(buf->planes[0]),
                sizeof(struct syna_tz_generic_buf));
    /* NOTE: the address would be set in vdec_push_es_buf() */

    return ret;
}

static int reg_vdec_video_buf(struct syna_vpu_tz_buf *buf,
                               void *pool, uint32_t idx)
{
    vdec_vid_buf_desc_t *buf_desc;
    const unsigned int video_max_plane = 5;
    phys_addr_t addr;
    uintptr_t vaddr;
    u32 offset, offset1;
    int ret;
    unsigned int i;

    for (i = 0; i < video_max_plane; i++) {
        if (i == 2 || i == 3) {
            //don't map mtr y/c
            buf->planes[i].gfp_flags |= BERLIN_GFP_FLAG_NO_MAPPING;
        }
        ret = vpu_tzmem_get(&buf->planes[i]);
        if (ret) {
            goto failed;
        }
    }

    buf_desc = pool;
    buf_desc += idx;
    TEE_MemFill(buf_desc, 0, sizeof(*buf_desc));

    addr = buf->planes[0].addr;
    offset = buf->planes[0].offset;

    buf_desc->base_addr = addr + offset;

    addr = buf->planes[1].addr ? buf->planes[1].addr : buf->planes[0].addr;
    offset1 = buf->planes[1].offset;

    buf_desc->base_addr_uv = addr + offset1;

    /**
     * NOTE: graphics buffer could come from a buffer, especially when
     * the IOMMU is used. so we have to check bytesused here.
     */

    buf_desc->size = buf->bytesused[0] ? buf->bytesused[0] - offset
                                       : buf->planes[0].size - offset;

    buf_desc->size_uv = buf->bytesused[1] ? buf->bytesused[1] - offset1
                                          : buf->planes[1].size - offset1;

    vaddr = (uintptr_t)buf->planes[0].vaddr;
    buf_desc->base_addr_virtual = (void *)(vaddr + offset);

    vaddr = (uintptr_t)(buf->planes[1].addr ? buf->planes[1].vaddr
                                            : buf->planes[0].vaddr);
    buf_desc->base_addr_uv_virtual = (void *)(vaddr + offset1);

    buf_desc->meta_addr = buf->planes[2].addr + buf->planes[2].offset;
    buf_desc->meta_addr_uv = buf->planes[3].addr ?
        buf->planes[3].addr + buf->planes[3].offset
        : buf->planes[2].addr + buf->planes[3].offset;
    /**
     * NOTE: meta buffers could all come from a buffer, so we have to use
     * bytesused here.
     */
    buf_desc->meta_buf_size = buf->bytesused[2] - buf->planes[2].offset;
    buf_desc->meta_buf_uv_size = buf->bytesused[3] - buf->planes[3].offset;

    addr = buf->planes[BERLIN_BUF_V4G_MVTP_PLANE_IDX].addr;
    vaddr = (uintptr_t)buf->planes[BERLIN_BUF_V4G_MVTP_PLANE_IDX].vaddr;
    offset = buf->planes[BERLIN_BUF_V4G_MVTP_PLANE_IDX].offset;

    buf_desc->tctx_addr = addr + offset;
    buf_desc->tctx_addr_virtual = (void *)(vaddr + offset);
    buf_desc->tctx_buf_size = buf->planes[BERLIN_BUF_V4G_MVTP_PLANE_IDX].size
                              - offset;

    ret = 0;
    return ret;

failed:
    for (i = 0; i < video_max_plane; i++) {
        vpu_tzmem_put(&buf->planes[i]);
    }
    return ret;
}

static int reg_venc_video_buf(struct syna_vpu_tz_buf *buf,
                               void *pool, uint32_t idx)
{
    venc_vid_buf_desc_t *buf_desc;
    const unsigned int video_max_plane = 3;
    int ret;
    unsigned int i;

    for (i = 0; i < video_max_plane; i++) {
        ret = vpu_tzmem_get(&buf->planes[i]);
        if (ret) {
            VMETA_LOGE("fail: venc vidoe map tzmem\n");
            goto failed;
        }
    }

    buf_desc = pool;
    buf_desc += idx;

    TEE_MemFill(buf_desc, 0, sizeof(*buf_desc));

    buf_desc->base_addr = buf->planes[0].addr
        + buf->planes[0].offset;
    buf_desc->base_addr_virtual = (void *)(buf->planes[0].vaddr
        + buf->planes[0].offset);

    if (buf->planes[1].size) {
        buf_desc->base_addr_uv = buf->planes[1].addr ?
            buf->planes[1].addr + buf->planes[1].offset
            : buf->planes[0].addr + buf->planes[1].offset;

        buf_desc->base_addr_uv_virtual = buf->planes[1].vaddr ?
            (void *)(buf->planes[1].vaddr + buf->planes[1].offset)
            : (void *)(buf->planes[0].vaddr + buf->planes[1].offset);
    }

    if (buf->planes[2].size) {
        buf_desc->base_addr_uv1 = buf->planes[2].addr ?
            buf->planes[2].addr + buf->planes[2].offset
            : buf->planes[0].addr + buf->planes[2].offset;

        buf_desc->base_addr_uv1_virtual = buf->planes[2].vaddr ?
            (void *)(buf->planes[2].vaddr + buf->planes[2].offset)
            : (void *)(buf->planes[0].vaddr + buf->planes[2].offset);
    }

    return ret;

failed:
    for (i = 0; i < video_max_plane; i++) {
        vpu_tzmem_put(&buf->planes[i]);
    }
    return ret;
}

static int reg_venc_es_buf(struct syna_vpu_tz_buf *buf,
                            void *pool, uint32_t idx)
{
    venc_strm_buf_desc_t *buf_desc;
    int ret;

    buf_desc = pool;
    buf_desc += idx;

    ret = vpu_tzmem_get(&buf->planes[0]);
    if (ret)
        return ret;

    // for export motion vectors
    if (buf->planes[1].size > 0) {
        ret = vpu_tzmem_get(&buf->planes[1]);
        if (ret)
            return ret;
    }

    TEE_MemFill(buf_desc, 0, sizeof(*buf_desc));

    buf_desc->addr = buf->planes[0].addr
        + buf->planes[0].offset;
    buf_desc->vaddr = (void *)(buf->planes[0].vaddr
        + buf->planes[0].offset);
    buf_desc->size = buf->planes[0].size;

    return ret;
}

int vpu_reg_buffer(struct syna_vpu_tz_buf *buf,
                            void *pool, uint32_t idx, uint32_t type)
{
    int ret;
    switch (type) {
    case ENCODER_PICTURE:
        ret = reg_venc_video_buf(buf, pool, idx);
        break;
    case ENCODER_STREAM:
        ret = reg_venc_es_buf(buf, pool, idx);
        break;
    case DECODER_PICTURE:
        ret = reg_vdec_video_buf(buf, pool,idx);
        break;
    case DECODER_STREAM:
        ret = reg_vdec_es_buf(buf, pool, idx);
        break;
    default:
        ret = -1;
        VMETA_LOGE("unknow type %ld\n", type);
        break;
    }
    return ret;
}

int vpu_unreg_buffer(struct syna_vpu_tz_buf *buf, void *pool, uint32_t idx,
                          uint32_t type)
{
    void *desc = NULL;
    uint32_t item_size;
    uint8_t plane_num = 0;
    int i = 0;
    int ret = 0;

    switch (type) {
    case ENCODER_PICTURE:
        item_size = sizeof(venc_vid_buf_desc_t);
        plane_num = 3;
        break;
    case ENCODER_STREAM:
        item_size = sizeof(venc_strm_buf_desc_t);
        plane_num = 1;
        if (buf->planes[1].size > 0) {
            plane_num++;
        }
        break;
    case DECODER_PICTURE:
        item_size = sizeof(vdec_vid_buf_desc_t);
        plane_num = 5;
        break;
    case DECODER_STREAM:
        item_size = sizeof(struct i_vdec_bs_buf);
        plane_num = 1;
        break;
    default:
        ret = -1;
        VMETA_LOGE("unknow type %ld\n", type);
        return ret;
    }

    desc = (void *)((uintptr_t)pool + item_size * idx);
    TEE_MemFill(desc, 0, item_size);

    for (i = 0; i < plane_num; i++) {
        ret = vpu_tzmem_put(&buf->planes[i]);
        if (ret) {
            VMETA_LOGE("!!fail, venc reg %ld, %x\n", type, ret);
        }
    }
    return ret;
}
