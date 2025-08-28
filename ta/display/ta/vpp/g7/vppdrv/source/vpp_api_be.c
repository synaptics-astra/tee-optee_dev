/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "Galois_memmap.h"
#include "galois_io.h"
#include "vpp_module.h"
#include "vpp_apifuncs.h"
#include "vpp_api_types.h"
#include "vpp_be.h"
#include "vpp_be_hdr.h"
#include "vpp_be_hdmitx.h"
#include "vpp_hdcp_keyload.h"
#include "tee_ta_vpp_be.h"
#include "vpp_dv_api.h"
#include "SNPS_HDMI_tx.h"

#define IS_CLRFMT_420(fmt)      \
        ((fmt == OUTPUT_COLOR_FMT_YCBCR420) ||     \
        (fmt == OUTPUT_COLOR_FMT_YCBCR420_P3D65) ||   \
        (fmt == OUTPUT_COLOR_FMT_YCBCR420_R2020))

#define HDMI_COLORFMT420_CHANGE(hdmiTx, clrfmt)     \
        ((hdmiTx != NULL) &&        \
        (IS_CLRFMT_420(((BE_HDMITX_DATA*)hdmiTx)->videoParams.VideoFmt) || IS_CLRFMT_420(clrfmt)) &&      \
        (((BE_HDMITX_DATA*)hdmiTx)->videoParams.VideoFmt != clrfmt))


/********************************************************************************
 * FUNCTION: set the source of video output
 * INPUT: vout - id of the video output
 *            cpcbID - id of the CPCB
 *            interlaced - OUTPUT_PROGRESSIVE or OUTPUT_INTERLACED
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutSource(int handle, int vout, int cpcbID, int interlaced)
{
    CMD cmd;
    int params[3];
    VPP_OBJ *vpp_obj;
    int i, old_cpcbID;

 //   VPPAPI_DBG_INFO("vout [%d], cpcbID [%d] interlaced [%d]\n",vout, cpcbID, interlaced);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (interlaced < OUTPUT_PROGRESSIVE || interlaced > OUTPUT_AUTO_SELECT)
        return (MV_VPP_EBADPARAM);

    if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (cpcbID<FIRST_CPCB || cpcbID>=MAX_NUM_CPCBS)
        return (MV_VPP_EBADPARAM);


    old_cpcbID = vpp_obj->vout[vout].dvID;
    if(old_cpcbID == CPCB_INVALID) {
        /*vout has not been connected to a CPCB*/
        vpp_obj->vout[vout].dvID = cpcbID;
        vpp_obj->dv[cpcbID].voutID[vpp_obj->dv[cpcbID].num_of_vouts] = vout;
        vpp_obj->dv[cpcbID].num_of_vouts++;
    }else if(old_cpcbID != cpcbID){
        /*vout has been connected to a different CPCB*/
        for (i=0; i<vpp_obj->dv[old_cpcbID].num_of_vouts; i++) {
            if (vpp_obj->dv[old_cpcbID].voutID[i] == vout) {
                vpp_obj->dv[old_cpcbID].voutID[i] = vpp_obj->dv[old_cpcbID].voutID[vpp_obj->dv[old_cpcbID].num_of_vouts-1];
                vpp_obj->dv[old_cpcbID].voutID[vpp_obj->dv[old_cpcbID].num_of_vouts-1] = VOUT_INVALID;
                vpp_obj->dv[old_cpcbID].num_of_vouts--;
                break;
            }
        }

        vpp_obj->vout[vout].dvID = cpcbID;
        vpp_obj->dv[cpcbID].voutID[vpp_obj->dv[cpcbID].num_of_vouts] = vout;
        vpp_obj->dv[cpcbID].num_of_vouts++;
    }

    if (vpp_obj->dv[cpcbID].status == STATUS_ACTIVE){
        i = cpcbID;
    } else if ((old_cpcbID != CPCB_INVALID) && (vpp_obj->dv[old_cpcbID].status == STATUS_ACTIVE)) {
        i = old_cpcbID;
    } else {
        /* The CPCB is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    /* set vout source*/
    params[0] = vout;
    params[1] = cpcbID;
    params[2] = interlaced;

    make_cmd(&cmd, (FUNCPTR)VPP_SetVoutSource, 3, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[i].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[i].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[i].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[i].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: This function is extension of MV_VPPOBJ_SetVoutMute.
 *           Compared with MV_VPPOBJ_SetVoutMute, this function will mute
 *           audio/video or both
 *           Notice, this function is only effect on HDMI port
 * INPUT: vout - id of the video output
 *        mute - 1:mute, 0:un-mute
 *        bVideo - 1:mute or un-mute video, 0:do nothing on video
 *        bAudio - 1:mute or un-mute audio, 0:do nothing on audio
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutMute_Ext(int handle, int vout, int mute, int bVideo, int bAudio)
{
    int cpcbID;
    int port;
    int ret;
    VPP_OBJ *vpp_obj;
    VPP_HDMI_HDCP_VERSION HDCPVersion;

    VPPAPI_DBG_INFO(" vout ext [%d], mute [%d] \n", vout, mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, vout)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if(vout != VOUT_HDMI) {
        /*Do nothing except HDMI*/
        return (MV_VPP_OK);
    }

    if ((FALSE == bVideo) && (FALSE == bAudio)) {
        return (MV_VPP_OK);
    } else if ((TRUE == bVideo) && (TRUE == bAudio)) {
        port = VPP_HDMI_AV_PORT_GCP;
    } else if (FALSE == bAudio) {
        if (mute == FALSE) {
            if (((BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx))->authRequired == false) {
                /*authentication not required. fwd request to unmute..*/
                VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, mute);
                return MV_VPP_OK;
            }
            ret = VPP_BE_GetCurrentHdcpVersion(vpp_obj, &HDCPVersion);
            if ((ret == MV_VPP_OK) &&
                ((HDCPVersion == VPP_HDMI_HDCP_VER_HDCP_2_3) || (HDCPVersion == VPP_HDMI_HDCP_VER_HDCP_1_x))) {
                    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, mute);
            } else {
                VPPHDMI_DBG_INFO("unmute HDMI handled internally after authentication\n");
            }
            return MV_VPP_OK;
        }
        port = VPP_HDMI_AV_PORT_VIDEO;
    } else {
        port = VPP_HDMI_AV_PORT_AUDIO;
    }
    return VPP_BE_SetHdmiMute (vpp_obj, port, mute);
}

