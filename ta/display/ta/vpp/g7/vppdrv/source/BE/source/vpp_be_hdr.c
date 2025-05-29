/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_BE_HDR_C_
#define _VPP_BE_HDR_C_

#include "vpp_module.h"
#include "vpp_be.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdr.h"
#include "vpp_cfg.h"
#include "hdmi_aux_data_pack.h"
#include "vpp_be_hdmitx.h"
#include "vpp_dv_api.h"

#define HDR_TO_NONHDR_CHNAGEOVER_INTVL 300

#define GET_HDMI_OUTFORMAT() ((vpp_obj->m_HdrMode == VPP_HDR_MODE_AUTO)? \
        pHdmiTx->hdmi_outformat:vpp_obj->m_outformat)

//#define HDR_DBG
#ifdef HDR_DBG
#define HDR_DBG_PRINT   VPPAPI_DBG_INFO
#else
#define HDR_DBG_PRINT(...)
#endif

static const HDR_MODULE_TYPE HDR_MODULE_SELECTION[VPP_OUTFORMAT_MAX] = {
    HDR_MODULE_DOLBYVISION,      /*VPP_OUTFORMAT_DOVI*/
    HDR_MODULE_DOLBYVISION,      /*VPP_OUTFORMAT_HDR*/
    HDR_MODULE_DOLBYVISION,      /*VPP_OUTFORMAT_SDR*/
    HDR_MODULE_DCM,              /*VPP_OUTFORMAT_SDR2020*/
    HDR_MODULE_DCM,              /*VPP_OUTFORMAT_HLG*/
    HDR_MODULE_DCM,              /*VPP_OUTFORMAT_HDR10PLUS*/
};

static const UINT8 SignalDelay[VPP_OUTFORMAT_MAX][VPP_OUTFORMAT_MAX] = {
    {0, 2, 2, 2, 2, 2},  //DOVI to other mode
    {2, 0, 2, 0, 0, 0},  //HDR to other modes
    {2, 2, 0, 2, 2, 2},  //SDR to other modes
    {2, 0, 2, 0, 0, 0},  //SDR2020 to other modes
    {2, 0, 2, 0, 0, 0},  //HLG to other modes
    {2, 0, 2, 0, 0, 0}   //HDR10PLUS to other modes
};

static int HdmiTxUpdateVSInfoFrame (VPP_OBJ *vpp_obj, BOOL dolbyvision_en, VBUF_INFO *pinfo);
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
static int VPP_BE_HDR10Plus_HDMItx_UpdateVSIF(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo);
#endif

/********************************************************************************
 * FUNCTION: Enable\disable HDR
 * INPUT: Enable - 2: on; 0:ff; 1: auto
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_HDREnable (int handle, INT Enable)
{
    int retCode = MV_VPP_OK;
#ifdef CONFIG_HDR_SUPPORT
    VPP_OBJ *vpp_obj;
    CMD cmd;
    int cpcbID;
    int params[1];

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

    VPPHDMI_DBG_INFO(" HDR ....... %x \n",Enable);

    if(Enable == vpp_obj->m_HdrMode)
    {
        return MV_VPP_OK;
    }
    params[0] = Enable;
    make_cmd(&cmd, (FUNCPTR)VPP_DVAPI_setHDRmode, 1, params);

    MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
    ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

    /* commit commands */
    cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

    MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);

#else
   retCode = (MV_VPP_EHWSTATEWRONG);
#endif
   return (retCode);
}

void VPP_BE_HDR_GetMetadata(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    int i;
    VPP_HDMI_PKT_HDR_INFO_FRAME *phdrInfo = &vpp_obj->hdrInfo;

    if (vpp_obj->transfer_characteristics != pinfo->transfer_characteristics) {
        vpp_obj->transfer_characteristics = pinfo->transfer_characteristics;
    }
    if (vpp_obj->alt_transfer_characteristics != pinfo->alt_transfer_characteristics) {
        vpp_obj->alt_transfer_characteristics = pinfo->alt_transfer_characteristics;
    }
    if (isHDR(pinfo) || isLDR(pinfo))
    {
        /*pack the HDR info frame*/
        for(i = 0; i < 3; i++) {
            phdrInfo->display_primaries_x[i] = (pinfo->sei_display_primary_x[i]);
            phdrInfo->display_primaries_y[i] = (pinfo->sei_display_primary_y[i]);
        }
        phdrInfo->white_point_x = pinfo->sei_white_point_x;
        phdrInfo->white_point_y = pinfo->sei_white_point_y;
        phdrInfo->max_display_mastering_lumi = ((pinfo->sei_max_display_mastering_lumi/10000) & 0xFFFF); //convert to UINT16
        phdrInfo->min_display_mastering_lumi = ((pinfo->sei_min_display_mastering_lumi) & 0xFFFF);//convert to UINT16
        phdrInfo->Maximum_Content_Light_Lvl = pinfo->MaxCLL;
        phdrInfo->Maximum_Frame_Average_Light_Lvl = pinfo->MaxFALL;
    }
    return;
}
/***************************************************************************************
 * Description: reset CSC
 * PARAMS: none
 * RETURN:  1 - succeed
 *                others - failed to load CSC register default values to BCM buffer
 ***************************************************************************************/
