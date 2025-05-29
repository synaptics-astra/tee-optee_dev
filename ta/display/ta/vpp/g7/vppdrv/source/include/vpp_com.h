/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_COMMON_H_
#define _VPP_COMMON_H_

/* maximum number of VPP objects */
#ifdef MAX_NUM_OBJECTS
#error "duplicated MACRO definition of MAX_NUM_OBJECTS."
#else
#define MAX_NUM_OBJECTS  1
#endif

/* maximum number of integer parameters of callback functions */
#ifdef MAX_NUM_PARAMS
#error "duplicated MACRO definition of MAX_NUM_PARAMS."
#else
#define MAX_NUM_PARAMS    10
#endif

/* maximum number of frames in frame buffer queue */
#ifdef MAX_NUM_FRAMES
#error "duplicated MACRO definition of MAX_NUM_FRAMES"
#else
#define MAX_NUM_FRAMES    20
#endif

/* maximum number of commands in command queue */
#ifdef MAX_NUM_CMDS
#error "duplicated MACRO definition of MAX_NUM_CMDS"
#else
#define MAX_NUM_CMDS      500
#endif

/* maximum number of events */
#ifdef MAX_NUM_EVENTS
#error "duplicated MACRO definition of MAX_NUM_EVENTS"
#else
#define MAX_NUM_EVENTS    32
#endif

/* BCM buffer size */
#ifdef BCM_BUFFER_SIZE
#error "duplicated MACRO definition of BCM_BUFFER_SIZE"
#else
/* DV1_BCM_BUFFER_SIZE + DV2_BCM_BUFFER_SIZE + DV3_BCM_BUFFER_SIZE = BCM_BUFFER_SIZE */
#define BCM_BUFFER_SIZE   0x17700//96000
#define DV1_BCM_BUFFER_SIZE   0x7d00//32000
#define DV2_BCM_BUFFER_SIZE   0x7d00//32000
#define DV3_BCM_BUFFER_SIZE   0x7d00//32000
#define VDE_BCM_BUFFER_SIZE   0x800
#endif

/* 3D De-interlacer field buffer size */
#ifdef DEINT_BUFFER_SIZE
#error "duplicated MACRO definition of DEINT_BUFFER_SIZE"
#else
//#define DEINT_BUFFER_SIZE   (1920*1082*26*3/16) // 3 1080I field (26-bit per pixel)
//#define DEINT_BUFFER_SIZE   (1920*1081*26*2/8) // 2 1080P frame (26-bit per pixel)
#define DEINT_BUFFER_SIZE   (1920*1081*26*3/8) // 6 1080I frame (26-bit per pixel)
#endif

/* CC data queue size */
#ifdef CC_DATA_QUEUE_SIZE
#error "duplicated MACRO definition of CC_DATA_QUEUE_SIZE"
#else
#define CC_DATA_QUEUE_SIZE   (100)
#endif

/* WSS data queue size */
#ifdef WSS_DATA_QUEUE_SIZE
#error "duplicated MACRO definition of WSS_DATA_QUEUE_SIZE"
#else
#define WSS_DATA_QUEUE_SIZE   (10)
#endif

/* CGMS data queue size */
#ifdef CGMS_DATA_QUEUE_SIZE
#error "duplicated MACRO definition of CGMS_DATA_QUEUE_SIZE"
#else
#define CGMS_DATA_QUEUE_SIZE   (10)
#endif

/* TT data length per frame */
#ifdef TT_DATA_LEN_PER_FRAME
#error "duplicated MACRO definition of TT_DATA_LEN_PER_FRAME"
#else
#define TT_DATA_LEN_PER_FRAME   (40 * 4)
#endif

/* TT data queue size */
#ifdef TT_DATA_QUEUE_SIZE
#error "duplicated MACRO definition of TT_DATA_QUEUE_SIZE"
#else
#define TT_DATA_QUEUE_SIZE   (TT_DATA_LEN_PER_FRAME * 10)
#endif

/* DMA command buffer size */
#ifdef DMA_CMD_BUFFER_SIZE
#error "duplicated MACRO definition of DMA_CMD_BUFFER_SIZE"
#else
#define DMA_CMD_BUFFER_SIZE   (100 * 8)
#endif

#ifndef DOLBY_HDMI_MDS_SIZE
  #define DOLBY_HDMI_MDS_SIZE          0x3800 // size DM_Metadata_SZ/(packet body size) * packet size
#endif

#ifndef DOLBY_CVM_LUT_SIZE
#define DOLBY_CVM_LUT_SIZE           0x1000 // 4K size
#define DOLBY_CVM_TmLUTI_OFFSET      0x0000
#define DOLBY_CVM_SmLUTI_OFFSET      0x0100
#define DOLBY_CVM_TmLUTS_OFFSET      0x0200
#define DOLBY_CVM_SmLUTS_OFFSET      0x0300
#define DOLBY_G2L_LUT_SIZE           0x400 // 1K size
#endif
/* Hardware interface constants */

/* Register width in Berlin is 32 bits */
typedef unsigned int REG_WIDTH;
#define SIZE_OF_REG     sizeof(REG_WIDTH)

// Packed structure and packed array attributes
#ifndef PACKED_STRUCT
    #ifdef __LINUX__
        #define PACKED_STRUCT __attribute__((packed))
        #define PACKED_ARRAY  __attribute__((aligned(1)))
    #else
        #define PACKED_STRUCT
        #define PACKED_ARRAY
    #endif
#endif

// VPP debug handling
#ifdef VPP_DEBUG
#define DbgEnterFunction(para)  DBGPRINTF(para);
#define DbgExitFunction(para)   DBGPRINTF(para);
#define DbgAssert(para)         ASSERT (para)
#define assert_static(cond)
#else
#define DbgEnterFunction(para)
#define DbgExitFunction(para)
#define DbgAssert(para)
#define assert_static(cond)
#endif

/*For simplify sending unified cmds */
#define CMD_BEGIN(param_len)  \
    CMD cmd; \
    int params[param_len]; \
    VPP_OBJ *vpp_obj; \
    int cpcbID;

#define CMD_BEGIN_NO_PARAM()  \
    CMD cmd; \
    VPP_OBJ *vpp_obj; \
    int cpcbID;

#define CMD_CHECK(handle)   \
    if (g_vpp->magic != VPP_MAGIC) \
        return (MV_VPP_EUNCONFIG); \
    \
    if (handle<0 || handle>=MAX_NUM_OBJECTS) \
        return (MV_VPP_EBADPARAM); \
    \
    vpp_obj = g_vpp->vpp_objs[handle]; \
    \
    if (!vpp_obj) \
        return (MV_VPP_ENODEV); \
    \
    if (vpp_obj->status == STATUS_INACTIVE) \
        return (MV_VPP_EUNCONFIG); \
    \
    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN)) == CPCB_INVALID) {\
        return (MV_VPP_EUNSUPPORT);\
    }

#define CMD_COMMIT(param_len, func) \
    make_cmd(&cmd, (FUNCPTR)func, param_len, params); \
    \
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock); \
    \
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd); \
    \
    /* commit commands */ \
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq); \
    \
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

#ifdef __cplusplus
extern "C" {
#endif

/* callback function type definition */
typedef int (*FUNCPTR)(void *, void *);
#ifdef __cplusplus
}
#endif

#endif
