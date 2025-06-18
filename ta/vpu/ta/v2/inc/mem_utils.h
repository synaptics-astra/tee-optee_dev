#ifndef _VPU_MEM_UTILS_H_
#define _VPU_MEM_UTILS_H_

#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#else
#include "tee_internal_core_api.h"
#include "tee_ta.h"
#endif // CONFIG_OPTEE
#include "vpu_api.h"
#include "vpu_fw_data.h"

typedef u32 phys_addr_t;

struct i_vdec_bs_buf {
    vdec_strm_buf_desc_t desc;
    struct syna_tz_generic_buf buf;
};

#define TZMEM_INIT_PHY(ptzmem, t, phy, sz, off, flag)                          \
  do {                                                                         \
    TEE_MemFill((ptzmem), 0, sizeof(*(ptzmem)));                               \
    (ptzmem)->type = (t);                                                      \
    (ptzmem)->addr = (phy);                                                    \
    (ptzmem)->size = (sz);                                                     \
    (ptzmem)->offset = (off);                                                  \
    (ptzmem)->gfp_flags = (flag);                                              \
  } while (0)

#define TZMEM_INIT_MEMID(ptzmem, t, id, sz, off, flag)                         \
  do {                                                                         \
    TEE_MemFill((ptzmem), 0, sizeof(*(ptzmem)));                               \
    (ptzmem)->type = (t);                                                      \
    (ptzmem)->memid = (id);                                                    \
    (ptzmem)->size = (sz);                                                     \
    (ptzmem)->offset = (off);                                                  \
    (ptzmem)->gfp_flags = (flag);                                              \
  } while (0)

TEE_Result vpu_tzmem_get(struct syna_tz_generic_buf *tzmem);

TEE_Result vpu_tzmem_put(struct syna_tz_generic_buf *tzmem);

int vpu_reg_buffer(struct syna_vpu_tz_buf *buf, void *pool,
                       uint32_t idx, uint32_t type);

int vpu_unreg_buffer(struct syna_vpu_tz_buf *buf, void *pool,
                          uint32_t idx,  uint32_t type);
#endif //_VPU_MEM_UTILS_H_