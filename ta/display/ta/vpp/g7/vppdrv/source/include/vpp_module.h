/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_MODULE_H_
#define _VPP_MODULE_H_

#include "tee_ta_mmap_priv.h"
#include "tee_ta_mmap_types.h"

#include "api_avio_dhub.h"
#include "OSAL_api.h"
#ifdef CONFIG_AMP_TEE_ENABLE
#include "tee_ta_osal.h"
#include "tee_ta_mmap_helper.h"
#endif

#include "avioGbl.h"
#include "com_type.h"
#include "vpp_com.h"
#include "vpp_debug.h"
#include "ta_log.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "cmdqueue.h"
#include "framequeue.h"
#include "vbibuf.h"
#include "galois_io.h"
#include "vpp_osal.h"

#include "vpp_scl_data.h"

#ifndef CONFIG_AMP_TEE_ENABLE
#include "isl/amp_shm.h"
#endif

#include "vdec_com.h"

#include "vpp.h"
#include "vpp_mp.h"
#include "vpp_gfx_vop.h"
#include "hdmiRxPipe.h"

#include "vpp_debug_data.h"

#define SHM_ALIGN_SIZE_4_KB 4096
#define VPP_SHM_ALIGN_ROUNDUP(size)  ((size + SHM_ALIGN_SIZE_4_KB - 1) & (~(SHM_ALIGN_SIZE_4_KB - 1)))

// VPP module magic number
#define VPP_MAGIC    0x1177FACE

#ifdef __cplusplus
extern "C" {
#endif

//Enable the below config macro to power save during built-in frame by hvaing 1:1 scaling
#define CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1

//ROTATION is not supported for compressed frame: Ignore user defined rotation for compressed frame
#define VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME

#if defined(_VPP_API_C_)
const ENUM_PLANE_ID chan_to_plane_map_table[MAX_NUM_CHANS] = {PLANE_MAIN, PLANE_PIP, PLANE_GFX1, PLANE_GFX2, PLANE_AUX, PLANE_OVP_EL, PLANE_MAIN_EL, PLANE_VMX};
#else
extern const ENUM_PLANE_ID chan_to_plane_map_table[MAX_NUM_CHANS];
#endif

#ifdef TZ_3_0
#define VPP_TA_ID "MV_VPP_PERSISTENT_ID"
typedef struct __VPP_StoreTable__
{
    int g_addr_val;
}VPP_StoreTable;
#define VPP_CA_INIT_MAGIC_NUM 0xFACE
#endif

/* helper MACROs */
#define PLANE_OF_CHAN(vpp_obj, chanID)     chan_to_plane_map_table[chanID]
#define CHAN_OF_PLANE(vpp_obj, planeID)    (vpp_obj->plane[planeID].chanID)
#define CPCB_OF_CHAN(vpp_obj, chanID)      (vpp_obj->chan[chanID].dvID)
#define CPCB_OF_VOUT(vpp_obj, voutID)      (vpp_obj->vout[voutID].dvID)

#define GFXPLANE_OF_OSDCHAN(plane)     ((plane == PLANE_GFX1))

#define IS_RES_NEED_REDUCED_BLANKING(resID) ((resID == RES_DSI_540x960P60) || \
                                            (resID == RES_DSI_1920x1200P60) || \
                                            (resID == RES_DSI_1920x1080P5994) || \
                                            (resID >= RES_CVT_RB_MIN && resID <= RES_CVT_RB_MAX))

#define IS_RES_NEED_FULL_BLANKING(resID)    ((resID == RES_DSI_800x1280P60) || \
    (resID == RES_DSI_WNC_800x1280P60) || (resID == RES_DSI_UBQ_800x1280P60))

#define VPP_GetResId(cpcbID)   vpp_obj->dv[cpcbID].dispOutParams.uiResId
#define VPP_IsResIdInRange(cpcbID, minResId, maxResId)   (VPP_GetResId(cpcbID) >= (minResId)\
                                                         && VPP_GetResId(cpcbID) <= (maxResId))

#define VPP_SetResId(cpcbID, resid)  vpp_obj->dv[cpcbID].dispOutParams.uiResId = resid

#ifdef RA_ClientIF_MP_MR0
#undef RA_ClientIF_CTRL0
#define RA_ClientIF_CTRL0 RA_ClientIF_MP_CTRL0
#undef RA_ClientIF_MR0
#define RA_ClientIF_MR0 RA_ClientIF_MP_MR0
#undef RA_ClientIF_MR1
#define RA_ClientIF_MR1 RA_ClientIF_MP_MR1
#undef RA_ClientIF_MR0_word
#define RA_ClientIF_MR0_word RA_ClientIF_MP_MR0_word
#undef RA_ClientIF_MR1_word
#define RA_ClientIF_MR1_word RA_ClientIF_MP_MR1_word
#undef RA_ClientIF_RdClientVmxVm
#define RA_ClientIF_RdClientVmxVm RA_ClientIF_MP_RdClientVmxVm

#define VPP_MP_CLIENTIF_STRUCT(reg) T32ClientIF_MP_##reg
#else
#define VPP_MP_CLIENTIF_STRUCT(reg) T32ClientIF_##reg
#endif //RA_ClientIF_MP_MR0

#define ISSUE_CMD(q, cmd) \
do { \
    if (!cmdq_push((q), (cmd))){ \
        /* command queue is full, command push fail. */ \
        cmdq_uncommit((q)); \
        VPPAPI_DBG_INFO("VPP:  command queue is full, and command push is failed."); \
        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock); \
        return (MV_VPP_ECMDQFULL); \
    } \
} while (0)

#ifndef VPP_SHM_API
#define VPP_SHM_API
#endif

#ifdef DISABLE_BCM_ENGINE
#define START_1DDMA(dhubID, dmaID, start_addr, size, cfgQ, ptr) \
    dhub_channel_big_write_cmd(&(((HDL_dhub2d *)dhubID)->dhub), dmaID, (INT)start_addr, size, 0, 0, 0, 1, NULL, NULL)
#define CLEAR_1DDMA(dhubID, dmaID, bcmbuf) \
    do{ \
    }while(0)
#else
#define START_1DDMA(dhubID, dmaID, start_addr, size, cfgQ, ptr) \
    dhub_channel_big_write_cmd(&(((HDL_dhub2d *)dhubID)->dhub), dmaID, (INT)start_addr, size, 0, 0, 0, 1, cfgQ, ptr)
#define CLEAR_1DDMA(dhubID, dmaID, bcmbuf) \
    do{ \
    dhub_channel_clear_seq(dhubID, dmaID, bcmbuf); \
    }while(0)
#endif

#ifdef DISABLE_BCM_ENGINE
#define START_2DDMA(dhubID, dmaID, start_addr, stride, width, height, cfgQ) \
    dhub2d_channel_cfg((HDL_dhub2d *)dhubID, dmaID, start_addr, stride, width, height, 1, 0, 0, 0, 0, 1, NULL)
#define CLEAR_2DDMA(dhubID, dmaID, bcmbuf) \
    do{ \
        dhub2d_channel_clear_seq(dhubID, dmaID); \
        dhub2d_channel_start_seq(dhubID, dmaID); \
    }while(0)
#define CLEAR_2DDMA_DIRECT(dhubID, dmaID, bcmbuf) CLEAR_2DDMA(dhubID, dmaID, bcmbuf)
#else
#define START_2DDMA(dhubID, dmaID, start_addr, stride, width, height, cfgQ) \
    dhub2d_channel_cfg((HDL_dhub2d *)dhubID, dmaID, start_addr, stride, width, height, 1, 0, 0, 0, 0, 1, cfgQ)
#define CLEAR_2DDMA(dhubID, dmaID, bcmbuf) \
    do{ \
        dhub2d_channel_clear_seq(dhubID, dmaID); \
        dhub2d_channel_start_seq(dhubID, dmaID); \
    dhub2d_channel_clear_seq_bcm(dhubID, dmaID, bcmbuf); \
    }while(0)

#define CLEAR_2DDMA_DIRECT(dhubID, dmaID, bcmbuf) \
    do{ \
        dhub2d_channel_clear_seq(dhubID, dmaID); \
        dhub2d_channel_start_seq(dhubID, dmaID); \
        dhub2d_channel_clear_seq(dhubID, dmaID); \
    }while(0)
#endif

#ifdef DISABLE_BCM_ENGINE
#define START_2NDDMA(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, cfgQ) \
    dhub2nd_channel_cfg((HDL_dhub2d *)dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, 0, 0, 0, 1, NULL)
#define START_2NDDMA_MTRR(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, chkSemId, updSemId, cfgQ) \
    dhub2nd_channel_cfg((HDL_dhub2d *)dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, chkSemId, updSemId, 0, 1, cfgQ)
#define CLEAR_2NDDMA(dhubID, dmaID, bcmbuf) \
    do{ \
        dhub2nd_channel_clear_seq(dhubID, dmaID); \
        dhub2nd_channel_start_seq(dhubID, dmaID); \
    }while(0)
#define CLEAR_2NDDMA_DIRECT(dhubID, dmaID, bcmbuf) CLEAR_2NDDMA(dhubID, dmaID, bcmbuf)
#else
#define START_2NDDMA(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, cfgQ) \
    dhub2nd_channel_cfg((HDL_dhub2d *)dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, 0, 0, 0, 1, cfgQ)
#define START_2NDDMA_MTRR(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, chkSemId, updSemId, cfgQ) \
    dhub2nd_channel_cfg((HDL_dhub2d *)dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, chkSemId, updSemId, 0, 1, cfgQ)
#define CLEAR_2NDDMA(dhubID, dmaID, bcmbuf) \
    do{ \
        dhub2nd_channel_clear_seq(dhubID, dmaID); \
        dhub2nd_channel_start_seq(dhubID, dmaID); \
        dhub2nd_channel_clear_seq_bcm(dhubID, dmaID, bcmbuf); \
    }while(0)

#define CLEAR_2NDDMA_DIRECT(dhubID, dmaID, bcmbuf) \
    do{ \
        dhub2nd_channel_clear_seq(dhubID, dmaID); \
        dhub2nd_channel_start_seq(dhubID, dmaID); \
        dhub2nd_channel_clear_seq(dhubID, dmaID); \
    }while(0)
