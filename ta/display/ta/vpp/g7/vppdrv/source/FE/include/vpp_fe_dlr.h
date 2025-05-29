/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_FE_DLR_H_
#define _VPP_FE_DLR_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cfg.h"

/**********************************************************************
 *Data structure definitions for FE DLR  block driver                                                 *
 ***********************************************************************/

/*WPL defines how many beats (one beat is 64-bit word) read from DMA per line*/
#define PIXEL_PER_BEAT_YUV_422   4
#define PIXEL_PER_BEAT_ARGB_8888 2
#define PIXEL_PER_BEAT_YUV444 4
#define PIXEL_PER_BEAT_YUV_420SP 8
#define PIXEL_PER_BEAT_YUV_444_DWA   2
#define PIXEL_PER_BEAT_YUV_420SP_DWA   6
#define PIXEL_PER_BEAT_RGB_565 4
#define PIXEL_PER_BEAT_ARGB_4444 4

#ifdef CONFIG_AMP_VPP_ROTATION
#define VPP_IS_420SP_FMT(src_fmt) ((src_fmt == SRCFMT_YUV420SP) || \
    (src_fmt == SRCFMT_YUV420SP_TILED_MODE0) || \
    (src_fmt == SRCFMT_YUV420SP_TILED_MODE1) ||  \
    (src_fmt == SRCFMT_YUV420SP_DWA) || \
    (SRCFMT_YUV420SP_TILED_V4H8 == src_fmt))
#define VPP_IS_420SP_TILED_FMT(src_fmt)  ((src_fmt == SRCFMT_YUV420SP_TILED_MODE0) || \
    (src_fmt == SRCFMT_YUV420SP_TILED_MODE1) || \
    (SRCFMT_YUV420SP_TILED_V4H8 == src_fmt))
#else
#define VPP_IS_420SP_FMT(src_fmt) ((src_fmt == SRCFMT_YUV420SP) || \
    (src_fmt == SRCFMT_YUV420SP_TILED_MODE0) || \
    (src_fmt == SRCFMT_YUV420SP_TILED_MODE1) ||  \
    (src_fmt == SRCFMT_YUV420SP_DWA))
#define VPP_IS_420SP_TILED_FMT(iSrcFmt)  ((src_fmt == SRCFMT_YUV420SP_TILED_MODE0) || \
    (src_fmt == SRCFMT_YUV420SP_TILED_MODE1))
#endif

/*Enum for CSC mode*/
typedef enum VPP_FE_DLR_CSC_MODES
{
    VPP_FE_DLR_CSC_BYPASS_MODE = 0,
    VPP_FE_DLR_CSC_601_TO_709,
    VPP_FE_DLR_CSC_709_TO_601,
    VPP_FE_DLR_CSC_RGB_TO_YUV_709,
    VPP_FE_DLR_CSC_YUV_709_TO_RGB,
    VPP_FE_DLR_CSC_RGB_TO_YUV_601,
    VPP_FE_DLR_CSC_YUV_601_TO_RGB,
    VPP_FE_DLR_CSC_sRGB_TO_YUV_709,
    VPP_FE_DLR_CSC_YUV_709_TO_sRGB,
    VPP_FE_DLR_CSC_sRGB_TO_YUV_601,
    VPP_FE_DLR_CSC_YUV_601_TO_sRGB,
    VPP_FE_DLR_CSC_MAX_MODES
} VPP_FE_DLR_CSC_MODES;

