/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "com_type.h"
#include "hdmi_audio.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cfg.h"

static const HDMI_ACR_N_DATA AcrNTable[] =
{
    /* 32KHz   44.1KHz 48KHz */
    {  4096,   6272,   6144},   // RES_NTSC_M
    {  4096,   6272,   6144},   // RES_NTSC_J
    {  4096,   6272,   6144},   // RES_PAL_M
    {  4096,   6272,   6144},   // RES_PAL_BGH
    {  4096,   6272,   6144},   // RES_525I60
    {  4096,   6272,   6144},   // RES_525I5994
    {  4096,   6272,   6144},   // RES_625I50
    {  4096,   6272,   6144},   // RES_525P60
    {  4096,   6272,   6144},   // RES_525P5994
    {  4096,   6272,   6144},   // RES_625P50
    {  4096,   6272,   6144},   // RES_720P30
    {  11648,  17836,  11648},  // RES_720P2997
    {  4096,   6272,   6144},   // RES_720P25
    {  4096,   6272,   6144},   // RES_720P60
    {  11648,  17836,  11648},  // RES_720P5994
    {  4096,   6272,   6144},   // RES_720P50
    {  4096,   6272,   6144},   // RES_1080I60
    {  11648,  17836,  11648},  // RES_1080I5994
    {  4096,   6272,   6144},   // RES_1080I50
    {  4096,   6272,   6144},   // RES_1080P30
    {  11648,  17836,  11648},  // RES_1080P2997
    {  4096,   6272,   6144},   // RES_1080P25
    {  4096,   6272,   6144},   // RES_1080P24
    {  11648,  17836,  11648},  // RES_1080P2398
    {  4096,   6272,   6144},   // RES_1080P60
    {  11648,  8918,   5824},   // RES_1080P5994
    {  4096,   6272,   6144},   // RES_1080P50
    {  4096,   6272,   6144},   // RES_LVDS_1080P48
    {  4096,   6272,   6144},   // RES_LVDS_1080P50
    {  4096,   6272,   6144},   // RES_LVDS_1080P60
    {  4096,   6272,   6144},   // RES_LVDS_2160P12
    {  4096,   6272,   6864},   // RES_VGA_480P60
    {  4576,   7007,   6144},   // RES_VGA_480P5994
    {  4096,   6272,   6144},   // RES_720P50_3D
    {  4096,   6272,   6144},   // RES_720P60_3D
    {  11648,  8918,   5824},   // RES_720P5994_3D
    {  4096,   6272,   6144},   // RES_1080P24_3D
    {  11648,  8918,   5824},   // RES_1080P2398_3D
    {  4096,   6272,   6144},   // RES_1080P30_3D
    {  11648,  8918,   5824},  // RES_1080P2997_3D
    {  4096,   6272,   6144},   // RES_1080P25_3D
    {  4096,   6272,   6144},   // RES_1080I60_FP
    {  11648,  8918,   5824},  // RES_1080I5994_FP
    {  4096,   6272,   6144},   // RES_1080I50_FP
    //Since the RES_LVDS_1920X540P60_3D/RES_LVDS_1920X540P30_3D/RES_LVDS_1920X540P24_3D not added related array here.
    //Added the below three lines to advoid the array overlap,
    //but not sure the value is correct or not .
    {  4096,   6272,   6144},   // RES_LVDS_1920X540P60_3D
    {  4096,   6272,   6144},   // RES_LVDS_1920X540P30_3D
    {  4096,   6272,   6144},   // RES_LVDS_1920X540P24_3D
    {  4096,   6272,   6144},   /* RES_LVDS_720P100_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_720P120_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_1080P48_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_1080P50_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_1080P51_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_1920x540P100_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_1920X540P120_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_960x1080P120_3D */
    {  4096,   6272,   6144},   /* RES_LVDS_960x1080P120_3D */
/*For 4K*/
    {  11648,  17836,  11648},  // RES_4Kx2K2398 	84
    {  4096,   6272,   6144},   // RES_4Kx2K24 	85
    {  4096,   6272,   6144},   // RES_4Kx2K24_SMPTE 	86
    {  4096,   6272,   6144},   // RES_4Kx2K25 	87
    {  11648,  17836,  11648},  // RES_4Kx2K2997 	88
    {  4096,   6272,   6144},   // RES_4Kx2K30 	89
    {  4096,   6272,   6144},   // RES_4Kx2K50 	90
    {  11648,  8918,   5824},   // RES_4Kx2K5994 	91
    {  4096,   6272,   6144},   // RES_4Kx2K60 	92
    {  4096,   6272,   6144},   // RES_4Kx2K30_HDMI 	93
    {  4096,   6272,   6144},   ///* RES_4Kx1K120 */	94
    {  4096,   6272,   6144},   ///* RES_720P_4Kx1K120_3D */	95
    {  4096,   6272,   6144},   // RES_720P100 	96
    {  5824,   4459,   5824},   // RES_720P11988 //TODO: check values on QD 	97
    {  4096,   6272,   6144},   // RES_720P120 	98
    {  0,   0,   0},   // RES_RESET
};