#endif

#define VPP_REG_WRITE32(reg, val) VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr+(reg), (val))
#define VPP_REG_WRITE8(reg, val) VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr+((reg)<<2), (val))

#define GLB_REG_WRITE32(reg, val)  MV_MEMIO32_WRITE(reg, val)
#define GLB_REG_READ32(reg)  MV_MEMIO32_READ(reg)
#define VPP_REG_READ32_RAW(reg) MV_MEMIO32_READ(vpp_obj->base_addr+(reg))
#define VPP_REG_READ8_RAW(reg)  MV_MEMIO32_READ(vpp_obj->base_addr+((reg)<<2))
#define VPP_REG_WRITE32_RAW(reg, val) MV_MEMIO32_WRITE((vpp_obj->base_addr+(reg)), val)
#define VPP_REG_WRITE8_RAW(reg, val)  MV_MEMIO32_WRITE((vpp_obj->base_addr+((reg)<<2)), val)

#ifndef VPP_IN_TRUST_ZONE
#define VPP_CB_FUNC(cb, obj, event, info, ctx) cb(event, info, ctx);
#else
#define VPP_CB_FUNC(cb, obj, event, info, ctx) cb(obj, event, info, ctx);
#endif

#define VPP_HDR_MODE_DEFAULT  VPP_HDR_MODE_AUTO

/* default window attributes */
#define DEFAULT_BGCOLOR     0 // green
#define DEFAULT_ALPHA       0xff // opaque
#define DEFAULT_GLOBAL_ALPHA_FLAG   1   //use plane/global alpha

#define CONFIG_TA_LOG_ENABLE
#define CONFIG_ANDROID_LOG
#define CONFIG_LOG_ERROR
#define CONFIG_LOG_INFO

// Helper Macros for MTR and MMU feature.
#define VPP_BITMASK_MTR  0x01
#define VPP_BITMASK_MMU  0x02
// This macro checks if MTR bit of x is set. x can be 'pinfo->m_is_compressed' or 'plane->m_mtrR__en'.
#define IS_MTR_ENABLED(x) ((x) & VPP_BITMASK_MTR)
// This macro checks if MMU bit of x is set. x can be 'pinfo->m_is_compressed' or 'plane->m_mtrR__en'.
#define IS_MMU_ENABLED(x) ((x) & VPP_BITMASK_MMU)
// This macro checks if MMU bit of x is only set. x can be 'pinfo->m_is_compressed' or 'plane->m_mtrR__en'.
#define IS_MMU_ONLY_ENABLED(x) (!IS_MTR_ENABLED(x) &&  IS_MMU_ENABLED(x))

#define BCM_BUF_COUNT  2

// This macro gets the Luma offset using tile height
#define VPP_CROP_GetActiveLumaOffset(pinfo, plane)                              \
    ((UNSG32)((uintptr_t)pinfo->m_pbuf_start)) +                                \
    pinfo->m_disp_offset + pinfo->m_content_offset + plane->crop_actv_offset +  \
    (pinfo->m_buf_stride * (int)(pinfo->m_luma_top_ofst / plane->tile.height))

// This macro gets the Chroma offset using tile height
#define VPP_CROP_GetActiveChromaOffset(pinfo, plane)                              \
    (int)pinfo->m_buf_pbuf_start_UV +                                             \
    pinfo->m_disp_offset + pinfo->m_content_offset + plane->crop_actv_offset_UV + \
    (pinfo->m_buf_stride * (int)(pinfo->m_chroma_top_ofst / plane->tile.height))

#define OVPS_PARAMS vpp_obj->povps_obj_params

#define MAX_NUM_FEATURE_CFG 1

/* definition of suspend type */
typedef enum __POWER_MODE__{
    POWER_MODE_NORMAL = 0,      // Not suspended
    POWER_MODE_SUSPEND = 1,
    POWER_MODE_SUSPEND_OPTIMIZED = 2,
}POWER_MODE;

/* definition of A/V PLL types */
enum {
    CTRLA_BK = 0,
    CTRLB_BK,
    CTRLC_BK,
    CTRLD_BK,
    CTRLE_BK,
    CTRLF_BK,
    CTRLG_BK,
    CTRLBK_MAX,
};
enum {
    APLL0_IDX = 0,
    APLL1_IDX,
    VPLL0_IDX,
    VPLL1_IDX,
    PLLBK_MAX,
};

/* definition of video resolution type */
enum {
    TYPE_SD = 0,
    TYPE_HD = 1,
    TYPE_UHD = 2,
};

/*Client State*/
typedef enum OVPS_CLIENT_STATE_t{
   OVPS_CLIENT_INACTIVE = 0,
   OVPS_CLIENT_ACTIVE = 1,
}OVPS_CLIENT_STATE;

typedef enum ITMO_FILTER_t
{
    ITMO_FILTER_MIN = 0,
    ITMO_FILTER_DEFAULT = ITMO_FILTER_MIN,
    ITMO_FILTER_DEFAULT_SHARP,
    ITMO_FILTER_STARTUP,
    ITMO_FILTER_STARTUP_SHARP,
    ITMO_FILTER_BRIGHT,
    ITMO_FILTER_BRIGHT_SHARP,
    ITMO_FILTER_CONTRAST,
    ITMO_FILTER_CONTRAST_SHARP,
    ITMO_FILTER_DARK,
    ITMO_FILTER_DARK_SHARP,
    ITMO_FILTER_MAX
}ITMO_FILTER;

typedef enum{
    OUTPUT_MODE_DOVI     = 0,
    OUTPUT_MODE_HDR      = 1,
    OUTPUT_MODE_SDR      = 2,
    OUTPUT_MODE_SDR2020  = 3,
}OUTPUT_MODE_T;

typedef struct ITMO_FILTER_LUT_t
{
    INT YfLutOffset_FRFR;
    INT YLutOffset_FRFR;
    INT YfLut_FRFR[65];
    INT YLut_FRFR[161];
    INT UVLut_FRFR[9];
    INT YfLutOffset_RRFR;
    INT YLutOffset_RRFR;
    INT YfLut_RRFR[65];
    INT YLut_RRFR[161];
    INT UVLut_RRFR[9];
    INT YfLutOffset_ORFR;
    INT YLutOffset_ORFR;
    INT YfLut_ORFR[65];
    INT YLut_ORFR[161];
    INT UVLut_ORFR[9];
    INT YFootRoom_OR;
    INT YHeadRoom_OR;
    INT UVFootRoom_OR;
    INT UVHeadRoom_OR;
    INT YfLutOffset_FRRR;
    INT YLutOffset_FRRR;
    INT YfLut_FRRR[65];
    INT YLut_FRRR[161];
    INT UVLut_FRRR[9];
    INT YfLutOffset_RRRR;
    INT YLutOffset_RRRR;
    INT YfLut_RRRR[65];
    INT YLut_RRRR[161];
    INT UVLut_RRRR[9];
    INT YfLutOffset_ORRR;
    INT YLutOffset_ORRR;
    INT YfLut_ORRR[65];
    INT YLut_ORRR[161];
    INT UVLut_ORRR[9];
}ITMO_FILTUR_LUT;


/* definition of video scan mode */
enum {
    SCAN_PROGRESSIVE = 0,
    SCAN_INTERLACED  = 1,
};

/* definition of video frame-rate */
enum {
        FRAME_RATE_23P98  = 0,
        FRAME_RATE_24     = 1,
        FRAME_RATE_25     = 2,
        FRAME_RATE_29P97  = 3,
        FRAME_RATE_30     = 4,
        FRAME_RATE_47P96  = 5,
        FRAME_RATE_48     = 6,
        FRAME_RATE_50     = 7,
        FRAME_RATE_59P94  = 8,
        FRAME_RATE_60     = 9,
        FRAME_RATE_100    = 10,
        FRAME_RATE_119P88 = 11,
        FRAME_RATE_120    = 12,
        FRAME_RATE_89P91  = 13,
        FRAME_RATE_90     = 14
};

/* definition of HV scaling coeff selection */
enum {
    MODE_COEFF_SEL_AUTO      = 0,
    MODE_COEFF_SEL_FORCE_H   = 1,
    MODE_COEFF_SEL_FORCE_V   = 2,
    MODE_COEFF_SEL_FORCE_HV  = 3,
};

    /* definition of stages of a pipeline */
    enum {
        STAGE_FE    = 0,
        STAGE_SCL   = 1,
        STAGE_CPCB  = 2,
        STAGE_BE    = 3,
        MAX_NUM_STAGES = 4
    };
enum {
	VPP_VOP2_OUTSEL_DP2 = 0,
	VPP_VOP2_OUTSEL_DP1 = 1
};

/*Select the line to be picked up by VPP when line dropping is enable for more than 2x down scaling */
typedef enum __LINEDROPPING_STYLE__ {
    LINEDROPPING_SEL_LINE_FIRST,
    LINEDROPPING_SEL_LINE_MID,
    LINEDROPPING_SEL_LINE_LAST,
}LINEDROPPING_STYLE;

/* offline scaling threshold ratio */
#define OFFLINE_SCL_THRESH_VRATIO 1

    /* offline frame buffer size */
#define OFFLINE_BUFFER_SIZE   ((1920 * 1080 * 2 * 12)/8)  // 2K 422 12 Bit

#define OFFLINE_NO_OF_BUFFERS   (2)
#define OFFLINE_BUFFER_INDEX_MASK   (0x1)
#define OFFLINE_BUFFER_INVALID_INDEX   (-1)

//#define MUTE_OPTIMISED

#define METADATA_NO_OF_BUFFERS   (3)

#ifdef ENABLE_OVP
#define MAX_CHANNEL_ID_PER_PALNE   4
#else
#define MAX_CHANNEL_ID_PER_PALNE   2
#endif

#define OVP_LUMA_BUFFER_SIZE   (3840 * 2160 * 10/8)
#define OVP_CHROMA_BUFFER_SIZE   (3840 * 1080 * 10/8)