/********************************************************************************
 * FUNCTION: mute/un-mute a video output
 * INPUT: vout - id of the video output
 *            mute - 1:mute, 0:un-mute
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutMute(int handle, int vout, int mute)
{
    int cpcbID;
    int ret;
    VPP_OBJ *vpp_obj;
    VPP_HDMI_HDCP_VERSION HDCPVersion;

    VPPAPI_DBG_INFO(" vout [%d], mute [%d] \n", vout, mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
        return (MV_VPP_EBADPARAM);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, vout)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if(mute == true){
        /*HDMI mute*/
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, mute);
    } else {
        if (((BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx))->authRequired == false) {
            /*authentication not required. fwd request to unmute..*/
            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, mute);
            return MV_VPP_OK;
        }

        ret = VPP_BE_GetCurrentHdcpVersion(vpp_obj, &HDCPVersion);
        if ((ret == MV_VPP_OK) &&
            ((HDCPVersion == VPP_HDMI_HDCP_VER_HDCP_2_3) || (HDCPVersion == VPP_HDMI_HDCP_VER_HDCP_1_x))) {
            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, mute);
        } else {
            VPPHDMI_DBG_INFO("unmute HDMI handled internally, after auth\n");
        }
    }

    return (MV_VPP_OK);
}


/********************************************************************************
 * FUNCTION: enable/disable HDMI output
 * INPUT: Enable - 1:enable, 0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDMIEnable(int handle, int vout, int Enable)
{
        int cpcbID;
        VPP_OBJ *vpp_obj;


        VPPAPI_DBG_INFO(" vout [%d], mute [%d] \n",  vout, Enable);

        if (g_vpp->magic != VPP_MAGIC){
            return (MV_VPP_EUNCONFIG);
        }

        if (handle<0 || handle>=MAX_NUM_OBJECTS)
            return (MV_VPP_EBADPARAM);

        vpp_obj = g_vpp->vpp_objs[handle];

        if (!vpp_obj)
            return (MV_VPP_ENODEV);

        if (vout<FIRST_VOUT || vout>=MAX_NUM_VOUTS)
            return (MV_VPP_EBADPARAM);

        if (vpp_obj->status == STATUS_INACTIVE){
            /* VPP module is not configured */
            return (MV_VPP_EUNCONFIG);
        }

        if ((cpcbID = CPCB_OF_VOUT(vpp_obj, vout)) == CPCB_INVALID) {
              /* Output is not connected to any DV in configuration */
            return (MV_VPP_EUNSUPPORT);
        }


        if (!Enable)
        {
            /*HDMI mute*/
            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, 1);
        }

        MV_VPPOBJ_SetHdmiPhyPower(handle, Enable);
        /* on enable, force hotplug called which will unmute out */
        return (MV_VPP_OK);

}

int MV_VPPOBJ_SetHdmiPhyPower(int handle, int enable)
{
    /*  This API to be called from display service only.
        TODO: change enable to BOOL from int */

    VPP_OBJ *vpp_obj;
    int cpcbID;
    int retVal;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (enable == VPP_BE_GetHdmiPhyExpectedState(vpp_obj)) {
        VPPHDMI_DBG_INFO("HDMI Tx phy already in expected state %d\n", enable);
        return (MV_VPP_OK);
    }

    if(!(vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_HDMITX))
    {
        VPPAPI_DBG_INFO("Skip 5V, VPP_BE_SetHdmiPhyPower\n");
        return (MV_VPP_OK);
    }

    /* Enable HDMI 5V before powering up Phy */
    if(enable == TRUE) {
        VPP_SetHdmi5vState(enable);
        ((BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx))->resumeInProgress = 1;
    }

    /* Configure hdmi for PHY state change */
    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiPhyStateChange(vpp_obj, enable)))
        return retVal;

    /* Start PHY state change */
    if (MV_VPP_OK != (retVal = VPP_BE_SetHdmiPhyPower(vpp_obj, enable)))
        return retVal;

    /* Disable HDMI 5V after phy power down */
    if(enable == FALSE)
        VPP_SetHdmi5vState(enable);

    return (MV_VPP_OK);
}