BOOL VppHdmiGetAcrNVal (
            CEA_AUDIO_SAMP_FREQ AudSampFreq,
            INT Resolution,
            UINT32 *pAcrN)
{
    UINT16  Index = 0;
    *pAcrN = 0;

    if (!(Resolution >= FIRST_RES && Resolution < MAX_NUM_RESS))
    {
        // Invalid resolution
        return FALSE;
    }

    Index = Resolution;

    switch (AudSampFreq)
    {
        case CEA_AUDIO_SAMP_FREQ_32_KHZ:
            *pAcrN = AcrNTable[Index].NVal_32KHz;
            break;

        case CEA_AUDIO_SAMP_FREQ_44_1_KHZ:
            *pAcrN = AcrNTable[Index].NVal_44_1KHz;
            break;

        case CEA_AUDIO_SAMP_FREQ_48_KHZ:
            *pAcrN = AcrNTable[Index].NVal_48KHz;
            break;

        case CEA_AUDIO_SAMP_FREQ_88_2_KHZ:
            *pAcrN = AcrNTable[Index].NVal_44_1KHz * 2;
            break;

        case CEA_AUDIO_SAMP_FREQ_176_4_KHZ:
            *pAcrN = AcrNTable[Index].NVal_44_1KHz * 4;
            break;

        case CEA_AUDIO_SAMP_FREQ_96_KHZ:
            *pAcrN = AcrNTable[Index].NVal_48KHz * 2;
            break;

        case CEA_AUDIO_SAMP_FREQ_192_KHZ:
            *pAcrN = AcrNTable[Index].NVal_48KHz * 4;
            break;

        default:
            *pAcrN = 0;
            return FALSE;
    }
    return TRUE;
}

BOOL VppHdmiPackAudioChnSts(
            PHDMI_AUD_CHN_STS_DATA pAudioChnSts,
            UINT8 *pDataBuf)
{
    UINT8 SampFreqID;

    if (pAudioChnSts == NULL || pDataBuf == NULL)
    {
        return FALSE;
    }

    // Map Sampling frequency
    switch (pAudioChnSts->SampFreq)
    {
        case 32000:
            SampFreqID = 0x03;
            break;
        case 44100:
            SampFreqID = 0x00;
            break;
        case 48000:
            SampFreqID = 0x02;
            break;
        case 88200:
            SampFreqID = 0x08;
            break;
        case 96000:
            SampFreqID = 0x0A;
            break;
        case 176400:
            SampFreqID = 0x0C;
            break;
        case 192000:
            SampFreqID = 0x0E;
            break;
        case 768000:
            SampFreqID = 0x09;
            break;
        default:
            SampFreqID = 0x01;
            break;
    }

    //pDataBuf[0] = (pAudioChnSts); // Leave all 0s for now
    pDataBuf[0] = pAudioChnSts->NonPcm << 1;
    pDataBuf[1] = pAudioChnSts->CategoryCode;
    pDataBuf[2] = (pAudioChnSts->SrcNum) | (pAudioChnSts->ChnNum << 4);
    pDataBuf[3] = (SampFreqID); //| (pAudioChnSts->ClkAccuracy << 4);

    // Max length is specified to be 24/20
    // Word length and original sampling frequency not indicated by default
    if (pAudioChnSts->WordLength == 24) {
        pDataBuf[4] = 0x0B;
    } else if (pAudioChnSts->WordLength == 20){
        pDataBuf[4] = 0x0A;
    } else if (pAudioChnSts->WordLength == 16){
        pDataBuf[4] = 0x02;
    }

    return TRUE;
}
