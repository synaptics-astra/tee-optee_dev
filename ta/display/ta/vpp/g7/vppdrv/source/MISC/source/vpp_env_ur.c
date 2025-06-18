/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_env_ur.c
//! \brief      get/read all env/config information required by VPP driver
//              to implement Nagra Content Usage Rules Enforcement
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifdef CONFIG_VPP_ENV_SA_SUPPORT

#include "vpp_env_ur.h"
#include "vpp_be_prv.h"
#include "vpp_cfg.h"
#include "vpp_be.h"
#include "vpp_apifuncs.h"
#include "tee_internal_sa_api.h"
#include "vpp_be_hdmitx.h"
#include "tee_ta_vpp.h"
#include "vpp_dv_api.h"

//Enable the following to handle non Nagra environment or NONE policy handling
#define VPP_ENV_SA_HANDLE_POLICY_NONE
//Enable to check 'sa.receivedUsageRules==0' to enter POLICY_NONE mode
//#define VPP_ENV_SA_CHECK_RECIEVED_UR_FOR_POLICY_NONE

//Select one of the policy for UR handling
#define VPP_ENV_SA_DEFAULT_POLICY   VPP_ENV_SA_UR_POLICY_MUTE
//#define VPP_ENV_SA_DEFAULT_POLICY   VPP_ENV_SA_UR_POLICY_SCALING
//#define VPP_ENV_SA_DEFAULT_POLICY   VPP_ENV_SA_UR_POLICY_RESOLUTION

//Enable to apply the UserRules to GFX plane also
//#define VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE

//Enable to apply mute/unmute of planes during resolution change
//#define VPP_ENV_SA_MUTE_PLANE_DURING_RES_CHANGE

//Define the behaviour for res cap option 0 - enable to allow muting, otherwise don't allow scaling
#define VPP_ENV_SA_MUTE_FOR_RES_CAP_OPT_0

//Enable to parse all entries, otherwise read/parse only active entry
#define VPP_ENV_SA_PARSE_ALL_ENTRIES

//Enable following macro for VPP_ENV debug prints, Default=Enable
#define VPP_ENV_DEBUG_ENABLE

//Enable/disable the bootup log from VPP ENV module - default disabled
#define VPP_ENV_BOOTUP_LOG_ENABLED   false

/*AndriodN: VPPAPI_DBG_INFO - logcat, printf/printk - Console */
#define VPP_ENV_PRINT_PREFIX    "VPPDRV:VPP_ENV_SA:"
#define VPP_ENV_PRINT(...)      VPPAPI_DBG_INFO(__VA_ARGS__)

#ifdef VPP_ENV_DEBUG_ENABLE
#define VPP_ENV_DEBUG_PRINT(...)            VPP_ENV_PRINT(__VA_ARGS__)
#define VPP_ENV_DEBUG_PRINT_DETAILS(...)
#define VPP_ENV_DEBUG_PRINT_DYNAMIC(...)    { if(vpp_obj->vpp_sa.dbg_print_flag) VPP_ENV_PRINT(__VA_ARGS__); }
#else
#define VPP_ENV_DEBUG_PRINT(...)
#define VPP_ENV_DEBUG_PRINT_DETAILS(...)
#define VPP_ENV_DEBUG_PRINT_DYNAMIC(...)
#endif
//Enable unit testing of hdcp version and resolution capping SA entry
//#define VPP_ENV_UNIT_TEST_HDCP_VERSION

//Enable to check secure playback using muteflag
//#define VPP_ENV_SA_CHECK_SECURE_PLAYBACK_BY_MUTEFLAG

#ifdef VPP_ENV_UNIT_TEST_HDCP_VERSION
#define VPP_ENV_SA_COMMAND_ID                 0xF0
#define VPP_ENV_SA_HDCP_VERSION_COMMAND_ID      1
#define VPP_ENV_SA_VID_RES_CAP_WID_COMMAND_ID   2
#define VPP_ENV_SA_VID_RES_CAP_HGT_COMMAND_ID   3
#define VPP_ENV_SA_DEV_RES_CAP_WID_COMMAND_ID   4
#define VPP_ENV_SA_DEV_RES_CAP_HGT_COMMAND_ID   5
#define VPP_ENV_SA_FLAGS_COMMAND_ID             6
#define VPP_ENV_SA_POLICY_COMMAND_ID            7
#define VPP_ENV_SA_DETAIL_PRINT_COMMAND_ID      8
#endif //VPP_ENV_UNIT_TEST_HDCP_VERSION

#define VPP_ENV_SA_IS_WIN_DIFFERENT(win1, win2) ((win1.x!=win2.x)||(win1.y!=win2.y)||(win1.width!=win2.width)||(win1.height!=win2.height))
#define VPP_ENV_SA_IS_WIN_SAME(win1, win2)      (!VPP_ENV_SA_IS_WIN_DIFFERENT(win1,win2))


#ifdef VPP_ENV_SA_ENABLE_SCALING
/* Data structure to store the RES_CAP and dispwin mapping */
typedef struct __VPP_ENV_RES_CAP_WIN_MAP__ {
    VPP_ENV_SA_VID_RES_CAP resCap;
    UINT32 resId;
    VPP_WIN resCapWindow;
}VPP_ENV_RES_CAP_WIN_MAP;

static VPP_ENV_RES_CAP_WIN_MAP g_vppEnvResCapWindowMap[] = {
    {VPP_ENV_SA_VID_RES_CAP_OPT_2,  RES_525P60,     {0, 0,  720,    480}},
    {VPP_ENV_SA_VID_RES_CAP_OPT_3,  RES_720P60,     {0, 0,  1280,   720}},
    {VPP_ENV_SA_VID_RES_CAP_OPT_4,  RES_1080P60,    {0, 0,  1920,   1080}},
    {VPP_ENV_SA_VID_RES_CAP_OPT_5,  RES_4Kx2K60,    {0, 0,  3840,   2160}},
};

//Get the datastructure assocaited with the VPP_ENV_SA_VID_RES_CAP
static VPP_ENV_RES_CAP_WIN_MAP* VPP_ENV_SA_getResCapWinMap(VPP_ENV_SA_VID_RES_CAP resCap) {
    int i,n;

    n = sizeof(g_vppEnvResCapWindowMap) / sizeof(VPP_ENV_RES_CAP_WIN_MAP);

    for(i=0; i<n; i++) {
        if(g_vppEnvResCapWindowMap[i].resCap == resCap) {
            break;
        }
    }

    VPP_ENV_DEBUG_PRINT_DETAILS(VPP_ENV_PRINT_PREFIX "Searching ResCapWinMap(%d) : %d/%d - (%d,%d,%d,%d)\n",
            resCap, i, n-1, i<n?g_vppEnvResCapWindowMap[i].resCapWindow.x:0, i<n?g_vppEnvResCapWindowMap[i].resCapWindow.y:0,
            i<n?g_vppEnvResCapWindowMap[i].resCapWindow.width:0, i<n?g_vppEnvResCapWindowMap[i].resCapWindow.height:0);

    return i<n ? &g_vppEnvResCapWindowMap[i] : NULL;
}
#endif