void VPP_BE_HDR_Process(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    BE_HDMITX_DATA      *pHdmiTx;
    UINT8 next_mode, m_outformat;
    INT32 nr, dr;

    UINT8 dv_status;
    dv_status = VPP_DV_GetCapability(vpp_obj, DV_CAP_STATUS);

#ifdef HDR_DBG
    UINT8 edr_capable, edr_enable, edr_state, dv_ll;
    edr_capable = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDRCAP);
    edr_enable = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDREN);
    edr_state = VPP_DV_GetCapability(vpp_obj, DV_CAP_STDDV);
    dv_ll = VPP_DV_GetCapability(vpp_obj, DV_CAP_LL);
    static int count;
    ++count;

    if(count % 600 == 0)
    {
        HDR_DBG_PRINT("HDR[%d %d %d %d %d] EDR[%d %d %d %d], cm %d O[%d]\r\n", \
                vpp_obj->HDR_capable, vpp_obj->HDR_enable,vpp_obj->HDR_Status, vpp_obj->m_HdrMode, \
                vpp_obj->m_HdmiOutputMode, edr_capable, edr_enable, edr_state, dv_ll, \
                vpp_obj->current_mode, vpp_obj->m_outformat);
    }
#endif

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    m_outformat = GET_HDMI_OUTFORMAT();

    if(m_outformat == VPP_OUTFORMAT_HLG)
        next_mode = OUTPUT_MODE_HDR;
    else if(m_outformat == VPP_OUTFORMAT_HDR10PLUS)
        next_mode = OUTPUT_MODE_HDR;
    else if ((m_outformat == VPP_OUTFORMAT_SDR2020) && isLDR(pinfo))
        next_mode = OUTPUT_MODE_HDR;
    else if (m_outformat == VPP_OUTFORMAT_SDR2020)
        next_mode = OUTPUT_MODE_SDR;
    else
        next_mode = m_outformat;

    if(next_mode == OUTPUT_MODE_HDR)
        vpp_obj->HDR_Status = TRUE;
    else
        vpp_obj->HDR_Status = FALSE;

    if(!dv_status)
    {
        if(isHDR(pinfo) || isHLG(pinfo) || isHDR10PLUS(pinfo) || isLDR(pinfo))
        {
            vpp_obj->isHDRContent = TRUE;
        }
        else
            vpp_obj->isHDRContent = FALSE;
    }

    if((vpp_obj->current_mode == OUTPUT_MODE_SDR) && (next_mode == OUTPUT_MODE_DOVI))
    {
        //Stop any SDR related signalling
        vpp_obj->current_mode = next_mode;
        vpp_obj->transMode = 0; //transition stage not required.
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_DV_PLAYBACK_START);
        HdmiTxUpdateVSInfoFrame(vpp_obj,TRUE, pinfo);
        HDR_DBG_PRINT("SDR to DOVI--->\r\n");
        HDR_DBG_PRINT("HDR[%d %d %d] EDR[%d %d %d], cm %d\r\n", \
                vpp_obj->HDR_capable, vpp_obj->HDR_enable,vpp_obj->HDR_Status,\
                edr_capable, edr_enable, edr_state, vpp_obj->current_mode);
    }
    else if((vpp_obj->current_mode == OUTPUT_MODE_HDR) && \
            ((next_mode == OUTPUT_MODE_DOVI) || (next_mode == OUTPUT_MODE_SDR)))
    {
        //Implement 2sec switching delay
        if(!vpp_obj->transMode){
            if(!video_framerate_num_den(vpp_obj->uiResID[CPCB_1], &nr, &dr))
            {
                vpp_obj->HDRCount = (nr * 5)/dr;
            }
            else
                vpp_obj->HDRCount = HDR_TO_NONHDR_CHNAGEOVER_INTVL;

            HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_HDR_PLAYBACK_STOPPED);
            if(next_mode == OUTPUT_MODE_DOVI){
                HDR_DBG_PRINT("HDR to DOVI--->\r\n");
                HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_DV_PLAYBACK_START);
                HdmiTxUpdateVSInfoFrame(vpp_obj,TRUE, pinfo);
            }
            else
            {
                HDR_DBG_PRINT("HDR to SDR--->\r\n");
                HdmiTxUpdateVSInfoFrame(vpp_obj,FALSE, pinfo);
            }
            vpp_obj->transMode = (1<<next_mode);
            HDR_DBG_PRINT("HDR[%d %d %d] EDR[%d %d %d], cm %d\r\n", \
                    vpp_obj->HDR_capable, vpp_obj->HDR_enable, vpp_obj->HDR_Status, \
                    edr_capable, edr_enable, edr_state, vpp_obj->current_mode);
        }else{

            if(vpp_obj->transMode != (1<<next_mode))
            {
                //Intermediate transitions
                if(next_mode == OUTPUT_MODE_DOVI)
                    HdmiTxUpdateVSInfoFrame(vpp_obj,TRUE, pinfo);
                else
                    HdmiTxUpdateVSInfoFrame(vpp_obj,FALSE, pinfo);
                vpp_obj->transMode = (1<<next_mode);
            }
            if(!vpp_obj->HDRCount){
                vpp_obj->current_mode = next_mode;
                vpp_obj->transMode = 0;
            }else{
                vpp_obj->HDRCount--;
                HDR_DBG_PRINT("Stop HDR[%d %d %d] EDR[%d %d %d], cm %d\r\n", \
                        vpp_obj->HDR_capable, vpp_obj->HDR_enable, vpp_obj->HDR_Status, \
                        edr_capable, edr_enable, edr_state, vpp_obj->current_mode);
                VPP_Memset(&vpp_obj->hdrInfo, 0, sizeof(vpp_obj->hdrInfo));
                BE_HDMITX_DisableHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_HDR_INFOFRAME, 1);
                if(vpp_obj->HDRCount && (MV_VPP_OK == HdmiTxConfigHDRInfoFrame(vpp_obj)))
                    BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_HDR_INFOFRAME, 0);
            }
        }
    }
    else if((vpp_obj->current_mode == OUTPUT_MODE_SDR) && (next_mode == OUTPUT_MODE_HDR))
    {
        vpp_obj->current_mode = next_mode;
        vpp_obj->transMode = 0; //transition stage not required.
        HdmiTxUpdateVSInfoFrame(vpp_obj,FALSE, pinfo);
        HDR_DBG_PRINT("SDR to HDR--->\r\n");
        HDR_DBG_PRINT("HDR[%d %d %d] EDR[%d %d %d], cm %d\r\n", \
                vpp_obj->HDR_capable, vpp_obj->HDR_enable,vpp_obj->HDR_Status, \
                edr_capable, edr_enable, edr_state, vpp_obj->current_mode);
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_HDR_PLAYBACK_STARTED);
    }
    else if((vpp_obj->current_mode == OUTPUT_MODE_DOVI) && \
            ((next_mode == OUTPUT_MODE_SDR) || (next_mode == OUTPUT_MODE_HDR)))
    {
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_DV_PLAYBACK_STOP);
        //Switch off VSIF signalling
        HdmiTxUpdateVSInfoFrame(vpp_obj,FALSE, pinfo);
        if(next_mode == OUTPUT_MODE_HDR){
            HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_HDR_PLAYBACK_STARTED);
            HDR_DBG_PRINT("DOVI to HDR--->\r\n");
        }else {
            HDR_DBG_PRINT("DOVI to SDR--->\r\n");
        }
        HDR_DBG_PRINT("HDR[%d %d %d] EDR[%d %d %d], cm %d\r\n", \
                vpp_obj->HDR_capable, vpp_obj->HDR_enable,vpp_obj->HDR_Status, \
                edr_capable, edr_enable, edr_state, vpp_obj->current_mode);
        vpp_obj->current_mode = next_mode;
        vpp_obj->transMode = 0;
    }

    if(vpp_obj->current_mode == next_mode)
    {
        if(vpp_obj->current_mode == OUTPUT_MODE_DOVI)
        {
            //handle Std mode to LL mode transitions
            if(vpp_obj->videofmtUpdated || \
                VPP_DV_HDMItx_SetSignalType(vpp_obj, pinfo) || \
                VPP_DV_HDMItx_GetL11MdStatus(vpp_obj)) {
                HdmiTxUpdateVSInfoFrame(vpp_obj, TRUE, pinfo);
            }
        }
        else
        {
            if(vpp_obj->videofmtUpdated)
                HdmiTxUpdateVSInfoFrame(vpp_obj,FALSE, pinfo);

            if(vpp_obj->current_mode == OUTPUT_MODE_HDR)
            {
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
                if(pinfo->m_hdr10plus_present_mode) {
                    VPP_BE_HDR10Plus_HDMItx_UpdateVSIF(vpp_obj, pinfo);
                    vpp_obj->hdr10plus_mode = TRUE;
                }
                /* Reset the HDR10+ metadata in VSIF, if HDR10+ metadata stopped */
                else if (vpp_obj->hdr10plus_mode) {
                    vpp_obj->hdr10plus_mode = FALSE;
                    HdmiTxUpdateVSInfoFrame(vpp_obj, FALSE, pinfo);
                }
#endif
                if(vpp_obj->HDRCount)
                {
                    vpp_obj->HDRCount = 0; //For HDR ->non HDR -> HDR transition
                    vpp_obj->transMode = 0; //transition stage not required.
                    HdmiTxNotifyEvent(vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_HDR_PLAYBACK_STARTED);
                }
                if((dv_status == FALSE) && vpp_obj->isHDRContent && (vpp_obj->m_HdrMode == VPP_HDR_MODE_AUTO)){
                    VPP_BE_HDR_GetMetadata(vpp_obj, pinfo); //For non-Dolby passthrough cases
                }
                BE_HDMITX_DisableHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_HDR_INFOFRAME, 1);
                if(MV_VPP_OK == HdmiTxConfigHDRInfoFrame(vpp_obj))
                    BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_HDR_INFOFRAME, 0);
            }
        }
    }
    VPP_BE_HDR_BT2020Process(vpp_obj, pinfo);

    if(pHdmiTx->hdmiEarlySignalCount) {
        --pHdmiTx->hdmiEarlySignalCount;
    }
}