/*Enum for dataloader in-line channel*/
typedef enum VPP_FE_DLR_CHANNEL
{
    VPP_FE_DLR_CHANNEL_MIN = 0,
    VPP_FE_DLR_CHANNEL_MAIN_0 = VPP_FE_DLR_CHANNEL_MIN,
    VPP_FE_DLR_CHANNEL_MAIN_1,
#ifdef ENABLE_OVP
    VPP_FE_DLR_CHANNEL_MAIN_2,
    VPP_FE_DLR_CHANNEL_MAIN_3,
#endif
    VPP_FE_DLR_CHANNEL_OFFLINE_RD_0,
    VPP_FE_DLR_CHANNEL_PIP_0,
    VPP_FE_DLR_CHANNEL_PIP_1,
    VPP_FE_DLR_CHANNEL_EL_PIPE,
    VPP_FE_DLR_CHANNEL_GFX1_0,
    VPP_FE_DLR_CHANNEL_GFX2_0,
    VPP_FE_DLR_CHANNEL_VMX_0,
    VPP_FE_DLR_CHANNEL_EDR_0,
    VPP_FE_DLR_CHANNEL_OVP_EL_0,
    VPP_FE_DLR_CHANNEL_OVP_EL_1,
    VPP_FE_DLR_CHANNEL_MAX
}VPP_FE_DLR_CHANNEL;

/*Enum for dummy tg*/
typedef enum VPP_FE_DLR_PIPE
{
    VPP_FE_DLR_MIN = 0,
    VPP_FE_DLR_MAIN = VPP_FE_DLR_MIN,
    VPP_FE_DLR_PIP,
    VPP_FE_DLR_GFX1,
    VPP_FE_DLR_GFX2,
    VPP_FE_DLR_VMX,
    VPP_FE_DLR_MAX = VPP_FE_DLR_VMX
}VPP_FE_DLR_PIPE;

typedef enum VPP_FE_DLR_RASTERSCAN_t
{
    VPP_FE_DLR_RASTERSCAN_MIN   =   0,
    VPP_FE_DLR_RASTERSCAN_BL    =   0,
    VPP_FE_DLR_RASTERSCAN_EL,
    VPP_FE_DLR_RASTERSCAN_PIP,
    VPP_FE_DLR_RASTERSCAN_MAX
}VPP_FE_DLR_RASTERSCAN;

typedef enum VPP_FE_DLR_RASTERSCAN_OUT_t
{
    VPP_FE_DLR_RASTERSCAN_OUT_MIN   =   0,
    VPP_FE_DLR_RASTERSCAN_OUT_DVCOMP   =   0,
    VPP_FE_DLR_RASTERSCAN_OUT_OVPSCL,
    VPP_FE_DLR_RASTERSCAN_OUT_MAX
}VPP_FE_DLR_RASTERSCAN_OUT;

/*input data format*/
typedef enum VPP_FE_DLR_FMT_SRC_FMT
{
    VPP_FE_DLR_SRC_FMT_YUV422 = 0,
    VPP_FE_DLR_SRC_FMT_ARGB32 = 0,
    VPP_FE_DLR_SRC_FMT_ARGB24,
    VPP_FE_DLR_SRC_FMT_ARGB4444,
    VPP_FE_DLR_SRC_FMT_RGB565,
    VPP_FE_DLR_SRC_FMT_ARGB1555,
    VPP_FE_DLR_SRC_FMT_LUT8,
    VPP_FE_DLR_SRC_FMT_MAX

}VPP_FE_DLR_FMT_SRC_FMT;

/*input data components order in OSD or PG graphic planes. In B0, for graphic planes*/
typedef enum VPP_FE_DLR_FMT_ORDER_OSD_PG
{
    VPP_FE_DLR_ORDER_OSD_PG_ARGB = 0,
    VPP_FE_DLR_ORDER_OSD_PG_ABGR,
    VPP_FE_DLR_ORDER_OSD_PG_RGBA,
    VPP_FE_DLR_ORDER_OSD_PG_BGRA,
    VPP_FE_DLR_ORDER_OSD_PG_MAX

}VPP_FE_DLR_FMT_ORDER_OSD_PG;

