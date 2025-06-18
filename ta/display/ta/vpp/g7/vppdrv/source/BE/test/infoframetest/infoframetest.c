// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "vpp_api.h"
#include "SNPS_HDMI_tx.h"
#include "core/packets.h"
#include "core/fc_packets.h"
#include "core/fc_spd.h"
#include "vpp_api_types.h"
#include "util/util.h"



void sendVndrSpecifiInfoFrame(u8 isSend, int autoSend)
{
    SNPS_HDMI_TX *snps_hdmi = BE_HDMITX_get_platform();
    if(!isSend)
    {
        packets_StopSendVsd (&snps_hdmi->hdmi_tx);
        return;
    }
    u8 packet_data[3] = {0,0,0}; //PB4-PB6
    packet_data[0] = set8(0, PACKET_HDMIVIDEOFORMAT_MASK, VPP_HDMI_VIDEO_FMT_3D_FMT);
    packet_data[1] = set8(0, PACKET_3D_STRUCTURE_MASK, VPP_HDMI_3D_STRUCT_FRAME_PACKING);
    packet_data[2] = set8(0, PACKET_3D_EXT_DATA_MASK,  0);

    fc_vsd_vendor_OUI(&snps_hdmi->hdmi_tx, 0x000c03);
    if (fc_vsd_vendor_payload(&snps_hdmi->hdmi_tx, packet_data, sizeof(packet_data))) {
        return MV_VPP_EBADPARAM;    /* DEFINE ERROR */
    }

    if (autoSend)
        fc_packets_AutoSend(&snps_hdmi->hdmi_tx, TRUE, VSD_TX);
    else
        fc_packets_ManualSend (&snps_hdmi->hdmi_tx, VSD_TX);
}

void sendGMDPacket(u8 isSend)
{
    SNPS_HDMI_TX *snps_hdmi = BE_HDMITX_get_platform();
    if(!isSend)
    {
        fc_gamut_enable_tx(&snps_hdmi->hdmi_tx, 0);
        return;
    }
    videoParams_t video;
    video.mColorimetry = EXTENDED_COLORIMETRY;
    video.mExtColorimetry = XV_YCC601;
    video.mColorimetryDataBlock = TRUE;
    
    packets_colorimetry_config(&snps_hdmi->hdmi_tx, &video);
}

void SendSPDPacket(u8 isSend, int autoSend)
{
    SNPS_HDMI_TX *snps_hdmi = BE_HDMITX_get_platform();
    if(!isSend)
    {
        packets_StopSendSpd (&snps_hdmi->hdmi_tx); 
        return;
    }
    fc_spd_info_t spd_data;
    char *vName = "Synaptics";
    char *pName = "Dolphin";
    spd_data.vName = vName;
    spd_data.vLength = 9;
    spd_data.pName = pName;
    spd_data.pLength = 7;
    spd_data.code = 700;
    spd_data.autoSend = autoSend;
    fc_spd_config(&snps_hdmi->hdmi_tx, &spd_data);
}

void sendDRMPacket(u8 isSend)
{
    SNPS_HDMI_TX *snps_hdmi = BE_HDMITX_get_platform();
    if(!isSend)
    {
        fc_drm_packet_update(&snps_hdmi->hdmi_tx, 0);
        fc_PacketTxEnableDRM (&snps_hdmi->hdmi_tx, 0);
        return;
    }
    int header  = 5;
    u8 data[5] = {1, 2, 0, 2, 1};
    fc_drm_packet_header(&snps_hdmi->hdmi_tx, header);
    fc_drm_packet_body(&snps_hdmi->hdmi_tx, data, 5);
    fc_drm_packet_update(&snps_hdmi->hdmi_tx, 1);
}