static void VPP_ENV_SA_showSAEntry(struct SecurityAttribute entry, int ndx) {
    static int bEnableDebugLog=VPP_ENV_BOOTUP_LOG_ENABLED;

    if (bEnableDebugLog) {
        VPP_ENV_DEBUG_PRINT(VPP_ENV_PRINT_PREFIX "SA Entry[%d] : index:%d, sessionId:%d, uuid:%d\n",
                                ndx, entry.index, entry.sessionId, entry.uuid);
        VPP_ENV_DEBUG_PRINT(VPP_ENV_PRINT_PREFIX "SA Entry[%d] : "
                                "index:%d, receivedUsageRules:%d, hdcpVersion:%d\n",
                                ndx, entry.index, entry.receivedUsageRules, entry.hdcpVersion);
        VPP_ENV_DEBUG_PRINT(VPP_ENV_PRINT_PREFIX "SA Entry[%d] : "
                                "index:%d, hdcpCappingResolution:%dx%d, deviceCappingResolution:%dx%d\n",
                                ndx,entry.index, entry.hdcpCappingResolutionWidth, entry.hdcpCappingResolutionHeight,
                                entry.deviceCappingResolutionWidth, entry.deviceCappingResolutionHeight);
        VPP_ENV_DEBUG_PRINT(VPP_ENV_PRINT_PREFIX "SA Entry[%d] : "
                                "index:%d, resolutionCappingRotationAllowed:%d, resolutionCappingAspectAllowed:%d, deviceCappingRotationAllowed:%d, deviceCappingAspectAllowed:%d\n",
                                ndx, entry.index, entry.resolutionCappingRotationAllowed, entry.resolutionCappingAspectAllowed, entry.deviceCappingRotationAllowed, entry.deviceCappingAspectAllowed);
    }
}

/* Check if playback is secured/unsecured */
bool VPP_ENV_SA_isSecurePlayback(INT handle, INT planeID)
{
    bool isSecurePB = false;

    if(IS_DV_CertTest())
        return isSecurePB;

    VBUF_INFO *pInfo = NULL;
    VDEC_VID_BUF_DESC *inline_frame;
    VPP_OBJ *vpp_obj =  g_vpp->vpp_objs[handle];
    PLANE *plane = &(vpp_obj->plane[planeID]);
    VPP_SHM_BUFFER_DESC bufDesc;

    if (plane->status == STATUS_DISP_VIDEO)
        inline_frame = plane->curr_frame;
    else if (plane->status == STATUS_DISP_LOGO)
        inline_frame = plane->curr_still_picture;
    else if (plane->status == STATUS_DISP_PATGEN)
        inline_frame = plane->dummy_frame;
    else
        inline_frame = NULL;

    if (inline_frame) {
#ifndef CONFIG_AMP_TEE_ENABLE
        pInfo = (VBUF_INFO *)inline_frame->user_data;
#else
        pInfo = (VBUF_INFO *)inline_frame;
#endif
    }


    if(!IS_VIDEO_PLAYBACK(pInfo)) {
        return 0;
    }

    VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)pInfo->m_pbuf_start, pInfo->m_buf_size, VPP_SHM_BUFFER_TYPE_INPUT);
    if(TEE_ERROR_BAD_PARAMETERS == VPP_ValidateInput(&bufDesc, 1)) {
        isSecurePB = true;
    }
    else if((planeID == PLANE_MAIN
#ifdef ENABLE_PIP_GFX0_PATH
                || planeID == PLANE_PIP //TODO: Disable using static table if(vpp_plane_map[PlaneId].is_valid)
#endif
        )) {
        if(!IS_VPP_ENV_SA_POLICY_NONE(vpp_obj)) {
            #ifdef  VPP_ENV_SA_CHECK_SECURE_PLAYBACK_BY_MUTEFLAG
            if(vpp_obj->vpp_sa.muteFlag == 1)
            #else
            if(!((vpp_obj->vpp_sa.hdcp_version == 0 || vpp_obj->vpp_sa.hdcp_version == -1)&&
                vpp_obj->vpp_sa.videoCappingResolution.width == 0 &&
                vpp_obj->vpp_sa.videoCappingResolution.height == 0 &&
                vpp_obj->vpp_sa.deviceCappingResolution.width == 10000 &&
                vpp_obj->vpp_sa.deviceCappingResolution.height == 10000))
            #endif
            {
                isSecurePB = true;
            }
        }
    }
    return isSecurePB;
}

static void VPP_ENV_SA_parseAndCombineEntries(VPP_OBJ *vpp_obj, VPP_ENV_SA_ENTRY sa) {
    static int bEnableDebugLog=VPP_ENV_BOOTUP_LOG_ENABLED;
    unsigned int nEntries, ndx, processCnt;
    struct SecurityAttribute entry;
    VPP_ENV_SA_HDCP_VERSION hdcp_ver = 0;
    VPP_ENV_SA_HDCP_CSM_TYPE hdcp_csm_type = VPP_ENV_SA_HDCP_CSM_TYPE_AUTO;
    //VPP_ENV_SA_VID_RES_CAP vid_res = 0;
#ifdef VPP_ENV_SA_CHECK_BOTH_WIDTH_AND_HEIGHT
    VPP_ENV_SA_CAPWIN temp_res = {0,0};
#endif
    VPP_ENV_SA_CAPWIN vid_res = {0,0};
    VPP_ENV_SA_CAPWIN dev_res = {0,0};
    VPP_ENV_SA_FLAGS temp_flags={{0,0,0,0,0}};
    VPP_ENV_SA_FLAGS flags={{0,0,0,0,0}};
    unsigned int ur_policy = vpp_obj->vpp_sa.ur_policy;
    TEE_Result res;

#ifdef VPP_ENV_SA_PARSE_ALL_ENTRIES
    nEntries = getActiveEntries();
#else
    nEntries = 1;
#endif

    if(bEnableDebugLog) {
        //Show SA entry details for debugging purpose
        VPP_ENV_DEBUG_PRINT(VPP_ENV_PRINT_PREFIX "getActiveEntries : %x\n", nEntries);
    }

    //If SA entry, then apply policy otherwise no/none/bypass/ignore policy
    ur_policy = nEntries? VPP_ENV_SA_DEFAULT_POLICY:VPP_ENV_SA_UR_POLICY_NONE;

    for(processCnt = ndx = 0; ndx<nEntries; ndx++)
    {
        //Reset the data
        memset(&entry, 0, sizeof(struct SecurityAttribute));

        //Read/Retrieve SA entry
        res = getActiveSecurityAttributeEntry(&entry);
        if(TEE_SUCCESS != res) {
            if(bEnableDebugLog) {
                VPP_ENV_DEBUG_PRINT(VPP_ENV_PRINT_PREFIX "getActiveSecurityAttributeEntry Failed with error : %x\n", res);
            }
            //TBD: Do we need to go into POLICY_NONE for getEntry() failure
            //ur_policy = VPP_ENV_SA_UR_POLICY_NONE;
            continue;
        }

        if(bEnableDebugLog) {
            //Show SA entry details for debugging purpose
            VPP_ENV_SA_showSAEntry(entry, ndx);
        }

        //the most restrictive hdcp version should be used - 4 is most restrictive then 2
        if(entry.hdcpVersion >= hdcp_ver) {
            hdcp_ver = entry.hdcpVersion;
        } else if (entry.hdcpVersion == -1) {
            if(bEnableDebugLog) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "entry deleted : hdcp version %d\n", entry.hdcpVersion);
            }
            ur_policy = VPP_ENV_SA_UR_POLICY_NONE;
        }