#ifdef CONFIG_AMP_VPP_HDR10_PLUS
static int VPP_BE_HDR10Plus_HDMItx_UpdateVSIF(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    BE_HDMITX_DATA      *pHdmiTx;
    UINT8   DataBuf[31];
    VPP_HDMI_PKT auxPkt;
    UINT8 loop;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (!pHdmiTx->edidInfo.HdrCapInfo.isHDR10PlusSupported)
        return MV_VPP_OK;

    GaloisMemClear (DataBuf, 31);
    GaloisMemClear (&auxPkt, sizeof(auxPkt));

    auxPkt.vndrSpecInfoFrm.ieeeRegID = HDR10PLUS_IEEE_CODE;
    auxPkt.vndrSpecInfoFrm.application_ver = pinfo->m_hdr10plus_meta.application_ver;
    auxPkt.vndrSpecInfoFrm.targeted_sys_dis_max_lum = pinfo->m_hdr10plus_meta.targeted_sys_dis_max_lum;
    auxPkt.vndrSpecInfoFrm.avg_maxrgb = pinfo->m_hdr10plus_meta.avg_maxrgb;
    auxPkt.vndrSpecInfoFrm.knee_point_x = pinfo->m_hdr10plus_meta.knee_point_x;
    auxPkt.vndrSpecInfoFrm.knee_point_y = pinfo->m_hdr10plus_meta.knee_point_y;
    auxPkt.vndrSpecInfoFrm.num_bezier_curve_anchors = pinfo->m_hdr10plus_meta.num_bezier_curve_anchors;
    auxPkt.vndrSpecInfoFrm.gph_ovl_flag = 0;
    auxPkt.vndrSpecInfoFrm.vsif_timing_mode = 1;
    for (loop = 0; loop < MAX_NUM_DISTRIBUTIONS; loop++) {
        auxPkt.vndrSpecInfoFrm.distribution_values[loop] = pinfo->m_hdr10plus_meta.distribution_values[loop];
    }
    for (loop = 0; loop < MAX_NUM_BEZIER_CURVE_ANCHORS; loop++) {
        auxPkt.vndrSpecInfoFrm.bezier_curve_anchors[loop] = pinfo->m_hdr10plus_meta.bezier_curve_anchors[loop];
    }

    // Only for debug purpose. To be removed
    HDR_DBG_PRINT("application_ver = [0x%X]\n", auxPkt.vndrSpecInfoFrm.application_ver);
    HDR_DBG_PRINT("targeted_sys_dis_max_lum = [0x%X]\n", auxPkt.vndrSpecInfoFrm.targeted_sys_dis_max_lum);
    HDR_DBG_PRINT("avg_maxrgb = [0x%X]\n", auxPkt.vndrSpecInfoFrm.avg_maxrgb);
    HDR_DBG_PRINT("knee_point_x = [0x%X]\n", auxPkt.vndrSpecInfoFrm.knee_point_x);
    HDR_DBG_PRINT("knee_point_y = [0x%X]\n", auxPkt.vndrSpecInfoFrm.knee_point_y);
    HDR_DBG_PRINT("num_bezier_curve_anchors = [0x%X]\n", auxPkt.vndrSpecInfoFrm.num_bezier_curve_anchors);
    for (loop = 0; loop < MAX_NUM_DISTRIBUTIONS; loop++) {
        HDR_DBG_PRINT("distribution_values[%d] = [0x%X]\n", loop, auxPkt.vndrSpecInfoFrm.distribution_values[loop]);
    }
    for (loop = 0; loop < MAX_NUM_BEZIER_CURVE_ANCHORS; loop++) {
        HDR_DBG_PRINT("bezier_curve_anchors[%d] = [0x%X]\n", loop, auxPkt.vndrSpecInfoFrm.bezier_curve_anchors[loop]);
    }

    VppPackVndrSpecInfoFrame (vpp_obj, &auxPkt.vndrSpecInfoFrm, DataBuf);

    // Set host packet data
    BE_HDMITX_SetVndrSpecInfoFrame(pHdmiTx, &auxPkt.vndrSpecInfoFrm, DataBuf);
    BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_VENDOR_INFOFRAME, 1);
    return (MV_VPP_OK);
}
#endif