int MV_VPPOBJ_EnableHdmiAudioFmt(int handle, int enable)
{
    int cpcbID;
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    return VPP_BE_EnableHdmiAudioFmt (vpp_obj, enable);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Audio format
 * INPUT: sampFreq - Audio sampling frequency
 *      : sampSize - Word length
 *      : mClkFactor - 128/256/384/512 times frequency
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioFmt(int handle, int enable, VPP_HDMI_AUDIO_CFG *pAudioCfg)
{
    CMD cmd;
    int cpcbID;
    int params[2];
    int     retVal;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" enable [%d], # of channels [%d], port # [%d], sample freq [%d], sample size [%d], mclk [%d], audioFmt [%d], HBR [%d] \n", enable, pAudioCfg->numChannels, pAudioCfg->portNum, pAudioCfg->sampFreq, pAudioCfg->sampSize, pAudioCfg->mClkFactor, pAudioCfg->audioFmt, pAudioCfg->hbrAudio);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiAudioFmt (vpp_obj,enable,pAudioCfg)))
    {
        return retVal;
    }

    params[0] = VPP_HDMI_SET_AUDIO_FMT;
    params[1] = enable;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}
/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiVideoFmt(int handle, int color_fmt, int bit_depth, int pixel_rept)
{
    CMD     cmd;
    int     cpcbID;
    int     resID;
    int     params[1];
    VPP_OBJ *vpp_obj;
    int     retVal;

    VPPAPI_DBG_INFO(" color_fmt [%d], bit_depth [%d], pixel_rept [%d] \n", color_fmt, bit_depth, pixel_rept);
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    /* Configure AVPLL */
    resID = VPP_GetResId(cpcbID);

    if (resID == RES_INVALID)
        return (MV_VPP_EBADPARAM);

    /* Config video format */
    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiVideoFmt (vpp_obj, color_fmt, bit_depth, pixel_rept)))
        return retVal;

    /*allow color format, bitdepth update. avoid PHY configuration for Panel res*/
    if(IS_DSI_PANEL_RES(resID))
        return (MV_VPP_OK);

    params[0] = VPP_HDMI_SET_VIDEO_FMT;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);
    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT:  MUX VALUE for Ch3Ch2Ch1Ch0 [Input value of Hex where each Nibble corresponds to each channel]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiTmdsMuxSel(int handle, UINT32 mux_value)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: Set Hdmi 3D structure
 * INPUT: structure - structure ID
 *      : ext_data - extra data for structure 8 or above
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmi3DStructure(int handle, int structure, int ext_data)
{
    CMD     cmd;
    int     cpcbID;
    int     params[4];
    int     retCode;
    VPP_OBJ *vpp_obj;
    VPP_HDMI_PKT auxPkt;
    RESOLUTION_INFO  *pResinfo;

//    VPPAPI_DBG_INFO(" structure [%d], ext_data [%d] \n", structure, ext_data);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
        /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }
    pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));

    if (!pResinfo->flag_3d && (VPP_GetResId(cpcbID)<FIRST_RES_3D)){
        return (MV_VPP_EBADCALL);
    }

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = VPP_HDMI_PKT_ID_VENDOR_INFOFRAME;
    params[2] = 1; /* repeat */

    auxPkt.vndrSpecInfoFrm.ieeeRegID = 0x000c03;
    if (((BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx))->edidInfo.support_3D) {
        auxPkt.vndrSpecInfoFrm.HdmiVideoFmt = VPP_HDMI_VIDEO_FMT_3D_FMT;
    }
    else {
        auxPkt.vndrSpecInfoFrm.HdmiVideoFmt = VPP_HDMI_VIDEO_FMT_NO_INFO;
    }
    auxPkt.vndrSpecInfoFrm.Hdmi_3D_Structure = structure;
    auxPkt.vndrSpecInfoFrm.Hdmi_3D_Meta_Present = 0;
    auxPkt.vndrSpecInfoFrm.Hdmi_3D_Ext_Data = ext_data;
    if (MV_VPP_OK != (retCode = VPP_BE_HdmiFormatAuxPkt (vpp_obj, TRUE, VPP_HDMI_PKT_ID_VENDOR_INFOFRAME, &auxPkt)))
        return retCode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: set Hdmi audio VUC bits
 * INPUT: pVUCCfg        - pointer to VUC config structure
 *        loadOnAudioCfg - 1 for loading these data when audio
 *                       - is configured subsequently
 *                       - 0 for loading it on the next VBI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioVUCBits(int handle, VPP_HDMI_AUDIO_VUC_CFG *pVUCCfg, int loadOnAudioCfg)
{
    CMD     cmd;
    int     cpcbID;
    int     params[2];
    int     retVal;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" loadOnAudioCfg [%d] \n",  loadOnAudioCfg);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = VPP_HDMI_SET_AUDIO_VUC_BITS;
    if (MV_VPP_OK != (retVal = VPP_BE_ConfigHdmiAudioVUCBits
                               (vpp_obj, pVUCCfg, loadOnAudioCfg, &params[1])))
    {
        return retVal;
    }
    if (params[1])
    {
        make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);
        MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);

        ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

        /* commit commands */
        cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    }
    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetHdmiAudioFifoReset(int handle, int bSet)
{
    /* function depricated */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: mute/un-mute hdmi audio output
 * INPUT: mute - 1:mute, 0:un-mute
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioMute(int handle, int mute)
{
    int cpcbID;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" mute [%d] \n",  mute);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*HDMI audio mute*/
    return VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_AUDIO, mute);
}