#ifdef TZ_5_3
        if((entry.ContentStreamType == VPP_ENV_SA_HDCP_CSM_TYPE_0) ||
            (entry.ContentStreamType == VPP_ENV_SA_HDCP_CSM_TYPE_1)) {
            hdcp_csm_type = entry.ContentStreamType;
        }
#endif

        if(ndx) {
        #ifdef VPP_ENV_SA_CHECK_BOTH_WIDTH_AND_HEIGHT
            //The most restrictive version of resolution capping is 0x0
            VPP_ENV_SA_CAPWIN_SET(temp_res, entry.hdcpCappingResolutionWidth, entry.hdcpCappingResolutionHeight);
            if(IS_VPP_ENV_SA_CAPWIN_LESS(temp_res, vid_res)) {
                vid_res = temp_res;
            }

            //The most restrictive version of device capping is 0x0
            VPP_ENV_SA_CAPWIN_SET(temp_res, entry.deviceCappingResolutionWidth, entry.deviceCappingResolutionHeight);
            if(IS_VPP_ENV_SA_CAPWIN_LESS(temp_res, dev_res)) {
                dev_res = temp_res;
            }
        #else
            VPP_ENV_SA_CAPWIN_SET_MIN(vid_res, entry.hdcpCappingResolutionWidth, entry.hdcpCappingResolutionHeight);
            VPP_ENV_SA_CAPWIN_SET_MIN(dev_res, entry.deviceCappingResolutionWidth, entry.deviceCappingResolutionHeight);
        #endif
        } else {
            //Init the value for 1st iteration
            VPP_ENV_SA_CAPWIN_SET(vid_res, entry.hdcpCappingResolutionWidth, entry.hdcpCappingResolutionHeight);
            VPP_ENV_SA_CAPWIN_SET(dev_res, entry.deviceCappingResolutionWidth, entry.deviceCappingResolutionHeight);
        }

        //The most restrictive version of any flag is TRUE/ENABLED, So OR flags from various entry
        VPP_ENV_SA_FLAGS_SET(temp_flags, entry.resolutionCappingRotationAllowed, entry.resolutionCappingAspectAllowed, entry.deviceCappingRotationAllowed, entry.deviceCappingAspectAllowed);
        VPP_ENV_SA_FLAGS_OR(flags, temp_flags);

#ifdef VPP_ENV_SA_CHECK_RECIEVED_UR_FOR_POLICY_NONE
        if(!entry.receivedUsageRules) {
            ur_policy = VPP_ENV_SA_UR_POLICY_NONE;
        }
#endif //VPP_ENV_SA_CHECK_RECIEVED_UR_FOR_POLICY_NONE

        //increase the processing count
        processCnt++;
    }

    if(processCnt == nEntries) {
        //Update the local variable only when all entries are processed and merged
        switch(sa) {
            case VPP_ENV_SA_ENTRY_HDCP_VER:
                vpp_obj->vpp_sa.hdcp_version = hdcp_ver;
                break;
            case VPP_ENV_SA_ENTRY_VIDEO_RESCAP:
                vpp_obj->vpp_sa.videoCappingResolution  = vid_res;
                break;
            case VPP_ENV_SA_ENTRY_DEVICE_RESCAP:
                vpp_obj->vpp_sa.deviceCappingResolution = dev_res;
                break;
            case VPP_ENV_SA_ENTRY_FLAGS:
                vpp_obj->vpp_sa.flags.flags = (UINT8)flags.flags;
                break;
            case VPP_ENV_SA_ENTRY_HDCP_CSM_TYPE:
                vpp_obj->vpp_sa.hdcp_strm_type = hdcp_csm_type;
                break;
            case VPP_ENV_SA_ENTRY_ALL:
                vpp_obj->vpp_sa.hdcp_version = hdcp_ver;
                vpp_obj->vpp_sa.videoCappingResolution  = vid_res;
                vpp_obj->vpp_sa.deviceCappingResolution = dev_res;
                vpp_obj->vpp_sa.flags.flags = (UINT8)flags.flags;
                vpp_obj->vpp_sa.hdcp_strm_type = hdcp_csm_type;
                break;
        }
    }

    //Check if the policy is changed, because receivedUsageRules=0 or failed to getActiveEntries()/getActiveSecurityAttributeEntry()
    if(ur_policy != vpp_obj->vpp_sa.ur_policy) {

        if(VPP_ENV_BOOTUP_LOG_ENABLED || bEnableDebugLog) {
            VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "Policy changed : %x -> %x\n", vpp_obj->vpp_sa.ur_policy, ur_policy);
        }
        #ifdef VPP_ENV_SA_HANDLE_POLICY_NONE
            vpp_obj->vpp_sa.ur_policy = ur_policy;
        #endif //VPP_ENV_SA_HANDLE_POLICY_NONE
    }

    if(bEnableDebugLog) {
        bEnableDebugLog = false;
    }

#ifdef VPP_ENV_UNIT_TEST_HDCP_VERSION
    if(vpp_obj->vpp_sa.dbg_print_details_flag == 1) {
        bEnableDebugLog = true;
        vpp_obj->vpp_sa.dbg_print_details_flag_user = 0;
    }
#endif //VPP_ENV_UNIT_TEST_HDCP_VERSION

}

/*Read and update the SA attribute value*/
static void VPP_ENV_SA_update(VPP_OBJ *vpp_obj) {
    //TBD: Read the SA attribute and update the value in vpp_obj->vpp_sa : hdcp_version & video_resolution_capping
    VPP_ENV_SA_parseAndCombineEntries(vpp_obj, VPP_ENV_SA_ENTRY_ALL);


#ifdef VPP_ENV_UNIT_TEST_HDCP_VERSION
    //Copy used defined value to the SA variable
    vpp_obj->vpp_sa.hdcp_version = vpp_obj->vpp_sa.dbg_hdcp_version_user;
    vpp_obj->vpp_sa.videoCappingResolution = vpp_obj->vpp_sa.dbg_videoCappingResolution_user;
    vpp_obj->vpp_sa.deviceCappingResolution = vpp_obj->vpp_sa.dbg_deviceCappingResolution_user;
    vpp_obj->vpp_sa.flags = vpp_obj->vpp_sa.dbg_flags_user;
    vpp_obj->vpp_sa.ur_policy = vpp_obj->vpp_sa.dbg_ur_policy_user;
    vpp_obj->vpp_sa.dbg_print_details_flag = vpp_obj->vpp_sa.dbg_print_details_flag_user;
#endif //VPP_ENV_UNIT_TEST_HDCP_VERSION

}