/*************************************************************************
 * FUNCTION: Update Vendor Specific Infoframe for EDR requirements
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 **************************************************************************/
static int HdmiTxUpdateVSInfoFrame (VPP_OBJ *vpp_obj, BOOL dolbyvision_en, VBUF_INFO *pinfo)
{
    BE_HDMITX_DATA      *pHdmiTx;
    UINT8               DataBuf[31];
    VPP_HDMI_PKT auxPkt;
    UINT8 edr_capable, dv_profile;
    int resID;
    RESOLUTION_INFO  *pResinfo;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    GaloisMemClear (DataBuf, 31);
    GaloisMemClear (&auxPkt, sizeof(auxPkt));
    VPP_DV_HDMItx_SetSignalType(vpp_obj, pinfo);
    edr_capable = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDRCAP);
    dv_profile = VPP_DV_HDMItx_UpdateVSIF(vpp_obj, dolbyvision_en);
    resID = pHdmiTx->videoParams.DispRes;
    pResinfo = VPP_GetResInfo(resID);

    if((edr_capable == VPP_HDMI_SINK_NON_DOVI) || (edr_capable == VPP_HDMI_SINK_STD_DOVI))
    {
        auxPkt.vndrSpecInfoFrm.ieeeRegID = 0x000c03;
        // Read the host packet content corresponding to AVI
        BE_HDMITX_GetHostPktTxData (pHdmiTx, VPP_HDMI_PKT_ID_VENDOR_INFOFRAME, DataBuf);
        if(DataBuf[7] && VPP_IsResIdInRange(CPCB_1, RES_MIN_4Kx2K, RES_4Kx2K30))
        {
            auxPkt.vndrSpecInfoFrm.HdmiVideoFmt = VPP_HDMI_VIDEO_FMT_EXTENDED_RES_FMT;
            auxPkt.vndrSpecInfoFrm.HDMI_VIC = DataBuf[8];
        }
        else
            auxPkt.vndrSpecInfoFrm.HdmiVideoFmt=VPP_HDMI_VIDEO_FMT_NO_INFO;

        if(dolbyvision_en && dv_profile)
        {
            auxPkt.vndrSpecInfoFrm.PacketLength = DOLBY_VSIF_LENGTH;
        }
        else
        {
            //if 7th byte is valid, resolution is betn 4k24-4k30,don't stop that
            if(DataBuf[7] && (VPP_IsResIdInRange(CPCB_1, RES_MIN_4Kx2K, RES_4Kx2K30)))
                auxPkt.vndrSpecInfoFrm.PacketLength = 5;
            else
                auxPkt.vndrSpecInfoFrm.PacketLength = 4;
        }
        VppPackVndrSpecInfoFrame (vpp_obj, &auxPkt.vndrSpecInfoFrm, DataBuf);
        // Set host packet data
        BE_HDMITX_SetVndrSpecInfoFrame(pHdmiTx, &auxPkt.vndrSpecInfoFrm, DataBuf);
        BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_VENDOR_INFOFRAME, 1);
    }

    if(!dolbyvision_en)
    {
        if (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR444)
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_444;
        else if (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR422)
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_422;
        else if (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR420)
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_420;
        else
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_RGB;

        if (!pHdmiTx->edidInfo.HdrCapInfo.dolbyvisionvsvdb.is422_12bitSupported)
            pHdmiTx->aviParams.RGBQuantRange = VPP_HDMI_GET_RGB_RANGE(pHdmiTx, pResinfo);
        else
            pHdmiTx->aviParams.YCCQuantRange = CEA_YCC_QUANT_LTD_RANGE;

        //Avoid resetting BT.2020 for DOVI to HDR/HLG switching
        if(vpp_obj->hdmi_bt2020en)
            pHdmiTx->aviParams.ExtCalorimetry = CEA_EXT_CAL_BT2020YCC_FMT;
        else
            pHdmiTx->aviParams.ExtCalorimetry = CEA_EXT_CAL_MAX;
    }

    // Set host packet data
    BE_HDMITX_UpdateAVIInfoFrame (pHdmiTx);
    BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_AVI_INFOFRAME, 1);
    return (MV_VPP_OK);
}

