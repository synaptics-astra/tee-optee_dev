/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
//-----------------------------------------------------------------------------
//! \file       vpp_env_ur.h
//! \brief      The interfacing function to get/read all env/config information
//!             required by VPP driver to implement Nagra Content Usage Rules Enforcement
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_ENV_UR_H_
#define _VPP_ENV_UR_H_

#include "vpp_module.h"

#ifdef __cplusplus
extern "C" {
#endif

//The enum for various different security attributes required by VPP Driver
typedef enum __VPP_ENV_SA_ENTRY__ {
    VPP_ENV_SA_ENTRY_HDCP_VER,
    VPP_ENV_SA_ENTRY_VIDEO_RESCAP,
    VPP_ENV_SA_ENTRY_DEVICE_RESCAP,
    VPP_ENV_SA_ENTRY_FLAGS,
    VPP_ENV_SA_ENTRY_HDCP_CSM_TYPE,
    VPP_ENV_SA_ENTRY_ALL,
}VPP_ENV_SA_ENTRY;

//The enum for various HDCP version defined in hdcp_version SA entry
typedef enum __VPP_ENV_SA_HDCP_VERSION__ {
    VPP_ENV_SA_HDCP_VERSION_NONE    = 0,
    VPP_ENV_SA_HDCP_VERSION_1_0     = 1,
    VPP_ENV_SA_HDCP_VERSION_2_0     = 2,
    VPP_ENV_SA_HDCP_VERSION_2_1     = 3,
    VPP_ENV_SA_HDCP_VERSION_2_2     = 4,
    //any value > VPP_ENV_SA_HDCP_VERSION_2_2, should be considered as VPP_ENV_SA_HDCP_VERSION_2_2
}VPP_ENV_SA_HDCP_VERSION;

typedef enum __VPP_ENV_SA_HDCP_CSM_TYPE__
{
    VPP_ENV_SA_HDCP_CSM_TYPE_0    = 0,
    VPP_ENV_SA_HDCP_CSM_TYPE_1    = 1,
    VPP_ENV_SA_HDCP_CSM_TYPE_AUTO = 2,
}VPP_ENV_SA_HDCP_CSM_TYPE;

//The enum for various down/re-scaling option defined in video_resolution_capping SA entry
typedef enum __VPP_ENV_SA_VID_RES_CAP__ {
    //Don't allow downscaling, and avoid user request for dispwin
    VPP_ENV_SA_VID_RES_CAP_OPT_0,
    //if hdcp version not reached, then scale HD->SD
    VPP_ENV_SA_VID_RES_CAP_OPT_1,
    //Limit o/p to SD (720x480)
    VPP_ENV_SA_VID_RES_CAP_OPT_2,
    //Limit o/p to 1280x720
    VPP_ENV_SA_VID_RES_CAP_OPT_3,
    //Limit o/p to 1920x1080
    VPP_ENV_SA_VID_RES_CAP_OPT_4,
    //Limit o/p to 3840x2160
    VPP_ENV_SA_VID_RES_CAP_OPT_5,
    //Don't allow downscaling, but allow user request for dispwin
    VPP_ENV_SA_VID_RES_CAP_OPT_15,
}VPP_ENV_SA_VID_RES_CAP;

//Various handy macro around flags
//Access individual flags
#define VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(flags)     flags.resolutionCappingRotationAllowed
#define VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(flags)       flags.resolutionCappingAspectAllowed
#define VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(flags)     flags.deviceCappingRotationAllowed
#define VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(flags)       flags.deviceCappingAspectAllowed
//initialize all flags
#define VPP_ENV_SA_FLAGS_SET(flags, resCapRotFlag, resCapAspFlag, devCapRotFlag, devCapAspFlag) \
    VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(flags)    = resCapRotFlag; \
    VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(flags)      = resCapAspFlag; \
    VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(flags)    = devCapRotFlag; \
    VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(flags)      = devCapAspFlag;
//perform FLAGS_A|=FLAGS_B
#define VPP_ENV_SA_FLAGS_OR(FLAGS_A, FLAGS_B) \
    VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(FLAGS_A)   |= VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(FLAGS_B);   \
    VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(FLAGS_A)     |= VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(FLAGS_B);     \
    VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(FLAGS_A)   |= VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(FLAGS_B);   \
    VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(FLAGS_A)     |= VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(FLAGS_B);
//Check if devCap Flags changes
#define IS_VPP_ENV_SA_DEV_CAP_FLAGS_CHANGED(FLAGS_A, FLAGS_B) \
    ((VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(FLAGS_A) !=  VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(FLAGS_B)) ||   \
     VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(FLAGS_A) != VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(FLAGS_B))
//Check if resCap Flags changes
#define IS_VPP_ENV_SA_RES_CAP_FLAGS_CHANGED(FLAGS_A, FLAGS_B) \
    ((VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(FLAGS_A) !=  VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(FLAGS_B)) ||   \
     VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(FLAGS_A) != VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(FLAGS_B))
//Check if devCap or resCap Flags changes
#define IS_VPP_ENV_SA_FLAGS_CHANGED(FLAGS_A, FLAGS_B) \
    (IS_VPP_ENV_SA_RES_CAP_FLAGS_CHANGED(FLAGS_A, FLAGS_B) || IS_VPP_ENV_SA_DEV_CAP_FLAGS_CHANGED(FLAGS_A, FLAGS_B))

//Various handy macro around 'VPP_ENV_SA_CAPWIN'
//Initialize the CAPWIN
#define VPP_ENV_SA_CAPWIN_SET(CAPWIN_A, wid, hgt)      \
    CAPWIN_A.width = wid;    CAPWIN_A.height = hgt;
//Initialize the CAPWIN with min value
#define VPP_ENV_SA_CAPWIN_SET_MIN(CAPWIN_A, wid, hgt)               \
    CAPWIN_A.width = CAPWIN_A.width<wid? CAPWIN_A.width:wid;        \
    CAPWIN_A.height = CAPWIN_A.height<hgt? CAPWIN_A.height:hgt;

//Check if CAPWIN_A = CAPWIN_B
#define IS_VPP_ENV_SA_CAPWIN_EQUAL(CAPWIN_A, CAPWIN_B)      \
    ((CAPWIN_A.width == CAPWIN_B.width) && (CAPWIN_A.height == CAPWIN_B.height))
//Check if CAPWIN_A < CAPWIN_B (either wid/hgt)
#define IS_VPP_ENV_SA_CAPWIN_LESS(CAPWIN_A, CAPWIN_B)       \
    ((CAPWIN_A.width < CAPWIN_B.width) || (CAPWIN_A.height < CAPWIN_B.height))
//Check if CAPWIN_A > CAPWIN_B (either wid/hgt)
#define IS_VPP_ENV_SA_CAPWIN_GREATER(CAPWIN_A, CAPWIN_B)    \
    ((CAPWIN_A.width > CAPWIN_B.width) || (CAPWIN_A.height > CAPWIN_B.height))

//Check if ROTATE(CAPWIN_A) = CAPWIN_B
#define IS_VPP_ENV_SA_CAPWIN_ROTATE_EQUAL(CAPWIN_A, CAPWIN_B)      \
    ((CAPWIN_A.height == CAPWIN_B.width) && (CAPWIN_A.width == CAPWIN_B.height))
//Check if ROTATE(CAPWIN_A) < CAPWIN_B (either wid/hgt)
#define IS_VPP_ENV_SA_CAPWIN_ROTATE_LESS(CAPWIN_A, CAPWIN_B)       \
    ((CAPWIN_A.height < CAPWIN_B.width) && (CAPWIN_A.width < CAPWIN_B.height))
//Check if ROTATE(CAPWIN_A) > CAPWIN_B (either wid/hgt)
#define IS_VPP_ENV_SA_CAPWIN_ROTATE_GREATER(CAPWIN_A, CAPWIN_B)    \
    ((CAPWIN_A.height > CAPWIN_B.width) || (CAPWIN_A.width > CAPWIN_B.height))

//Check if ASPECT(CAPWIN_A) = ASPECT(CAPWIN_B)
#define IS_VPP_ENV_SA_CAPWIN_ASPECT_EQUAL(CAPWIN_A, CAPWIN_B)      \
    ((CAPWIN_A.height*CAPWIN_A.width) == (CAPWIN_B.width*CAPWIN_B.height))
//Check if ASPECT(CAPWIN_A) < ASPECT(CAPWIN_B)
#define IS_VPP_ENV_SA_CAPWIN_ASPECT_LESS(CAPWIN_A, CAPWIN_B)      \
    ((CAPWIN_A.height*CAPWIN_A.width) < (CAPWIN_B.width*CAPWIN_B.height))
//Check if ASPECT(CAPWIN_A) > ASPECT(CAPWIN_B)
#define IS_VPP_ENV_SA_CAPWIN_ASPECT_GREATER(CAPWIN_A, CAPWIN_B)      \
    ((CAPWIN_A.height*CAPWIN_A.width) > (CAPWIN_B.width*CAPWIN_B.height))

//The enum for various UR policy supported by the VPP driver for handling SA entry
typedef enum __VPP_ENV_SA_UR_POLICY__ {
    VPP_ENV_SA_UR_POLICY_NONE,
    VPP_ENV_SA_UR_POLICY_MUTE,
    VPP_ENV_SA_UR_POLICY_SCALING,
    VPP_ENV_SA_UR_POLICY_RESOLUTION
}VPP_ENV_SA_UR_POLICY;

#define IS_VPP_ENV_SA_POLICY_NONE(vpp_obj)          (vpp_obj->vpp_sa.ur_policy == VPP_ENV_SA_UR_POLICY_NONE)
#define IS_VPP_ENV_SA_POLICY_MUTE(vpp_obj)          (vpp_obj->vpp_sa.ur_policy == VPP_ENV_SA_UR_POLICY_MUTE)
#define IS_VPP_ENV_SA_POLICY_SCALING(vpp_obj)       (vpp_obj->vpp_sa.ur_policy == VPP_ENV_SA_UR_POLICY_SCALING)
#define IS_VPP_ENV_SA_POLICY_RESOLUTION(vpp_obj)    (vpp_obj->vpp_sa.ur_policy == VPP_ENV_SA_UR_POLICY_RESOLUTION)
#define IS_VIDEO_PLAYBACK(pInfo)        \
    (pInfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || SRCFMT_YUV420SP_TILED_MODE1 ||  \
    SRCFMT_YUV420SP_TILED_V4H8 || pInfo->m_srcfmt == SRCFMT_YUV420SP || pInfo->m_srcfmt == SRCFMT_YUV422)

#ifdef CONFIG_VPP_ENV_SA_SUPPORT
/******************************************************************************
 * FUNCTION: initialize the ENV SA moudle
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 ******************************************************************************/
void VPP_ENV_SA_Initialize(VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION: check if setPlaneMute is allowed/denied
 *           plane unmute/unmute from user is taken backup and restored laster on exiting mute state
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        planeID, Plane ID to be muted/unmuted
 *        mute, mute/unmute flag
 ******************************************************************************/
bool VPP_ENV_SA_IsMuteAllowed(VPP_OBJ *vpp_obj, int planeID, int mute);

/******************************************************************************
 * FUNCTION: Get the VPP related attribute from the security attribute table
 *           This has to be invoked before processing cmdq,
 *           Since mute command can be added based on HDCP_VERSION SA attribute value
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 ******************************************************************************/
void VPP_ENV_SA_GetAttributes(VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION: update the dispwin for various video plane based on video resolution capping security attribute
 *           Please refer to various caping option in enum - VPP_ENV_SA_VID_RES_CAP
 *           This need to be triggered for each plane at the begining of startChannelLoader
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        planeID, Plane whose dispwin need to be altered
 ******************************************************************************/
void VPP_ENV_SA_ProcessResolutionCap(VPP_OBJ *vpp_obj, int planeID);

/******************************************************************************
 * FUNCTION: Get the resolution cap based on video resolution capping security attribute
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 * OUTPUT: Resolution ID corresponding to the video resolution capping security attribute
 ******************************************************************************/
unsigned int VPP_ENV_SA_GetResCapResolution(VPP_OBJ *vpp_obj, unsigned int user_resID);

/******************************************************************************
 * FUNCTION: Register the callback function for resolution chage because of res capping due to hdcp failure
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        callbackFunc function pointer
 ******************************************************************************/
void VPP_ENV_SA_RegisterCallback(VPP_OBJ *vpp_obj, void (*callbackFunc) (UINT32 resId));

/******************************************************************************
 * FUNCTION: Check if the play back is secure
 * INPUT: Handle for vpp_obj
 ******************************************************************************/
bool VPP_ENV_SA_isSecurePlayback(INT handle,INT planeID);

/******************************************************************************
 * FUNCTION: get the current plane Mute status as hardware
 *           note: mute status is overridden by SA UR (HDCP version, etc.)
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        planeID, Plane ID whose mute status is required
 ******************************************************************************/
bool VPP_ENV_SA_GetMuteStatus(VPP_OBJ *vpp_obj, int planeID);
#else //!CONFIG_VPP_ENV_SA_SUPPORT

#define VPP_ENV_SA_Initialize(vpp_obj)
#define VPP_ENV_SA_IsMuteAllowed(vpp_obj, planeID, mute)    true
#define VPP_ENV_SA_GetAttributes(vpp_obj)
#define VPP_ENV_SA_ProcessResolutionCap(vpp_obj, planeID)
#define VPP_ENV_SA_GetResCapResolution(vpp_obj, user_resID) user_resID
#define VPP_ENV_SA_RegisterCallback(vpp_obj, callbackFunc)
#define VPP_ENV_SA_isSecurePlayback(handle, planeID)  false
#define VPP_ENV_SA_GetMuteStatus(vpp_obj, planeID)    vpp_obj->plane[planeID].mute

#endif //CONFIG_VPP_ENV_SA_SUPPORT

#ifdef __cplusplus
}
#endif

#endif //_VPP_ENV_UR_H_