/* Mute/unmute planes during setres callback */
#ifdef VPP_ENV_SA_MUTE_PLANE_DURING_RES_CHANGE
static void VPP_ENV_SA_setPlaneMute(VPP_OBJ *vpp_obj, bool muteFlag) {
    if(muteFlag || vpp_obj->vpp_sa.cpcb_res_change_mute_flag) {
        int planeID;
        int params[3];

        params[1] = CPCB_1;
        vpp_obj->vpp_sa.cpcb_res_change_mute_flag = muteFlag;

        for(planeID = FIRST_PLANE; planeID < MAX_NUM_PLANES; planeID++) {
            params[0] = planeID;
             params[2] = muteFlag?muteFlag:vpp_obj->vpp_sa.mute_user[planeID];
            VPP_SetPlaneMute( vpp_obj, params);
        }
    }
}
#else //VPP_ENV_SA_MUTE_PLANE_DURING_RES_CHANGE
#define VPP_ENV_SA_setPlaneMute(...)
#endif //VPP_ENV_SA_MUTE_PLANE_DURING_RES_CHANGE

static void VPP_ENV_SA_doPlaneMute(VPP_OBJ *vpp_obj, bool muteFlag, bool skipGfx) {
    int params[3];
    int planeID;

    VPPAPI_DBG_INFO("muteFlag:%d - skipgfx:%d\n", muteFlag, skipGfx);
    params[1] = CPCB_1;
    params[2] = muteFlag;
    //If current action is mute, then take backup of mute status of video plane and mute them
    for(planeID = FIRST_PLANE; planeID < MAX_NUM_PLANES; planeID++) {
        if((skipGfx == true) && (PLANE_GFX1 == planeID)) {
            //Do not control scale/mute; allow application to take care of GFX plane
            continue;
        }
        //vpp_obj->vpp_sa.mute_user[planeID] = vpp_obj->plane[planeID].mute;
        params[0] = planeID;
        //MV_VPPOBJ_SetPlaneMute(0, planeID, true);
        VPP_SetPlaneMute( vpp_obj, params);
    }
}

static void VPP_ENV_SA_doPlaneMute_User(VPP_OBJ *vpp_obj, bool skipGfx) {
    int params[3];
    int planeID;

    params[1] = CPCB_1;
    //If current action is mute, then take backup of mute status of video plane and mute them
    for(planeID = FIRST_PLANE; planeID < MAX_NUM_PLANES; planeID++) {
        if((skipGfx == true) && (PLANE_GFX1 == planeID)) {
            //Do not control scale/mute; allow application to take care of GFX plane
            continue;
        }
        VPPAPI_DBG_INFO("plane-%d muteFlag:%d\n", planeID, vpp_obj->vpp_sa.mute_user[planeID]);
        params[2] = vpp_obj->vpp_sa.mute_user[planeID];
        params[0] = planeID;
        VPP_SetPlaneMute( vpp_obj, params);
    }
}

/******************************************************************************
 * FUNCTION: initialize the ENV SA moudle
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 ******************************************************************************/
void VPP_ENV_SA_Initialize(VPP_OBJ *vpp_obj) {
    //Default value for dynamic debug print is Disable
    vpp_obj->vpp_sa.dbg_print_flag = 0;

#ifdef VPP_ENV_SA_DEFAULT_POLICY
    vpp_obj->vpp_sa.ur_policy = VPP_ENV_SA_DEFAULT_POLICY;
#else
    vpp_obj->vpp_sa.ur_policy = VPP_ENV_SA_UR_POLICY_MUTE;
#endif

    //TBD: Move the global variable g_vppEnvResCapWindowMap to vpp_obj->vpp_sa, so that per instant dataspace is reduced in TZ3.0
}

/******************************************************************************
 * FUNCTION: check if setPlaneMute is allowed/denied
 *           plane unmute/unmute from user is taken backup and restored later on exiting mute state
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        planeID, Plane ID to be muted/unmuted
 *        mute, mute/unmute flag
 ******************************************************************************/
bool VPP_ENV_SA_IsMuteAllowed(VPP_OBJ *vpp_obj, int planeID, int mute) {

    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if((planeID>=FIRST_PLANE && planeID<MAX_NUM_PLANES) && (mute <= 1)) {
        //Record the valid user request for mute/unmute, so that correct state is restored back later
        if(vpp_obj->vpp_sa.hdcpFailFlag || vpp_obj->vpp_sa.muteFlag)
        {
            VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user configured mute flag : planeID:%d, mute:%d, hdcpFailFlag:%d, muteFlag:%d\n",
                     planeID, mute, vpp_obj->vpp_sa.hdcpFailFlag, vpp_obj->vpp_sa.muteFlag);
        }
        vpp_obj->vpp_sa.mute_user[planeID] = mute;
    }
    else {
#ifdef VPP_ENV_UNIT_TEST_HDCP_VERSION
        int command=mute&0xF0;
        int sub_command=mute&0x0F;
        int value=(mute&0xFFFF00)>>8;
        static int vid_res_width;
        static int dev_res_width;
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "Command : %x, SubCommand:%x, value:%x\n", command, sub_command, value);
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "Current policy: %x, hdcp:%x, video_res:%dx%d, device_res:%dx%d, Flags:%x, hdcpFailFlag:%d, muteFlag:%d, devCapFailFlag:%d\n",
                vpp_obj->vpp_sa.ur_policy, vpp_obj->vpp_sa.hdcp_version,
                vpp_obj->vpp_sa.videoCappingResolution.width, vpp_obj->vpp_sa.videoCappingResolution.height,
                vpp_obj->vpp_sa.deviceCappingResolution.width, vpp_obj->vpp_sa.deviceCappingResolution.height,
                vpp_obj->vpp_sa.flags, vpp_obj->vpp_sa.hdcpFailFlag, vpp_obj->vpp_sa.muteFlag, vpp_obj->vpp_sa.devCapFailFlag);
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "Current frame: MAIN=%dx%d, PIP=%dx%d\n",
                vpp_obj->vpp_sa.frame_size[PLANE_MAIN].width, vpp_obj->vpp_sa.frame_size[PLANE_MAIN].height, vpp_obj->vpp_sa.frame_size[PLANE_PIP].width, vpp_obj->vpp_sa.frame_size[PLANE_PIP].height);
        if(VPP_ENV_SA_COMMAND_ID == command) {
            //Invalid planeID, Let us recieve the HDCP version through mute for planeID=VPP_ENV_SA_HDCP_VERSION_COMMAND_ID
            if(VPP_ENV_SA_HDCP_VERSION_COMMAND_ID == sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user hdcp_version changed : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_hdcp_version_user, value);
                vpp_obj->vpp_sa.dbg_hdcp_version_user = value;
            } else if(VPP_ENV_SA_VID_RES_CAP_WID_COMMAND_ID== sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user video_resolution_capping : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_videoCappingResolution_user.width, value);
                vid_res_width = value;
            } else if(VPP_ENV_SA_VID_RES_CAP_HGT_COMMAND_ID== sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user video_resolution_capping : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_videoCappingResolution_user.height, value);
                vpp_obj->vpp_sa.dbg_videoCappingResolution_user.height = value;
                vpp_obj->vpp_sa.dbg_videoCappingResolution_user.width = vid_res_width;
            } else if(VPP_ENV_SA_DEV_RES_CAP_WID_COMMAND_ID== sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user device_resolution_capping : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_deviceCappingResolution_user.width, value);
                dev_res_width = value;
            } else if(VPP_ENV_SA_DEV_RES_CAP_HGT_COMMAND_ID== sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user device_resolution_capping : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_deviceCappingResolution_user.height, value);
                vpp_obj->vpp_sa.dbg_deviceCappingResolution_user.height = value;
                vpp_obj->vpp_sa.dbg_deviceCappingResolution_user.width = dev_res_width;
            } else if(VPP_ENV_SA_FLAGS_COMMAND_ID == sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user flags: %d->%d\n",
                    vpp_obj->vpp_sa.dbg_flags_user.flags, value);
                vpp_obj->vpp_sa.dbg_flags_user.flags= value;
            } else if(VPP_ENV_SA_POLICY_COMMAND_ID == sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user policy : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_ur_policy_user, value);
                vpp_obj->vpp_sa.dbg_ur_policy_user = value;
            } else if(VPP_ENV_SA_DETAIL_PRINT_COMMAND_ID == sub_command) {
                VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "user detail debug : %d->%d\n",
                    vpp_obj->vpp_sa.dbg_print_details_flag_user, value);
                vpp_obj->vpp_sa.dbg_print_details_flag_user = value;
            }
            //Don't allow mute() when command is given for processing
            return false;
        }
