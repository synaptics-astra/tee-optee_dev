/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __HDMI_AUX_DATA_PACK_H__
#define __HDMI_AUX_DATA_PACK_H__

#include "com_type.h"
#include "hdmi_cmn.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
BOOL VppFillChkSum (
            UINT8 *pDataBuf,
            UINT8 Length);

BOOL VppPackAudioInfoFrame(
            PAUDIO_INFO_FRAME pAudioInfo,
            UINT8 *pDataBuf);

BOOL VppPackSPDInfoFrame(
            VPP_HDMI_PKT_SPD_INFOFRM *pSPDData, UINT8 *pDataBuf);

BOOL VppPackMpegSrcInfoFrame(
            VPP_HDMI_PKT_MPEG_SRC_INFOFRM *pMpegSrcData, UINT8 *pDataBuf);

BOOL VppPackVndrSpecInfoFrame(VPP_OBJ *vpp_obj,
            VPP_HDMI_PKT_VNDRSPEC_INFOFRM *pVndrSpecInfoFrm, UINT8 *pDataBuf);

BOOL VppPackACPPacket(
            VPP_HDMI_PKT_ACP *pAcpPktData, UINT8 *pDataBuf);

BOOL VppPackGMDPacket(
            VPP_HDMI_PKT_GMD *pGmdPktData, UINT8 *pDataBuf);

BOOL VppPackISRC1Packet(
            VPP_HDMI_PKT_ISRC1 *pIsrc1PktData, UINT8 *pDataBuf);

BOOL VppPackISRC2Packet(
            VPP_HDMI_PKT_ISRC2 *pIsrc2PktData, UINT8 *pDataBuf);

BOOL VppPackHDRInfoframe(VPP_OBJ *vpp_obj,
            VPP_HDMI_PKT_HDR_INFO_FRAME *phdrPktData, UINT8 *pDataBuf);
#endif // __HDMI_AUX_DATA_PACK_H__