typedef struct EVENT_CALLBACK_T{
    volatile INT status;         // registered or not
    FUNCPTR cb_func;    // callback function
    INT params[MAX_NUM_PARAMS];     // function parameters
} EVENT_CALLBACK;

typedef struct CAPTURE_VID_FRAME_T{
    volatile BOOL               gCapStartFlag; //the capture start falg set by user
    VPP_CAPTURE_FRAME_SETTING   gCapFrameSetting;
    VPP_CAPTURE_FRAME_INFO      gCapFrameInfo;   //the information that need to notify user
    VPP_CAPTURE_CALLBACK        appCbFn;    //the call back functions
}CAPTURE_VID_FRAME;

typedef struct OFFLINE_BUFF_DESCR_T
{
    AMP_SHM_HANDLE handle;
    void *addr; // offline buffer address
    UINT32 stride;
    VPP_WIN disp_win; // display window in end display for content in this offline buffer
    VPP_WIN content_win; // content window in end display for content in this offline buffer
    VPP_WIN crop_win_final;
    VPP_WIN crop_disp_win_final;
    UINT32 uiBitsPerPixel;
}OFFLINE_BUFF_DESCR;

typedef struct OVP_BUFF_DESCR_T
{
    AMP_SHM_HANDLE handle;
    void *addr; // offline buffer address
    UINT32 stride;
    VPP_WIN ovp_win; // display window in end display for content in this offline buffer
}OVP_BUFF_DESCR;

typedef struct METADATA_BUF_DESCR_T
{
    AMP_SHM_HANDLE handle;
    void *virt_addr;
    void *phy_addr;
    UINT32 raw_md_size;
    UINT32 pkd_md_size;
}METADATA_BUF_DESCR;

#define     stDhubCropOffset                           {\
    UINT32 u32_Tile_Xoffset                                   : 11;\
    UINT32 u32_Tile_Yoffset                                   : 14;\
    UINT32 RSVDxC_b25                                  :  7;\
}

/* This structure is requied for DHUB cropping of compressed inputs */
/* Fill the X-offset & y-offset (in multiple of tile units)with required cropping */
/* and add to the base address recived from VBUF info */
typedef union U32_DhubCropOffset
{
    UINT32 u32;
    struct stDhubCropOffset;
}U32_DhubCropOffset;

/*DV EL crop values*/
typedef struct VPP_DV_EL_INPUT_CROP_T
{
    UINT32 leftCrop;
    UINT32 rightCrop;
    UINT32 topCrop;
    UINT32 bottomCrop;
}VPP_DV_EL_INPUT_CROP;

/* Dolby crop values */
typedef struct VPP_DV_COMP_OUT_CROP_T
{
    UINT32 leftcrop;
    UINT32 rightcrop;
    UINT32 topcrop;
    UINT32 bottomcrop;
}VPP_DV_COMP_OUT_CROP;

typedef struct PLANE_T {
    volatile INT status;         // active or not
#ifdef MUTE_OPTIMISED
    volatile INT status_bk; //to save the plane status during mute/unmute
#endif
    INT chanID;         // ID of the channel, which this plane belongs to
    INT srcfmt;         // video component data format in input frame buffer
    INT order;          // video component data order in input frame buffer
    INT bits_per_pixel; // Bit depth - 8/10/12
    INT bpp;            // Total bits per pixel

    INT wpl;            // beat per line of current frame

    INT dmaRID[MAX_CHANNEL_ID_PER_PALNE];         // plane inline read DMA channel ID
    void *dmaRdhubID;     // ID of Dhub on which inline read DMA channel is connected
    /* offline write-back/read-back DMAs are only available for Berlin Main/PIP/AUX plane */
    INT offline_dmaWID;  // plane offline write-back DMA channel ID
    INT offline_dmaWdhubID;     // ID of Dhub on which offline write-back DMA channel is connected

    /* de-interlacer write/read DMAs are only available for Berlin Main plane */
    INT deint_dmaWID;    // plane de-interlacer write-back DMA channel ID
    void *deint_dmaWdhubID;     // ID of Dhub on which de-interlacer write-back DMA channel is connected
    INT deint_dmaR0ID;    // plane de-interlacer read-back 0 DMA channel ID
    void *deint_dmaR0dhubID;     // ID of Dhub on which de-interlacer read-back DMA channel is connected
    INT deint_dmaR1ID;    // plane de-interlacer read-back 1 DMA channel ID
    void *deint_dmaR1dhubID;     // ID of Dhub on which de-interlacer read-back DMA channel is connected

    INT frm_count;      // frame interrupt count since last valid PTS
    PTS pts;            // last valid PTS of this plane
    PTS delta_pts;      // delta PTS from last valid PTS in unit of 4 frames' PTS

    VPP_WIN actv_win;   // plane display content location and size in reference window
    INT disp_offset;
    INT crop_disp_offset;
    INT crop_disp_offset_UV;
    INT aligned_active_height;
    // this information is a copy from frame descriptor for register updating purpose.

    VPP_WIN stWinFrameDesc_Bl;
    VPP_WIN crop_actv_win;   // plane current cropped display content location and size in reference window
    INT crop_actv_offset;    // plane current cropped display content offset from frame buffer base address
    INT crop_actv_offset_UV;    // plane current cropped display content offset from frame buffer base address of UV
    VPP_WIN crop_scl_actv_win;   // plane current scaled cropped display content location and size in display window
    VPP_WIN main_crop_scl_actv_win;   // plane current scaled cropped display content location and size in display window
    INT scaler_crop_offset;    // plane current cropped offset in its scaler

    VPP_WIN ref_win;    // reference window
    VPP_WIN frame_win;  // content window
    INT ref_win_updated; // 1: indicate ref_win is updated by user function call
    INT disp_win_updated; // 1: indicate disp_win is updated by user function call
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
    VPP_WIN ref_win_user;   //reference window configured by user during boot
    int ref_win_user_init;  //Flag to indicate if refwin is initialized at bootup time
#endif

    VPP_WIN crop_win;   // crop window
    INT crop_win_valid; // 1: indicate crop_win is valid, 0: not valid
    INT source_win_valid; // 1: indicate source_win is valid, 0: not valid

    FRAMEQUEUE inputq;         // input frame buffer queue of this plane
    FRAMEQUEUE outputq;        // recycle frame buffer queue of this plane

    INT frame_type;            // current frame type, 0: for interlaced, 1: for progressive
    INT field;                 // current field for interlaced input 0: first field, 1: second field
    void *vp_buffs[3];      // video processor field buffer
    void *vp_buffs_right[3];      // video processor field buffer for 3D right view
    VPP_SHM_HANDLE vp_buff_handle;     //the field buffer handle
    INT vp_buff_base;       // base address of field buffers
    INT vp_buff_w_index;    // index of next field buffer
    INT vp_buff_r0_index;   // index of current field buffer
    INT vp_buff_r1_index;   // index of previous field buffer
    INT vp_buff_num;        // 2 or 3

    INT deint_mode;            // deinterlacer mode
    INT uiPseudoDeintMode;
    VPP_DINT_PRESET_MODE_PARAMS   DeintPreset;
    INT vp_updated;            // vp(deinterlacer & vnr) setting updated

    /* for 3D de-interlacer first garbage frame skip */
    INT skip_frame;

    INT mute; // 1: mute, 0: un-mute
    /* Added for suspend and resume */
    INT borderAlpha;
    INT src_pol;
    INT bg_pol;
#ifdef TBD_REMOVED
    INT gfxIdxStart;
    INT gfxIdxCount;
    INT gfxClut[256];
#endif
    VDEC_VID_BUF_DESC *curr_frame;      // current frame in video display mode
    VDEC_VID_BUF_DESC *prev_curr_frame;      // prev current frame in video display mode for back buffer
    VDEC_VID_BUF_DESC *curr_still_picture;   // current logo frame in still picture display mode
    VDEC_VID_BUF_DESC *still_picture;   // saved logo frame in still picture display mode
    VDEC_VID_BUF_DESC *prev_still_picture;   // previous logo frame in still picture display mode
    VDEC_VID_BUF_DESC *dummy_frame;     // dummy frame for GFX-0/GFX-1/GFX-2 planes.
    INT *vpp_last_frame_addrY;
    INT *vpp_last_frame_addrUV;
#if !defined(VPP_IN_TRUST_ZONE)
    VPP_SHM_HANDLE vbi_cfgq_shm_handle;
#endif
    OFFLINE_BUFF_DESCR offline_buff_descrs[OFFLINE_NO_OF_BUFFERS];    // offline frame buffer descriptors
    INT wb_buff_index;         // index of current write-back offline frame buffer descriptor
    INT rb_buff_index;         // index of current read-back offline frame buffer descriptor

#if defined(PE_3D_BD)
    UINT32 ss_type;
    UINT32 ss_offset_mode;
    UINT32 ss_offset_seq_id;
    INT8 ss_offset_default;
    INT8 ss_offset_current;
#endif

    UINT32 ui420UpsCfg0_Y;
    UINT32 ui420UpsCfg0_C;
    UINT32 ui420UpsSPCfg0;
    UINT32 ui422Ups10Cfg5;
    UINT8 uchClientMode; //Unique Id for bitdepth and src_format

    VPP_TILE tile;

    //Scalar dropping related variables
    UINT32 lineDroppingStrides; //Stride factor - Number of lines dropped will be factor/2 (1-0,2-1)
    LINEDROPPING_STYLE lineDroppingStyle;   //The line to be selected from the line drop group

    //disp_offset & content_offset are in bytes
    UINT32 leftCropAdjustment;      //left crop Adjustment required because of disp_offset & content_offset
    UINT32 cropDispContentOffset;   //The adjustment to be done for final base address because of disp_offset & content_offset


    /* Internal variable for crop/disp window to support Interlaced output */
    VPP_WIN crop_win_final;   // crop window
    UINT8  uchFieldBuffInUse;
    UINT8  uchFrameBuffInUse;

#ifdef VPP_ENABLE_FRAME_CAPTURE
    CAPTURE_VID_FRAME CaptureVidFrm;
#endif
    UINT8  uchInverseScanMode;
    UINT8  uchInverseScanModeUpdated;
    UINT8  uchRotationAngle;
#ifdef VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME
    UINT8  uchUserRotationAngle;
#endif //VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME
    UINT8  uchRotationAngleUpdate;
    UINT32 uiFeDlrInHres;
    UINT32 uiFeDlrInVres;
    UINT32 color_primaries;

    UINT32 m_uiTileCfg6_Y;
    UINT32 m_uiTileCfg6_C;

	UINT32 m_uiTileCfg10_Y;
	UINT32 m_uiTileCfg10_C;

    UINT32 m_colorprimaries;
    UINT8 m_color_range;

    UINT8 m_mtrR__en;                  //MTR Enable flag for BL
	UINT8 m_mtrR__en_el;               //MTR Enable flag for EL
    UINT8 m_mtrR_updSemId_Y;           //MTR SemId & chkSemId's for EL & BL
    UINT8 m_mtrR_updSemId_UV;
    UINT8 m_mtrR_chkSemId_Y;
    UINT8 m_mtrR_chkSemId_UV;
    UINT8 m_mtrR_updSemId_Y_El;
    UINT8 m_mtrR_updSemId_UV_El;
    UINT8 m_mtrR_chkSemId_Y_El;
    UINT8 m_mtrR_chkSemId_UV_El;

    UINT8 scl_mode; /*Inline/offline/etc */
    UINT8 sclModeChanged;
    T32OfflineRd_CTRL3 stOfflineRdCtrl3;
    UINT8 cpcb_source_window_update_required_offline;
    UINT8 cpcb_bg_window_update_required_offline;
    UINT8 sclLineDropEn;
    UINT8 offline_scl_bpp;
    //Force inline scaling for N frame before switching to offline mode
    UINT8 force_scl_mode_state;
    UINT8 force_scl_mode_count;
    VPP_SCL_RES force_scl_res;
    UINT8 offline_scl_srcfmt;

    UINT32 m_leftpad_y;                //Luma left padding in compressed input
    UINT32 m_toppad_y;                 //Luma Top padding
    UINT32 m_rightpad_y;               //Luma Right padding
    UINT32 m_bottompad_y;
    UINT32 m_leftpad_c;
    UINT32 m_toppad_c;
    UINT32 m_rightpad_c;
    UINT32 m_bottompad_c;
    UINT32 m_SampleCropEnable;
	UINT32 m_SampleCropEnable_el;
    UINT32 m_toppadadjust_y;
    UINT32 m_toppadadjust_c;
    UINT32 m_leftpadadjust_y;
    UINT32 m_leftpadadjust_c;
    UINT32 m_BL_scl_crop;
    U32_DhubCropOffset el_uDhubCropOffset_y;
    U32_DhubCropOffset el_uDhubCropOffset_c;
    U32_DhubCropOffset uDhubCropOffset_y;
    U32_DhubCropOffset uDhubCropOffset_c;

    UINT32 m_Aligned_Luma_Width;
    UINT32 m_Aligned_Luma_Height;
    UINT32 m_Aligned_Chroma_Width;
    UINT32 m_Aligned_Chroma_Height;

    UINT32 m_El_Aligned_Luma_Width;
    UINT32 m_El_Aligned_Luma_Height;
    UINT32 m_El_Aligned_Chroma_Width;
    UINT32 m_El_Aligned_Chroma_Height;
    UINT32 m_SeparateElcropRequired;
    UINT32 m_El_crop_recalculation_required;
    UINT32 m_Bl_Crop_Out_Width;
    UINT32 m_Bl_Crop_Out_Height;
    UINT32 tileStrideFrac_EL;
    UINT32 tileStrideFrac_BL;
    UINT32 m_PicHeightAdjusted;
    UINT32 m_ChromaHeightAdjusted;
    UINT32 m_DolbyCompression_en;
    UINT32 m_dolby_ref_win_updated;
    UINT32 dolbyLineDropEn;
    UINT32 m_ref_win_update_pan_scan;
    UINT32 m_DVCompUpdateAfterLineDrop;
    VPP_DV_EL_INPUT_CROP stElPipeCropRes;
    VPP_DV_COMP_OUT_CROP stDolbyCrop;
    VPP_WIN dolby_aligned_ref_win;
    VPP_WIN dolby_user_ref_win;
    VPP_WIN dolby_mod_ref_win;
    UINT32 dolby_scl_crop_left_adjust;
    UINT32 dolby_scl_crop_right_adjust;

    void *pDVplaneConfig;
    UINT32 uiDolbyPresentMode; //Dolby present mode
    INT edr_dmaRID;
    INT edr_dmaRdhubID;
    INT mdrd_dmaRID;
    INT mdrd_dmaRdhubID;

    T32DIF_CFG0   y_dif_cfg0;
    T32DIF_CFG0   c_dif_cfg0;

    VPP_WIN ref_win_user_back;

#ifdef CONFIG_AMP_DCM
    T32VDCM_CFG0 stVdcmCfg0;
    UINT32 m_uiDcmEnable; //VDCM/GDCM Enable/Disable status
    UINT32 m_uiDcmEnableUpdated;
    INT32 m_iDcmInMode;
    INT32 m_iDcmOutMode;
    INT32 m_iDcmOetf;
    VPP_WIN m_stDcmAoiWin;  //Area Of Intreset window sizes
    UINT32 m_uiDcmAoiWinUpdated;
    UINT32 m_uiDcmAoiEnable;
    UINT32 m_uiDcmAoiReg;
    INT iDcmInHdrMode;     //Applicable only for HDR10, HLG, SDR of DCM block
    INT iDcmOutHdrMode;    //Applicable only for HDR10, HLG, SDR of DCM block
#endif
    void *pNgptvParams;

    UINT8  uiBufSrc;

} PLANE;