#endif //VPP_ENV_UNIT_TEST_HDCP_VERSION
    }

#ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
    //Do not control mute; allow application to take care of GFX plane
    if(PLANE_GFX1 == planeID) {
        return true;
    }
#endif //VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE

    if((pHdmiTx->planeControlOnHdcpFail == true) &&
       (vpp_obj->vpp_sa.muteByHdcpFailFlag == true)) {
        //If the plane is muted because of hdcp failure, add corresponding check.
        return false;
    }

#ifdef VPP_ENV_SA_HANDLE_POLICY_NONE
    //If policy not enabled for this platform, then return immediately
    if(IS_VPP_ENV_SA_POLICY_NONE(vpp_obj)) {
        return true;
    }
#endif //VPP_ENV_SA_HANDLE_POLICY_NONE

    //Allow muting, only if we have not forced muting of planes becaues of UR
    return !vpp_obj->vpp_sa.muteFlag;
}

/******************************************************************************
 * FUNCTION: get the current plane Mute status as hardware
 *           note: mute status is overridden by SA UR (HDCP version, etc.)
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        planeID, Plane ID whose mute status is required
 ******************************************************************************/
bool VPP_ENV_SA_GetMuteStatus(VPP_OBJ *vpp_obj, int planeID) {

    bool status;

    status = vpp_obj->vpp_sa.muteFlag?vpp_obj->vpp_sa.muteFlag:vpp_obj->vpp_sa.mute_user[planeID];

    #ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
    if(PLANE_GFX1 == planeID) {
        status = vpp_obj->vpp_sa.mute_user[planeID];
    }
    #endif //VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE

    return status;
}

/******************************************************************************
 * FUNCTION: Get the VPP related attribute from the security attribute table
 *           This has to be invoked before processing cmdq,
 *           Since mute command can be added based on HDCP_VERSION SA attribute value
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 ******************************************************************************/
void VPP_ENV_SA_GetAttributes(VPP_OBJ *vpp_obj) {
    unsigned int prev_sa_hdcp_version = vpp_obj->vpp_sa.hdcp_version;
#if HDCP2X_ENABLED
    unsigned int prev_sa_hdcp_strm_type = vpp_obj->vpp_sa.hdcp_strm_type;
#endif
    VPP_ENV_SA_CAPWIN prev_videoCappingResolution = vpp_obj->vpp_sa.videoCappingResolution;
    VPP_ENV_SA_CAPWIN prev_deviceCappingResolution = vpp_obj->vpp_sa.deviceCappingResolution;
    VPP_ENV_SA_FLAGS prev_flags = vpp_obj->vpp_sa.flags;
    bool prev_hdcpFailFlag = vpp_obj->vpp_sa.hdcpFailFlag;
    bool prev_muteFlag = vpp_obj->vpp_sa.muteFlag;
    bool prev_authReqState = vpp_obj->vpp_sa.curr_req_hdcp_version;
    unsigned int prev_sa_ur_policy = vpp_obj->vpp_sa.ur_policy;
    //VPP_HDMI_HDCP_VER_HDCP_1_x/VPP_HDMI_HDCP_VER_HDCP_2_2
    VPP_HDMI_HDCP_VERSION prev_sink_hdcp_version = vpp_obj->vpp_sa.curr_sink_hdcp_version;
    DV *pDV = &(vpp_obj->dv[0]);
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    int planeID = PLANE_INVALID;
    int  chanID;
    bool showLog = false;
    bool frame_size_changed = false;
    int i;
#if HDCP2X_ENABLED
     HDCP2X_STEAM_TYPE hdcp2xStrmType;
#endif
    //Get the latest SA values into vpp_sa.hdcp_version & vpp_sa.video_resolution_capping
    VPP_ENV_SA_update(vpp_obj);

#if HDCP2X_ENABLED
    if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_TXRX) {
        // for repeater, CSM from upstream is passed downstream
        if (prev_sa_hdcp_strm_type != vpp_obj->vpp_sa.hdcp_strm_type)
        {
            //Update content type to HDMI Tx
            hdcp2xStrmType = (vpp_obj->vpp_sa.hdcp_strm_type == VPP_ENV_SA_HDCP_CSM_TYPE_0)?HDCP2X_CSM_TYPE_0:
                         (vpp_obj->vpp_sa.hdcp_strm_type == VPP_ENV_SA_HDCP_CSM_TYPE_1)?HDCP2X_CSM_TYPE_1:
                         HDCP2X_CSM_TYPE_AUTO;
            /*TODO: check if update required on switch between hdcp modes */
            VPP_BE_UpdateContentStrmInfo(vpp_obj, hdcp2xStrmType);
        }
    }
