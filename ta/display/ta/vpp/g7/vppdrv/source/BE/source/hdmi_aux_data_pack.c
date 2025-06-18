/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "com_type.h"
#include "hdmi_cmn.h"
#include "hdmi_aux_data_pack.h"
#include "vpp_module.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdr.h"
#include "vpp_osal.h"
#include "vpp_dv_api.h"

#define MSB_VAL_SHORT_INT(Val) ((Val >> 8) & 0xFF)
#define LSB_VAL_SHORT_INT(Val) ((Val) & 0xFF)
/*-----------------------------------------------------------------------------
 * Description  : This function calculates checksum for the info frame
 *              : packets
 * Arguments    : pDataBuf - Pointer to data buffer whose checksum has to be
 *              : calculated
 *              : Length - Length of data buffer
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Checksum is calculated such that byte-wide sum of all three
 *              : bytes of the Packet Header and all valid bytes of the
 *              : InfoFrame Packet contents (determined by length), plus the
 *              : checksum itself, equals zero. (Ref Hdmi Spec 1.3)
 *-----------------------------------------------------------------------------
 */
BOOL VppFillChkSum (
            UINT8 *pDataBuf,
            UINT8 Length)
{
    UINT8 ChkSum = 0;
    UINT8 i;

    if (pDataBuf == NULL || Length == 0)
    {
        return FALSE;
    }

    for (i = 0; i < Length; i++)
    {
        ChkSum += pDataBuf[i];
    }

    pDataBuf[3] = (0x100 - ChkSum);
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as Audio info frame
 *              : packet
 * Arguments    : pAudioInfo - Pointer to Audio info frame data (Input)
 *              : pDataBuf - Pointer to data buffer to pack the
 *              : info frame (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref CEA-861-D
 *-----------------------------------------------------------------------------
 */
// Audio information frame
BOOL VppPackAudioInfoFrame(
            PAUDIO_INFO_FRAME pAudioInfo,
            UINT8 *pDataBuf)
{
    BOOL    RetCode;

    if (pAudioInfo == NULL || pDataBuf == NULL)
    {
        return FALSE;
    }

    // Packet Header
    pDataBuf[0] = (1 << 7) | (CEA_INFO_FRM_AUDIO); // Info frame type
    pDataBuf[1] = CEA_AUD_FRM_VERS;                // Version
    pDataBuf[2] = CEA_AUD_FRM_LEN;                 // Info frame length
    // Checksum will be calculated and filled in the end
    pDataBuf[3] = 0x0;

    // Packet Data Bytes
    // Data byte 1
    pDataBuf[4] = (pAudioInfo->CodingType << 4) |
                  (pAudioInfo->ChannelCnt);

    // Data byte 2
    pDataBuf[5] = (pAudioInfo->SamplingFreq << 2) |
                  (pAudioInfo->SampleSize);

    // Data byte 3 - Reserved
    pDataBuf[6] = 0x00;

    // Data byte 4
    pDataBuf[7] = pAudioInfo->SpkrAlloc;

    // Data byte 5
    pDataBuf[8] = (pAudioInfo->DownMixInhFlag << 7) |
                  (pAudioInfo->LvlShiftVal << 3);

    // Rest of the data bytes are reserved
    pDataBuf[9]  = 0x00;
    pDataBuf[10] = 0x00;
    pDataBuf[11] = 0x00;
    pDataBuf[12] = 0x00;
    pDataBuf[13] = 0x00;

    // Length of the packet is length of the data bytes +
    // size of header (4 bytes)
    RetCode = VppFillChkSum(pDataBuf, (CEA_AUD_FRM_LEN + FRM_HDR_LEN));
    return RetCode;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as source product
 *              : desc packet
 * Arguments    : pSPDData - Pointer to SPD data (Input)
 *              : pDataBuf - Pointer to data buffer to pack the
 *              : info frame (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref CEA-861-D
 *-----------------------------------------------------------------------------
 */
BOOL VppPackSPDInfoFrame(
            VPP_HDMI_PKT_SPD_INFOFRM *pSPDData, UINT8 *pDataBuf)
{
    BOOL    RetCode;
    UINT8   Pos;
    UINT8   i;

    if (pSPDData == NULL || pDataBuf == NULL)
    {
        return FALSE;
    }

    // Packet Header
    pDataBuf[0] = (1 << 7) | (CEA_INFO_FRM_SPD); // Info frame type
    pDataBuf[1] = CEA_SPD_FRM_VERS;              // Version
    pDataBuf[2] = CEA_SPD_FRM_LEN;               // Info frame length

    // Checksum will be calculated and filled in the end
    pDataBuf[3] = 0x0;

    Pos = FRM_HDR_LEN;
    // Packet Data Bytes
    // Data byte 1-8
    for (i = 0; i < 8; i++)
    {
        pDataBuf[Pos++] = pSPDData->vendorName[i];
    }

    // Data byte 9-24
    for (i = 0; i < 16; i++)
    {
        pDataBuf[Pos++] = pSPDData->prodDescChar[i];
    }

    // Data byte 25
    pDataBuf[Pos] = pSPDData->srcDev;

    // Length of the packet is length of the data bytes +
    // size of header (4 bytes)
    RetCode = VppFillChkSum(pDataBuf, (CEA_SPD_FRM_LEN + FRM_HDR_LEN));
    return RetCode;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as Mpeg source info
 *              : frame packet
 * Arguments    : pMpegSrcData - Pointer to Mpeg source info data (Input)
 *              : pDataBuf - Pointer to data buffer to pack the
 *              : info frame (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref CEA-861-D
 *-----------------------------------------------------------------------------
 */
// MPEG source info frame
BOOL VppPackMpegSrcInfoFrame(
            VPP_HDMI_PKT_MPEG_SRC_INFOFRM *pMpegSrcData, UINT8 *pDataBuf)
{
    BOOL    RetCode;
    UINT8   Pos;
    UINT8   i;

    if (pMpegSrcData == NULL || pDataBuf == NULL)
    {
        return FALSE;
    }

    // Packet Header
    pDataBuf[0] = (1 << 7) | (CEA_INFO_FRM_MPEG_SRC); // Info frame type
    pDataBuf[1] = CEA_MPEG_SRC_FRM_VERS;              // Version
    pDataBuf[2] = CEA_MPEG_SRC_FRM_LEN;               // Info frame length

    // Checksum will be calculated and filled in the end
    pDataBuf[3] = 0x0;

    Pos = FRM_HDR_LEN;
    // Packet Data Bytes
    // Data byte 1-4
    pDataBuf[Pos++] = (pMpegSrcData->bitRate & 0xFF);
    pDataBuf[Pos++] = (pMpegSrcData->bitRate >> 8)& 0xFF;
    pDataBuf[Pos++] = (pMpegSrcData->bitRate >> 16)& 0xFF;
    pDataBuf[Pos++] = (pMpegSrcData->bitRate >> 24)& 0xFF;

    pDataBuf[Pos++] = (pMpegSrcData->mpegFrameType) | (pMpegSrcData->repeatedField << 4);

    for (i = 0; i < 5; i++)
        pDataBuf[Pos++] = 0;

    // Length of the packet is length of the data bytes +
    // size of header (4 bytes)
    RetCode = VppFillChkSum(pDataBuf, (CEA_MPEG_SRC_FRM_LEN + FRM_HDR_LEN));
    return RetCode;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as vendor specific
 *              : infoframe packet
 * Arguments    : vpp_obj - VPP pointer
 *              : pVndrSpecInfoFrm - Pointer to Vendor specific info frame
 *              : data (Input)
 *              : pDataBuf - Pointer to data buffer to pack the
 *              : info frame (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref HDMI-Spec-1.3a and 1.4
 *-----------------------------------------------------------------------------
 */
BOOL VppPackVndrSpecInfoFrame(VPP_OBJ *vpp_obj,
            VPP_HDMI_PKT_VNDRSPEC_INFOFRM *pVndrSpecInfoFrm, UINT8 *pDataBuf)
{
    BOOL    retCode;

    if (pVndrSpecInfoFrm == NULL || pDataBuf == NULL)
    {
        return FALSE;
    }

    // Packet Header
    pDataBuf[0] = (1 << 7) | (CEA_INFO_FRM_VENDOR_SPEC);  // Info frame type
    pDataBuf[1] = 1;              // Version

    // Checksum will be calculated and filled in the end
    pDataBuf[3] = 0x0;
    pDataBuf[4] = pVndrSpecInfoFrm->ieeeRegID;
    pDataBuf[5] = pVndrSpecInfoFrm->ieeeRegID >> 8;
    pDataBuf[6] = pVndrSpecInfoFrm->ieeeRegID >> 16;
    if(pVndrSpecInfoFrm->ieeeRegID == 0x000C03)
    {
        pDataBuf[7] = 0x00;
        pDataBuf[7] |= ((pVndrSpecInfoFrm->HdmiVideoFmt & 0x07) << 5);
        if((pVndrSpecInfoFrm->HdmiVideoFmt & 0x07) == VPP_HDMI_VIDEO_FMT_EXTENDED_RES_FMT)
        {
            pDataBuf[8]= pVndrSpecInfoFrm->HDMI_VIC;
            pDataBuf[2] = pVndrSpecInfoFrm->PacketLength;               // Info frame length
        }
        else if((pVndrSpecInfoFrm->HdmiVideoFmt & 0x07) == VPP_HDMI_VIDEO_FMT_3D_FMT)
        {
            pDataBuf[8]  = 0x00;
            pDataBuf[8] |= ((pVndrSpecInfoFrm->Hdmi_3D_Structure & 0x0F) << 4);
            pDataBuf[8] |= ((pVndrSpecInfoFrm->Hdmi_3D_Meta_Present & 0x01) << 3);
            pDataBuf[2] = 5;               // Info frame length

            if(pVndrSpecInfoFrm->Hdmi_3D_Structure >= 0x08 && pVndrSpecInfoFrm->Hdmi_3D_Structure <= 0x0F)
            {
                pDataBuf[9] = 0x00;
                pDataBuf[9] |= ((pVndrSpecInfoFrm->Hdmi_3D_Ext_Data & 0x0F) << 4);
                pDataBuf[2] = 6;               // Info frame length
            }

            if((pVndrSpecInfoFrm->Hdmi_3D_Meta_Present & 0x01) == 0x01)
            {
                pDataBuf[10] = 0x00;
                pDataBuf[10] |= ((pVndrSpecInfoFrm->Hdmi_3D_Metadata_Type & 0x07) << 5);
                pDataBuf[10] |= (pVndrSpecInfoFrm->Hdmi_3D_Metadata_Length & 0x1F);
                if(pVndrSpecInfoFrm->Hdmi_3D_Metadata_Length & 0x1F)
                {
                    VPP_Memcpy(&pDataBuf[11], pVndrSpecInfoFrm->Hdmi_3D_Metadata, pVndrSpecInfoFrm->Hdmi_3D_Metadata_Length & 0x1F);
                }
                pDataBuf[2] = 7 + (pVndrSpecInfoFrm->Hdmi_3D_Metadata_Length & 0x1F);               // Info frame length
            }
        }
        else if((pVndrSpecInfoFrm->HdmiVideoFmt & 0x07) == VPP_HDMI_VIDEO_FMT_NO_INFO)
        {
            pDataBuf[2] = pVndrSpecInfoFrm->PacketLength;
            VPP_Memset(&pDataBuf[7],0,DOLBY_VSIF_LENGTH);
        }
    }
    else if(pVndrSpecInfoFrm->ieeeRegID == 0x00D046)
    {
        /*Dolby vision VSIF*/
        VPP_DV_HDMItx_PackVSInfoFrame(vpp_obj, pVndrSpecInfoFrm, pDataBuf);
    }
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
    else if(pVndrSpecInfoFrm->ieeeRegID == HDR10PLUS_IEEE_CODE)
    {
        /*HDR10+  VSIF*/
        UINT8   num;
        pDataBuf[2] = HDR10PLUS_VSIF_LENGTH;               // Info frame length
        pDataBuf[7] = ((pVndrSpecInfoFrm->application_ver & 3) << 6) | ((pVndrSpecInfoFrm->targeted_sys_dis_max_lum & 0x1F) << 1);
        pDataBuf[8] = pVndrSpecInfoFrm->avg_maxrgb;
        for (num = 0; num < MAX_NUM_DISTRIBUTIONS; num++) {
            pDataBuf[9+num] = pVndrSpecInfoFrm->distribution_values[num];
        }
        pDataBuf[18] = ((pVndrSpecInfoFrm->num_bezier_curve_anchors & 0xF) << 4) | ((pVndrSpecInfoFrm->knee_point_x & 0x3C0) >> 6);
        pDataBuf[19] = ((pVndrSpecInfoFrm->knee_point_x & 0x3F)<<2) | ((pVndrSpecInfoFrm->knee_point_y & 0x300) >> 8);
        pDataBuf[20] = (pVndrSpecInfoFrm->knee_point_y & 0xFF);
        for (num = 0; num < MAX_NUM_BEZIER_CURVE_ANCHORS; num++) {
            pDataBuf[21+num] = pVndrSpecInfoFrm->bezier_curve_anchors[num];
        }
        pDataBuf[30] = ((pVndrSpecInfoFrm->gph_ovl_flag & 1) << 7) | ((pVndrSpecInfoFrm->vsif_timing_mode & 1) << 6);
    }
#endif
    // Length of the packet is length of the data bytes +
    // size of header (4 bytes)
    retCode = VppFillChkSum(pDataBuf, (pDataBuf[2] + FRM_HDR_LEN));
    return retCode;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as Audio content
 *              : protection packet
 * Arguments    : pAcpPktData - Pointer to ACP data (Input)
 *              : pDataBuf - Pointer to data buffer to pack (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref HDMI-Spec-1.3a
 *-----------------------------------------------------------------------------
 */
BOOL VppPackACPPacket(
            VPP_HDMI_PKT_ACP *pAcpPktData, UINT8 *pDataBuf)
{
    if (pAcpPktData == NULL || pDataBuf == NULL) {
        return FALSE;
    }
    if (pAcpPktData->dataLen > 28) {
        return FALSE;
    }
    pDataBuf[0] = VPP_HDMI_PKT_ID_ACP;
    pDataBuf[1] = pAcpPktData->type & 0x03;
    pDataBuf[2] = 0x00; //Reserved

    if (pAcpPktData->type == VPP_HDMI_PKT_ACP_DVD_AUDIO ||
        pAcpPktData->type == VPP_HDMI_PKT_ACP_SACD)
    {
        GaloisMemcpy (&pDataBuf[3], pAcpPktData->dataBuf, pAcpPktData->dataLen);
    }
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as Gamut meta data packet
 * Arguments    : pGmdPktData - Pointer to GMD data (Input)
 *              : pDataBuf - Pointer to data buffer to pack (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref HDMI-Spec-1.3a
 *-----------------------------------------------------------------------------
 */
BOOL VppPackGMDPacket(
            VPP_HDMI_PKT_GMD *pGmdPktData, UINT8 *pDataBuf)
{
    if (pGmdPktData == NULL || pDataBuf == NULL) {
        return FALSE;
    }
    pDataBuf[0]  = VPP_HDMI_PKT_ID_GAMUT_METADATA;
    pDataBuf[1]  = pGmdPktData->affectedGamutSeqNum & 0x0F;
    pDataBuf[1] |= (pGmdPktData->gdbProfile & 0x07) << 4;
    pDataBuf[1] |= (pGmdPktData->nextField & 0x01) << 7;
    pDataBuf[2]  = pGmdPktData->curGamutSeqNum & 0x0F;
    pDataBuf[2] |= (pGmdPktData->seqInfo & 0x03) << 4;
    pDataBuf[2] |= (pGmdPktData->noCurGBD & 0x01) << 7;

    GaloisMemcpy (&pDataBuf[3], pGmdPktData->gbdData, 28);
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as ISRC1 packet
 * Arguments    : pIsrc1PktData - Pointer to ISRC1 data (Input)
 *              : pDataBuf - Pointer to data buffer to pack (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref HDMI-Spec-1.3a
 *-----------------------------------------------------------------------------
 */
BOOL VppPackISRC1Packet(
            VPP_HDMI_PKT_ISRC1 *pIsrc1PktData, UINT8 *pDataBuf)
{
    if (pIsrc1PktData == NULL || pDataBuf == NULL) {
        return FALSE;
    }
    pDataBuf[0]  = VPP_HDMI_PKT_ID_ISRC1;
    pDataBuf[1]  = pIsrc1PktData->sts & 0x07;
    pDataBuf[1] |= (pIsrc1PktData->valid & 0x01) << 6;
    pDataBuf[1] |= (pIsrc1PktData->cont & 0x01)  << 7;
    pDataBuf[2]  = 0x00;

    GaloisMemcpy (&pDataBuf[3], pIsrc1PktData->upc_ean_fld, 16);
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data as ISRC2 packet
 * Arguments    : pIsrc1PktData - Pointer to ISRC2 data (Input)
 *              : pDataBuf - Pointer to data buffer to pack (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref HDMI-Spec-1.3a
 *-----------------------------------------------------------------------------
 */
BOOL VppPackISRC2Packet(
            VPP_HDMI_PKT_ISRC2 *pIsrc2PktData, UINT8 *pDataBuf)
{
    if (pIsrc2PktData == NULL || pDataBuf == NULL) {
        return FALSE;
    }
    pDataBuf[0]  = VPP_HDMI_PKT_ID_ISRC2;
    pDataBuf[1]  = 0x00;
    pDataBuf[2]  = 0x00;

    GaloisMemcpy (&pDataBuf[3], pIsrc2PktData->upc_ean_fld, 16);
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Description  : This function packs the given data buffer to HDR infoframe
 * Arguments    : pDataBuf - Pointer to data buffer which has the info frame
 *              : data (Input)
 *              : phdrPktData - Pointer to HDR info frame data (Output)
 * Returns      : TRUE, on success
 *              : FALSE, otherwise
 * Notes        : Ref CEA-861.3
 *-----------------------------------------------------------------------------
 */
BOOL VppPackHDRInfoframe(VPP_OBJ *vpp_obj,
            VPP_HDMI_PKT_HDR_INFO_FRAME *phdrPktData, UINT8 *pDataBuf)

{
    int i = 0,pos = 0;
    BOOL    RetCode;

    if (phdrPktData == NULL || pDataBuf == NULL) {
        return FALSE;
    }
    pDataBuf[0]  = (1 << 7) | (CEA_INFO_FRM_HDR);  // Info frame type;
    pDataBuf[1]  = CEA_HDR_FRM_VERS;
    pDataBuf[2]  = CEA_HDR_FRM_LEN;

    // Checksum will be calculated and filled in the end
    pDataBuf[3] = 0x0;

    //Dosent require sending Dynamic and mastering infoframe set all value sto zeroes
    if((vpp_obj->HDR_Status != FALSE) && IS_HDR_ENABLED()) {
        pDataBuf[4] = phdrPktData->EOTF;
        pDataBuf[5]  = 0x0; /*Static Metadata type 1*/
    }

    /*Other than SMPTE 2048 payload is zero or while changeover*/
    if(phdrPktData->display_primaries_x[0] == 0) {
        // size of header (4 bytes)
        RetCode = VppFillChkSum(pDataBuf, (CEA_HDR_FRM_LEN + FRM_HDR_LEN));
        return RetCode;
    }
    pos = 6;
    for (i = 0 ; i < 3 ; i++ ) {
        pDataBuf[pos]   = LSB_VAL_SHORT_INT(phdrPktData->display_primaries_x[i]); /*LSB */
        pDataBuf[pos+1] = MSB_VAL_SHORT_INT(phdrPktData->display_primaries_x[i]); /*MSB*/
        pDataBuf[pos+2] = LSB_VAL_SHORT_INT(phdrPktData->display_primaries_y[i]); /*LSB */
        pDataBuf[pos+3] = MSB_VAL_SHORT_INT(phdrPktData->display_primaries_y[i]); /*MSB*/
        pos = pos+4;
    }
    pos = 17;
    pDataBuf[++pos] = LSB_VAL_SHORT_INT(phdrPktData->white_point_x); /*LSB */
    pDataBuf[++pos] = MSB_VAL_SHORT_INT(phdrPktData->white_point_x); /*MSB */
    pDataBuf[++pos] = LSB_VAL_SHORT_INT(phdrPktData->white_point_y); /*LSB */
    pDataBuf[++pos] = MSB_VAL_SHORT_INT(phdrPktData->white_point_y); /*MSB */

    pDataBuf[++pos] = LSB_VAL_SHORT_INT(phdrPktData->max_display_mastering_lumi);
    pDataBuf[++pos] = MSB_VAL_SHORT_INT(phdrPktData->max_display_mastering_lumi);
    pDataBuf[++pos] = LSB_VAL_SHORT_INT(phdrPktData->min_display_mastering_lumi);
    pDataBuf[++pos] = MSB_VAL_SHORT_INT(phdrPktData->min_display_mastering_lumi);

    pDataBuf[++pos] = LSB_VAL_SHORT_INT(phdrPktData->Maximum_Content_Light_Lvl);
    pDataBuf[++pos] = MSB_VAL_SHORT_INT(phdrPktData->Maximum_Content_Light_Lvl);

    pDataBuf[++pos] = LSB_VAL_SHORT_INT(phdrPktData->Maximum_Frame_Average_Light_Lvl);
    pDataBuf[++pos] = MSB_VAL_SHORT_INT(phdrPktData->Maximum_Frame_Average_Light_Lvl);

    // Length of the packet is length of the data bytes +
    // size of header (4 bytes)
    RetCode = VppFillChkSum(pDataBuf, (CEA_HDR_FRM_LEN + FRM_HDR_LEN));

    return RetCode;
}