typedef struct VPP_DHUB_CFG_2NDDMA_PARAM_s
{
    /* Input params */
    INT dhubId;
    INT dmaId;
    UNSG32 addr;
    SIGN32 width; /* Number of bytes in a line or tile line */
    SIGN32 stride; /* Number of bytes in a line or tile line */
    SIGN32 height; /* Number of lines or tile lines */
    INT *cfgQ;
    UINT8 mtrREn;
    UINT8 updSemId;
    UINT8 chkSemId;
    PLANE *plane;
    /* Output params */
}VPP_DHUB_CFG_2NDDMA_PARAM;

typedef struct CHAN_T {
    volatile INT status;          // active or not
    INT dvID;            // ID of the DV, which this channel belongs to
    INT dvlayerID;       // ID of DV layer, which this channel connects to
    INT zorder;          // Z-order of DV plane, which this channel connects to
    INT num_of_planes;   // number of the planes, which belong to this channel
    INT planeID[MAX_NUM_PLANES]; // ID of the planes, which belong to this channel

    VPP_WIN crop_disp_win;           // channel cropped display window in end display
    VPP_WIN disp_win;                // channel display window in end display
    VPP_WIN_ATTR disp_win_attr;      // channel display window attribute: bgcolor and alpha
    INT disp_win_updated;            // 1: display window is changed, 0: display window is not changed

    INT scl_nl_enable;   // 1: enable non linear horizontal scaling, 0: disable non linear horizontal scaling
    INT scl_nl_center;   // [0, 255]
    INT scl_nl_cratio;   // [32, 64]

    /* Added for suspend and resume */
    INT16 	picCtrlVal[PICTURE_CTRL_MAX]; //TOTAL_VPP_CMU_SET = VPP_CMU_SET_MAX
    INT 	aceLevel;
    INT 	brLevel;
    INT16 	colorLevel[TOTAL_VPP_CMU_COLOR];
    INT16	colorHue[TOTAL_VPP_CMU_COLOR];
    INT		icrMode;
    INT		ftdcMode;
    INT		enFTTaper;
    INT		gmMode;

    /* Internal variable for crop/disp window to support Interlaced output */
    VPP_WIN crop_disp_win_final;
    VPP_WIN disp_win_final;
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
    INT disp_win_user_updated;  //Is display window updated by the user
    VPP_WIN disp_win_user;      //User configured display window
    VPP_WIN ref_win_user;       //User or previous non built-in frame reference window
    INT built_in_frame_pushed;  //Is previous frame pushed is built-in or non built-in frame
    INT built_in_frame_scl_required;  //Is force scaling enabled by the application
#endif
    VPP_WIN disp_win_user_back; //Back up display_window : used when dolby L5 metadata is present
} CHAN;

typedef struct DV_T {
    volatile INT status;      // active or not
    INT num_of_chans;   // number of the channels, which belong to this channel
    INT chanID[MAX_NUM_CHANS]; // ID of the channels, which belong to this DV
    INT num_of_vouts;   // number of the VOUTs, which belong to this channel
    INT voutID[MAX_NUM_VOUTS]; // ID of the VOUTs, which belong to this DV
    INT output_res_updated;    // 1: output_res is changed, 0: output_res is not changed
    volatile INT vbi_num;     // VBI interrupt counter
    CMDQUEUE cmdq;   // command queue for pipeline: CPCB
    INT LumaKey;        // 1: Luma key enabled    0: Luma Key disabled
    INT vco_freq;       // VCO frequency of CPCB AVPLL
    MV_OSAL_HANDLE_MUTEX_t cmdq_lock;

    INT viewID; // 0: left view  1:right view

    DHUB_CFGQ vbi_dma_cfgQ[BCM_BUF_COUNT];
    DHUB_CFGQ *curr_cpcb_vbi_dma_cfgQ;
    DHUB_CFGQ vbi_bcm_cfgQ[BCM_BUF_COUNT];
    DHUB_CFGQ *curr_cpcb_vbi_bcm_cfgQ;
    DHUB_CFGQ vde_bcm_cfgQ[BCM_BUF_COUNT];
    DHUB_CFGQ *curr_cpcb_vde_bcm_cfgQ;
    INT skip_vde_int;
    /* Added for suspend and resume */
    VPP_BG_COLOR bgColor;
    VPP_DISP_OUT_PARAMS     dispOutParams;
    /* Added for Gamma LUT Customization */
    VPP_GAMMA_LUT_INFO gammaLutinfo;
} DV;