#endif


    if (IS_VPP_ENV_SA_POLICY_NONE(vpp_obj)) {

        //Get the HDCP version of the sink
        VPP_BE_GetCurrentHdcpVersion(vpp_obj, &vpp_obj->vpp_sa.curr_sink_hdcp_version);

        if ((vpp_obj->vpp_sa.curr_sink_hdcp_version != prev_sink_hdcp_version) ||
            (prev_sa_ur_policy != vpp_obj->vpp_sa.ur_policy) ||
            (prev_authReqState != pHdmiTx->authRequired) ||
            (vpp_obj->vpp_sa.req_plane_mute_on_fail != pHdmiTx->planeControlOnHdcpFail)) {
            VPPAPI_DBG_INFO(" -- sink authentication state change... %d -> %d\n",
                                    prev_sink_hdcp_version, vpp_obj->vpp_sa.curr_sink_hdcp_version);
            VPPAPI_DBG_INFO(" -- sa_policy change.. %d -> %d\n",
                                    prev_sa_ur_policy, vpp_obj->vpp_sa.ur_policy);
            VPPAPI_DBG_INFO(" -- requested auth state.. %d -> %d\n",
                                    prev_authReqState, pHdmiTx->authRequired);
            VPPAPI_DBG_INFO(" -- planeControlOnHdcpFail change.. %d -> %d\n",
                           vpp_obj->vpp_sa.req_plane_mute_on_fail, pHdmiTx->planeControlOnHdcpFail);

            if (pHdmiTx->planeControlOnHdcpFail == true) {
                if((vpp_obj->vpp_sa.curr_sink_hdcp_version <= VPP_HDMI_HDCP_VER_NONE) &&
                    (pHdmiTx->authRequired == true)) {
                    vpp_obj->vpp_sa.hdcpFailFlag = true;
                    vpp_obj->vpp_sa.muteFlag = true;
                    vpp_obj->vpp_sa.muteByHdcpFailFlag = true;
                    #ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
                    VPP_ENV_SA_doPlaneMute(vpp_obj, true, true);
                    #else
                    VPP_ENV_SA_doPlaneMute(vpp_obj, true, false);
                    #endif
                } else {
                    vpp_obj->vpp_sa.hdcpFailFlag = false;
                    vpp_obj->vpp_sa.muteFlag = false;
                    vpp_obj->vpp_sa.muteByHdcpFailFlag = false;

                    //if fallthrough won't happen as sa_policy is disabled, handle unmute here itself
                    VPP_ENV_SA_doPlaneMute_User(vpp_obj, false);
                }
            } else {
                /* for repeater mode, content is not blocked locally as upstream should block same */
                vpp_obj->vpp_sa.hdcpFailFlag = (vpp_obj->vpp_sa.curr_sink_hdcp_version <= VPP_HDMI_HDCP_VER_NONE)? true: false;
                /* muteFlag and muteByHdcpFailFlag used for blocking unmute from app. here it is allowed */
                vpp_obj->vpp_sa.muteFlag = false;
                vpp_obj->vpp_sa.muteByHdcpFailFlag = false;
                VPP_ENV_SA_doPlaneMute_User(vpp_obj, false);
            }

            showLog = true;
            vpp_obj->vpp_sa.curr_req_hdcp_version = pHdmiTx->authRequired;
            vpp_obj->vpp_sa.req_plane_mute_on_fail = pHdmiTx->planeControlOnHdcpFail;
        }
    }

#ifdef VPP_ENV_SA_HANDLE_POLICY_NONE
    //If policy not enabled for this platform, then return immediately
    if(IS_VPP_ENV_SA_POLICY_NONE(vpp_obj)) {
        return;
    }