int VPP_BE_HDR_BT2020Process(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
#ifdef CONFIG_BT2020_SUPPORT
    BE_HDMITX_DATA  *pHdmiTx;
    int color_fmt, bit_depth;
    UINT8 m_outformat;
    int local_prev_state, current_ll_state;

    local_prev_state = vpp_obj->hdmi_bt2020en;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    color_fmt = pHdmiTx->videoParams.VideoFmt;
    bit_depth = pHdmiTx->videoParams.ColorDepth;

    vpp_obj->hdmi_bt2020en = FALSE;
    current_ll_state = FALSE;
    //Enable BT.2020 only for 4K 10/12-bit output (spec) and if sink supports BT2020.
    m_outformat = GET_HDMI_OUTFORMAT();
    if(m_outformat != VPP_OUTFORMAT_SDR) {
        vpp_obj->hdmi_bt2020en = TRUE;
        if(VPP_DV_GetCapability(vpp_obj, DV_CAP_STATUS) && \
              VPP_DV_GetCapability(vpp_obj, DV_CAP_EDREN)) {
            current_ll_state = TRUE;
        }
    }

    if((local_prev_state != vpp_obj->hdmi_bt2020en) || (current_ll_state != vpp_obj->previus_ll_state) || \
            vpp_obj->videofmtUpdated) {
        local_prev_state = vpp_obj->hdmi_bt2020en;
        vpp_obj->previus_ll_state = current_ll_state; //LL <-> HDR switching handling

        VPPAPI_DBG_FIELD("CSC Changed BT2020 %s\n",
                vpp_obj->hdmi_bt2020en ? "Enabled" : "Disabled");

        //Only for non-dolby cases, configure the output to BT2020
        if(!VPP_DV_GetCapability(vpp_obj, DV_CAP_STATUS))
            VPP_BE_SetVoutFormat(vpp_obj, VOUT_HDMI, color_fmt, bit_depth);
        VPP_BE_SendHdmiAVIInfoFrame(vpp_obj);
    }
#endif
    return MV_VPP_OK;
}