typedef struct VOUT_T {
    volatile INT status;      // active or not
    INT dvID;        // ID of the DV, which this VOUT belongs to
    INT type;        // HDMI, component, S-video, CVBS
    VBIBUF cc_buf[2];  //CC data buffer, 0 for top field cc data; 1 for bottom field cc data
    VBIBUF wss_buf;  //WSS data buffer
    VBIBUF cgms_buf;  //CGMS data buffer
    VBIBUF tt_buf;  //TT data buffer
    INT tt_ctrl;  //TT control
    VPP_SHM_HANDLE tt_dma_handle;
    UINT8 *tt_dma_buf;  //TT data buffer for DMA
    UINT8 cc_top_field_enable;
    UINT8 cc_bottom_field_enable;
    MV_OSAL_HANDLE_MUTEX_t cc_buf_lock;
    MV_OSAL_HANDLE_MUTEX_t wss_buf_lock;
    MV_OSAL_HANDLE_MUTEX_t cgms_buf_lock;
    MV_OSAL_HANDLE_MUTEX_t tt_buf_lock;
    /* Added for suspend and resume */
    INT mute;
} VOUT;

/************************************************/
/************** FE data structures **************/
/************************************************/

/* structure of FE block */
typedef struct FE_T {
    /* VP register local copy */
    UINT32 VpDeintMode;
    UINT32 VpCtrlFifo;
    UINT32 VpFbCtrl;

    /* De-Interlacer register local copy */
    UINT32 DinStmCtrl1;
    UINT32 DinStmCtrl2;
    UINT32 DinViCtrl;
    UINT32 DinMcbCtrl1;
    UINT32 DinMcbCtrl2;
    UINT32 DinMcbCtrl3;
    UINT32 DinMeCtrl1;
    UINT32 DinMeCtrl2;
    UINT32 DinFilmCadenceCtrl;
    UINT32 DinFilmCadenceDetMode1;
    UINT32 DinFilmCadenceDetMode2;
    UINT32 DinProgFlagCtrl;
    UINT32 DinFilmCad32Ctrl1;
    UINT32 DinFilmCad32Ctrl2;
    UINT32 DinFilmCad22Ctrl;
    UINT32 DinFilmCadSplCadLen;

    /* De-Interlacer static variable */
    UINT32 EnableIPodSplModeVar;
    /*  Control register for composer   */
    UINT32 uiDvComposerCtrl[2];
    /*  DV comp wrap DP */
    UINT32 uiDvCompWrapDp[2];
    UINT32 uiDvCompYUV2RGB[2];
} FE;

/************************************************/
/************** SCL data structures **************/
/************************************************/

/* structure of SCL block */
typedef struct SCL_T {
    /* Added for suspend and resume */
    INT gfxInputMode0;
    INT gfxInputMode1;
} SCL;

/************************************************/
/************** CPCB data structures **************/
/************************************************/

/* structure of CPCB block */
typedef struct CPCB_T {
    /*CPCB interlacer local buffer*/
    UINT32 VppCpcb0IntCtrl;
    UINT32 Cpcb0SrcAlphaPol;
    UINT32 Cpcb0BgAlphaPol;
} CPCB;

/************************************************/
/************** BE data structures **************/
/************************************************/

typedef struct ENC_T {
   // DACs control
   UINT32 SdDacMode;
   UINT32 HdDacMode;

    /* BE Encoder DACs Ctrl0 */
    UINT32 DacCtrl0_a;
    UINT32 DacCtrl0_b;
    UINT32 DacCtrl0_c;
    UINT32 DacCtrl0_d;

   // SD/HD WSS/CGMS control
   UINT32 SdWssCgmsCtrl;
   UINT32 HdWssCgmsCtrl;

   // mute control
   UINT32 EncIOMuteCtrl;

   // HD Output control0
   UINT32 HdCtrl0;
    /* Added for suspend and resume */
    INT SdOverSampleFactor;
    INT HdOverSampleFactor;
    INT SdMacMode;
    INT HdMacMode;
} ENC;

typedef struct HDMI_TX_DRV_T
{
    UINT32      acrCtrl;
    UINT32      audCtrl;
    UINT32      audCtrl1;
    UINT32      i2sCtrl;
    UINT32      gcpCfg;
    UINT32      videoCtrl;
    UINT8       videoMute;
    UINT32      hostPktCtrl;
    UINT8       hostPktData[6][32];
    UINT32      hostPktCfg[6];
} HDMI_TX_DRV;
typedef struct HDMI_HDCP_DRV_T
{
    UINT8       ctrlReg;
    UINT8       stsReg;
    void        *srmData;
    UINT16      prevARi;
    UINT16      prevBRi;
} HDMI_HDCP_DRV;

/* structure of BE block */
typedef struct BE_T {
    UINT32 VppBeVopClkPol;
    UINT32 VppBeVopHDMIFrmtCtrl;

    // Addr of Hdmi Tx data store
    void *hdmiTx;

    // CEC enable control
    BOOL        enableCec;
    // CEC feature configuration
    UINT32      cecFeatureSupportCfg;
    // Addr of Hdmi Cec data store
    void *hdmiCec;
    // HDMI Tx driver data
    HDMI_TX_DRV     hdmiTxDrv;

    // HDCP driver data
    HDMI_HDCP_DRV   hdmiHdcpDrv;

    // HDMI Interrupt Status for DSR handling
    UINT8           hdmiIntSts;
    UINT8           hdmiFrmIntrTrig;

    // Task control flag
    UINT8           hdmiHdcpTaskExit;
    UINT8           hdmihpdTaskExit;
    UINT8           cecTaskExit;

    // CPCB0 Interrupt status for DSR handling
    UINT8            cpcb0IntSts;
    /* Added for suspend and resume */
    UINT32          cecPhyAddr;

    UINT32 m_uiDvOcscCtrl;

} BE;

/* DSI related config Info */
typedef struct VPP_DSI_INFO_T
 {
    ENUM_CPCB_TG_RES        CurrentResID;
    void                    *pmipi_dsi_dev;
    RESOLUTION_INFO         dsiResinfoTable;
    CPCB_TG_TIMING_PARAMS   dsiTgCalibrationParams;
    VPP_MIPI_LOAD_CONFIG    MipiCfg;
} VPP_DSI_INFO;

typedef struct HDMI_VOP_T {
    T32HDMI_VOP_CFG0 hdmi_vop_cfg0;
    T32HDMI_VOP_CFG1 hdmi_vop_cfg1;
    T32HDMI_VOP_CFG2 hdmi_vop_cfg2;
    T32HDMI_VOP_CFG3 hdmi_vop_cfg3;
    T32HDMI_VOP_CFG4 hdmi_vop_cfg4;
    T32HDMI_VOP_CFG7 hdmi_vop_cfg7;
} HDMI_VOP;

//#ifdef TZ_3_0
typedef struct _tag_vpp_callback_params {
    UINT64             CallbackFuncAddr;
    UINT64             UserDataAddr;
    PVOID              buffAddr;
} Vpp_Callback_Params;
//#endif
/*This structure is to help in moving the globally structure
  used in various places to one common global structure.
  The main intention of this movement is , in 3.0 TZ the global
  items might be accessed from various instances. */
typedef struct VPP_GLOBAL{
   Vpp_Callback_Params hdmi_cb_params;
   Vpp_Callback_Params cec_cb_params;
   Vpp_Callback_Params hdmicpcb0_cb_params;
   Vpp_Callback_Params hdcp_cb_params;
   VPP_HDMI_CEC_EVT_INFO gCecEvtInfo;
#ifdef EARC_RX_ENABLED
   Vpp_Callback_Params earc_cb_params;
   EARC_EVT_INFO gEarcEvtInfo;
#endif /* EARC_RX_ENABLED */
}VPP_GLOBAL;

//Security Attribute Table entries required by VPP TA
typedef struct __VPP_ENV_SA_CAPPING_WINDOW__ {
    UINT16 width;
    UINT16 height;
}VPP_ENV_SA_CAPWIN;

#define wVPP_ENV_SA_FLAGS  {                    \
    UINT8 resolutionCappingRotationAllowed:1;   \
    UINT8 resolutionCappingAspectAllowed:1;     \
    UINT8 deviceCappingRotationAllowed:1;       \
    UINT8 deviceCappingAspectAllowed:1;         \
    UINT8 reserved:4;                           \
    }

typedef union __VPP_ENV_SA_FLAGS__ {
    struct wVPP_ENV_SA_FLAGS;
    UINT8 flags;
}VPP_ENV_SA_FLAGS;

typedef struct __VPP_ENV_SA__ {
    //Variable to hold data read from SA table entry
    unsigned int hdcp_version;
    unsigned int hdcp_strm_type;
    VPP_ENV_SA_CAPWIN videoCappingResolution;
    VPP_ENV_SA_CAPWIN deviceCappingResolution;
    VPP_ENV_SA_FLAGS flags;

    //Variable to hold the frame size of each plane
    VPP_ENV_SA_CAPWIN frame_size[MAX_NUM_PLANES];

    //Policy for the UR handling
    unsigned int ur_policy;

    //Current hdcp version of the sink
    VPP_HDMI_HDCP_VERSION curr_sink_hdcp_version;
    //Current requested hdcp version
    unsigned int curr_req_hdcp_version;
    //Current requested plane control for hdcp failure
    unsigned int req_plane_mute_on_fail;
    //flag to indicate if hdcp requirement set in SA entry is failed or not
    bool hdcpFailFlag;
    //flag to indicate if video capping requirement set in SA entry is failed or not
    bool vidCapFailFlag;
    //flag to indicate if device capping requirement set in SA entry is failed or not
    bool devCapFailFlag;
    //flag to indicate if video mute/disable triggered by hdcp_version
    bool muteFlag;
    //flag to indicate if video mute/disable triggered by hdcp failure
    bool muteByHdcpFailFlag;

    //Flag to indicate if video_resolution_capping is updated
    bool video_resolution_capping_updated;
    //Flag to indicate if device_resolution_capping is updated
    bool device_resolution_capping_updated;
    //Flag to indicate if flags is updated
    bool flags_updated;
    //Flag to indicate if framesize is updated
    bool frame_size_updated[MAX_NUM_PLANES];

    //User Configured mute value for various plane
    bool mute_user[MAX_NUM_PLANES];
    //User Configured dispwin for various plane
    VPP_WIN dispwin_user[MAX_NUM_PLANES];
    //User Configured resolution
    unsigned int cpcb_resolution_user;
    //Resolution change mute flag
    bool cpcb_res_change_mute_flag;

    //Callback function to request resolution change whenever hdcp fails
    void (*VPP_ENV_SA_callbackFunc) (UINT32 resId);

    //Variable for unit testing purpose
    unsigned int dbg_hdcp_version_user;
    VPP_ENV_SA_CAPWIN dbg_videoCappingResolution_user;
    VPP_ENV_SA_CAPWIN dbg_deviceCappingResolution_user;
    VPP_ENV_SA_FLAGS dbg_flags_user;

    unsigned int dbg_ur_policy_user;
    bool dbg_print_details_flag_user;

    //Debug flag
    bool dbg_print_flag;
    bool dbg_print_details_flag;
}VPP_ENV_SA;