/********************************************************************************
 * FUNCTION: Set Hdmi output mode (Hdmi/Dvi)
 * INPUT: hdmiMode - 0:DVI, 1:HDMI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiMode(int handle, int hdmiMode)
{
    int cpcbID;
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO(" HdmiMode [%d] \n", hdmiMode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*HDMI Mode*/
    return VPP_BE_SetHdmiMode (vpp_obj, hdmiMode);
}

/********************************************************************************
 * FUNCTION: Set HDMI Video Information
 * INPUT: aspect ratio
 *      : scan info
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiVideoInfo(int handle, int aspRatio, int scanInfo)
{
    CMD cmd;
    int cpcbID;
    int params[4];
    int retVal;
    VPP_OBJ *vpp_obj;

 //   VPPAPI_DBG_INFO("aspRatio [%d], scanInfo [%d] \n",  aspRatio, scanInfo);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*Video info*/
    if (MV_VPP_OK != (retVal = VPP_BE_SetHdmiVideoInfo (vpp_obj, aspRatio, scanInfo)))
        return retVal;

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = VPP_HDMI_PKT_ID_AVI_INFOFRAME;
    params[2] = 0;
    params[3] = TRUE; // repeat on
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set HDMI Audio Information
 * INPUT: Speaker Map
 *      : Level Shift to denote attenuation (0 - 15)dB
 *      : Down mix inhibit flag
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioInfo(int handle, VPP_HDMI_SPKR_ALLOC *pSpkrMap,
                               int lvlShiftVal, int downMixInhFlag)
{
    CMD cmd;
    int cpcbID;
    int params[4];
    int retVal;
    VPP_OBJ *vpp_obj;

 //   VPPAPI_DBG_INFO("SpeakerMap [0x%x], lvlShiftVal [%d], MixInh [%d]\n",  (int)(*(char *)pSpkrMap), lvlShiftVal, downMixInhFlag);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    /*Audio info*/
    if (MV_VPP_OK != (retVal = VPP_BE_SetHdmiAudioInfo (vpp_obj, *(unsigned char*)(pSpkrMap), lvlShiftVal, downMixInhFlag)))
        return retVal;

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = VPP_HDMI_PKT_ID_AUDIO_INFOFRAME;
    params[2] = 1;
    params[3] = TRUE; // repeat on
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);
    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set SRM Data
 * INPUT: pSrmData - Pointer to SRM data and length (Input)
 *      : pDevSignData - Pointer to device signature to check content
 *      :              - validity (Input)
 *      : pVernNum - Pointer to return SRM info parsed and updated (output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetSRM (int handle, VPP_HDCP_SRM_DATA *pSrmData,
                      VPP_DEV_SIGN_DATA *pDevSignData, VPP_HDCP_SRM_INFO *pSrmInfo)
{
    VPP_OBJ *vpp_obj;
    int     retCode = MV_VPP_OK;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (pSrmData == NULL || pDevSignData == NULL || pSrmInfo == NULL)
        return (MV_VPP_EBADPARAM);

    retCode = VPP_BE_ConfigureSRM (vpp_obj, pSrmData, pDevSignData, pSrmInfo);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Get SRM Version
 * INPUT: pHdcp22SrmVersion - HDCP 2.2 SRM File version (Output)
 *      : pHdcp22SrmVersion - HDCP 1.4 SRM File version (Output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetSRMVersion(int handle, UINT16 *pHdcp22SrmVersion, UINT16 *pHdcp14SrmVersion)
{
    VPP_OBJ *vpp_obj;
    int     retCode = MV_VPP_OK;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (pHdcp22SrmVersion == NULL || pHdcp14SrmVersion == NULL)
        return (MV_VPP_EBADPARAM);

    retCode = VPP_BE_GetSRMVersion (vpp_obj, pHdcp22SrmVersion, pHdcp14SrmVersion);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Enable/Disable HDCP SRM check. This API controls if revoked list
 *         : check would be done
 * INPUT   : enable - Enable/Disable HDCP SRM Check
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_EnableSRMCheck(int handle, int enable)
{
    VPP_OBJ *vpp_obj;
    int     retCode = MV_VPP_OK;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    retCode = VPP_BE_EnableSRMCheck (vpp_obj, enable);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Before loading HDCP Keys to the registers, check if the VPP status is ready
 * INPUT: handle    - VPP object handle
 *      : pHdcpKeys - Pointer to HDCP keys to be loaded
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - HDCP is enabled currently(Keys cannot be loaded
 *                         - with HDCP on)
 ********************************************************************************/
int MV_VPPOBJ_LoadHDCPKeys_PreCheck(int handle, UINT8 *pHdcpKeys)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: Enable/Disable HDCP
 * INPUT: Enable - Control to turn HDCP on/off
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_EnableHDCP(int handle, int enable)
{
    /* should be called only from displayservice*/
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO(" enable [%d] \n",  enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_EnableHdcp(vpp_obj, enable);
}
/********************************************************************************
 * FUNCTION: Read HDCP Status
 * OUTPUT:  - pHDCPState (Authorization state)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPState(int handle, int *pHDCPState)
{
    VPP_OBJ *vpp_obj;

 //   VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPState==NULL)
        return (MV_VPP_EBADPARAM);

    /*HDCP control*/
    return VPP_BE_GetHdcpState (vpp_obj, pHDCPState);
}

/********************************************************************************
 * FUNCTION: Read HDCP Version
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPVersion(int handle, int *pHDCPVersion)
{
    /* This API will be depricated soon. Use MV_VPPOBJ_GetCurrentHDCPVersion for reading active HDCP version */
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPVersion == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetHdcpVersion(vpp_obj, pHDCPVersion);
}

/********************************************************************************
 * FUNCTION: Read Current HDCP Version
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.2 = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_ENOSINKCNCTED - No sink connected
 ********************************************************************************/
int MV_VPPOBJ_GetCurrentHDCPVersion(int handle, VPP_HDMI_HDCP_VERSION *pHDCPVersion)
{
#ifdef VPPDRV_INTR_MONITOR_FREEZE_ON_BOOTUP
    VPPDRV_vppIntrResetSkipCount(g_vpp->vpp_objs[handle]);
#endif //VPPDRV_INTR_MONITOR_FREEZE_ON_BOOTUP

    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPVersion == NULL)
        return (MV_VPP_EBADPARAM);


    return VPP_BE_GetCurrentHdcpVersion (vpp_obj, pHDCPVersion);
}
/********************************************************************************
 * FUNCTION: Set HDCP Version
 * INPUT :  - iHDCPVersion (value to be set: HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDCPVersion(int handle, int iHDCPVersionSel)
{
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO("enter : version : %d \n", iHDCPVersionSel);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(VPP_HDMI_HDCP_VER_ERR == iHDCPVersionSel)
    {
        VPPAPI_DBG_ERROR("invalid value for hdcp version.\n");
        return (MV_VPP_EBADPARAM);
    }


    return VPP_BE_SetHdcpVersion (vpp_obj, iHDCPVersionSel);
}


int MV_VPPOBJ_HDCPRepUpstreamCSMUpdate(int handle)
{
#if defined (HDCP_REPEATER_MODE)
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

#if HDCP2X_ENABLED
     return VPP_BE_UpdateContentStrmInfo(vpp_obj, HDCP2X_CSM_TYPE_AUTO);
#else
     return MV_VPP_EUNSUPPORT;
#endif
#else
     return MV_VPP_EUNSUPPORT;
#endif
}

/********************************************************************************
 * FUNCTION: Used for setting DS hdcp version from userspace for repeater mode of operation
 * INPUT :  - iHDCPVersion (value to be set: HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetRptrDownstreamHDCPVersion(int handle, int iHDCPVersionSel)
{
#if defined (HDCP_REPEATER_MODE)
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO("enter : version : %d \n", iHDCPVersionSel);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS || (VPP_HDMI_HDCP_VER_ERR == iHDCPVersionSel))
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_SetRptrDsHdcpVersion(vpp_obj, iHDCPVersionSel);
#else
    return MV_VPP_EUNSUPPORT;
#endif
}

/********************************************************************************
 * FUNCTION: Get HDCP Ksv and BKsv
 * INPUT: pHdcpKsv - HDCP ksv data structure
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPKsv(int handle, VPP_HDCP_KSV_DATA *pHdcpKsv)
{
    VPP_OBJ *vpp_obj;
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (pHdcpKsv == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetHdcpKsv (vpp_obj, pHdcpKsv);
}

/********************************************************************************
 * FUNCTION: Get HDCP Aksv
 * INPUT: pHdcpAksv - HDCP Aksv
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPAksv(int handle, VPP_HDMI_HDCP_AKSV *pHdcpAksv)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}


/********************************************************************************
 * FUNCTION: Read Max HDCP capability of Sink
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.2 = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_ENOSINKCNCTED - No sink connected
 ********************************************************************************/
int MV_VPPOBJ_GetSinkHDCPCapability(int handle, VPP_HDMI_HDCP_VERSION *pHDCPVersion)
{

    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if(pHDCPVersion == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetSinkHdcpcapability (vpp_obj, (int *)pHDCPVersion);
}

/********************************************************************************
 * FUNCTION: Set TX HDCP mode
 * INPUT:  - Mode(Repeater/Transmitter)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDCPRepeaterMode(int handle, UINT32 Mode)
{
    VPP_OBJ *vpp_obj;
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_SetHdcpRepeaterMode (vpp_obj, Mode);
}

/********************************************************************************
 * FUNCTION: Get HPD status
 * INPUT: pHpdStatus
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHPDStatus(int handle, int *pHpdStatus)
{
    VPP_OBJ *vpp_obj;
    int retVal = MV_VPP_OK;

//   VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pHpdStatus == NULL)
        return (MV_VPP_EBADPARAM);

    retVal = VPP_BE_GetHPDStatus (vpp_obj, pHpdStatus);

#if defined(FPGA)
    /* In FPGA ignore the HDP status */
    *pHpdStatus = true;
#endif

    return retVal;
}

/********************************************************************************
 * FUNCTION: Get HDR10 playback status
 * OUTPUT: pIsHdrPlaying (1-HDR10 playing, 0-HDR10 not playing)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_GetHDR10PlaybackStatus(int handle, BOOL *pIsHdrPlaying)
{
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pIsHdrPlaying == NULL)
        return (MV_VPP_EBADPARAM);

    *pIsHdrPlaying = (vpp_obj->isHDRContent) &
        (vpp_obj->HDR_capable) &
        (vpp_obj->HDR_enable) &0x1;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Get Hdmi Sink Capabilities
 * INPUT: pSinkCaps
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMISinkCaps(int handle, VPP_HDMI_SINK_CAPS *pSinkCaps)
{
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pSinkCaps == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetSinkCaps (vpp_obj, pSinkCaps);
}

/********************************************************************************
 * FUNCTION: Get Hdmi Sink feature with Mask
 * INPUT: pSinkFeatureMap
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMISinkFeatureMap(int handle, VPP_HDMI_SINKCAP_BITMASK *pSinkFeatureMap)
{
    /* cut down version of sinkcaps used for fastlogo/linux,
     * used to check basic features like 4k_support,exposed via mask
     */
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (pSinkFeatureMap == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetSinkFeatureMap(vpp_obj, pSinkFeatureMap);
}

/********************************************************************************
 * FUNCTION: Get Hdmi Raw EDID
 * INPUT: pRawEdid
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMIRawEdid(int handle, VPP_HDMI_RAW_EDID *pRawEdid)
{
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }
    if (pRawEdid == NULL)
        return (MV_VPP_EBADPARAM);

    return VPP_BE_GetRawEdid (vpp_obj, pRawEdid);
}

/********************************************************************************
 * FUNCTION: Register for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterHdcpCallback (int handle, UINT32 eventCode,
        MV_VPP_EVENT_CALLBACK callbackFn,
        void *contextParam)
{
    VPP_OBJ *vpp_obj;

#if defined(VPP_DEBUG)
    VPP_DBG_PRINTF("%s: eventCode [%d], callbackFn [0x%x] \n", __FUNCTION__, eventCode, (int)callbackFn);
#endif

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_HdcpRegisterCB (vpp_obj, eventCode,
                                    (void *)callbackFn, contextParam);
}

/********************************************************************************
 * FUNCTION: Unregister for HDCP event notifications
 * INPUT: eventCode - Event code for which callback was registered
 *      : callbackFn - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterHdcpCallback (int handle)
{
    VPP_OBJ *vpp_obj;

#if defined(VPP_DEBUG)
    VPP_DBG_PRINTF("%s:  \n", __FUNCTION__);
#endif

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_HdcpUnregisterCB(vpp_obj);
}


/********************************************************************************
 * FUNCTION: Register for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterHdmiCallback (int handle, UINT32 eventCode,
                                    MV_VPP_EVENT_CALLBACK callbackFn,
                                    void *contextParam)
{
    VPP_OBJ *vpp_obj;

    VPPAPI_DBG_INFO(" eventCode [%d], callbackFn [0x%x] \n",  eventCode, (int)callbackFn);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_HdmiRegisterCB (vpp_obj, eventCode, callbackFn, contextParam);
}

/********************************************************************************
 * FUNCTION: Unregister for HDMI event notifications
 * INPUT: eventCode - Event code for which callback was registered
 *      : callbackFn - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterHdmiCallback (int handle)
{
    VPP_OBJ *vpp_obj;

//    VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    return VPP_BE_HdmiUnregisterCB(vpp_obj);
}

/********************************************************************************
 * FUNCTION: Send HDMI specified auxillary packets
 * INPUT: pktType - Packet Type
 *      : pAuxPkt - Pointer to auxillary packet data
 *      : repeatPkt - Transmit packet once/repeat every VSync
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SendHdmiAuxPkt (int handle, VPP_HDMI_PKT_ID pktType,
                              VPP_HDMI_PKT *pAuxPkt, BOOL repeatPkt)
{
    VPP_OBJ *vpp_obj;
    CMD     cmd;
    int     cpcbID;
    int     params[4];
    int     retCode = MV_VPP_OK;

//    VPPAPI_DBG_INFO(" pktType [%d] \n",  pktType);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    params[0] = VPP_HDMI_SEND_HOST_PKT;
    params[1] = pktType;
    params[2] = repeatPkt;

    if (MV_VPP_OK != (retCode = VPP_BE_HdmiFormatAuxPkt (vpp_obj, TRUE, pktType, pAuxPkt)))
        return retCode;

    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 4, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (retCode);
}

/********************************************************************************
 * FUNCTION: Stop HDMI auxillary packets
 * INPUT: pktType - Packet Type
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_StopHdmiAuxPkt (int handle, VPP_HDMI_PKT_ID pktType)
{
    VPP_OBJ *vpp_obj;
    CMD     cmd;
    int     cpcbID;
    int     params[2];
    int     retCode;

//    VPPAPI_DBG_INFO(" pktType [%d] \n", pktType);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);
    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);
    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
          /* Output is not connected to any DV in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if (MV_VPP_OK != (retCode = VPP_BE_HdmiFormatAuxPkt (vpp_obj, FALSE, pktType, NULL)))
        return retCode;

    params[0] = VPP_HDMI_STOP_HOST_PKT;
    params[1] = pktType;
    make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 2, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Configure the features that needs to be enabled for CEC (Feature
 *         : macros are defined in vpp_api_types.h)
 * INPUT:    handle            - VPP object handle
 *      :    featureSupportCfg - Config setting to indicate features to be
 *      :                      - enabled
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_VPP_EBADCALL - CEC is already enabled, API can be called only
 *                           - if CEC is disabled
 ********************************************************************************/
int MV_VPPOBJ_CecConfigFeatureSupport (int handle, int featureSupportCfg)
{
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO(" featureSupportCfg [0x%x]\n", featureSupportCfg);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

#ifndef ENABLE_KILL_PLAYER
    if (vpp_obj->be.enableCec == TRUE) {
        return (MV_VPP_EBADCALL);
    }
#endif

    vpp_obj->be.cecFeatureSupportCfg = featureSupportCfg;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Returns CEC feature configuration currently set
 * INPUT:    handle             - VPP object handle
 *      :    pFeatureSupportCfg - Pointer to return the feature configuration
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_CecGetFeatureSupportConfig (int handle, int *pFeatureSupportCfg)
{
    VPP_OBJ *vpp_obj;

    //VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (pFeatureSupportCfg == NULL) {
        return (MV_VPP_EBADPARAM);
    }
    *pFeatureSupportCfg = vpp_obj->be.cecFeatureSupportCfg;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Enable/Disable HDMI CEC feature
 * INPUT:    handle - VPP object handle
 *      :    enable - TRUE, to enable CEC; FALSE, to disable CEC
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecEnable (int handle, int enable)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" enable [%d]\n", enable);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (!(vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_HDMITX))
    {
        VPPAPI_DBG_INFO("Skip VPP_BE_CEC_Enable\n");
        return (MV_VPP_OK);
    }

    if (vpp_obj->be.enableCec == enable) {
        // Nothing has to be done
        return (MV_VPP_OK);
    }

    retCode = VPP_BE_CEC_Enable (vpp_obj, enable);
    return (retCode);
}
/********************************************************************************
 * FUNCTION: Set CEC device physical address - Setting to the invalid address
 *         : would put CEC in idle state
 * INPUT:    handle  - VPP object handle
 *      :    phyAddr - Physical address of the device
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecSetPhysicalAddr (int handle, int phyAddr)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" phyAddr [0x%x]\n",  phyAddr);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == FALSE) {
        return (MV_VPP_EBADCALL);
    };

    retCode = VPP_BE_CEC_SetPhysicalAddr (vpp_obj, phyAddr);
    vpp_obj->be.cecPhyAddr =(UINT32)phyAddr;
    return (retCode);
}
/********************************************************************************
 * FUNCTION: Allocate/Unallocate logical address for the given device type
 * INPUT:    handle          - VPP object handle
 *      :    alloc           - Allocate/Unallocate logical address
 *      :    deviceType      - Device type
 *      :    initialPollAddr - Initial polling address; -1, if default is to be
 *      :                      used
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecAllocLogicalAddr (int handle, BOOL alloc,
                                   VPP_CEC_DEVICE_TYPE deviceType,
                                   int initialPollAddr)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" alloc [%d], deviceType [%d], initialPollAddr [%d]\n",
//                        alloc, deviceType, initialPollAddr);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == FALSE) {
        return (MV_VPP_EBADCALL);
    };
    retCode = VPP_BE_CEC_AllocLogicalAddr (vpp_obj, alloc, deviceType, initialPollAddr);
    return (retCode);
}
/********************************************************************************
 * FUNCTION: Start transmission of given message on CEC line
 * INPUT:    handle  - VPP object handle
 *      :    pCecMsg   - Pointer to CEC message (Input)
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecSendMessage (int handle, PVPP_CEC_MSG pCecMsg)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" src [%d], dest [%d], opcode [%d] \n", pCecMsg->srcAddr, pCecMsg->destAddr, pCecMsg->opcode);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    if (vpp_obj->be.enableCec == FALSE) {
        return (MV_VPP_EBADCALL);
    }

    retCode = VPP_BE_CEC_SendMessage (vpp_obj, pCecMsg);
    return (retCode);
}

/********************************************************************************
 * FUNCTION: Register for CEC event notifications
 * INPUT: handle  - VPP object handle
 *      : eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CecRegisterCallback (int handle, UINT32 eventCode,
                                   MV_VPP_EVENT_CALLBACK callbackFn,
                                   void *contextParam)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO(" eventCode [%d], callbackFn [0x%x] \n", eventCode, (int)callbackFn);

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    retCode = VPP_BE_CEC_RegisterCB (vpp_obj, eventCode, (UINT32)callbackFn, (UINT32)contextParam);
    return (retCode);
}

/********************************************************************************
 * FUNCTION: Unregister for CEC event notifications
 * INPUT: handle  - VPP object handle
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CecUnregisterCallback (int handle)
{
    VPP_OBJ *vpp_obj;
    int retCode = MV_VPP_OK;

    //VPPAPI_DBG_INFO("enter\n");

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        return (MV_VPP_EUNCONFIG);
    }

    retCode = VPP_BE_CEC_UnregisterCB (vpp_obj);
    return (retCode);
}

int MV_VPPOBJ_StartHdcp14(int handle, int index)
{
    return MV_VPP_EUNSUPPORT;
}

/********************************************************************************
 * FUNCTION: Set Hdmi Tx Phy settings
 * INPUT:  Phy setting values, Size
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiPhy(int handle, unsigned int * phySetting, unsigned int PhyBufSize)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    if (!(vpp_obj->iVppFeatureCfg[0] & VPP_FEATURE_HDMITX))
    {
        VPPAPI_DBG_INFO("Skip VPP_BE_SetHdmiPhy\n");
        return (MV_VPP_OK);
    }

    return VPP_BE_SetHdmiPhy(vpp_obj, phySetting, PhyBufSize);
}

int MV_VPPOBJ_SetHdmiOutputMode(int handle, int mode)
{
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, PLANE_MAIN)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if(VPP_HDMIOutputModeSupport(vpp_obj, mode) != MV_VPP_OK) {
        /* mode not supported */
        return (MV_VPP_EUNSUPPORT);
    }

    return VPP_DVAPI_setHDMImode(vpp_obj, mode);
}