static BOOL isHDROutputSupported(VPP_OBJ *vpp_obj, INT color_fmt, INT bit_depth)
{
    BOOL support;
    switch(vpp_obj->m_outformat) {
        case VPP_OUTFORMAT_DOVI:
            support = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDREN);
            break;
        case VPP_OUTFORMAT_HDR:
            support = vpp_obj->HDR_enable;
            break;
        case VPP_OUTFORMAT_HLG:
            support = vpp_obj->HLG_enable;
            break;
        case VPP_OUTFORMAT_HDR10PLUS:
            support = vpp_obj->HDR10PLUS_enable;
            break;
        case VPP_OUTFORMAT_SDR2020:
            support = isBT2020HDMIOutputSupported(color_fmt, bit_depth);
            break;
        default:
            support = TRUE;
            break;
    }
    return support;
}

/*
 *  Output priority order as follows Dolby vision → HDR/HLG → SDR2020->SDR.
 *  In HDR auto mode, output follows above priority order.
 *  That is output will be either same or lower priority than input.
 *  Example 1: If input is HDR and TV don't support HDR,
 *             then HDR input will not be converted to Dolby vision.
 *             Either HDR or SDR will be the output.
 *  Example 2: If input is Dolby vision and TV supports HDR,
 *             then output will be either HDR or SDR.
 *  Example 3: If TV supports only HDR and input is Dolby vision/HLG,
 *             output will be converted to HDR.
 */
static UINT8 VPP_BE_GetAutoOutputFormat(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    UINT8 m_outformat, edr_enable;
    int color_fmt, bit_depth;
    BE_HDMITX_DATA      *pHdmiTx;
    VPP_HDR_CONFIG *hdr = (VPP_HDR_CONFIG *)vpp_obj->phdr_cfg;
    BOOL isDoviInput = isDOVI(pinfo);
    BOOL isHDRInput = isHDR(pinfo);
    BOOL isHLGInput = isHLG(pinfo);
    BOOL isSDR2020Input = isSDR2020(pinfo);
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
    BOOL isHDR10PLUSInput = isHDR10PLUS(pinfo);
#endif

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    color_fmt = pHdmiTx->videoParams.VideoFmt;
    bit_depth = pHdmiTx->videoParams.ColorDepth;
    edr_enable = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDREN);

    /*Don't use builtin frame for switching output*/
    if(pinfo->builtinFrame && (vpp_obj->m_outformat != VPP_OUTFORMAT_SDR2020) &&
            (vpp_obj->m_outformat != VPP_OUTFORMAT_HLG) &&
            (vpp_obj->m_outformat != VPP_OUTFORMAT_HDR10PLUS) &&
            (hdr->u8AutoModeType == VPP_HDR_AUTOMODE_1) &&
            isHDROutputSupported(vpp_obj, color_fmt, bit_depth)) {
        m_outformat = vpp_obj->m_outformat;
    }
    else if(edr_enable && isDoviInput) {
        m_outformat = VPP_OUTFORMAT_DOVI;
    }
    else if(vpp_obj->HLG_enable && isHLGInput) {
        m_outformat = VPP_OUTFORMAT_HLG;
    }
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
    else if(vpp_obj->HDR10PLUS_enable && isHDR10PLUSInput) {
        m_outformat = VPP_OUTFORMAT_HDR10PLUS;
    }
#endif
    else if(vpp_obj->HDR_enable && (isDoviInput || isHDRInput)) {
        m_outformat = VPP_OUTFORMAT_HDR;
    }
    else if(isBT2020HDMIOutputSupported(color_fmt, bit_depth) && isSDR2020Input) {
        m_outformat = VPP_OUTFORMAT_SDR2020;
    }
    else {
        m_outformat = VPP_OUTFORMAT_SDR;
    }
    return m_outformat;
}