typedef struct VPP_WRITEBACK_CTRL_t
{
    UINT8   m_uchMode;
    UINT8   m_uchModeUpdated;
    T32HDMIRX_PIPE_CTRL m_stHdmiPipeCtrl;
    T32HDMIRX_PIPE_CTRL0 m_stHdmiPipeCtrl0;
    T32HDMIRX_PIPE_BYPASS_CTRL m_stBypassCtrl;
    T32HDMIRX_PIPE_CFG0 m_stHdmiPipeCfg0;
    T32HDMIRX_PIPE_CFG01 m_stHdmiPipeCfg01;
    T32HDMIRX_PIPE_CFG4 m_stHdmiPipeCfg4;

}VPP_WRITEBACK_CTRL;

typedef struct VPP_CMU_CTLPARAMS_T
{
    INT32 cmu_enabled;
    /*reg*/
    T32EECMU_TG_CTRL cmu_tg_ctl;
    T32EECMU_CTRL0 cmu_ctl0;

    /*Parms*/
    VPP_VDO_ACE_REG ACEParam;
    VPP_CMU_FTDC_PARAMS             FTDCParam;

    UINT32 uiWrapVsCtrl;
    UINT32 stBrCfg0;

    UINT32 stpixvalCfg0;

    UINT32 stHsCfg0;
    UINT32 stHsCfg1;
    UINT32 stHsCfg2;
    UINT32 stHsCfg3;
    UINT32 stHsCfg4;
    UINT32 stHsCfg5;
    UINT32 stHsCfg6;
    UINT32 stHsCfg7;
    UINT32 stHsCfg8;
    UINT32 stHsCfg9;
    UINT32 stHsCfg10;

    INT lti_gain;
    INT cti_gain;

    VPP_CMU_EE_CTRL    EECtrl;
    VPP_CMU_EE_PARAMS  EEParam;

} VPP_CMU_CTL_PARAMS;

typedef struct VPP_GAMMA_CTL_PARAMS_T
{
	UINT32 stGammaCfg0;
	UINT32 stGammaVop2Cfg0;

	AMP_SHM_HANDLE GmBcmBuf_handle[2][2];
	UINT32 *GmBcmBuf[2][2];
	UINT32 GmBcmBufWIdx[2];
	UINT32 GmBcmBufRIdx[2];
} VPP_GAMMA_CTL_PARAMS;

typedef struct VPP_DETEE_CTL_PARAMS_T
{
	INT32 detee_enabled;
	T32DETEE_TG_CTRL detee_tg_ctl;
	VPP_DETEE_GDET_CTRL     DETEEGDetCtrl;
	VPP_DETEE_GDET_PARAMS   DETEEGDetParam;

} VPP_DETEE_CTL_PARAMS;

#define VPP_AXI_L_METER_READ_LUTS    64
#define VPP_AXI_L_METER_WRITE_LUTS    64
typedef struct VPP_AXI_L_METER_t
{
    UINT32 uiReadLuts;
    UINT32 uiNumOfFrames;
    UINT32 uiCaptureCycles;
    UINT32 uiHistWidth;
    UINT32 uiHistHeight;
    UINT32 uiEnable;
    UINT32 uiUpdateRequired;
    UINT32 auiHistReadLut[VPP_AXI_L_METER_READ_LUTS];
    UINT32 auiHistWriteLut[VPP_AXI_L_METER_WRITE_LUTS];
    UINT32 uiStatus;
}VPP_AXI_L_METER;

#define MAX_VPP_OVP_OBJ (10)

typedef struct OVPS_CLIENT_OBJ_t {
	OVPS_CLIENT_STATE client_state;
	int rqstCount;
	int outputQCount;
}OVPS_CLIENT_OBJ;

typedef struct OVPS_OBJ_PARAMS_T {
    VPP_WIN ovp_win;
    INT ovp_win_updated;
    INT ovp_win_outfmt;
    INT ovp_win_stride;
    INT ovp_source;
    INT ovp_source_display;
    INT ovp_output_bit_depth;
    INT ovp_share_vidbuf;
    INT ovp_y_dmaWID;    // OVP Y write-back DMA channel ID
    INT ovp_y_dmaWdhubID; // ID of Dhub on which OVP Y write-back DMA channel is connected
    INT ovp_uv_dmaWID;    // OVP Y write-back DMA channel ID
    INT ovp_uv_dmaWdhubID; // ID of Dhub on which OVP Y write-back DMA channel is connected
    OVP_BUFF_DESCR ovp_buff_descr[2][2];
    T32OVPSCLWRAP_CTRL ovpsclwrap_ctrl;
    T32OVPSCLWRAP_CTRL0 ovpsclwrap_ctrl0;
    T32OVPSCLWRAP_CTRL1 ovpsclwrap_ctrl1;
    T32OVPSCLWRAP_CTRL2 ovpsclwrap_ctrl2;
    T32OVPSCL_CTRL ovp_y_scl_ctrl;
    T32OVPSCL_CTRL ovp_uv_scl_ctrl;
    UINT32 readTiledfromOVPclient;
}OVPS_OBJ_PARAMS;

typedef struct  OVPS_INTR_RECOVERY_t
{
    int vbi_count;	//Count of VBI interrupts, for the current frame resubmission
    int rqst_resubmit; //Set when the resubmission is needed for the current frame
    int intr_issue_count; //Count of interrupt misses
    int ovps_start;
}OVPS_INTR_RECOVERY;

typedef struct  VPP_OVP_OBJ_t
{
    int in_progress;
    int init_success;

    PLANE  plane;
    CHAN   chan;

    DHUB_CFGQ *curr_ovpscl_dma_cfgQ;
    DHUB_CFGQ *curr_ovpscl_bcm_cfgQ;
    UINT8 m_vpp_sched_request;  /*Submit OVPS BCM Buffer along with VPP BCMQ0*/
    UINT8 m_BCM_Sched; /*Q12 or Q0*/

    BCMBUF ovpscl_bcm_buf[BCM_BUF_COUNT];
    BCMBUF *pOvpSclBcmBuf;    //pointer to the VBI BCM buffer in use

    DHUB_CFGQ ovpscl_dma_cfgQ[BCM_BUF_COUNT];
    DHUB_CFGQ ovpscl_bcm_cfgQ[BCM_BUF_COUNT];

    BCMBUF ovpscl_clear_bcm_buf[BCM_BUF_COUNT];
    BCMBUF *pOvpSclClearBcmBuf;

    BCMBUF *pOvpSclStartBcmBuf;

    INT32 intr_en;
    INT32 suspend;

    INT32 output_res;

    void* client_info;

    MV_OSAL_HANDLE_MUTEX_t ovps_lock;
    //Needed for protecting the inputQ from concurrent accesses
    MV_OSAL_HANDLE_MUTEX_t ovps_inputQ_lock;

    MV_HANDLE_Timer_t ovps_monitor; /*Timer for interrupt monitor*/
    Vpp_Callback_Params cb;
    MV_VPP_EVENT_CALLBACK callback;
    UINT32 eventCode;

    OVPS_OBJ_PARAMS ovps_obj_params;

    OVPS_CLIENT_OBJ client_obj[MAX_VPP_OVP_OBJ];
    MV_OSAL_HANDLE_MUTEX_t client_obj_lock;  // protect client_obj[MAX_VPP_OVP_OBJ];
    MV_OSAL_HANDLE_MUTEX_t ovps_isr_mutex_handle;
    MV_OSAL_HANDLE_SEM_t ovps_sem_handle;

     UINT32 uiAviomtrKickoffOvps;
} VPP_OVP_OBJ;

#define OVPS_OVPOBJ_INIT() VPP_OVP_OBJ *ovp_obj = &g_vpp->vpp_objs[0]->ovp_obj

#define OVPS_VPPOBJ_INIT() VPP_OBJ *vpp_obj; \
{ \
    if(g_vpp->magic != VPP_MAGIC) { \
        return MV_VPP_EUNCONFIG; \
    } \
    vpp_obj = g_vpp->vpp_objs[0]; \
    if (!vpp_obj) { \
        return MV_VPP_ENODEV; \
    } \
}
#define OVPS_VPPOVPSOBJ_INIT() OVPS_VPPOBJ_INIT();OVPS_OVPOBJ_INIT();