/*input data components order in PG graphic plane. In B0, for graphic planes*/
typedef enum VPP_FE_DLR_FMT_ORDER_PG
{
    VPP_FE_DLR_ORDER_PG_AVYU = 0,
    VPP_FE_DLR_ORDER_PG_AUYV,
    VPP_FE_DLR_ORDER_PG_VYUA,
    VPP_FE_DLR_ORDER_PG_UYVA,
    VPP_FE_DLR_ORDER_PG_MAX

}VPP_FE_DLR_FMT_ORDER_PG;

/*input data components order in video or PIP planes. In B0, for video planes*/
typedef enum VPP_FE_DLR_FMT_ORDER_VIDEO
{
    VPP_FE_DLR_ORDER_VIDEO_AVYU = 0,
    VPP_FE_DLR_ORDER_VIDEO_AUYV,
    VPP_FE_DLR_ORDER_VIDEO_VYUA,
    VPP_FE_DLR_ORDER_VIDEO_UYVA,
    VPP_FE_DLR_ORDER_VIDEO_MAX

}VPP_FE_DLR_FMT_ORDER_VIDEO;

/*input data format for main video*/
typedef enum VPP_FE_DLR_MAIN_INPUT_FMT
{
    VPP_FE_DLR_MAIN_INTERLACED = 0,
    VPP_FE_DLR_MAIN_PROGRESSIVE

}VPP_FE_DLR_MAIN_INPUT_FMT;

/*data loader input resolution*/
typedef struct VPP_FE_DLR_INPUT_RES_T
{
    UINT32 HRes;
    UINT32 VRes;

}VPP_FE_DLR_INPUT_RES, *PVPP_FE_DLR_INPUT_RES;

/*data loader crop*/
typedef struct VPP_FE_DLR_INPUT_CROP_T
{
    UINT32 leftCrop;
    UINT32 rightCrop;
    UINT32 topCrop;
    UINT32 bottomCrop;
}VPP_FE_DLR_INPUT_CROP, *PVPP_FE_DLR_INPUT_CROP;


/*data loader plane start location*/
typedef struct VPP_FE_DLR_PLANE_START_LOC_T
{
    UINT32 HLoc;
    UINT32 VLoc;

}VPP_FE_DLR_PLANE_START_LOC, *PVPP_FE_DLR_PLANE_START_LOC;

#define SWAP_W_H(x,y)   \
{\
    int tmp;    \
    tmp = x;    \
    x = y;      \
    y = tmp;    \
};


/**********************************************************************
 *Function definition for FE DLR block driver                                                             *
 ***********************************************************************/