/*To be called only in auto mode*/
BOOL VPP_BE_HDR_SetHDMIEarlySignaling(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    UINT8 m_outformat;
    BE_HDMITX_DATA  *pHdmiTx;
    int chanID;
    CHAN *chan;
    VPP_HDR_CONFIG *hdr = (VPP_HDR_CONFIG *)vpp_obj->phdr_cfg;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    chanID = CHAN_OF_PLANE(vpp_obj, PLANE_MAIN);
    chan = &vpp_obj->chan[chanID];

    m_outformat = VPP_BE_GetAutoOutputFormat(vpp_obj, pinfo);
    /*In Auto mode, change HDMI signaling earlier than video boundary*/
    if((m_outformat != pHdmiTx->hdmi_outformat) && (!pHdmiTx->hdmiEarlySignalCount)) {
        pHdmiTx->hdmiEarlySignalCount = hdr->u8SwitchingDelay[pHdmiTx->hdmi_outformat][m_outformat];
        VPPAPI_DBG_INFO("Mode switch: %d -> %d delay: %d\r\n", pHdmiTx->hdmi_outformat, m_outformat, pHdmiTx->hdmiEarlySignalCount);
        pHdmiTx->hdmi_outformat = m_outformat;
    }
    if(pHdmiTx->hdmiEarlySignalCount) {
        chan->built_in_frame_scl_required = 1;/*1:1 builtin frame scaling for 2 frames*/
        return TRUE;
    } else {
        return FALSE;
    }
}

void VPP_BE_HDR_SetConfig(VPP_OBJ *vpp_obj, VPP_HDR_CONFIG_PARAM *param)
{
    UINT8 i, j;
    VPP_HDR_CONFIG *hdr = (VPP_HDR_CONFIG *)vpp_obj->phdr_cfg;

    if(param) {
        /*user configuration*/
        hdr->u8ConfigEn = param->u8ConfigEn;
        hdr->u8AutoModeType = param->u8AutoModeType;
        HDR_DBG_PRINT("CfgEn: 0x%x AutoMode: 0x%x\r\n", hdr->u8ConfigEn, hdr->u8AutoModeType);
        for(i = 0; i< VPP_OUTFORMAT_MAX; i++) {
            hdr->HDR_MODULE_SELECTION[i] = (HDR_MODULE_TYPE)(param->uiModuleSelect >> (i * 4)) & 0xF;
            HDR_DBG_PRINT("Mode[%d]: 0x%x\r\n", i, hdr->HDR_MODULE_SELECTION[i]);
            for(j = 0; j < VPP_OUTFORMAT_MAX; j++) {
                hdr->u8SwitchingDelay[i][j] = (UINT8)((param->uiSwitchingDelay[i] >> (j * 4)) & 0xF);
                HDR_DBG_PRINT("Delay[%d][%d] = %d\r\n", i, j, hdr->u8SwitchingDelay[i][j]);
            }
        }
    } else {
        /*default setting*/
        hdr->u8ConfigEn = true;
        hdr->u8AutoModeType = VPP_HDR_AUTOMODE_0;
        for(i = 0; i< VPP_OUTFORMAT_MAX; i++) {
            hdr->HDR_MODULE_SELECTION[i] = HDR_MODULE_SELECTION[i];
            for(j = 0; j < VPP_OUTFORMAT_MAX; j++) {
                hdr->u8SwitchingDelay[i][j] = SignalDelay[i][j];
            }
        }
    }
}

void VPP_BE_HDR_SetOutputMode(VPP_OBJ *vpp_obj, DV *pDV)
{
    PLANE *plane;
    int chanID, planeID, i;
    UINT8 m_outformat;
    VBUF_INFO *pinfo;
    VDEC_VID_BUF_DESC *inline_frame;
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    for (i = 0; i < pDV->num_of_chans; i++) {
        chanID = pDV->chanID[i];
        if (vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
            planeID = PLANE_OF_CHAN(vpp_obj, chanID);
            plane = &vpp_obj->plane[planeID];

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
                pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
                pinfo = (VBUF_INFO *)inline_frame;
#endif
                if( !pinfo )
                    continue;

                if( planeID == PLANE_MAIN )
                {
                    switch(vpp_obj->m_HdrMode)
                    {
                        case VPP_HDR_MODE_ON:
                            {
                                m_outformat = vpp_obj->m_HdmiOutputMode;
                                pHdmiTx->hdmi_outformat = VPP_OUTFORMAT_SDR;
                                break;
                            }
                        case VPP_HDR_MODE_OFF:
                            {
                                m_outformat = VPP_OUTFORMAT_SDR;
                                pHdmiTx->hdmi_outformat = VPP_OUTFORMAT_SDR;
                                break;
                            }
                        case VPP_HDR_MODE_AUTO:
                        default:
                            {
                                m_outformat = VPP_BE_GetAutoOutputFormat(vpp_obj, pinfo);
                                break;
                            }
                    } //end of HdrMode switch

                    if (vpp_obj->m_outformat != m_outformat) {
                        VPPAPI_DBG_FIELD("Output format switch (%d -> %d)\r\n", vpp_obj->m_outformat, m_outformat);
                        vpp_obj->m_outformat = m_outformat;
                        vpp_obj->videofmtUpdated = 1;
                    }
                }
            }
        }
    }
}