/************************************************/
/*********** VPP object data structures *********/
/************************************************/
typedef struct VPP_OBJ_T {
    volatile INT    status;                 // active or not
    volatile POWER_MODE suspend;      // Normal, SUSPEND, OPTIMIZED SUSPEND

    PLANE  plane[MAX_NUM_PLANES];  // CURSOR, OSD, IG, PG, PIP, MAIN, BG
    CHAN   chan[MAX_NUM_CHANS];    // PIP, MAIN and OSD
    DV     dv[MAX_NUM_CPCBS];        // CPCB-0
    VOUT   vout[MAX_NUM_VOUTS];    // HDMI, Component, S-Video and CVBS
    VOUT_DISP_MODE m_VoutMode;     // default, ovl, pip, gfx1, gfx2
    EVENT_CALLBACK event_cb[MAX_NUM_EVENTS]; // event callbacks

    INT base_addr;       // VPP object hardware base address
    INT hdtx_base_addr;       // HDTX hardware base address
    INT dcm_base_addr_mp;  //VPP DCM hardware base address
    INT dcm_base_addr_gfx;  //VPP DCM hardware base address
    INT base_addr_mp;
    INT base_addr_gfx_vop;
    INT base_addr_avioMtr;

    FE fe;               // FE hardware related settings
    SCL scl;             // SCL hardware related settings
    CPCB cpcb;           // CPCB hardware related settings
    BE be;               // BE hardware related settings
    //VPP_VDO_DEINT_REG    deintreg;
    VPP_DEINTUSER_CAD_PARAMS_NEW DEINTUserCad1;
    VPP_DEINTUSER_CAD_PARAMS_NEW DEINTUserCad2;
    VPP_DEINTEXCL_CTRL_PARAMS    DEINTExclCtrl;
    VPP_FE_DEINT_LPF_CTRL         DEINTLPFCtrl;
    UINT16                         FilmCadence;
    VPP_FE_DEINT_PRESET_MODE     DEINTPreset;
    VPP_DEINT_FORCED2D_CTRL         DEINTF2DCtrl;
    VPP_DEINT_FILM_MODE_ENTRY_PARAM    DEINTFilmMode;
    UINT8                            DEINTOSDCtrl;
    VPP_DEINTADV_MD_PARAMS          DEINTADVCtrl;

    UINT disp_dsr_count;  // Disp interrupt counter
#ifndef CONFIG_AMP_TEE_ENABLE
    VDEC_VID_BUF_DESC *main_last_frame;
#else
    //We get VBUF_INFO in TZ code
    void *main_last_frame;
#endif
    INT main_frame_repeat;
    INT main_frame_first_field_cnt;
    INT main_last_takeout_field; // main last actual taken out field for interlace input, 0: bottom_field, 1: to field.
    HDL_semaphore *pSemHandle; // semaphore handler of CPCB semaphores

    BCMBUF vbi_bcm_buf[MAX_NUM_CPCBS][BCM_BUF_COUNT];
    //the interrupts of different CPCBs come async, so record VBI  BCM buffer in use for each CPCB
    BCMBUF *pVbiBcmBufCpcb[MAX_NUM_CPCBS];
    BCMBUF *pVbiBcmBuf;    //pointer to the VBI BCM buffer in use

    BCMBUF vbi_clear_bcm_buf[MAX_NUM_CPCBS][BCM_BUF_COUNT];
    //the interrupts of different CPCBs come async, so record VBI  BCM buffer in use for each CPCB
    BCMBUF *pVbiClearBcmBufCpcb[MAX_NUM_CPCBS];
    BCMBUF *pVbiClearBcmBuf;    //pointer to the VBI BCM buffer in use

    // ---- TBD their is only one CPCB now, same apply for above variable also
    BCMBUF vde_bcm_buf[MAX_NUM_CPCBS][BCM_BUF_COUNT];
    //pointer to the VDE BCM buffer in use ---- TBD their is only one CPCB now
    BCMBUF *pVdeBcmBuf[BCM_BUF_COUNT];

    MV_OSAL_HANDLE_MUTEX_t bcmbuf_lock;
    INT vde_int_enable[MAX_NUM_CPCBS];

#if defined(PE_3D_BD)
    UINT8 ss_offset_meta_data[32];
#endif
    UINT8         mv_mode;
    INT bypass_mode; // 1: VPP in byapss mode

#ifndef CONFIG_AMP_TEE_ENABLE
    MV_OSAL_HANDLE_TASK_t vpp_isr_task;
    MV_OSAL_HANDLE_TASK_t cec_isr_task;
    MV_OSAL_HANDLE_TASK_t vpp_dsr_task;
    MV_OSAL_HANDLE_SEM_t  vpp_dsr_sem;
#endif
    DHUB_CFGQ             *curr_vbi_bcm_cfgQ;

    UINT    vpp_isr_need_exit;

    VPP_SHM_HANDLE scl_coeffs_handle[MAX_NUM_PREDEFINED_COEFFS];
    UINT32 *scl_coeffs[MAX_NUM_PREDEFINED_COEFFS];
    //For customized SCL coeff
    VPP_SHM_HANDLE cust_scl_coeffs_handle[VPP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
    UINT16 *cust_scl_coeffs[VPP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
    BOOL    cust_scl_id_flag[VPP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
    BOOL   cust_sc_bPrepared[VPP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
    BOOL   cust_sc_bLoaded[VPP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
    UINT32 dhub_cmdQ[MAX_DHUB_CHANNEL_NUMBER + 1];
    void (*hdmi_cpcb0_dsr_cb)(int id, void *, void *);

#ifdef XVYCC_SUPPORT
    BOOL xvYCC_enable;
    BOOL xvYCC_capable;
    xvYCC_DATA xvYCC_data;
    int xvYCC_mode;
#endif
    /* HDR Feature*/
    BOOL HDR_enable;  //Resolution + EDID
    BOOL HDR_capable; //EDID Info
    BOOL HLG_enable;
    BOOL HLG_capable;
    BOOL HDR10PLUS_enable;
    BOOL HDR10PLUS_capable;
    BOOL HDR_Status;   //HDR enabled or not
    INT32  HDRCount;
    BOOL isHDRContent;
    UINT32 transfer_characteristics;
    UINT8 alt_transfer_characteristics;
    /* HDR Mode transision handling */
    UINT8 current_mode;
    UINT8 transMode;
    BOOL previus_ll_state;
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
    BOOL hdr10plus_mode;
#endif

    // MV enable flag from OTP
    BOOL MV_enable;

    void *pDVconfig;

#ifdef PE_3D_FMT_CONVERT
    UINT32 src_3D_cvt_mode;
    BOOL   src_3D_inv_mode;
#endif

#ifdef VPP_ENABLE_FRAME_CAPTURE
    MV_OSAL_HANDLE_SEM_t vpp_FrameCapture_sem;
    VPP_SHM_HANDLE vpp_FrameBuff_start;
    UINT8 *vpp_FrameBuff_addr;
#endif

    /* Added for suspend and resume */
    INT lumaKeyLow_key;
    INT lumaKeyHigh_key;

    VPP_RES_CALLBACK ResCallback;
    INT uiResID[MAX_NUM_CPCBS];

    HDMI_VOP          hdmi_vop;
    T32OVL_ALP_UD     gOvlAlpUd;
    T32OVL_ALP_UD1     gOvlAlpUd1;
    T32OVL_SENSE_CTRL gOvlSenseCtrl;
    /* SCL shadow registers */
    UINT32 uiSclFrcCfg0[VPP_FRC_SCL_MAX];
    UINT32 uiSclFrcCfg1[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg0[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg3[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg6[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg7[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg8[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg15[VPP_FRC_SCL_MAX];
    UINT32 uiSclCfg18[VPP_FRC_SCL_MAX];
    UINT32 uiCpcb0Ctrl8;
    UINT32 uiCpcb0Ctrl9;

    VPP_MP_CLIENTIF_STRUCT(CTRL0) clientif_ctrl0;
#ifdef RA_avioGbl_BLANK_CG
    T32avioGbl_BLANK_CG avioGbl_blank_cg;
#endif
    T32avioGbl_CTRL stAvioGblCtrl;
    T32GFX0_gfxScl    gfx0_gfxscl;
    T32GFX1_gfxScl    gfx1_gfxscl;
    T32GFX1_gfxScl    gfx2_gfxscl;
    T32OVL_ALPHA_CTRL ovl_alpha_ctrl;
    T32GFX0_CTRL      gfx0_ctrl;
    T32GFX0_CTRL0     gfx0_ctrl0;
    T32GFX0_DB_CTRL   gfx0_DB_ctrl;
    T32GFX0_CTRL1     gfx0_ctrl1;
    T32PIP_PIPE_CTRL1 gfx0_pip_pipe_ctrl1;
    T32GFX1_CTRL      gfx1_ctrl;
    T32GFX1_CTRL1     gfx1_ctrl1;
    T32GFX1_CTRL      gfx2_ctrl;
    T32GFX1_CTRL1     gfx2_ctrl1;
    T32MAINPL_CTRL4   mainpl_ctrl4;
    T32MAINPL_CTRL5   mainpl_ctrl5;
    T32CPCB0_CTRL2    cpcb0_ctrl2;
    T32CPCB0_CTRL0    cpcb0_ctrl0;
#ifdef EN_VPP_DYNAMIC_CLKGATE
    T32VPTOP_CFG0 vptop_cfg0;
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */

    T32SCL_CFG14            gfx0_scl_pwrGate;
    T32SCL_CFG14            gfx1_scl_pwrGate;
    T32ADPSCL_CFG20         hdmirx_scl_cfg20;
    T32ADPSCL_CFG20         vidpl_scl_cfg20;
    T32ADPSCL_CFG20         vidpl_ovp_scl_luma_cfg20;
    T32ADPSCL_CFG20         vidpl_ovp_scl_chroma_cfg20;
    T32UPS_420_422_CFG8     vidpl_ups420_422_cfg8;
    T32UPS_420_422_CFG8     pippl_ups420_422_cfg8;
    T32UPS_420_422_HDMI_12b_CFG8 hdmirx_ups420_422_cfg8;
    T32GFX0_UPS420_CTRL     gfx0_ups420_ctrl;
    T32UPS_420_422_SP_CFG8  gfx0_ups420_422sp_cfg8;
    T32DNS422_420_CFG1      hdmi_vop_dns422_420_cfg1;
    T32DINT_CFG43           vptop_dint_cfg43;
    T32DINTDMX_CFG4         vptop_dint_dmx_cfg4;
    T32GFX0_SCL_BYPASS      gfx0_scl_bypass;
    T32GFX1_SCL_BYPASS      gfx1_scl_bypass;

    T32HDMIRX_PIPE_BYPASS_CTRL hdmirx_pipe_ctrl;
    T32vidpl_ADPSCLctrl1   vidpl_ADPSCLctrl1;
    T32CPCB0_CTRL0 stCpcb0_ctrl;
    T32VOP_CTRL stVop_ctrl;
#ifdef CONFIG_DEINT_ENABLE
    T32DINT_CFG0  dint_cfg0;
    T32DINT_CFG1  dint_cfg1;
    T32DINT_CFG5  dint_cfg5;
    T32DINT_CFG40 dint_cfg40;
    T32DINT_CFG19 dint_cfg19;
    T32DINT_CFG20 dint_cfg20;
    T32DINT_CFG21 dint_cfg21;
    T32DINT_CFG29 dint_cfg29;
    T32DINT_CFG25 dint_cfg25;
#endif
    T32VPTOP_CTRL_CFG0 vptop_ctrl_cfg0;

    UINT32     disptg_intl_count;
    T32OVL_APM ovl_apm;
    T32TG_PRG_CTRL1 disptg_prg_ctrl1[MAX_NUM_CPCBS];
    T32tgWrap_CTRL1 tgwrap_ctrl1;
    T32DTG_CTRL1 dtg_ctrl1[MAX_NUM_CPCBS];
    VPP_SCL_RES scl_res[VPP_FRC_SCL_MAX];
    VPP_SCL_CTRL scl_ctrl[VPP_FRC_SCL_MAX];
    VPP_CLKGATE_ENABLE_OPT enClkGate; /* Enable it to dynamically clock gate unused IPs */
    VPP_SCL_RES sclResCfg; /* i/o resolution configured for offline scaling */
    T32Vpp_CTRL       vpp_Ctrl;
    T32Vpp_rst vpp_rst;
    T32Vpp_rst1 vpp_rst1;
    T32MAINPL_CTRL0 mainplCtrl0;
    T32EL_PIPE_CTRL1 stElPipeCtrl1;
    T32EL_PIPE_CTRL2 stElPipeCtrl2;
    T32MAINPL_CTRL7 stMainPlCtrl7;
    T32MAINPL_CTRL10 stMainPlCtrl10;
    T32MAINPL_CTRL11 stMainPlCtrl11;
    T32MAINPL_CTRL12 stMainPlCtrl12;
    T32MAINPL_CTRL13 stMainPlCtrl13;
    T32MAINPL_CTRL14 stMainPlCtrl14;
    T32vidpl_SCL_CLKEN_CTRL stVidplSclClkenCtrl;
    T32Vpp_CPCB0_PL_EN cpcb0_pl_en;
    T32VOP2_TG_CTRL vop2_tg_ctl;
    T32VOP2_CTRL0 vop2_ctrl0;
    UINT8 is_uf_triggered[MAX_NUM_CPCBS];
#ifdef VPP_HANDLE_SPURIOUS_INTERRUPTS
    INT16 activePlaneCount;
    INT8 force_uf;
#endif
    UINT8 m_uchDvTestPathMode;
    UINT8 m_uchDvTestPathModeUpdate;
    UINT8 m_uchDvTestPath;
    UINT8 m_uchDvEnableDump; //Enable/Disable dolby dump feature
    UINT8 m_HdmiOutputMode; //0 - SDR, 1 - DoVi STD, 2 - DoVi LL, 4 - HDR

    UINT8 m_HdrMode; //0 - off, 1 - auto, 2 - on
    VPP_COLORTEMP clrtemp;
    UINT8 need_dcm_update;
    UINT8 m_outformat;
#ifdef CONFIG_AMP_DOLBY_DUMP
    VPP_DV_FRAME_DUMP vpp_fd_config;
    int vpp_fd_init;
    int vpp_fd_consumedLastFrame;
    UINT32 vpp_frame_count;
#endif //CONFIG_AMP_DOLBY_DUMP


    UINT32 m_uiTileCfg6;
    UINT8 BT2020_capability;
    BOOL hdmi_bt2020en;
    UINT32 CscInp_colorprim;
    VPP_WRITEBACK_CTRL stVppWriteBackCtrl;
    INT32 ups420_422_cfg6_y;
    INT32 ups420_422_cfg6_c;

    T32CPCB0_CTRL10 m_stCpcb0_ctrl10;

    /* For customized SCL coeffs */
    INT hscl_coeff_update[VPP_FRC_SCL_MAX];
    INT vscl_coeff_update[VPP_FRC_SCL_MAX];
    INT hscl_coeff[VPP_FRC_SCL_MAX];
    INT vscl_coeff[VPP_FRC_SCL_MAX];
    INT scl_force_sel[VPP_FRC_SCL_MAX];   // 0:auto selection, 1:force selection scaler H coeff, 2:force selection scaler V coeff, 3:force selection both
    INT scl_coeff_update[VPP_FRC_SCL_MAX];// 1: scaler coeff & nl setting need update in ISR, 0: scaler coeff do not need update in ISR

    OVPS_OBJ_PARAMS *povps_obj_params;
#ifdef TZ_3_0
    HDL_dhub2d VPP_dhubHandle;
#endif
    VPP_GLOBAL global;
    UINT32 resIdUpdated[MAX_NUM_CPCBS];
    T32VDCM_CFG0 stVdcmCfg0;
    UINT32 m_uiDcmEnable; //VDCM/GDCM Enable/Disable status
    UINT32 m_uiDcmEnableUpdated;
    VPP_WIN m_stDcmAoiWin;  //Area Of Intreset window sizes
    UINT32 m_uiDcmAoiWinUpdated;
    UINT32 m_uiDcmAoiEnable;
    UINT32 m_uiDcmAoiReg;
    BOOL enable_vbibcm_autopush[MAX_NUM_CPCBS];
    UINT32 videofmtUpdated;
    //Underflow Count
    UINT32 uiUfCount[MAX_NUM_CPCBS];

    //Data structure to store various debug/monitor state/context variables
    VPP_DEBUG_DATA vppdbg_data;

    UINT32 uiRasterScanRdCtrl1;
    UINT32 uiDvCompElUsingPip;
    T32VIPSCLTOP_CTRL5 stVipSclTopCtrl5;
    UINT32 uiNgOtpEnParams;
    UINT32 uiAviomtrKickoff;
    VPP_DETEE_CTL_PARAMS detee;
    VPP_CMU_CTL_PARAMS cmu;
    VPP_GAMMA_CTL_PARAMS gamma;

    VPP_AXI_L_METER     stAxiLmeter;
    ITMO_FILTUR_LUT stITMO_fill_lut[ITMO_FILTER_MAX];
    VPP_ENV_SA vpp_sa;

    BOOL  bIsHDROutput;

    VPP_HDMI_PKT_HDR_INFO_FRAME hdrInfo;
    float gSaturation[VPP_MAX_CHANNELS];
    float gHue[VPP_MAX_CHANNELS];
    float gContrast[VPP_MAX_CHANNELS];
    float gBrightness[VPP_MAX_CHANNELS];

    VPP_OVP_OBJ ovp_obj;

    void *mKeystore;
    VOID *phdr_cfg;
    UINT32 avpll_regs[PLLBK_MAX][CTRLBK_MAX];

    VPP_DSI_INFO    dsiInfo;
    VPP_DISP_OUT_PARAMS dispOutParams[MAX_NUM_CPCBS];
    INT iVppFeatureCfg[MAX_NUM_FEATURE_CFG];
} VPP_OBJ;

/* definition of EE working mode */
enum {
    EE_MODE_OFF   = 0, /* disable EE */
    EE_MODE_ON    = 1, /* enable EE*/
};

/**************************************
 * Global Debug Configuration Structure
 *************************************/
typedef struct tagVPP_LOGGER {
    UINT    enable;
    VPP_LOG_LEVEL   log_lev[VPP_LOG_MODULE_MAX];
    VPP_LOG_PATH    outpath;
} VPP_LOGGER;

#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
typedef struct tagVPP_FrameInfo {
    VBUF_INFO  frame_info_local;
    VBUF_INFO  *frame_info_input;
} VPP_FrameInfo;
VBUF_INFO* VPP_FrameInfoPool_AllocFrameInfo(int planeId, VBUF_INFO *pVbufInfo);
VBUF_INFO* VPP_FrameInfoPool_FreeFrameInfo(VBUF_INFO *pVbufInfo);
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

/**************************************
 * Fastlogo Configuration Structure
 *************************************/
typedef struct tagVPP_FASTLOGO {

    // VPP REGISTER REGION
    TEE_TA_REG_REGION vppRegRegion;

#ifdef CONFIG_VPP_FASTLOGO_PUSHPAR_SUPPORT
#define VPP_MAX_FRAMES_PER_PLANE    3
    //Variable to support PUSHPAR feature
    VBUF_INFO  frame_info[MAX_NUM_PLANES][VPP_MAX_FRAMES_PER_PLANE];
    INT        currFrame[MAX_NUM_PLANES];
#endif //CONFIG_VPP_FASTLOGO_PUSHPAR_SUPPORT

    //flush buffer before submitting to HW
    BOOL enableFlushCache;
    //VBI sema interrupt enabled state
    BOOL enableInterrupt[MAX_NUM_CPCBS];

#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
    FRAMEQUEUE frame_info_pool;
    int frame_info_pool_count;
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

    //Reserved for future purpose
    void *rsvd;
} VPP_FASTLOGO;


typedef struct VPP_PTM_OBJ_T{
    void *ptmSession;
    INT session_count;
    UINT32 referencedAddr[MAX_NUM_PLANES];
} VPP_PTM_OBJ;

/************************************************/
/******** VPP container data structures *********/
/************************************************/
typedef struct VPP_T {
    INT magic;
    INT curr_obj_num;
    VPP_OBJ *vpp_objs[MAX_NUM_OBJECTS];
    VPP_PTM_OBJ ptm_obj;
    void *vmx_obj;
#if defined(VPP_SUS_RES)
    INT suspend_status[MAX_NUM_OBJECTS];
    VPP_OBJ *vpp_suss[MAX_NUM_OBJECTS];
#endif
    VPP_LOGGER VPP_Debug;

#ifdef CONFIG_VPP_FASTLOGO_SUPPORT
    VPP_FASTLOGO VPP_fl;
#endif

} VPP;

/************************************************/
/*********** global VPP container ***************/
/************************************************/
#ifdef _VPP_API_C_
VPP *g_vpp=NULL;
VPP vpp;
#else
extern VPP *g_vpp;
extern VPP vpp;
#endif


/**************************************************************************/
/****************** Common VPP object Function Calls **********************/
/**************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