int MV_VPPOBJ_SetHDRConfig(int handle, VPP_HDR_CONFIG_PARAM *param)
{
    VPP_OBJ *vpp_obj;
    int chanID;
    int cpcbID;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if ((chanID = CHAN_OF_PLANE(vpp_obj, PLANE_MAIN)) == CHAN_INVALID){
        /* plane is not connected to any channel in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID) {
        /* plane is not connected to any CPCB in configuration */
        return (MV_VPP_EUNSUPPORT);
    }

    return VPP_DVAPI_HDR_SetConfig(vpp_obj, param);
}

/********************************************************************************
* FUNCTION: Get Hdmi Tx Phy power status
* OUTPUT:  - pPowerState (Phy power state)
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
*         MV_EUNCONFIG - VPP not configured
*         MV_VPP_ENODEV - no device
********************************************************************************/
int MV_VPPOBJ_GetHdmiPowerStatus(int handle, int * pPowerState)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if(pPowerState == NULL)
        return (MV_VPP_EBADPARAM);

    /* return the phy state requested from app to reflect standby/resume
       status as required in VSSDK-8666. Returning current PHY status
       could return phy down even when resolution change which could cause
       error from DRM side. VSSDK-20248. */
    *pPowerState = VPP_BE_GetHdmiPhyExpectedState(vpp_obj);
    return (MV_VPP_OK);
}