/***************************************************************************************
 * FUNCTION: load default values of data loader to BCM buffer
 * PARAMS:  Channel - indicate the channel
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_LoadDefaultVal(VPP_OBJ *vpp_obj, INT Channel);

/***************************************************************************************
 * FUNCTION: set the input size in pixels for DBVIP
 * PARAMS:  ldr_id - id of a channel.
 *                pDBVIPSize - pointer to the struct of horizontal and vertial size.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetHDRInputSize(VPP_OBJ *vpp_obj, INT id, PVPP_FE_DLR_INPUT_RES pDBVIPSize);

/***************************************************************************************
 * FUNCTION: set the size of the plane.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 *                pPlaneSize - pointer to the struct of horizontal and vertial size.
 *                CropWpl - beat per line and crop
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetPlaneSize(VPP_OBJ *vpp_obj, INT Channel, PVPP_FE_DLR_INPUT_RES pReadSize, PVPP_FE_DLR_INPUT_RES pPlaneSize, UINT8 srcFmt, UINT8 BitsPerPixel);

/***************************************************************************************
 * FUNCTION: set input data format and order.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 *                pDataFmt - pointer to the struct of data format and order.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetPlanePackSel(VPP_OBJ *vpp_obj, INT Channel, UINT8 srcFmt, UINT8 BitsPerPixel);

/***************************************************************************************
 * FUNCTION: enable the channel.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 *                Enable - 1:enable, 0:no effect.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_EnableChannel(VPP_OBJ *vpp_obj, INT Channel, UINT8 srcFmt, BOOL Enable);

/***************************************************************************************
 * FUNCTION:  set CSC coefficients and offsets for MOSD or cursor channel.
 * PARAMS: Channel - id of a front-end dataloader channel.
 *               CscModes- CSC mode.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_LoadCSC(VPP_OBJ *vpp_obj, INT Channel, INT CscModes);

/***************************************************************************************
 * FUNCTION: clear all the FIFOs and pipline data inside the channel.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_ClearChannel(VPP_OBJ *vpp_obj, INT Channel);

/***************************************************************************************
 * FUNCTION: set dummy TG for main video channel.
 * PARAMS: TotRes - input total resolution.
 *                progressive - 1: prgressive, 0: interlaced.
 *                sensio - 1: sensio mode, 0: non-sensio mode.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes, int progressive);

INT FE_DLR_SetDummyTG_offlineRD(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes);

INT FE_DLR_Config(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt, INT order, INT bits_per_pixel, INT is_prog);

INT FE_DLR_SetClientSize(VPP_OBJ *vpp_obj, INT chanID, INT Channel, PVPP_FE_DLR_INPUT_RES pReadSize, PVPP_FE_DLR_INPUT_RES pPlaneSize, UINT8 srcFmt, UINT8 BitsPerPixel);

INT FE_DLR_SetNonStdResPixLineTotal(VPP_OBJ *vpp_obj, INT Channel, PVPP_FE_DLR_INPUT_RES pReadSize);

INT FE_DLR_420_422_UPS_Config_UVSwap(VPP_OBJ *vpp_obj, UINT32 ldr_id, UINT8 uiUVswapEn);
INT FE_DLR_GFX0_UPS8_Config_UVSwap(VPP_OBJ *vpp_obj, UINT8 uiUVswapEn);
INT FE_DLR_ConfigTilePackSel(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, INT iTileW, INT iTileH);
INT FE_DLR_422_444_UPSConfig_UVSwap(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, UINT8 uiUVswapEn);
INT FE_DLR_ConfigDifBlock(VPP_OBJ *vpp_obj, INT iPlaneId, INT iSrcFmt, INT iOrder, INT iBitsPerPixel, INT iRotation);
INT FE_DLR_ConfigElPipe(VPP_OBJ *vpp_obj, INT iSrcFmt, INT iOrder, INT iBitDepth, INT iEnable);
INT FE_DLR_ConfigElPipeTg(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes);
INT FE_DLR_ConfigOfflineRD(VPP_OBJ *vpp_obj, INT width, INT height);
void cropDispContent(VPP_WIN *crop_actv_win, int *crop_actv_offset, VPP_WIN *crop_scl_actv_win, VBUF_INFO *pinfo, VPP_WIN *ref_win, VPP_WIN * disp_win, RESOLUTION_INFO *cpcb_resinfo, int dataAlign_x, int dataAlign_y, int bpp, PLANE *plane);
void cropDispContent_UV(VPP_WIN *crop_actv_win, int *crop_actv_offset, VPP_WIN *crop_scl_actv_win, VBUF_INFO *pinfo, VPP_WIN *ref_win, VPP_WIN *disp_win, RESOLUTION_INFO *cpcb_resinfo, int dataAlign_x, int dataAlign_y, int bpp, PLANE *plane);

#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
INT FE_DLR_SetBitmap(VPP_OBJ *vpp_obj, UINT uiPlaneId);
#else
INT FE_DLR_SetBitmap(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt, INT order);
#endif

INT FE_DLR_EnableDeAlpha(VPP_OBJ *vpp_obj, UINT uiPlaneId, BOOL Enable);
UINT8 INTERNAL_FE_DLR_GetBitDepth(UINT8 src_fmt, UINT8 BitsPerPixel);
UINT8 INTERNAL_FE_DLR_GetUPS420DataCtrl(UINT8 src_fmt, UINT8 BitsPerPixel);
#endif