#endif //VPP_ENV_SA_HANDLE_POLICY_NONE

    VPP_BE_GetCurrentHdcpVersion(vpp_obj, &vpp_obj->vpp_sa.curr_sink_hdcp_version);
    //Initially assume no scaling changes required because of device resoluton capping
    vpp_obj->vpp_sa.device_resolution_capping_updated = FALSE;
    //Check if the device resolution capping itself is changed dynamically, then force scaling change
    if(!IS_VPP_ENV_SA_CAPWIN_EQUAL(prev_deviceCappingResolution, vpp_obj->vpp_sa.deviceCappingResolution)) {
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "vpp_sa.deviceCappingResolution changed : %dx%d->%dx%d\n",
            prev_deviceCappingResolution.width, prev_deviceCappingResolution.height, vpp_obj->vpp_sa.deviceCappingResolution.width, vpp_obj->vpp_sa.deviceCappingResolution.height);
        vpp_obj->vpp_sa.device_resolution_capping_updated = true;
    }

    //Initially assume no scaling changes required because of hdcp version or video resoluton capping (device or video)
    vpp_obj->vpp_sa.video_resolution_capping_updated = FALSE;
    //Check if the video resolution capping itself is changed dynamically, then force scaling change
    if(!IS_VPP_ENV_SA_CAPWIN_EQUAL(prev_videoCappingResolution, vpp_obj->vpp_sa.videoCappingResolution)) {
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "vpp_sa.videoCappingResolution changed : %dx%d->%dx%d\n",
            prev_videoCappingResolution.width, prev_videoCappingResolution.height, vpp_obj->vpp_sa.videoCappingResolution.width, vpp_obj->vpp_sa.videoCappingResolution.height);
        vpp_obj->vpp_sa.video_resolution_capping_updated = true;
    }

    //Initially assume no scaling changes required because of flags
    vpp_obj->vpp_sa.flags_updated = FALSE;
    //Check if the video resolution capping itself is changed dynamically, then force scaling change
    if(IS_VPP_ENV_SA_FLAGS_CHANGED(prev_flags, vpp_obj->vpp_sa.flags)) {
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "vpp_sa.flags changed : %x->%x\n", prev_flags, vpp_obj->vpp_sa.flags.flags);
        vpp_obj->vpp_sa.flags_updated = true;
    }

    //Initially assume no scaling changes required because of framesize
    for (i=0; i<pDV->num_of_chans; i++) {
        chanID = pDV->chanID[i];
        vpp_obj->vpp_sa.frame_size_updated[i] = false;

        if (vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
            PLANE *plane;
            VDEC_VID_BUF_DESC *inline_frame;
            VBUF_INFO *pinfo;
            VPP_ENV_SA_CAPWIN curr_frame_size;
            planeID = PLANE_OF_CHAN(vpp_obj, chanID);
            #ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
                //Do not control scale/mute; allow application to take care of GFX plane
                if(PLANE_GFX1 == planeID) {
                    continue;
                }
            #endif
#ifdef ENABLE_PIP_GFX0_PATH
            if(PLANE_PIP == planeID)
                continue;
#endif
            plane = &vpp_obj->plane[planeID];
            if (plane->status == STATUS_DISP_VIDEO)
                inline_frame = plane->curr_frame;
            else if (plane->status == STATUS_DISP_LOGO)
                inline_frame = plane->curr_still_picture;
            else if (plane->status == STATUS_DISP_PATGEN)
                inline_frame = plane->dummy_frame;
            else
                inline_frame = NULL;

            if(NULL == inline_frame) {
                VPP_ENV_SA_CAPWIN_SET(curr_frame_size, 0, 0);
            } else {
            #if !defined(VPP_IN_TRUST_ZONE)
                pinfo = (VBUF_INFO *)inline_frame->user_data;
            #else
                pinfo = (VBUF_INFO *)inline_frame;
            #endif
                VPP_ENV_SA_CAPWIN_SET(curr_frame_size, pinfo->m_active_width, pinfo->m_active_height);
                if(!IS_VPP_ENV_SA_CAPWIN_EQUAL(curr_frame_size, vpp_obj->vpp_sa.frame_size[i])) {
                    VPP_ENV_SA_CAPWIN_SET(vpp_obj->vpp_sa.frame_size[i], pinfo->m_active_width, pinfo->m_active_height);
                    vpp_obj->vpp_sa.frame_size_updated[i] = true;
                    frame_size_changed = true;  //Enable global flag for all plane
                }
            }
        }
    }

    //Check if HDCP_VERSION SA changed or the HDCP version of sink changed because of hot plug
    if( (prev_sa_hdcp_version != vpp_obj->vpp_sa.hdcp_version) ||
        (prev_sink_hdcp_version != vpp_obj->vpp_sa.curr_sink_hdcp_version) ||
        (prev_sa_ur_policy != vpp_obj->vpp_sa.ur_policy)) {
        vpp_obj->vpp_sa.hdcpFailFlag = false;
        VPPAPI_DBG_INFO("checking for hdcp mute %d:%d:%d:%d:%d:%d\n", prev_sa_hdcp_version, vpp_obj->vpp_sa.hdcp_version, prev_sink_hdcp_version,
                         vpp_obj->vpp_sa.curr_sink_hdcp_version, prev_sa_ur_policy, vpp_obj->vpp_sa.ur_policy);
        showLog = true;
        //Check if hdcpFailFlag need to be changed, based on sink hdcp version and sa entry
        switch(vpp_obj->vpp_sa.hdcp_version) {
        case VPP_ENV_SA_HDCP_VERSION_NONE:
            //no HDCP encryption is required on any digital video output like HDMI
        break;
        case VPP_ENV_SA_HDCP_VERSION_1_0:
            //version 1.0 is at least required
            if(vpp_obj->vpp_sa.curr_sink_hdcp_version < VPP_HDMI_HDCP_VER_HDCP_1_x) {
                vpp_obj->vpp_sa.hdcpFailFlag = true;
            }
        break;
        case VPP_ENV_SA_HDCP_VERSION_2_0:
            //version 2.0 is at least required
        case VPP_ENV_SA_HDCP_VERSION_2_1:
            //version 2.1 is at least required
        case VPP_ENV_SA_HDCP_VERSION_2_2:
            //version 2.2 is at least required
        default:
            //Any other upper value should be considered as version 2.2 minimum
            if(vpp_obj->vpp_sa.curr_sink_hdcp_version < VPP_HDMI_HDCP_VER_HDCP_2_2) {
                vpp_obj->vpp_sa.hdcpFailFlag = true;
            }
        break;
        }
    }

    //Check if hdcp fail flag or video_resolution_capping or device_resolution_capping or flags or frame size is changed, then take necessary action
    if( (prev_hdcpFailFlag != vpp_obj->vpp_sa.hdcpFailFlag) ||
        (vpp_obj->vpp_sa.video_resolution_capping_updated) ||
        (vpp_obj->vpp_sa.device_resolution_capping_updated) ||
        (vpp_obj->vpp_sa.flags_updated) ||
        (frame_size_changed) ) {
        //VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "  entering the business of mute \n");
        //First check for device capping
        if(IS_VPP_ENV_SA_DEV_CAP_FLAGS_CHANGED(vpp_obj->vpp_sa.flags, prev_flags) || vpp_obj->vpp_sa.device_resolution_capping_updated || frame_size_changed) {
            for (i=0; i<pDV->num_of_chans; i++) {
                chanID = pDV->chanID[i];
                if (vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
                    #ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
                        //Do not control scale/mute; allow application to take care of GFX plane
                        if(PLANE_GFX1 == chanID) {
                            continue;
                        }
                    #endif
#ifdef ENABLE_PIP_GFX0_PATH //TODO: Disable in the beginning of loop using static table if(vpp_plane_map[PlaneId].is_valid)
                    if(PLANE_PIP == chanID)
                        continue;
#endif
                    //Check Normal condition fails (frame_size > deviceCappingResolution)
                    if(IS_VPP_ENV_SA_CAPWIN_GREATER(vpp_obj->vpp_sa.frame_size[i], vpp_obj->vpp_sa.deviceCappingResolution)) {
                        vpp_obj->vpp_sa.devCapFailFlag = true;
                    }
                    else
                    {
                        vpp_obj->vpp_sa.devCapFailFlag = false;
                    }
                    //If normal fails & roation check enabled, Check Rotation condition passes (ROTATE(frame_size)) > deviceCappingResolution)
                    if(vpp_obj->vpp_sa.devCapFailFlag && VPP_ENV_SA_DEV_CAP_ROTATION_ALLOWED_FLAG(vpp_obj->vpp_sa.flags) && IS_VPP_ENV_SA_CAPWIN_ROTATE_LESS(vpp_obj->vpp_sa.frame_size[i],vpp_obj->vpp_sa.deviceCappingResolution) ) {
                        vpp_obj->vpp_sa.devCapFailFlag = false;
                    }
                    //if normal fails & aspect check enabled, Check Aspect condition passes (ASPECT(frame_size)) > (ASPECT(deviceCappingResolution))
                    if(vpp_obj->vpp_sa.devCapFailFlag && VPP_ENV_SA_DEV_CAP_ASPECT_ALLOWED_FLAG(vpp_obj->vpp_sa.flags) && IS_VPP_ENV_SA_CAPWIN_ASPECT_LESS(vpp_obj->vpp_sa.frame_size[i],vpp_obj->vpp_sa.deviceCappingResolution) ) {
                        vpp_obj->vpp_sa.devCapFailFlag = false;
                    }
                    if(true == vpp_obj->vpp_sa.devCapFailFlag) {
                        break;      //If one video plane fails the check, then enough
                    }
                }
            }
        }

        //Check HDCP & video_resolution_capping only if device capping test is passed (devCap test should not be failed).
        if(vpp_obj->vpp_sa.devCapFailFlag) {
            //Device capping failed, So Mute, Scaling is not taken care for now
            vpp_obj->vpp_sa.muteFlag = true;
        } else if(vpp_obj->vpp_sa.hdcpFailFlag == true) {
            //VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX " vpp_obj->vpp_sa.hdcpFailFlag \n");
            //HDCP failed, check if we can do downscaling or mute based on video_resolution_capping
            for (i=0; i<pDV->num_of_chans; i++) {
                chanID = pDV->chanID[i];
                if (vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
                    #ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
                        //Do not control scale/mute; allow application to take care of GFX plane
                        if(PLANE_GFX1 == chanID) {
                            continue;
                        }
                    #endif
#ifdef ENABLE_PIP_GFX0_PATH
                    if(PLANE_PIP == chanID)
                        continue;
#endif
                    //Check Normal condition fails (frame_size > videoCappingResolution)
                    if(IS_VPP_ENV_SA_CAPWIN_GREATER(vpp_obj->vpp_sa.frame_size[i],vpp_obj->vpp_sa.videoCappingResolution)) {
                        vpp_obj->vpp_sa.vidCapFailFlag = true;
                    }
                    else
                    {
                        vpp_obj->vpp_sa.vidCapFailFlag = false;
                    }
                    //If normal fails & roation check enabled, Check Rotation condition passes (ROTATE(frame_size)) > deviceCappingResolution)
                    if(vpp_obj->vpp_sa.vidCapFailFlag && VPP_ENV_SA_RES_CAP_ROTATION_ALLOWED_FLAG(vpp_obj->vpp_sa.flags) && IS_VPP_ENV_SA_CAPWIN_ROTATE_LESS(vpp_obj->vpp_sa.frame_size[i], vpp_obj->vpp_sa.videoCappingResolution) ) {
                        vpp_obj->vpp_sa.vidCapFailFlag = false;
                    }
                    //if normal fails & aspect check enabled, Check Aspect condition passes (ASPECT(frame_size)) > (ASPECT(deviceCappingResolution))
                    if(vpp_obj->vpp_sa.vidCapFailFlag && VPP_ENV_SA_RES_CAP_ASPECT_ALLOWED_FLAG(vpp_obj->vpp_sa.flags) && IS_VPP_ENV_SA_CAPWIN_ASPECT_LESS(vpp_obj->vpp_sa.frame_size[i], vpp_obj->vpp_sa.videoCappingResolution) ) {
                        vpp_obj->vpp_sa.vidCapFailFlag = false;
                    }

                    if(IS_VPP_ENV_SA_POLICY_MUTE(vpp_obj)) {
                        if(vpp_obj->vpp_sa.vidCapFailFlag) {
                            //Force muting instead of downscaling or due to invalid capping options
                            vpp_obj->vpp_sa.muteFlag = true;
                        }
                    } else {
                        //downscaling not implemented
                    }
                }
            }
        } else {
            //HDCP success, then unmute or revert any downscaling forced
            vpp_obj->vpp_sa.muteFlag = false;
        }

        if(prev_muteFlag != vpp_obj->vpp_sa.muteFlag) {
            /* Notify SMP pass/fail to user space through HDMI event */
            HdmiTxNotifyEvent(vpp_obj, pHdmiTx->eventCode, vpp_obj->vpp_sa.muteFlag == true? MV_VPP_EVENT_HDMI_SMP_FAIL:MV_VPP_EVENT_HDMI_SMP_PASS);

            //Apply muting only when dev res cap is failed, or MUTE_ONLY policy enabled
            if((vpp_obj->vpp_sa.muteFlag == true)) {
               if((IS_VPP_ENV_SA_POLICY_MUTE(vpp_obj)) || vpp_obj->vpp_sa.devCapFailFlag) {
                    #ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
                        VPP_ENV_SA_doPlaneMute(vpp_obj, true, true);
                    #else
                        VPP_ENV_SA_doPlaneMute(vpp_obj, true, false);
                    #endif
                } else {
                    //No need to mute but change resolution/scaling, so reset muteFlag
                    vpp_obj->vpp_sa.muteFlag = false;
                }
            } else if (vpp_obj->vpp_sa.muteFlag == false) {
                //If current action is unmute, then restore backup mute status of video plane
                VPP_ENV_SA_doPlaneMute(vpp_obj, vpp_obj->vpp_sa.mute_user[planeID], false);
            }
        }

        //Force scaling changes during hdcp fail flag or devCap fail flag change
        vpp_obj->vpp_sa.video_resolution_capping_updated = true;
    }

    if(showLog) {
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "Current policy: %x, devCapFailFlag:%d, vidCapFailFlag:%d, video_res:%dx%d, device_res:%dx%d\n",
                vpp_obj->vpp_sa.ur_policy, vpp_obj->vpp_sa.devCapFailFlag, vpp_obj->vpp_sa.vidCapFailFlag,
                vpp_obj->vpp_sa.videoCappingResolution.width, vpp_obj->vpp_sa.videoCappingResolution.height,
                vpp_obj->vpp_sa.deviceCappingResolution.width, vpp_obj->vpp_sa.deviceCappingResolution.height);
        VPP_ENV_DEBUG_PRINT_DYNAMIC(VPP_ENV_PRINT_PREFIX "hdcp_version changed:%d->%d, sink_hdcp_version:%d->%d, hdcpFailFlag:%d->%d, muteFlag:%d->%d, ApplyResCap:%d\n",
            prev_sa_hdcp_version, vpp_obj->vpp_sa.hdcp_version,  prev_sink_hdcp_version, vpp_obj->vpp_sa.curr_sink_hdcp_version,
            prev_hdcpFailFlag, vpp_obj->vpp_sa.hdcpFailFlag, prev_muteFlag, vpp_obj->vpp_sa.muteFlag,
            vpp_obj->vpp_sa.video_resolution_capping_updated);
    }

    /*If video_resolution_capping_updated,
     * then we can inform/callback application to set suitable resolution based on vpp_sa.video_resolution_capping */
    if(IS_VPP_ENV_SA_POLICY_RESOLUTION(vpp_obj) && vpp_obj->vpp_sa.video_resolution_capping_updated) {
    }
}