int MV_VPPOBJ_SetHDMISignalSwap(int handle, int HdmiSignalSwap)
{
    /* depricated API */
    return (MV_VPP_EUNSUPPORT);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Config values
 * INPUT:  HDMI config values, Size
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiConfig(int handle, unsigned int * config, unsigned int bufSize)
{
    VPP_OBJ *vpp_obj;
    int retVal;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->status == STATUS_INACTIVE){
        /* VPP module is not configured */
        return (MV_VPP_EUNCONFIG);
    }

    retVal = VPP_BE_SetHDMIConfig(vpp_obj, (HDMI_CONFIG *)config, bufSize);
    //handle bootup HPD state
    VPP_BE_ProcessHPDStateChange(vpp_obj);
    return retVal;
}

INT MV_VPPOBJ_LoadHdcpKeys(INT handle, VPP_HDCP_KEYSTORE_DATA *keydata)
{
    VPP_OBJ *vpp_obj;
    VPP_HDCP_KEYSTORE *keystore;
    VPP_HDCP_KEYSTORE_TYPE keyType;

    if (g_vpp->magic != VPP_MAGIC) {
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    if (vpp_obj->mKeystore == NULL) {
        VPPAPI_DBG_ERROR("Keystore not initialized\n");
        return (MV_VPP_ENOMEM);
    }

    keyType = (VPP_HDCP_KEYSTORE_TYPE) keydata->keystoreType;
    if(keyType >= VPP_HDCP_KEYSTORE_MAX) {
        return MV_VPP_EBADPARAM;
    }

    keystore = (VPP_HDCP_KEYSTORE *) vpp_obj->mKeystore;
    VPP_Memcpy(&keystore->keyData[keyType], keydata, sizeof(VPP_HDCP_KEYSTORE_DATA));

    return VPP_HDCP_KEYLOAD_LoadKey(vpp_obj, keydata->keystoreType);
}

int MV_VPPOBJ_SetSpecialSinkIds(int handle, unsigned char *sinkIdList, unsigned int bufSize)
{
    VPP_OBJ *vpp_obj;
    int cpcbID;
    CMD_CHECK(handle);

    return VPP_BE_SetEDIDBlackList(vpp_obj, sinkIdList, bufSize);
}
