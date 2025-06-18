/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_FRAMECAPTURE_H_
#define _VPP_FRAMECAPTURE_H_

/* To enable the frame capture of 1080p resolution */
#define CONFIG_VPP_ARGB_CAPTURE_RATIO 1
#define CONFIG_VPP_YUV_CAPTURE_RATIO 1

/* enable this to dump the frames as YUV422 */
#undef CONFIG_CAPTURE_YUV422_MODE

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VPP_CAPTURE_YUV420SP_TILE_SD_MODE,
    VPP_CAPTURE_YUV420SP_TILE_HD_MODE,
    VPP_CAPTURE_YUV420SP_SD_MODE,
    VPP_CAPTURE_YUV420SP_HD_MODE,
    VPP_CAPTURE_YUV422_SD_MODE,
    VPP_CAPTURE_YUV422_HD_MODE,
    VPP_CAPTURE_ARGB_MODE,
};

/***************************************************************************************
 * FUNCTION: Creat Frame Capture task
 * PARAMS:   None
 *
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to Creat Frame Capture task
 ***************************************************************************************/
HRESULT VPP_CreateFrameCaptureTask(VPP_OBJ *vpp_obj);

/***************************************************************************************
 * FUNCTION: Destory Frame Capture task
 * PARAMS:   None
 *
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to Destory Frame Capture task
 ***************************************************************************************/
HRESULT VPP_DestroyFrameCaptureTask(VPP_OBJ *vpp_obj);

/***************************************************************************************
 * FUNCTION: capture frame task
 * PARAMS:   None
 *
 * RETURN:  None
 ***************************************************************************************/
void VPP_CaptureFrameTask(VPP_OBJ *vpp_obj);

#ifdef __cplusplus
}
#endif

#endif