/**
 *  * Function executed upon calling setHdmiVidFmt
 *  * Function will check HDR mode setting requirements & update modes as per best output method.
 *  * That is DOVI, HDR, SDR.
 *  * If color format & bitdepth, unable to support HDR output
 *  * then m_HdmiOutputMode = SDR & m_HdrMode = OFF
 *  * If sink supports DOVI, HDR, SDR. Then output will be DOVI.
 *  * Unless m_HdrMode = AUTO, output will behave as forced output mode.
 */
void VPP_BE_HDR_SetCapability(VPP_OBJ *vpp_obj)
{
    UINT32 output_mode, hdr_mode;
    BE_HDMITX_DATA  *pHdmiTx;
    int color_fmt, bit_depth, resID;
    VPP_HDMI_DOLBYVISION_CAP *dolbycap;
    UINT8 edr_enable;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    color_fmt = pHdmiTx->videoParams.VideoFmt;
    bit_depth = pHdmiTx->videoParams.ColorDepth;
    resID = pHdmiTx->videoParams.DispRes;
    dolbycap = &pHdmiTx->edidInfo.HdrCapInfo.dolbyvisionvsvdb;

    edr_enable = VPP_DV_CfgEdrSupport(vpp_obj, dolbycap, color_fmt, bit_depth, resID);
#ifdef CONFIG_HDR_SUPPORT
    if((bit_depth != OUTPUT_BIT_DEPTH_8BIT) && (OUTPUT_COLOR_FMT_RGB888 != color_fmt))
    {
        vpp_obj->HDR_enable = vpp_obj->HDR_capable;
        vpp_obj->HLG_enable = vpp_obj->HLG_capable;
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
        vpp_obj->HDR10PLUS_enable = vpp_obj->HDR10PLUS_capable;
#else
         vpp_obj->HDR10PLUS_enable = FALSE;
#endif
    }
    else
    {
        vpp_obj->HDR_enable = FALSE;
        vpp_obj->HLG_enable = FALSE;
        vpp_obj->HDR10PLUS_enable = FALSE;
    }
#endif

    VPPAPI_DBG_FIELD("HDR (EDR %d HDR %d HLG %d HDR10PLUS %d)............\n",
        edr_enable, vpp_obj->HDR_enable, vpp_obj->HLG_enable, vpp_obj->HDR10PLUS_enable);

    /*Default HDR Mode setting*/
    if((!edr_enable) && (!vpp_obj->HDR_enable)) {
        output_mode = VPP_HDMI_OUTPUT_MODE_SDR;
        hdr_mode = VPP_HDR_MODE_AUTO;
    } else if(edr_enable) {
        output_mode = VPP_HDMI_OUTPUT_MODE_DOVI;
        hdr_mode = VPP_HDR_MODE_DEFAULT;
    } else {
        output_mode = VPP_HDMI_OUTPUT_MODE_HDR10;
        hdr_mode = VPP_HDR_MODE_DEFAULT;
    }

    if((output_mode != vpp_obj->m_HdmiOutputMode) || (vpp_obj->m_HdrMode != hdr_mode)) {
        vpp_obj->m_HdmiOutputMode = output_mode;
        vpp_obj->m_HdrMode = hdr_mode;
        vpp_obj->videofmtUpdated = 1;
    }
    return;
}

INT VPP_BE_HDR_Init(VPP_OBJ *vpp_obj)
{
    VPP_HDR_CONFIG *phdr = NULL;

    VPP_OSAL_SHM_Dynamic_Malloc(VPP_SHM_SECURE, sizeof(VPP_HDR_CONFIG), \
            32, (VPP_SHM_HANDLE*)&phdr);
    if(!phdr) {
        return (MV_VPP_ENOMEM);
    }
    vpp_obj->phdr_cfg = phdr;
    VPP_BE_HDR_SetConfig(vpp_obj, NULL);//Initialize default config

    return MV_VPP_OK;
}

INT VPP_BE_HDR_Destroy(VPP_OBJ *vpp_obj)
{
    if(vpp_obj->phdr_cfg)
        VPP_OSAL_SHM_Dynamic_Free((VPP_SHM_HANDLE)vpp_obj->phdr_cfg);

    return MV_VPP_OK;
}

void VPP_BE_SelectHDRModule(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    VPP_HDR_CONFIG *hdr = (VPP_HDR_CONFIG *)vpp_obj->phdr_cfg;
    VPP_OUTFORMAT fmt = vpp_obj->m_outformat;
    BOOL hdrFlag = 0;
    if (hdr->HDR_MODULE_SELECTION[fmt] == HDR_MODULE_DOLBYVISION) {
        hdrFlag = 1;
    }
    VPP_DV_Updated_FrameDVMode(hdrFlag, pinfo);
}
#endif