/******************************************************************************
 * FUNCTION: Register the callback function for resolution chage because of res capping due to hdcp failure
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        callbackFunc function pointer
 ******************************************************************************/
void VPP_ENV_SA_RegisterCallback(VPP_OBJ *vpp_obj, void (*callbackFunc) (UINT32 resId)) {
    vpp_obj->vpp_sa.VPP_ENV_SA_callbackFunc = callbackFunc;
}

/******************************************************************************
 * FUNCTION: Get the resolution cap based on video resolution capping security attribute
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 * OUTPUT: Resolution ID corresponding to the video resolution capping security attribute
 ******************************************************************************/
unsigned int VPP_ENV_SA_GetResCapResolution(VPP_OBJ *vpp_obj, unsigned int user_resID) {
    UINT32 resID;

    resID = user_resID;

    //Return the current video caping resolution
    return resID;
}


/******************************************************************************
 * FUNCTION: update the dispwin for various video plane based on video resolution capping security attribute
 *           Please refer to various caping option in enum - VPP_ENV_SA_VID_RES_CAP
 *           This need to be triggered for each plane at the begining of startChannelLoader
 * INPUT: vpp_obj, VPP object containing the variables to be populated with respective property value
 *        planeID, Plane whose dispwin need to be altered
 ******************************************************************************/
void VPP_ENV_SA_ProcessResolutionCap(VPP_OBJ *vpp_obj, int planeID) {
#ifdef VPP_ENV_SA_ENABLE_SCALING
    RESOLUTION_INFO *resinfo;
    bool user_changed_dispwin=false;
    bool isScalingRequired=false;
    CHAN *chan = &(vpp_obj->chan[planeID]);
    RESOLUTION_INFO  *pResinfo;

    VPP_WIN newDispWin;
#endif
#ifdef VPP_ENV_SA_HANDLE_POLICY_NONE
    //If policy not enabled for this platform, then return immediately
    if(IS_VPP_ENV_SA_POLICY_NONE(vpp_obj)) {
        return;
    }
#endif //VPP_ENV_SA_HANDLE_POLICY_NONE

#ifdef VPP_ENV_SA_ENABLE_SCALING
    resinfo = VPP_GetResInfo(VPP_GetResId(CPCB_1));
#endif

    //If callback give for resolution change or mute applied, Then no need to apply scaling
    if(!IS_VPP_ENV_SA_POLICY_SCALING(vpp_obj)) {
        return;
    }

#ifndef VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE
    //Do not control scale/mute; allow application to take care of GFX plane
    if(PLANE_GFX1 == planeID) {
        return;
    }
#endif //VPP_ENV_SA_APPLY_UR_TO_GFX_PLANE

}

#endif //CONFIG_VPP_ENV_SA_SUPPORT
